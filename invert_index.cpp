/*
 * invert_index.cpp
 *
 *  Created on: June 16, 2012
 *      Author: Siriwat Kasamwattanarote
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
#include <unordered_map>
#include <cmath>
#include <algorithm>    // sort
#include <cstdlib>      // exit

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "../sifthesaff/SIFThesaff.h"
#include "ins_param.h"
#include "kp_dumper.h"
#include "gvp.h"

#include "invert_index.h"

#include "version.h"

using namespace std;
using namespace alphautils;

namespace ins
{

invert_index::invert_index(void)
{

}

invert_index::~invert_index(void)
{
    release_mem();
}

// Inverted index construction and IO
void invert_index::init(const ins_param& param, bool resume)
{
    run_param = param;

    cout << "Initializing memory... ";
	cout.flush();

    // Initializing
    cluster_size = run_param.CLUSTER_SIZE;
    min_cluster_id = cluster_size;
    max_cluster_id = 0;
    dataset_size = 0;
    total_df = 0;
    inv_idx_data = new vector<dataset_object*>[cluster_size];
    stopword_list = new bool[cluster_size];
    actual_cluster_amount = new size_t[cluster_size];
    idf = new float[cluster_size];
    cluster_offset = new size_t[cluster_size];

	for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
	{
        stopword_list[cluster_id]           = false;
		actual_cluster_amount[cluster_id]   = 0;
		idf[cluster_id]                     = 0.0f;
		cluster_offset[cluster_id]          = 0;
	}


    /// Load inverted file header if enabled resume
    if (resume && is_path_exist(run_param.inv_header_path))
        load_header();

    // Partitioning, this might be useful later
    /*partition_ready = false;
    partition_size = cluster_size / DB_MAX_PARTITION;
	partition_digit_length = int(log10(DB_MAX_PARTITION));
	file_digit_length = int(ceil(log10(partition_size)));*/

	// Memory
    mem_free = false;
    cache_free = true;
}

bool invert_index::build_header_from_bow_file()
{
    timespec start_time = CurrentPreciseTime();

    // Set bow path upon pooling enable/disable
    string bow_path;
    string bow_offset_path;
    if (run_param.pooling_enable)
    {
        bow_path = run_param.bow_pool_path;
        bow_offset_path = run_param.bow_pool_offset_path;
    }
    else
    {
        bow_path = run_param.bow_path;
        bow_offset_path = run_param.bow_offset_path;
    }

    // Load Bow offset
    vector<size_t> bow_offset;
    cout << "Loading BOW offset..."; cout.flush();
    if (!bin_read_vector_SIZET(bow_offset_path, bow_offset))
    {
        cout << "BOW offset file does not exits, (" << bow_offset_path << ")" << endl;
        exit(-1);
    }
    cout << "done!" << endl;

    cout << "Calculating header..."; cout.flush();
    ifstream InFile (bow_path.c_str(), ios::binary);
    if (InFile)
    {
        /// Load dataset_size
        dataset_size = bow_offset.size();

        /// Reset current actual_cluster_amount
        for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
        {
            actual_cluster_amount[cluster_id] = 0;
        }

        /// Count actual_cluster_amount
        int head_size = SIFThesaff::GetSIFTHeadSize();
        for (size_t bow_id = 0; bow_id < dataset_size; bow_id++)
        {
            /// Prepare buffer
            size_t curr_offset = bow_offset[bow_id];
            size_t buffer_size = 0;
            if (bow_id < dataset_size - 1)
                buffer_size = bow_offset[bow_id + 1] - curr_offset;
            else
            {
                InFile.seekg(0, InFile.end);
                buffer_size = InFile.tellg();
                buffer_size -= curr_offset;
            }
            char* bow_buffer = new char[buffer_size];
            char* bow_buffer_ptr = bow_buffer;
            InFile.seekg(curr_offset, InFile.beg);
            InFile.read(bow_buffer, buffer_size);

            /// Bow hist
            // Dataset ID (skip)
            bow_buffer_ptr += sizeof(size_t);

            // Non-zero count
            size_t bin_count = *((size_t*)bow_buffer_ptr);
            bow_buffer_ptr += sizeof(size_t);

            // ClusterID and FeatureIDs
            size_t feature_offset_size = sizeof(size_t) + sizeof(size_t) + head_size * sizeof(float);
                                     // image_id + sequence_id + (x + y + a + b + c)
            for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
            {
                // Cluster ID
                size_t cluster_id = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(size_t);
                /// Accumulating actual_cluster_amount
                actual_cluster_amount[cluster_id]++;

                // Weight (skip)
                bow_buffer_ptr += sizeof(float);

                // Feature count
                size_t feature_count = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(size_t);

                // Features (skip)
                bow_buffer_ptr += (feature_count * feature_offset_size);
            }

            // Release bow_buffer
            delete[] bow_buffer;

            percentout_timeleft(bow_id, 0, dataset_size, start_time, 10);
        }

        // Close file
        InFile.close();

        // Save header
        save_header();
    }
    else
    {
        cout << "Bow pool path incorrect! : " << bow_path << endl;
        return false;
    }
    cout << "done!" << endl;

    return true;
}

