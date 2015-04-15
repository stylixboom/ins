/*
 * qe.cpp
 *
 *  Created on: November 27, 2014
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
#include "ins_param.h"
#include "invert_index.h"
#include "bow.h"
#include "homography.h"

#include "qe.h"

#include "version.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace ins;

namespace ins
{

qe::qe(const ins_param& param, const float* idf)
{
    run_param = param;
    _idf = idf;

    bow_tools.init(run_param);
}

qe::~qe(void)
{
    reset_multi_bow();
}

// QE I/O
void qe::add_bow(const vector<bow_bin_object*>& bow_sig)
{
    _multi_bow.push_back(bow_sig);

    /// Update sequence_id for QE
    size_t sequence_id = _multi_bow.size() - 1;
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
}

void qe::add_bow_from_rank(const vector<result_object>& result, const int top_k)
{
    /// Bow loader
    bow bow_loader;
    bow_loader.init(run_param);

    /// Load bow for top_k result
    int top_load = result.size();
    if (top_load > top_k)
        top_load = top_k;
    // Reserve memory
    vector<size_t> load_list(top_load);
    timespec loadbow_time = CurrentPreciseTime();
    cout << "QE: load multi bow.."; cout.flush();
    vector< vector<bow_bin_object*> > loaded_multi_bow(top_load);
    for (int result_idx = 0; result_idx < top_load; result_idx++)
        load_list[result_idx] = result[result_idx].dataset_id;
    bow_loader.load_running_bows(load_list, loaded_multi_bow);
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(loadbow_time) << " s)" << endl;

    /// Update sequence_id for QE
    size_t begin_sequence_id = _multi_bow.size();
    for (size_t bow_idx = 0; bow_idx < loaded_multi_bow.size(); bow_idx++)
    {
        vector<bow_bin_object*>& bow = loaded_multi_bow[bow_idx];
        for (size_t bin_idx = 0; bin_idx < bow.size(); bin_idx++)
        {
            bow_bin_object* bin = bow[bin_idx];
            for (size_t feature_idx = 0; feature_idx < bin->features.size(); feature_idx++)
            {
                bin->features[feature_idx]->sequence_id = begin_sequence_id;
            }
        }

        // Increment sequence_id
        begin_sequence_id++;
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
}

void qe::set_multi_bow(vector< vector<bow_bin_object*> >& multi_bow)
{
    // Reset not empty
    if (_multi_bow.size())
        reset_multi_bow();

    // Swap
    _multi_bow.swap(multi_bow);
}

// QE
void qe::qe_basic(const vector<bow_bin_object*>& query_bow, vector<bow_bin_object*>& qe_bow_sig, vector<int>& inlier_count, vector<double>& ransac_score)
{
    /// 1. Make homography obj
    homography homo_tool(run_param);

    /// 2. Set query to source
    // Preparing source Point2f
    /// RANSAC note ///
    /*
    This implementation is a RANSAC homography checker.
    By input two sets of 2D point cloud,
    then RANSAC will calculate its H matrix.
    *** However, we are using BoW,
    which may contains several points in one cluster
    and it may lead to miss verification.
    So, we will select only the first keypoint in each cluster,
    and hope it will be a good representative for that class.

    At least, it will reduce a burstiness problem, hopefully.
    */
    vector<Point2f> src_pts;
    vector<size_t> src_cluster_ids;
    for (size_t bin_idx = 0; bin_idx < query_bow.size(); bin_idx++)
    {
        bow_bin_object* bin = query_bow[bin_idx];
        src_pts.push_back(Point2f(bin->features[0]->kp[0], bin->features[0]->kp[1]));       // Select only the first kp, hopefully it will be good for burstiness problem
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
            ref_pts.push_back(Point2f(bin->features[0]->kp[0], bin->features[0]->kp[1]));   // Select only the first kp, hopefully it will be good for burstiness problem
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

    /// 6. Read inlier bin
    vector<BIT1M> inliers_pack;
    homo_tool.get_inliers_pack(inliers_pack);

    /// 7. Build pool with inlier
    timespec qepool_time = CurrentPreciseTime();
    cout << "QE pooling.."; cout.flush();
    build_qe_pool(_multi_bow, inliers_pack, qe_bow_sig);
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(qepool_time) << " s)" << endl;


    /// 7. Return inlier and score
    homo_tool.get_inlier_count_pack(inlier_count);
    homo_tool.get_ransac_score_pack(ransac_score);
}

// Pooling
void qe::set_query_fg(const vector<bow_bin_object*>& query_bow)
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

void qe::build_qe_pool(vector< vector<bow_bin_object*> >& multi_bow, vector<BIT1M>& inliers_pack, vector<bow_bin_object*>& bow_sig)
{
    /// Merging sparse bow
    size_t multi_bow_size = multi_bow.size();
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE]();
    unordered_map<size_t, bow_bin_object*> sparse_bow;
    //cout << "skip: ";
    for (size_t bow_idx = 0; bow_idx < multi_bow_size; bow_idx++)
    {
        // Skip this image if found to be outlier
        if (!topk_inlier[bow_idx])
        {
            cout << "Skip image #" << bow_idx << endl;
            continue;
        }

        // Normalization before use
        bow_tools.logtf_unitnormalize(multi_bow[bow_idx]);

        // Pooling
        //int skip = 0;
        for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
        {
            size_t cluster_id = multi_bow[bow_idx][bin_idx]->cluster_id;

            /// Skip this word if not appear to be RANSAC "inlier"
            if (!inliers_pack[bow_idx][cluster_id])
            {
                //skip++;
                continue;
            }

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
            if (run_param.qe_pooling_mode == QE_POOL_SUM || run_param.qe_pooling_mode == QE_POOL_AVG)
            {
                sparse_bow[cluster_id]->weight += multi_bow[bow_idx][bin_idx]->weight;     // Reuse memory from outside
                sparse_bow[cluster_id]->features.insert(sparse_bow[cluster_id]->features.end(), multi_bow[bow_idx][bin_idx]->features.begin(), multi_bow[bow_idx][bin_idx]->features.end());

                // Clear feature vector
                vector<feature_object*>().swap(multi_bow[bow_idx][bin_idx]->features);
            }
        }
        //cout << multi_bow[bow_idx].size() << "->" << skip << ", ";
    }
    //cout << endl;
    /// Building compact bow
    for (auto spare_bow_it = sparse_bow.begin(); spare_bow_it != sparse_bow.end(); spare_bow_it++)
    {
        // Looking for non-zero bin of cluster,
        // then put that bin together in bow_sig by specified cluster_id

        // Averaging
        if (run_param.qe_pooling_mode == QE_POOL_AVG)
            spare_bow_it->second->weight /= multi_bow_size;

        // Keep it back to bow_sig
        bow_sig.push_back(spare_bow_it->second);
    }

    cout << "QE setup foreground.."; cout.flush();
    /// QE Pooling with query foreground constrain
    if (run_param.qe_query_mask_enable)
    {
        for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
        {
            if (_query_fg_mask[bow_sig[bin_idx]->cluster_id])
                bow_sig[bin_idx]->fg = true;
        }
    }
    cout << "done!" << endl;

    cout << "QE powerlaw..skipped" << endl;
    /*
    /// Power law
    if (run_param.powerlaw_enable)
        bow_tools.rooting_bow(bow_sig);
    */

    cout << "QE normalize..skipped" << endl;
    /*
    /// TF-IDF. Normalize
    bow_tools.logtf_idf_unitnormalize(bow_sig, _idf);
    */

    // Release memory
    delete[] non_zero_bin;
}

// Release memory
void qe::reset_multi_bow()
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
