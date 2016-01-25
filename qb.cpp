/*
 * qb.cpp
 *
 *  Created on: July 30, 2014
 *              Siriwat Kasamwattanarote
 */
#include <ctime>
#include <bitset>
#include <vector>
#include <deque>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <sstream>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>    // sort
#include <cstdlib>      // exit
#include <memory>       // uninitialized_fill_n
#include <omp.h>

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "../alphautils/imtools.h"
#include "invert_index.h"
#include "bow.h"
#include "ins_param.h"
#include "homography.h"

#include "qb.h"

#include "version.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace ins;

namespace ins
{

qb::qb(const ins_param& param, const float* idf, const string& query_name, const string& working_dir)
{
    run_param = param;
    _idf = idf;
    _query_name = query_name;
    if (working_dir == "")
        _working_dir = run_param.online_working_path;
    else
        _working_dir = working_dir;

    // RANSAC
    ransac_check = false;
    actual_topk = 0;

    bow_tools.init(run_param);
}

qb::~qb(void)
{
    reset_multi_bow();
}

// QB I/O
void qb::add_bow(const vector<bow_bin_object*>& bow_sig)
{
    _multi_bow.push_back(bow_sig);

    /// Update sequence_id for QB
    size_t sequence_id = (_multi_bow.size() - 1) * sequence_offset;
    vector<bow_bin_object*>& bow = _multi_bow.back();
    for (size_t bin_idx = 0; bin_idx < bow.size(); bin_idx++)
    {
        bow_bin_object* bin = bow[bin_idx];
        for (size_t feature_idx = 0; feature_idx < bin->features.size(); feature_idx++)
        {
            bin->features[feature_idx]->sequence_id = sequence_id;
        }
    }

    // Preparing flag
    _multi_bow_flag.push_back(BIT1M());
    BIT1M& bow_flag = _multi_bow_flag.back();
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
        bow_flag.set(bow_sig[bin_idx]->cluster_id);

    // Counting topk
    actual_topk++;
}

void qb::add_bow_from_rank(const vector<result_object>& result, const int top_k)
{
    /// Bow loader
    bow bow_loader;
    bow_loader.init(run_param);
	// If video, load only the first frame of it (to be verified on only the first frame)
	//if (run_param.pooling_enable)
	//	bow_loader.set_sequence_id_filter(0);

    /// Load bow for top_k result
    int top_load = result.size();
    if (top_load > top_k)
        top_load = top_k;
    // Reserve memory
    vector<size_t> load_list(top_load);
    vector< vector<bow_bin_object*> > loaded_multi_bow(top_load);
    for (int result_idx = 0; result_idx < top_load; result_idx++)
        load_list[result_idx] = result[result_idx].dataset_id;
    bow_loader.load_running_bows(load_list, loaded_multi_bow);

    /// Update sequence_id for QB
    size_t sequence_id = _multi_bow.size() * sequence_offset;
    for (size_t bow_idx = 0; bow_idx < loaded_multi_bow.size(); bow_idx++)
    {
        vector<bow_bin_object*>& bow = loaded_multi_bow[bow_idx];
        for (size_t bin_idx = 0; bin_idx < bow.size(); bin_idx++)
        {
			//cout << "bow: " << bow_idx << "/" << loaded_multi_bow.size() << " bin: " << bin_idx << " seq: ";
            bow_bin_object* bin = bow[bin_idx];
            for (size_t feature_idx = 0; feature_idx < bin->features.size(); feature_idx++)
            {
				//cout << feature_idx << "," << bin->features[feature_idx]->sequence_id << "->" << sequence_id + bin->features[feature_idx]->sequence_id << " ";
                bin->features[feature_idx]->sequence_id += sequence_id;
            }
			//cout << endl;
        }

        // Increment sequence_id
        sequence_id += sequence_offset;
    }

    // Adding to the end of vector if not empty
    if (_multi_bow.size())
        _multi_bow.insert(_multi_bow.end(), loaded_multi_bow.begin(), loaded_multi_bow.end());
    else
        _multi_bow.swap(loaded_multi_bow);

    // Preparing flag
    for (size_t bow_idx = 0; bow_idx < _multi_bow.size(); bow_idx++)
    {
        vector<bow_bin_object*>& curr_bow = _multi_bow[bow_idx];
        _multi_bow_flag.push_back(BIT1M());
        BIT1M& bow_flag = _multi_bow_flag.back();
        for (size_t bin_idx = 0; bin_idx < curr_bow.size(); bin_idx++)
            bow_flag.set(curr_bow[bin_idx]->cluster_id);
    }

    // Counting topk
    actual_topk += _multi_bow.size();
}

void qb::set_multi_bow(vector< vector<bow_bin_object*> >& multi_bow)
{
    // Reset not empty
    if (_multi_bow.size())
        reset_multi_bow();

    // Swap
    _multi_bow.swap(multi_bow);
}

void qb::build_transaction(const string& out, const vector< vector<bow_bin_object*> >& multi_bow, bool transpose)
{
    stringstream transaction_buffer;
    /// Transpose transaction
    if (transpose)
    {
        /*
        row: item label
        col: transaction_idx
        t1, t2, ..
        t3, t4, t5, ..
        ..
        */

        // Transposing transaction
        unordered_map< size_t, vector<size_t> > transposed_transaction;
        for (size_t bow_idx = 0; bow_idx < multi_bow.size(); bow_idx++)
        {
            // Skip this transaction if found to be outlier
            if (ransac_check && !topk_inlier[bow_idx])
            {
                //cout << "Skip outlier transaction: " << redc << toString(bow_idx) << endc << endl;
                continue;   /// Transpose mode, skip transaction by not add transaction_id to it
            }

            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
                transposed_transaction[multi_bow[bow_idx][bin_idx]->cluster_id].push_back(bow_idx);
        }

        // Writing transaction
        size_t skip_bin = 0;
        for (auto tt_it = transposed_transaction.begin(); tt_it != transposed_transaction.end(); tt_it++)
        {
            vector<size_t>& trans = tt_it->second;

            // Skip this bin if found to be occurred from only one transaction (image)
            if (trans.size() == 1)
            {
                skip_bin++;
                continue;
            }

            // Building
            for (size_t trans_idx = 0; trans_idx < trans.size(); trans_idx++)
                transaction_buffer << trans[trans_idx] << " ";
            transaction_buffer << endl;

            // Release memory
            vector<size_t>().swap(tt_it->second);
        }
        if (skip_bin)
            cout << "Skip " << redc << skip_bin << endc << " bin(s) in transposing transaction." << endl;

        // Release memory
        unordered_map< size_t, vector<size_t> >().swap(transposed_transaction);
    }
    /// Normal transaction
    else
    {
        /*
        row: transaction_idx
        col: item label
        i1, i2, i10, ..
        i3, i4, i6, ..
        ..
        */

        // Writing transaction
        for (size_t bow_idx = 0; bow_idx < multi_bow.size(); bow_idx++)
        {
            // Skip this transaction if found to be outlier
            if (ransac_check && !topk_inlier[bow_idx])
            {
                transaction_buffer << endl;     /// Normal mode, skip transaction by empty line
                continue;
            }

            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
                transaction_buffer << multi_bow[bow_idx][bin_idx]->cluster_id << " ";
            transaction_buffer << endl;
        }
    }

    /// Flush transaction
    text_write(out, transaction_buffer.str(), false);
}

size_t qb::import_fim2checkbin(const string& in, bool transpose)
{
    vector<string> raw_fim = text_readline2vector(in);

    // Total bin space
    BIT1M all_intersec_space;

    // For each sets.
    //timespec importTime = CurrentPreciseTime();
    //cout << "Importing FIM " << greenc << get_filename(in) << endc << "..."; cout.flush();
    size_t fim_size = raw_fim.size();
    /// Map mined transaction_id to item_id back using Galios Connection, as shown in Lattice Space
    if (transpose)
    {
        for (size_t set_idx = 0; set_idx < fim_size; set_idx++)
        {
            vector<string> Items;   // Item [0-(n-2)], Support [n-1] x,y,z,11.9%
            BIT1M curr_intersec_space;

            // Accumulate support score for each item
            StringExplode(raw_fim[set_idx], ",", Items);
            int item_count = int(Items.size() - 1);

            // Skip last fim if fpgrowth killed or timeout detected
            if (!str_contains(Items[item_count], "%"))
            {
                cout << "Skip last frequent item set caused by killed!" << endl;
                cout << "FIM output not yet done, but memory not enough!!" << endl;
                break;
            }

            // Reset curr_intersec_space to all 1
            curr_intersec_space.set();

            // Run from first to last (except percent %)
            for (int item_idx = 0; item_idx < item_count; item_idx++)
                curr_intersec_space &= _multi_bow_flag[atoi(Items[item_idx].c_str())];

            // Keep
            all_intersec_space |= curr_intersec_space;

            // Release memory
            vector<string>().swap(Items);
        }
    }
    else
    {
        vector<string> Items;   // Item [0-(n-2)], Support [n-1] as x,y,z,11.9%
        BIT1M curr_intersec_space;
        for (size_t set_idx = 0; set_idx < fim_size; set_idx++)
        {
            // Accumulate support score for each item
            StringExplode(raw_fim[set_idx], ",", Items);
            size_t item_count = Items.size() - 1;

            // Skip last fim if fpgrowth killed detected
            if (!str_contains(Items[item_count], "%"))
            {
                cout << "Skip last frequent item set caused by killed!" << endl;
                cout << "FIM output not yet done, but memory not enough!!" << endl;
                break;
            }

            // Reset curr_intersec_space to all 1
            curr_intersec_space.set();

            // Run from first to last (except percent %)
            for (size_t item_idx = 0; item_idx < item_count; item_idx++)
            {
                // Item id
                curr_intersec_space.set(strtoull(Items[item_idx].c_str(), NULL, 0));
            }

            // Keep
            all_intersec_space |= curr_intersec_space;

            // Release memory
            vector<string>().swap(Items);
        }
        // Release memory
        vector<string>().swap(Items);
    }
    //cout << "done! (in " << setprecision(2) << fixed << TimeElapse(importTime) << " s)" << endl;

    // Release memory
    vector<string>().swap(raw_fim);

    // Return number of bin
    return all_intersec_space.count();
}

// transaction_set will be filled with transpose mode
void qb::import_fim2itemset(const string& in, unordered_set<size_t>& item_set, unordered_set<int>& transaction_set, bool transpose)
{
    BIT1M frequent_item_count;
    frequent_item_count.reset();

    cout << "Reading FIM.."; cout.flush();
    vector<string> raw_fim = text_readline2vector(in);
    cout << "done!" << endl;

    // For each sets.
    timespec importTime = CurrentPreciseTime();
    cout << "Importing FIM " << greenc << get_filename(in) << endc << "..."; cout.flush();
    size_t fim_size = raw_fim.size();
    /// Map mined transaction_id to item_id back using Galios Connection, as shown in Lattice Space
    if (transpose)
    {
        /// Initial transaction counter space
        size_t transaction_count = _multi_bow.size();
        BIT1K transaction_freq;
        transaction_freq.reset();

        #pragma omp parallel shared(fim_size,raw_fim,frequent_item_count,transaction_freq)
        {
            #pragma omp for schedule(dynamic,1)
            for (size_t set_idx = 0; set_idx < fim_size; set_idx+=4)
            {
                vector<string> Items;   // Item [0-(n-2)], Support [n-1] x,y,z,11.9%
                // Accumulate support score for each item
                StringExplode(raw_fim[set_idx], ",", Items);
                int item_count = int(Items.size() - 1);

                // Skip last fim if fpgrowth killed detected
                if (!str_contains(Items[item_count], "%"))
                {
                    cout << "Skip last frequent item set caused by killed!" << endl;
                    cout << "FIM output not yet done, but memory not enough!!" << endl;
                    continue;
                }

                // Reset pattern_intersec_space to all 1
                BIT1M pattern_intersec_space;
                pattern_intersec_space.set();

                /// Mapping from transpose to normal pattern through original transaction
                // Run from first to last (except percent %)
                for (int item_idx = 0; item_idx < item_count; item_idx++)
                {
                    // Transaction id
                    int transaction_id = atoi(Items[item_idx].c_str());

                    // Accumulate transaction freq
                    transaction_freq.set(transaction_id);

                    pattern_intersec_space &= _multi_bow_flag[transaction_id];
                }

                /// Convert
                // Check if any item was intersected by all mined transactions, this will convert transaction_id to item_id
                for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
                {
                    // Found intersected pattern
                    if (pattern_intersec_space[cluster_id])
                    {
                        // Keep it
                        frequent_item_count.set(cluster_id);
                    }
                }

                percentout(set_idx, fim_size, 100);

                // Release memory
                vector<string>().swap(Items);
            }
        }

        // Constructing transaction_set result
        for (size_t trans_idx = 0; trans_idx < transaction_count; trans_idx++)
        {
            if (transaction_freq[trans_idx])
                transaction_set.insert(trans_idx);
        }
    }
    else
    {
        vector<string> Items;   // Item [0-(n-2)], Support [n-1] as x,y,z,11.9%
        for (size_t set_idx = 0; set_idx < fim_size; set_idx+=4)
        {
            // Accumulate support score for each item
            StringExplode(raw_fim[set_idx], ",", Items);
            size_t item_count = Items.size() - 1;

            // Skip last fim if fpgrowth killed detected
            if (!str_contains(Items[item_count], "%"))
            {
                cout << "Skip last frequent item set caused by killed!" << endl;
                cout << "FIM output not yet done, but memory not enough!!" << endl;
                continue;
            }

            // Run from first to last (except percent %)
            for (size_t item_idx = 0; item_idx < item_count; item_idx++)
            {
                // Item id
                frequent_item_count.set(strtoull(Items[item_idx].c_str(), NULL, 0));
            }
            percentout(set_idx, fim_size, 2);

            // Release memory
            vector<string>().swap(Items);
        }
        // Release memory
        vector<string>().swap(Items);
    }
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(importTime) << " s)" << endl;

    stringstream import_log;
    import_log << "Imported " << fim_size << " pattern(s) in " << setprecision(2) << fixed << TimeElapse(importTime) << " s" << endl;
    cout << "Imported " << redc << fim_size << endc << " pattern(s) in " << setprecision(2) << fixed << TimeElapse(importTime) << " s" << endl;
    text_write(in + ".imported.log", import_log.str());

    // Constructing item_set result
    for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
        if (frequent_item_count[cluster_id])
            item_set.insert(cluster_id);

    // Release memory
    vector<string>().swap(raw_fim);

    // Delete FIM result file
    remove(in.c_str());
}
/* Old code: Support weight calculation
void qb::import_fim2itemset(const string& in, unordered_set<size_t>& item_set, unordered_set<int>& transaction_set, bool transpose)
{
    int* frequent_item_count = new int[run_param.CLUSTER_SIZE];
    fill_n(frequent_item_count, run_param.CLUSTER_SIZE, 0);
    vector<string> raw_fim = text_readline2vector(in);

    // For each sets.
    timespec importTime = CurrentPreciseTime();
    cout << "Importing FIM " << greenc << get_filename(in) << endc << "..."; cout.flush();
    size_t fim_size = raw_fim.size();
    /// Map mined transaction_id to item_id back using Galios Connection, as shown in Lattice Space
    if (transpose)
    {
        /// Initial transaction counter space
        vector<int> transaction_freq(_multi_bow.size());

        #pragma omp parallel shared(fim_size,raw_fim,frequent_item_count,transaction_set)
        {
            #pragma omp for schedule(dynamic,1)
            for (size_t set_idx = 0; set_idx < fim_size; set_idx++)
            {
                vector<string> Items;   // Item [0-(n-2)], Support [n-1] x,y,z,11.9%
                // Accumulate support score for each item
                StringExplode(raw_fim[set_idx], ",", Items);
                int item_count = int(Items.size() - 1);

                // Skip last fim if fpgrowth killed detected
                if (!str_contains(Items[item_count], "%"))
                {
                    cout << "Skip last frequent item set caused by killed!" << endl;
                    cout << "FIM output not yet done, but memory not enough!!" << endl;
                    continue;
                }

                // Reset pattern_intersec_space to all 1
                BIT1M pattern_intersec_space;
                pattern_intersec_space.set();

                /// Mapping from transpose to normal pattern through original transaction
                // Run from first to last (except percent %)
                for (int item_idx = 0; item_idx < item_count; item_idx++)
                {
                    // Transaction id
                    int transaction_id = atoi(Items[item_idx].c_str());

                    // Accumulate transaction freq
                    #pragma omp atomic
                    transaction_freq[transaction_id]++;

                    pattern_intersec_space &= _multi_bow_flag[transaction_id];
                }

                /// Convert
                // Check if any item was intersected by all mined transactions, this will convert transaction_id to item_id
                for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
                {
                    // Found intersected pattern
                    if (pattern_intersec_space[cluster_id])
                    {
                        // Keep it
                        #pragma omp atomic
                        frequent_item_count[cluster_id]++;
                    }
                }

                percentout(set_idx, fim_size, 100);

                // Release memory
                vector<string>().swap(Items);
            }
        }

        // Constructing transaction_set result
        for (size_t trans_idx = 0; trans_idx < transaction_freq.size(); trans_idx++)
        {
            if (transaction_freq[trans_idx])
                transaction_set.insert(trans_idx);
        }
    }
    else
    {
        vector<string> Items;   // Item [0-(n-2)], Support [n-1] as x,y,z,11.9%
        for (size_t set_idx = 0; set_idx < fim_size; set_idx++)
        {
            // Accumulate support score for each item
            StringExplode(raw_fim[set_idx], ",", Items);
            size_t item_count = Items.size() - 1;

            // Skip last fim if fpgrowth killed detected
            if (!str_contains(Items[item_count], "%"))
            {
                cout << "Skip last frequent item set caused by killed!" << endl;
                cout << "FIM output not yet done, but memory not enough!!" << endl;
                continue;
            }

            // Run from first to last (except percent %)
            for (size_t item_idx = 0; item_idx < item_count; item_idx++)
            {
                // Item id
                frequent_item_count[strtoull(Items[item_idx].c_str(), NULL, 0)]++;
            }
            percentout(set_idx, fim_size, 2);

            // Release memory
            vector<string>().swap(Items);
        }
        // Release memory
        vector<string>().swap(Items);
    }
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(importTime) << " s)" << endl;

    // Constructing item_set result
    for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
        if (frequent_item_count[cluster_id])
            item_set.insert(cluster_id);

    // Release memory
    vector<string>().swap(raw_fim);
    delete[] frequent_item_count;
}
*/

void qb::import_fim2itemweight(const string& in, unordered_map<size_t, float>& item_weight, bool transpose)
{
    unordered_map<size_t, size_t> frequent_item_count;
    unordered_map<size_t, float> frequent_item_weight;
    vector<string> raw_fim = text_readline2vector(in);

    for (size_t set_idx = 0; set_idx < raw_fim.size(); set_idx++)
    {
        // Accumulate support score for each item
        vector<string> Items;   // Item [0-(n-2)], Support [n-1]
        StringExplode(raw_fim[set_idx], ",", Items);
        size_t item_count = Items.size() - 1;

        // Skip last fim if fpgrowth killed detected
        if (!str_contains(Items[item_count], "%"))
        {
            cout << "Skip last frequent item set caused by killed!" << endl;
            cout << "FIM output not yet done, but memory not enough!!" << endl;
            continue;
        }

        float fim_support = atof(str_replace_last(Items[item_count], "%", "").c_str()) * 0.01f;  // convert to 0-1 scale
        for (size_t item_idx = 0; item_idx < item_count; item_idx++)
        {
            size_t item = strtoull(Items[item_idx].c_str(), NULL, 0);
            if (frequent_item_count.find(item) == frequent_item_count.end())
            {
                frequent_item_count[item] = 0;
                frequent_item_weight[item] = 0;
            }
            frequent_item_count[item]++;
            frequent_item_weight[item] += fim_support;
        }
    }

    // Release memory
    unordered_map<size_t, size_t>().swap(frequent_item_count);

    // Return result
    frequent_item_weight.swap(item_weight);
}

// RANSAC homography check
void qb::topk_ransac_check(const vector<bow_bin_object*>& query_bow)
{
    // Enable ransac check, then enable inlier top-k filter through all QB, e.g. skip transaction making on outlier
    ransac_check = true;

    /// ==== RANSAC HOMOGRAPHY TEMPLATE STEPS COPIED FROM QE (qe.cpp) ====
    /// 1. Make homography obj
    homography homo_tool(run_param);

    /// 2. Set query to source
    // Preparing source Point2f
    vector<Point2f> src_pts;
    vector<size_t> src_cluster_ids;
    for (size_t bin_idx = 0; bin_idx < query_bow.size(); bin_idx++)
    {
        bow_bin_object* bin = query_bow[bin_idx];
		// Select only the first kp, hopefully it will be good for burstiness problem
        src_pts.push_back(Point2f(bin->features[0]->kp[0], bin->features[0]->kp[1]));
        src_cluster_ids.push_back(bin->cluster_id);
    }
    homo_tool.set_src_2dpts(src_pts, src_cluster_ids);

    // Release memory
    vector<Point2f>().swap(src_pts);
    vector<size_t>().swap(src_cluster_ids);

    /// 3. Set multi_bow to reference
    timespec qeinit_time = CurrentPreciseTime();
    cout << "QE: set reference bows.."; cout.flush();
    for (size_t bow_idx = 0; bow_idx < _multi_bow.size(); bow_idx++)
    {
        vector<Point2f> ref_pts;
        vector<size_t> ref_cluster_ids;
        vector<bow_bin_object*>& _bow = _multi_bow[bow_idx];
        for (size_t bin_idx = 0; bin_idx < _bow.size(); bin_idx++)
        {			
			bow_bin_object* bin = _bow[bin_idx];
			
			// Filtering only first frame to be verified
			if (bin->features[0]->sequence_id % sequence_offset > 0)
				continue;
			
			// Select only the first kp (features[0]), hopefully it will be good for burstiness problem										
			ref_pts.push_back(Point2f(bin->features[0]->kp[0], bin->features[0]->kp[1]));   
			ref_cluster_ids.push_back(bin->cluster_id);
        }
        homo_tool.add_ref_2dpts(ref_pts, ref_cluster_ids);

        // Release memory
        vector<Point2f>().swap(ref_pts);
        vector<size_t>().swap(ref_cluster_ids);

        percentout_timeleft(bow_idx, 0, _multi_bow.size(), qeinit_time, 1);
    }
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(qeinit_time) << " s)" << endl;

    /// 4. RUN HOMO-LO-RANSAC
    // This internally run in parallel
    timespec qehomo_time = CurrentPreciseTime();
    cout << "QE: RANSAC Homography.."; cout.flush();
    homo_tool.calc_homo();
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(qehomo_time) << " s)" << endl;

    /// 5. Verify inlier threshold with ADINT
    /// inl_th = 0 for enter ADINT mode
    if (run_param.qe_ransac_adint_enable)
        homo_tool.verify_topk(topk_inlier, 0);
    else
        homo_tool.verify_topk(topk_inlier, run_param.qe_ransac_threshold);

}

// Mining
void qb::mining_fim_bow(const int minsup, const int maxsup, vector<bow_bin_object*>& mined_bow_sig)
{
    /// Mining executable
    // Borgelt FPGrowth
    // Find Frequent Item Set by FP-growth
    // output item set format a,d,b:80
    // -tm = maximal
    // -k, output seperator
    // -v, support value seperator
    // -s lowerbound support percentage
    // -S lowerbound support percentage
    // -s-x lowerbound support transaction
    // -S-x lowerbound support transaction
    /// timeout 2 minutes then kill
    string fim_binary = "timeout -k 0m " + toString(run_param.qb_colossal_time) + "s /home/stylix/webstylix/code/datamining/borglet/Source/fpgrowth/src/fpgrowth";
    string transaction_path = _working_dir + "/" + _query_name + "_qbtrans.txt";
    string frequent_item_path = _working_dir + "/" + _query_name + "_qbfim_minsup_" + toString(minsup) + "_maxsup_" + toString(maxsup) + ".txt";
    string frequent_item_log_path = frequent_item_path + ".log";

    /// Preparing transaction
    build_transaction(transaction_path, _multi_bow, run_param.qb_transpose_enable);

    /// Mining
    string fim_cmd = fim_binary + " -tc -k, -v,%S%% -s" + toString(minsup) + " -S" + toString(maxsup) + " " + transaction_path + " " + frequent_item_path + " > " + frequent_item_log_path + " 2>&1";// + COUT2NULL;
    cout << "FIM CMD: " << fim_cmd << endl;
    exec(fim_cmd);

    /// Loading frequent item set
    unordered_set<size_t> frequent_item_set;
    unordered_set<int> frequent_transaction_set; // For transposed version
    import_fim2itemset(frequent_item_path, frequent_item_set, frequent_transaction_set, run_param.qb_transpose_enable);

    /// Constructing mined bow
    build_fim_pool(_multi_bow, frequent_item_set, frequent_transaction_set, mined_bow_sig, run_param.qb_transpose_enable);
}

void qb::mining_maxpat_bow(vector<bow_bin_object*>& mined_bow_sig)
{
    /// Mining executable
    // Borgelt FPGrowth
    // Find Frequent Item Set by FP-growth
    // output item set format a,d,b:80
    // -tm = maximal
    // -k, output seperator
    // -v, support value seperator
    // -s lowerbound support percentage
    // -S lowerbound support percentage
    // -s-x lowerbound support transaction
    // -S-x lowerbound support transaction
    /* Template from fprun.sh, after run, the result will be a total number of pattern within [minsup to maxsup] transaction
    fpgrowth -tc -k, -v,%S%% -s-minsup -S-maxsup transaction_pat 2>&1 | perl -lne '/\[\K.*set\(s\)/ and print $&' | cut -d " " -f 1;
	*/
	/// timeout 2 minute then kill
    string fim_binary = "timeout -k 0m " + toString(run_param.qb_colossal_time) + "s /home/stylix/webstylix/code/datamining/borglet/Source/fpgrowth/src/fpgrowth";
    string transaction_path = _working_dir + "/" + _query_name + "_qbtrans.txt";

    /// Preparing transaction
    build_transaction(transaction_path, _multi_bow, run_param.qb_transpose_enable);

    /// Finding maximum pattern
    int start_run = 0;
    int step = 5;
    int maxsup = 200;
    int maxpat = 0;
    int maxpat_runid = 0;
    int fallback_count = 0;
    int fallback_count_limit = 3;
    int fallback_maxsup_step = 200;
    do
    {
        int total_block = maxsup / step - start_run;
        int* pattern_count = new int[total_block];
        cout << "==== " << redc << "MAX PATTERN TRACER" << endc << " ====" << endl;
        cout << "Tracing.."; cout.flush();
        #pragma omp parallel shared(step,total_block,fim_binary,transaction_path,pattern_count)
        {
            #pragma omp for schedule(dynamic,1)
            // for omp reference
            // #pragma omp for schedule(dynamic,1) reduction(+ : total_kp)
            for (int run = 0; run < total_block; run++)
            {
                // Current support
                int curr_run = run + start_run;
                int sup = curr_run * step;
                string fim_cmd = fim_binary + " -tm -k, -v,%S%% -s-" + toString(sup) + " -S-" + toString(sup + step) + " " + transaction_path + " 2>&1 | perl -lne '/\\[\\K.*set\\(s\\)/ and print $&' | cut -d ' ' -f 1;";
                pattern_count[run] = atoi(exec(fim_cmd).c_str());
                //cout << "TRACE CMD: " << fim_cmd << endl;
            }

        }
        cout << "done!" << endl;

        /// Pattern slope tracking (to find some hole from killed processes)
        int trending_curr = 0;
        int possible_missing_higher_threshold = 1000;
        vector<int> trending_state;
        vector<int> patidx_state;
        // Slope trending construction
        for (int pat_idx = 0; pat_idx < total_block - 1; pat_idx++)
        {
            // State setting
            if (pattern_count[pat_idx] != 0 && pattern_count[pat_idx + 1] != 0)
            {
                if (pattern_count[pat_idx] > pattern_count[pat_idx + 1])
                    trending_curr = -1;
                else if (pattern_count[pat_idx] <= pattern_count[pat_idx + 1])
                    trending_curr = 1;
            }
            else
            {
                // Check if nearest non-zero is higher than threshold
                bool nearest_is_high = false;
                // Check left
                for (int high_idx = pat_idx; high_idx >= 0; high_idx--)
                {
                    // First non-zero
                    if (pattern_count[high_idx])
                    {
                        // Higher than threshold
                        if (pattern_count[high_idx] > possible_missing_higher_threshold)
                            nearest_is_high = true;
                        break;
                    }
                }
                if (!nearest_is_high)
                // Check right
                for (int high_idx = pat_idx; high_idx < total_block; high_idx++)
                {
                    // First non-zero
                    if (pattern_count[high_idx])
                    {
                        // Higher than threshold
                        if (pattern_count[high_idx] > possible_missing_higher_threshold)
                            nearest_is_high = true;
                        break;
                    }
                }

                // Nearest non-zero is really high
                if (nearest_is_high)
                    trending_curr = 0;
                // Skip
                else
                    continue;

            }

            // State constructing
            if (trending_state.size() == 0 || trending_state.back() != trending_curr)
            {
                trending_state.push_back(trending_curr);
                patidx_state.push_back(pat_idx);
            }
        }

        // Debug
        cout << "trending:" << endl;
        for (size_t trending_state_idx = 0; trending_state_idx < trending_state.size(); trending_state_idx++)
            cout << trending_state[trending_state_idx] << endl;

        /// Missing pattern check (killed happen, too long processing, need approximation)
        bool not_miss = true;
        size_t openzero_patidx = 0;
        size_t closezero_patidx = trending_state.size() - 1;
        if (trending_state.size() >= 2)
        {
            for (size_t trending_state_idx = 1; trending_state_idx < trending_state.size(); trending_state_idx++)
            {
                // Finding 1 0
                if (trending_state[trending_state_idx - 1] == 1 && trending_state[trending_state_idx] == 0)
                {
                    not_miss = false;
                    openzero_patidx = trending_state_idx;
                    continue;
                }

                // Finding 0 -1
                if (trending_state[trending_state_idx - 1] == 0 && trending_state[trending_state_idx] == -1)
                {
                    not_miss = false;
                    closezero_patidx = trending_state_idx;
                    continue;
                }
            }
        }
        /// Fall-back if multiple zero happen
        if (!not_miss)
        {
            int zeroth = 0;
            for (size_t trending_state_idx = 0; trending_state_idx < trending_state.size(); trending_state_idx++)
            {
                if (trending_state[trending_state_idx] == 0)
                    zeroth++;
            }
            /// Cancel approximation mode, confused by zero
            if (zeroth > 1)
                not_miss = true;
        }

        // Debug
        cout << "pattern:" << endl;
        for (int pat_idx = 0; pat_idx < total_block; pat_idx++)
            cout << (pat_idx + start_run) * step << "-" << (pat_idx + start_run) * step + step << " " << pattern_count[pat_idx] << endl;

        /// Finding MAX pattern
        if (not_miss)
        {
            for (int pat_idx = 0; pat_idx < total_block; pat_idx++)
            {
                if (maxpat < pattern_count[pat_idx])
                {
                    maxpat = pattern_count[pat_idx];
                    maxpat_runid = pat_idx + start_run;
                }
            }
        }
        /// Approximate MAX pattern, in case of processes got kill
        else
        {
            cout << "Found missing at [" << redc << (patidx_state[openzero_patidx] + start_run) * step << endc << "," << redc << (patidx_state[closezero_patidx] + start_run) * step << endc << "]" << endl;
            maxpat = -1;
            maxpat_runid = ((patidx_state[openzero_patidx] + patidx_state[closezero_patidx]) / 2) + start_run;
        }

        // Release memory
        delete[] pattern_count;

        // Next run
        start_run += total_block;
        /// In case pattern cannot find in this iteration, increasing maxsup higher, then checking again
        maxsup += fallback_maxsup_step;
    } while (maxpat == 0 && ++fallback_count < fallback_count_limit);

    int mine_minsup = maxpat_runid * step;
    int mine_maxsup = mine_minsup + step;
    cout << "Found max pattern at run_id:" << maxpat_runid << " " << mine_minsup << "-" << mine_maxsup << " support" << endl;

    /// Mining
    string frequent_item_path = _working_dir + "/" + _query_name + "_qbfim_minsup_-" + toString(mine_minsup) + "_maxsup_-" + toString(mine_maxsup) + ".txt";
    string frequent_item_log_path = frequent_item_path + ".log";
    string fim_cmd = fim_binary + " -tc -k, -v,%S%% -s-" + toString(mine_minsup) + " -S-" + toString(mine_maxsup) + " " + transaction_path + " " + frequent_item_path + " > " + frequent_item_log_path + " 2>&1";// + COUT2NULL;
    cout << "FIM CMD: " << fim_cmd << endl;
    exec(fim_cmd);

    /// Loading frequent item set
    unordered_set<size_t> frequent_item_set;
    unordered_set<int> frequent_transaction_set; // For transposed version
    import_fim2itemset(frequent_item_path, frequent_item_set, frequent_transaction_set, run_param.qb_transpose_enable);

    /// Constructing mined bow
    build_fim_pool(_multi_bow, frequent_item_set, frequent_transaction_set, mined_bow_sig, run_param.qb_transpose_enable);
}

void qb::mining_maxbin_bow(vector<bow_bin_object*>& mined_bow_sig)
{
    /// Mining executable
    // Borgelt FPGrowth
    // Find Frequent Item Set by FP-growth
    // output item set format a,d,b:80
    // -tm = maximal
    // -k, output seperator
    // -v, support value seperator
    // -s lowerbound support percentage
    // -S lowerbound support percentage
    // -s-x lowerbound support transaction
    // -S-x lowerbound support transaction
    /* Template from fprun.sh, after run, the result will be a total number of pattern within [minsup to maxsup] transaction
    fpgrowth -tc -k, -v,%S%% -s-minsup -S-maxsup transaction_pat 2>&1 | perl -lne '/\[\K.*set\(s\)/ and print $&' | cut -d " " -f 1;
	*/
    string fim_binary = "/home/stylix/webstylix/code/datamining/borglet/Source/fpgrowth/src/fpgrowth";
    string transaction_path = _working_dir + "/" + _query_name + "_qbtrans.txt";

    /// Preparing transaction
    build_transaction(transaction_path, _multi_bow, run_param.qb_transpose_enable);

    /// Finding maximum pattern
    int step = 5;
    int limitsup = 200;
    int total_block = limitsup / step;
    int* bin_count = new int[total_block];
    cout << "==== " << redc << "MAX BIN TRACER" << endc << " ====" << endl;
    cout << "Tracing.."; cout.flush();
    #pragma omp parallel shared(step,total_block,fim_binary,transaction_path,bin_count)
    {
        #pragma omp for schedule(dynamic,1)
        // for omp reference
        // #pragma omp for schedule(dynamic,1) reduction(+ : total_kp)
        for (int run = 0; run < total_block; run++)
        {
            // Current support
            int minsup = run * step;
            int maxsup = minsup + step;
            /// Mining
            string frequent_item_path = _working_dir + "/" + _query_name + "_qbfim_minsup_-" + toString(minsup) + "_maxsup_-" + toString(maxsup) + ".txt";
            string frequent_item_log_path = frequent_item_path + ".log";
            string fim_cmd = fim_binary + " -tc -k, -v,%S%% -s-" + toString(minsup) + " -S-" + toString(maxsup) + " " + transaction_path + " " + frequent_item_path + " > " + frequent_item_log_path + " 2>&1";// + COUT2NULL;
            //cout << "FIM CMD: " << fim_cmd << endl;
            exec(fim_cmd);

            // Count bin
            bin_count[run] = import_fim2checkbin(frequent_item_path, run_param.qb_transpose_enable);

            percentout(run, total_block, 1);
        }
    }
    cout << "done!" << endl;
    int maxbin = 0;
    int maxbin_runid = 0;
    //cout << "bin:" << endl;
    for (int bin_idx = 0; bin_idx < total_block; bin_idx++)
    {
        //cout << bin_count[bin_idx] << endl;
        if (maxbin < bin_count[bin_idx])
        {
            maxbin = bin_count[bin_idx];
            maxbin_runid = bin_idx;
        }
    }
    int mine_minsup = maxbin_runid * step;
    int mine_maxsup = mine_minsup + step;
    cout << "Found max pattern at run_id:" << maxbin_runid << " " << mine_minsup << "-" << mine_maxsup << " support" << endl;

    /// Mining
    string frequent_item_path = _working_dir + "/" + _query_name + "_qbfim_minsup_-" + toString(mine_minsup) + "_maxsup_-" + toString(mine_maxsup) + ".txt";

    /// Loading frequent item set
    unordered_set<size_t> frequent_item_set;
    unordered_set<int> frequent_transaction_set; // For transposed version
    import_fim2itemset(frequent_item_path, frequent_item_set, frequent_transaction_set, run_param.qb_transpose_enable);

    /// Constructing mined bow
    build_fim_pool(_multi_bow, frequent_item_set, frequent_transaction_set, mined_bow_sig, run_param.qb_transpose_enable);

    // Release memory
    delete[] bin_count;
}

// Pooling
void qb::set_query_fg(const vector<bow_bin_object*>& query_bow)
{
    // *** Not good to do

    // Reset previous mask
    _query_fg_mask.reset();

    // Masking
    for (size_t bin_idx = 0; bin_idx < query_bow.size(); bin_idx++)
    {
        if (query_bow[bin_idx]->fg)
        {
            _query_fg_mask.set(query_bow[bin_idx]->cluster_id);
        }
    }
}

void qb::build_fim_pool(vector< vector<bow_bin_object*> >& multi_bow, const unordered_set<size_t>& item_set, const unordered_set<int>& transaction_set, vector<bow_bin_object*>& bow_sig, bool transpose)
{
    /// Merging sparse bow
    size_t multi_bow_size = multi_bow.size();
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE]();
    unordered_map<size_t, bow_bin_object*> sparse_bow;
    for (size_t bow_idx = 0; bow_idx < multi_bow_size; bow_idx++)
    {
        // Skip transaction if not frequent (use with transpose mode)
        if (transpose && transaction_set.find(bow_idx) == transaction_set.end())
        {
            //cout << "skip transaction " << bow_idx << endl;
            continue;
        }

        // Normalization before use
        bow_tools.logtf_unitnormalize(multi_bow[bow_idx]);

        // Pooling
        for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
        {
            size_t cluster_id = multi_bow[bow_idx][bin_idx]->cluster_id;

            /// Skip this word if not appear in "FIM"
            if (item_set.find(cluster_id) == item_set.end())
                continue;

            // Initial sparse_bow
            if (!non_zero_bin[cluster_id])
            {
                // Create new bin
                bow_bin_object* new_bin = new bow_bin_object();     // <-- New memory create here
                new_bin->cluster_id = cluster_id;
                new_bin->weight = 0;

                // Keep in sparse_bow
                sparse_bow[cluster_id] = new_bin;
                non_zero_bin[cluster_id] = true;
            }

            // Update sparse_bow
            if (run_param.qb_pooling_mode == QB_POOL_SUM || run_param.qb_pooling_mode == QB_POOL_AVG)
            {
                sparse_bow[cluster_id]->weight += multi_bow[bow_idx][bin_idx]->weight;     // Reuse memory from outside
                sparse_bow[cluster_id]->features.insert(sparse_bow[cluster_id]->features.end(), multi_bow[bow_idx][bin_idx]->features.begin(), multi_bow[bow_idx][bin_idx]->features.end());

                // Clear feature vector
                vector<feature_object*>().swap(multi_bow[bow_idx][bin_idx]->features);
            }
        }
    }
    /// Building compact bow
    for (auto spare_bow_it = sparse_bow.begin(); spare_bow_it != sparse_bow.end(); spare_bow_it++)
    {
        // Looking for non-zero bin of cluster,
        // then put that bin together in bow_sig by specified cluster_id

        // Averaging
        if (run_param.qb_pooling_mode == QB_POOL_AVG)
            spare_bow_it->second->weight /= multi_bow_size;

        // Keep it back to bow_sig
        bow_sig.push_back(spare_bow_it->second);
    }