void invert_index::add(const size_t dataset_id, const vector<bow_bin_object*>& bow_sig)
{
	size_t bow_sig_size = bow_sig.size();

    // For each bin
	for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
	{
	    bow_bin_object* bin = bow_sig[bin_idx];
		size_t cluster_id = bin->cluster_id;

        // Finding min-max cluster_id (faster to run within boundary)
        if (min_cluster_id > cluster_id)
            min_cluster_id = cluster_id;
        if (max_cluster_id < cluster_id)
            max_cluster_id = cluster_id;

        // Prepare inv_index for new dataset
        dataset_object* new_dataset = new dataset_object();
        new_dataset->dataset_id = dataset_id;
        new_dataset->weight = bin->weight;
        if (run_param.INV_mode != INV_BASIC)
            swap(new_dataset->features, bin->features);  // <---- better to copy
        else        // release memory
        {
            vector<feature_object*>& features = bin->features;
            size_t feature_size = features.size();
            for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
            {
                delete[] features[feature_idx]->kp;             // delete float* kp[]
                delete features[feature_idx];                   // delete feature_object*
            }
            vector<feature_object*>().swap(bin->features);
        }

        // Cache counting
		if (mfu_cache_freq.find(cluster_id) == mfu_cache_freq.end())
            mfu_cache_freq[cluster_id] = 0;
		mfu_cache_freq[cluster_id]++;
		actual_cluster_amount[cluster_id]++;
		// Adding into inverted index table
		inv_idx_data[cluster_id].push_back(new_dataset);
	}

    // Set dataset_size by finding max dataset_id
	if (dataset_size < dataset_id + 1)
        dataset_size = dataset_id + 1;

    if (dataset_id < 0)
    {
        cout << "Dataset id is wrong (" << dataset_id << "), please check quantized file" << endl;
        exit(EXIT_FAILURE);
    }

	// Memory flag
    cache_free = false;
}

// Inverted index IO
void invert_index::flush()
{
    cout << "Flushing.. "; cout.flush();
    save();
    cout << string(11, '\b'); cout.flush();
    cout << "Releasing memory.. "; cout.flush();
    release_cache();
    cout << string(19, '\b'); cout.flush();
}

void invert_index::save_header()
{
    // Re calculate IDF first
    recalculate_idf();

    // Save header
    ofstream inv_header_File(run_param.inv_header_path.c_str(), ios::binary);
    if (inv_header_File.is_open())
	{
	    // Save total_df
	    inv_header_File.write(reinterpret_cast<char*>(&total_df), sizeof(total_df));

	    // Save dataset_size
        inv_header_File.write(reinterpret_cast<char*>(&dataset_size), sizeof(dataset_size));

        // Save cluster_amount (document frequency)
        inv_header_File.write(reinterpret_cast<char*>(actual_cluster_amount), cluster_size * sizeof(*actual_cluster_amount));

        // Save idf
        inv_header_File.write(reinterpret_cast<char*>(idf), cluster_size * sizeof(*idf));

        // Save cluster_offset
        inv_header_File.write(reinterpret_cast<char*>(cluster_offset), cluster_size * sizeof(*cluster_offset));

	    // Close file
		inv_header_File.close();
	}
}