    /// QB Pooling with query foreground constrain
    if (run_param.qb_query_mask_enable)
    {
		cout << "QB setup foreground.."; cout.flush();
        for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
        {
            if (_query_fg_mask[bow_sig[bin_idx]->cluster_id])
                bow_sig[bin_idx]->fg = true;
        }
		cout << "done!" << endl;
    }

    cout << "QB powerlaw..skipped" << endl;
    /*
    /// Power law
    if (run_param.powerlaw_enable)
        bow_tools.rooting_bow(bow_sig);
    */

    cout << "QB normalize..skipped" << endl;
    /*
    /// TF-IDF. Normalize
    bow_tools.logtf_idf_unitnormalize(bow_sig, _idf);
    */

    // Release memory
    delete[] non_zero_bin;
}

// Release memory
void qb::reset_multi_bow()
{
    if (_multi_bow.size())
    {
        // Clear buffer
        for (size_t bow_idx = 0; bow_idx < _multi_bow.size(); bow_idx++)
        {
            for (size_t bin_idx = 0; bin_idx < _multi_bow[bow_idx].size(); bin_idx++)
            {
                for (size_t feature_idx = 0; feature_idx < _multi_bow[bow_idx][bin_idx]->features.size(); feature_idx++)
                {
                    delete[] _multi_bow[bow_idx][bin_idx]->features[feature_idx]->kp;  // delete float* kp[]
                    delete _multi_bow[bow_idx][bin_idx]->features[feature_idx];        // delete feature_object*
                }
                vector<feature_object*>().swap(_multi_bow[bow_idx][bin_idx]->features);
                delete _multi_bow[bow_idx][bin_idx];                                   // delete bow_bin_object*
            }
            vector<bow_bin_object*>().swap(_multi_bow[bow_idx]);
        }
        vector< vector<bow_bin_object*> >().swap(_multi_bow);
    }
}

}
//;)