void invert_index::load_header()
{
    // Load existing header
    ifstream inv_header_File (run_param.inv_header_path.c_str(), ios::binary);
    if (inv_header_File)
    {
        // Load total_df
        inv_header_File.read((char*)(&total_df), sizeof(total_df));

        // Load dataset_size
        inv_header_File.read((char*)(&dataset_size), sizeof(dataset_size));

        // Load cluster_amount
        for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            inv_header_File.read((char*)(&actual_cluster_amount[cluster_id]), sizeof(actual_cluster_amount[cluster_id]));

        // Load idf
        for (size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            inv_header_File.read((char*)(&idf[cluster_id]), sizeof(idf[cluster_id]));

        // Load cluster_offset
        for (size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            inv_header_File.read((char*)(&cluster_offset[cluster_id]), sizeof(cluster_offset[cluster_id]));

        // Close file
        inv_header_File.close();
    }
    else
    {
        cout << "Missing inverted index header file: " << run_param.inv_header_path << endl;
        exit(EXIT_FAILURE);
    }

    // Sort DF for being used in loading top-x frequent cluster
    sort_cluster_df();
}

void invert_index::save()
{
    // Time check
    timespec start_time = CurrentPreciseTime();

	// Write data
	fstream inv_data_File;
    if (is_path_exist(run_param.inv_data_path))  // If already written, open file with append mode
        inv_data_File.open(run_param.inv_data_path.c_str(), ios::binary | ios::out | ios::app);
    else
        inv_data_File.open(run_param.inv_data_path.c_str(), ios::binary | ios::out);
    if (inv_data_File.is_open())
    {
        size_t current_offset = cluster_offset[min_cluster_id];
        for (size_t cluster_id = min_cluster_id; cluster_id <= max_cluster_id; cluster_id++)
        {
            vector<dataset_object*>& dataset_list = inv_idx_data[cluster_id];

            // Error checking
            size_t dataset_list_size = dataset_list.size();
            if (dataset_list_size != actual_cluster_amount[cluster_id])
            {
                cout << "ERROR! actual_cluster_amount not equal " << dataset_list_size << " - " << actual_cluster_amount[cluster_id] << " at cluster_id: " << cluster_id << endl;
                exit(EXIT_FAILURE);
            }

            // Write data
            //int head_size = SIFThesaff::GetSIFTHeadSize();
            int head_size = run_param.INV_mode;
            for (size_t dataset_idx = 0; dataset_idx < dataset_list_size; dataset_idx++)
            {
                /// INV BASIC
                dataset_object* dataset = dataset_list[dataset_idx];
                size_t feature_size = dataset->features.size();

                // Write dataset_id
                inv_data_File.write(reinterpret_cast<char*>(&dataset->dataset_id), sizeof(dataset->dataset_id));
                current_offset += sizeof(dataset->dataset_id);

                // Write weight
                inv_data_File.write(reinterpret_cast<char*>(&dataset->weight), sizeof(dataset->weight));
                current_offset += sizeof(dataset->weight);

                // Non basic inverted index
                if (run_param.INV_mode != INV_BASIC)
                {
                    /// INV 0
                    // Write feature_size
                    inv_data_File.write(reinterpret_cast<char*>(&feature_size), sizeof(feature_size));
                    current_offset += sizeof(feature_size);

                    // Write vector<feature_object*>
                    if (head_size)
                    {
                        /// INV 2,5
                        for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
                        {
                            feature_object* feature = dataset->features[feature_idx];

                            // Write image_id
                            inv_data_File.write(reinterpret_cast<char*>(&feature->image_id), sizeof(feature->image_id));
                            current_offset += sizeof(feature->image_id);

                            // Write sequence_id
                            inv_data_File.write(reinterpret_cast<char*>(&feature->sequence_id), sizeof(feature->sequence_id));
                            current_offset += sizeof(feature->sequence_id);

                            // Write x y a b c
                            inv_data_File.write(reinterpret_cast<char*>(feature->kp), head_size * sizeof(*(feature->kp)));
                            current_offset += head_size * sizeof(*(feature->kp));
                        }
                    }
                }
            }

            // Save next offset, (last cluster_id don't have its next)
            if (cluster_id < cluster_size - 1)
                cluster_offset[cluster_id + 1] = current_offset;

            //percentout(cluster_id - cluster_lower_bound, cluster_upper_bound - cluster_lower_bound, 10);
            percentout_timeleft(cluster_id, min_cluster_id, max_cluster_id, start_time, 500);
        }

        // Close file
        inv_data_File.close();
    }

    // Save header
    save_header();
}

void invert_index::load(int top)
{
	// Hybrid Memory Management
	top_cache = top;

	// Calc max index to load from ordered CLS
	size_t max_load_idx = cluster_size * top_cache / 100;

    map<size_t, size_t> cache_list;

    // Load x percent of total cluster into memory
	for (size_t fetch_cluster_idx = 0; fetch_cluster_idx < max_load_idx; fetch_cluster_idx++)
	{

        // Load from higher frequent cluster to lower frequent cluster
		size_t cluster_id = cluster_amount_sorted[fetch_cluster_idx].first;

        // Skip on stop list
        if (stopword_list[cluster_id])
            continue;

        // Accumulating cache hit
        // Hit-Miss check
        if (mfu_cache_freq.find(cluster_id) == mfu_cache_freq.end())
        {
            // Caching
            mfu_cache_freq[cluster_id] = 0;
            cache_list[cluster_id] = cluster_id;
        }
        mfu_cache_freq[cluster_id]++;
	}

	/// Caching
	cache_worker(cache_list);
}

void invert_index::cache_broker(const vector<bow_bin_object*>& bow_sig)
{
    // Cache list
    map<size_t, size_t> cache_list;

    int misspoint = 0;
    int hitpoint = 0;
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        size_t cluster_id = bow_sig[bin_idx]->cluster_id;

        // Skip on stop list
        if (stopword_list[cluster_id])
            continue;

        // Accumulating cache hit
        // Hit-Miss check
        if (mfu_cache_freq.find(cluster_id) == mfu_cache_freq.end())
        {
            // Caching
            mfu_cache_freq[cluster_id] = 0;
            cache_list[cluster_id] = cluster_id;
            misspoint++;
        }
        else
            hitpoint++;
        mfu_cache_freq[cluster_id]++;

    }
    // Caching
    cache_worker(cache_list);
    cout << "Total hit: " << hitpoint << " miss: " << misspoint << " HitRate: " << setprecision(2) << fixed << 100.0 * hitpoint / (hitpoint + misspoint) << "%" << endl;
}

void invert_index::cache_worker(const map<size_t, size_t>& cache_list)
{
    // Load inv_idx_data
    ifstream inv_data_File (run_param.inv_data_path.c_str(), ios::binary);
    if (inv_data_File)
    {
        int cache_done = 0;
        //int head_size = SIFThesaff::GetSIFTHeadSize();
        int head_size = run_param.INV_mode;
        cout << "Caching inverted index.."; cout.flush();
        for (auto cache_list_it = cache_list.begin(); cache_list_it != cache_list.end(); cache_list_it++, cache_done++)
        {
            // Get cluster_id
            size_t cluster_id = cache_list_it->first;

            // Seek to cluster_id position
            inv_data_File.seekg(cluster_offset[cluster_id]);

            // Get size of this cluster from this header
            size_t cluster_count = actual_cluster_amount[cluster_id];

            // Read dataset data
            for (size_t dataset_index = 0; dataset_index < cluster_count; dataset_index++)
            {
                /// INV BASIC
                dataset_object* read_dataset = new dataset_object();
                // Read dataset_id
                inv_data_File.read((char*)(&(read_dataset->dataset_id)), sizeof(read_dataset->dataset_id));
                // Read weight
                inv_data_File.read((char*)(&(read_dataset->weight)), sizeof(read_dataset->weight));

                // Non basic inverted index
                if (run_param.INV_mode != INV_BASIC)
                {
                    /// INV 0
                    // Read feature_size
                    size_t feature_size = 0;
                    inv_data_File.read((char*)(&feature_size), sizeof(feature_size));

                    // Read vector<feature_object*>
                    if (head_size)
                    {
                        /// INV 2,5
                        for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
                        {
                            feature_object* read_feature = new feature_object();
                            // Read image_id
                            inv_data_File.read((char*)(&(read_feature->image_id)), sizeof(read_feature->image_id));
                            // Read sequence_id
                            inv_data_File.read((char*)(&(read_feature->sequence_id)), sizeof(read_feature->sequence_id));
                            // Read x y a b c
                            read_feature->kp = new float[head_size];
                            for (int head_idx = 0; head_idx < head_size; head_idx++)
                                inv_data_File.read((char*)(&(read_feature->kp[head_idx])), sizeof(*(read_feature->kp)));

                            // Keep feature
                            read_dataset->features.push_back(read_feature);
                        }
                    }
                }
                inv_idx_data[cluster_id].push_back(read_dataset);
            }

            percentout(cache_done, cache_list.size(), 10);
        }
        cout << "done!" << string(10, ' ') << endl;

        // Close file
        inv_data_File.close();

        // Memory flag
        cache_free = false;
    }
    else
    {
        cout << "Missing inverted index data file: " << run_param.inv_data_path << endl;
        exit(EXIT_FAILURE);
    }
}

// Inverted index meta-data
void invert_index::sort_cluster_df()
{
    for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
    {
        // Prepare cluster amount to be sorted for high frequent word to be cached first
        cluster_amount_sorted.push_back(pair<size_t, size_t>(cluster_id, actual_cluster_amount[cluster_id]));
    }
    // Sort asc cluster_amount
    sort(cluster_amount_sorted.begin(), cluster_amount_sorted.end(), compare_pair_second<>());
}

void invert_index::recalculate_idf()
{
    // idf
    total_df = 0;
    for (size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
    {
        size_t cluster_amount = actual_cluster_amount[cluster_id];
        // If df != 0
        if (cluster_amount)
            idf[cluster_id] = log10(1 + (float(dataset_size) / cluster_amount));
        else
            idf[cluster_id] = 0;

        total_df += cluster_amount;
    }
}

float invert_index::get_idf_at(const size_t cluster_id)
{
    return idf[cluster_id];
}

float* invert_index::get_idf()
{
    return idf;
}

void invert_index::set_stopword_peak(int bin_amount)
{
    for (int cluster_idx = 0; cluster_idx < bin_amount; cluster_idx++)
        stopword_list[cluster_idx] = true;
}

int invert_index::set_stopword_list(int percent_top_stop_doc, int percent_bottom_stop_doc)
{
    // Return total number of stopped doc

    // Stop word
    // Ref http://lastlaugh.inf.cs.cmu.edu/alex/BagOfVisWords.MIR07.pdf saids no stopword for visual word

    size_t total_cluster_stop = 0;
    size_t total_top_stop = total_df * percent_top_stop_doc / 100;
    size_t total_bottom_stop = total_df * percent_bottom_stop_doc / 100;
    // Top stop
    size_t curr_top_stopped = 0;
    for (size_t cluster_idx = 0; cluster_idx < cluster_amount_sorted.size(); cluster_idx++)
    {
        // Enough stop
        if (curr_top_stopped >= total_top_stop)
            break;
        stopword_list[cluster_amount_sorted[cluster_idx].first] = true;
        curr_top_stopped += cluster_amount_sorted[cluster_idx].second;
        total_cluster_stop++;
    }
    // Bottom stop
    size_t curr_bottom_stopped = 0;
    for (size_t cluster_idx = cluster_amount_sorted.size() - 1; cluster_idx >= 0; cluster_idx--)
    {
        // Enough stop
        if (curr_bottom_stopped >= total_bottom_stop)
            break;
        stopword_list[cluster_amount_sorted[cluster_idx].first] = true;
        curr_bottom_stopped += cluster_amount_sorted[cluster_idx].second;
        total_cluster_stop++;
    }

    return total_cluster_stop;
}

// Memory management
void invert_index::update_cache_status(size_t index)
{
    // It's about
    // inv_cache_hit
    // mfu_cache_freq
}

// Retrieval
size_t invert_index::search(const vector<bow_bin_object*>& bow_sig, vector<result_object>& ret_ranklist, int sim_mode, int* sim_param)
{
	int totalMatch = 0;
	// video id | similarity
	// Result

    // Query comparor
	bool* dataset_hit = new bool[dataset_size];
	float* dataset_score = new float[dataset_size];
	// Reset score and match result
	for (size_t index = 0; index < dataset_size; index++)
	{
		dataset_hit[index] = false;
		dataset_score[index] = 0.0f;
	}

    /// Inverted index Caching
    cache_broker(bow_sig);

	/*cout << "Before search: ";
	for (size_t index = 0; index < dataset_size; index++)
		cout << setprecision(4) << fixed << dataset_score[index] << " ";
    cout << endl;*/

    cout << "Searching..";
	cout.flush();
	timespec searchTime = CurrentPreciseTime();
	if (sim_mode == SIM_L1)
        similarity_l1(bow_sig, dataset_hit, dataset_score);
    else
        similarity_gvp(bow_sig, dataset_hit, dataset_score, sim_param);
	cout << "done! (in " << setprecision(2) << fixed << TimeElapse(searchTime) << " s)" << endl;

    /*cout << "After search: ";
	for (size_t index = 0; index < dataset_size; index++)
		cout << setprecision(4) << fixed << dataset_score[index] << " ";
    cout << endl;*/

	// Making vector< pair<size_t, float> > for sorting
	vector< pair<result_object, float> > working_rank;
	cout << "Preparing..";
	cout.flush();
	timespec prepareTime = CurrentPreciseTime();
	for (size_t index = 0; index < dataset_size ; index++)
		if(dataset_hit[index])
			working_rank.push_back(pair<result_object, float>(result_object{index, dataset_score[index], ""}, dataset_score[index]));
	cout << "done! (in " << setprecision(2) << fixed << TimeElapse(prepareTime) << " s)" << endl;

    /*cout << "Before sort: ";
	for (size_t index = 0; index != dataset_size; index++)
		cout << ret_ranklist[index].first << ":" << ret_ranklist[index].second << " ";
    cout << endl;*/

	// Sort
	cout << "Sorting..";
	cout.flush();
	timespec sortTime = CurrentPreciseTime();
	// sort by freq, higher is better
	sort(working_rank.begin(), working_rank.end(), compare_pair_second<>());
	cout << "done! (in " << setprecision(2) << fixed << TimeElapse(sortTime) << " s)" << endl;

    // Saving
    for (auto working_rank_it = working_rank.begin(); working_rank_it != working_rank.end(); working_rank_it++)
        ret_ranklist.push_back(working_rank_it->first);

    /*cout << "After sort: ";
	for (size_t index = 0; index != dataset_size; index++)
		cout << ret_ranklist[index].first << ":" << ret_ranklist[index].second << " ";
    cout << endl;*/

    // Release memory
    delete[] dataset_score;
    delete[] dataset_hit;

	totalMatch = ret_ranklist.size();
	return totalMatch;
}

void invert_index::similarity_l1(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float* dataset_score)
{
    // bin cluster_id | weight
	size_t bow_sig_size = bow_sig.size();

	cout << "Non-zero bin count: " << bow_sig_size << endl;

	/*for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++)
    {
        cout << "cluster_id: " << bow_sig_it->cluster_id;
        cout << "weight: " << bow_sig_it->weight << endl;
    }*/

	// For each cluster_id in bow signature
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
    {
        bow_bin_object* a_query_bin = bow_sig[bin_idx];
		size_t cluster_id = a_query_bin->cluster_id;

        // Skip on stop list
        if (stopword_list[cluster_id])
            continue;

		// a_query_bin->weight = Normalized tf-idf of this bin
		float query_weight = a_query_bin->weight;

        /*
		// Asymmetric weight calculation
		if (run_param.asymmetric_enable)
        {
            float feature_weight = 1.0f;
            for (size_t feature_idx = 0; feature_idx < a_query_bin->features.size(); feature_idx++)
                feature_weight *= a_query_bin->features[feature_idx]->weight;
            query_weight *= feature_weight;
        }
        */

		// cluster_id | (dataset id, weight)
		// for each cluster_id match, accumulate similarity to each dataset
		vector<dataset_object*>& matched_dataset = inv_idx_data[cluster_id];
		size_t match_size = matched_dataset.size();
		for (size_t match_idx = 0; match_idx < match_size;  match_idx++)
		{
		    dataset_object* a_dataset = matched_dataset[match_idx];
			// Look if exist video id in result
			size_t dataset_id = a_dataset->dataset_id;
			// Histogram intersection
			float intersected_weight = query_weight;
			// cout << "bow weight: " << query_weight << " cluster_id: " << cluster_id << " dataset_id: " << dataset_id << " db weight: " << a_dataset->weight << endl;
			// a_dataset->weight = Normalized tf-idf of this bin
			if (intersected_weight > a_dataset->weight)
				intersected_weight = a_dataset->weight;

            // Asymmetric weight calculation
            if (run_param.asymmetric_enable)
            {
                if (a_query_bin->fg)
                    intersected_weight *= run_param.asym_fg_weight;
                else
                    intersected_weight *= run_param.asym_bg_weight;
            }

			dataset_hit[dataset_id] = true;
			dataset_score[dataset_id] += intersected_weight; // accumulated weight

            // Dump matching if enable
			if (run_param.matching_dump_enable)
            {
                for (auto a_dataset_features_it = a_dataset->features.begin(); a_dataset_features_it != a_dataset->features.end(); a_dataset_features_it++)
                    dumper.collect_kp(dataset_id, cluster_id, intersected_weight, a_query_bin->fg, (*a_dataset_features_it)->sequence_id, (*a_dataset_features_it)->kp);
            }
		}
    }
}

void invert_index::similarity_gvp(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float dataset_score[], int* sim_param)
{
    // Default GVP params
    /*int space_mode = 1; // DEGREE;
    int space_size = 10;
    int gvp_length = 2;*/
    int space_mode = sim_param[0]; // DEGREE;
    int space_size = sim_param[1];
    int gvp_length = sim_param[2];

	// Matched dataset
	// GVP space for dataset, for calculating score
	gvp_space gvp_processor; // doc_id, gvp_space
	gvp_processor.init_space(space_mode, space_size, gvp_length);

	cout << "Phase 1 of 2 | Calculating feature motion." << endl;
	// Phase 1
	// Calculating feature motion
	//cout << "Phase 1, calculating feature motion.."; cout.flush();
	size_t bow_sig_size = bow_sig.size();
	for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
    {
        bow_bin_object* a_query_bin = bow_sig[bin_idx];
        size_t cluster_id = a_query_bin->cluster_id;

        // Skip on stop list
        if (stopword_list[cluster_id])
            continue;

        // Calculating many-to-many feature from query to matched dataset
        // From query
        size_t query_feature_size = a_query_bin->features.size();
        for (size_t query_feature_idx = 0; query_feature_idx < query_feature_size; query_feature_idx++)
        {
            feature_object* a_query_feature = a_query_bin->features[query_feature_idx];

            // To matched dataset
            vector<dataset_object*>& matched_dataset = inv_idx_data[cluster_id];
            size_t match_size = matched_dataset.size();
            for (size_t match_idx = 0; match_idx < match_size;  match_idx++)
            {
                dataset_object* a_dataset = matched_dataset[match_idx];
                // to all features of each dataset
                size_t dataset_id = a_dataset->dataset_id;
                size_t dataset_feature_size = a_dataset->features.size();
                for (size_t dataset_feature_idx = 0; dataset_feature_idx < dataset_feature_size; dataset_feature_idx++)
                {
                    feature_object* a_dataset_feature = a_dataset->features[dataset_feature_idx];
                    gvp_processor.calc_motion(dataset_id, a_query_bin->weight, idf[cluster_id], a_dataset_feature->kp, a_query_feature->kp);

                    // Dump matching if enable
                    if (run_param.matching_dump_enable)
                        dumper.collect_kp(dataset_id, cluster_id, a_query_bin->weight, a_query_bin->fg, a_dataset_feature->sequence_id, a_dataset_feature->kp);
                }

                // Flag existing in dataset_hit
                dataset_hit[dataset_id] = true;
            }
        }
        //cout << "done";
    }
	//cout << "done!" << endl;

	cout << "Phase 2 of 2 | Calculating space score." << endl;
	// Phase 2
	// Calculate score
	//cout << "Phase 2, calculate space score.."; cout.flush();
	gvp_processor.get_score(dataset_score);
	//cout << "done!" << endl;
}

// Tools
// kp dumper
void invert_index::dump(const string& out_path, const vector<size_t>& dump_ids, const vector<string>& img_roots, const vector< vector<string> >& img_paths)
{
    dumper.dump(out_path, dump_ids, img_roots, img_paths);
}

// Release memory
void invert_index::release_cache(void)
{
    if (!cache_free)
    {
        // Reset fast boundary
        min_cluster_id = cluster_size;
        max_cluster_id = 0;

        timespec start_time = CurrentPreciseTime();
        size_t total_delete = 0;
        for (auto mfu_cache_freq_it = mfu_cache_freq.begin(); mfu_cache_freq_it != mfu_cache_freq.end(); mfu_cache_freq_it++)
        {
            // Get word_id
            size_t word_id = mfu_cache_freq_it->first;

            // Release Inverted index
            size_t doc_size = inv_idx_data[word_id].size();
            for (size_t doc_idx = 0; doc_idx < doc_size; doc_idx++)
            {
                vector<feature_object*>& features = inv_idx_data[word_id][doc_idx]->features;
                size_t feature_size = features.size();
                for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
                {
                    delete[] features[feature_idx]->kp;             // delete float* kp[]
                    delete features[feature_idx];                   // delete feature_object*
                }
                vector<feature_object*>().swap(features);
                delete inv_idx_data[word_id][doc_idx];              // delete dataset_object*
            }
            vector<dataset_object*>().swap(inv_idx_data[word_id]);

            percentout_timeleft(total_delete++, 0, mfu_cache_freq.size(), start_time, 1000);
        }

        unordered_map<size_t, int>().swap(mfu_cache_freq);
        cache_free = true;
    }
}

void invert_index::release_mem(void)
{
    // Release all
    if (!mem_free)
    {
        release_cache();

        total_df = 0;
        dataset_size = 0;

        delete[] idf;
        delete[] actual_cluster_amount;
        delete[] cluster_offset;
        delete[] inv_idx_data;                                                                      // delete vector<dataset_object*>*
        delete[] stopword_list;
        unordered_map<size_t, int>().swap(mfu_cache_freq);

        vector< pair<size_t, size_t> >().swap(cluster_amount_sorted);

        mem_free = true;
	}
}

}
//;)
