/*
 * invert_index.cpp
 *
 *  Created on: June 16, 2012
 *      Author: Siriwat Kasamwattanarote
 */

#include "invert_index.h"

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
    dataset_size = 0;
    cluster_lower_bound = cluster_size;
    cluster_upper_bound = 0;
    total_df = 0;
    inv_idx_data = new vector<dataset_object*>[cluster_size];
    stopword_list = new bool[cluster_size];
    actual_cluster_amount = new size_t[cluster_size];
    idf = new float[cluster_size];
    written_word = new bool[cluster_size];
    inv_cache_hit = new bool[cluster_size];

	for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
	{
        stopword_list[cluster_id]           = false;
		actual_cluster_amount[cluster_id]   = 0;
		idf[cluster_id]                     = 0.0f;
		written_word[cluster_id]            = false;
		inv_cache_hit[cluster_id]           = false;
	}
	cout << "OK!" << endl;


    if (resume)
    {
        /// Load inverted file header
        // Check if exist inverted file
        if (is_path_exist(run_param.invdef_path))
            load_header();
        else
        {
            cout << "Inverted index file " << run_param.invdef_path << " not found, cannot resume!" << endl;
            exit(EXIT_FAILURE)   ;
        }
    }

    // Matching dump clear flag
    matching_dump = false;

    // Partitioning
    partition_ready = false;
    partition_size = cluster_size / DB_MAX_PARTITION;
	partition_digit_length = int(log10(DB_MAX_PARTITION));
	file_digit_length = int(ceil(log10(partition_size)));

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

    /*
    dataset_size <-- pool_size, header_size
    // count actual_cluster_amount
    written_word <-- check from actual_cluster_amount
    recalculate_idf();
    total_df
    idf*cluster_size
    */

    cout << "Calculating header..."; cout.flush();
    ifstream InFile (run_param.bow_pool_path.c_str(), ios::binary);
    if (InFile)
    {
        /// Load dataset_size
        dataset_size = bow_offset.size();

        /// Reset current actual_cluster_amount
        for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
        {
            actual_cluster_amount[cluster_id] = 0;
            written_word[cluster_id] = false;
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
            size_t feature_offset_size = sizeof(size_t) + head_size * sizeof(float);
                                     // feature_id + (x + y + a + b + c)
            for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
            {
                // Cluster ID
                size_t cluster_id = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(size_t);
                /// Accumulating actual_cluster_amount
                actual_cluster_amount[cluster_id]++;
                written_word[cluster_id] = true;

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

            percentout_timeleft(bow_id, 0, dataset_size, start_time, 1000);
        }

        // Close file
        InFile.close();

        // Save header
        save_header();
    }
    else
    {
        cout << "Bow pool path incorrect! : " << run_param.bow_pool_path << endl;
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

        // Update cluster boundary (for faster access within boundary)
        if (cluster_lower_bound > cluster_id)
            cluster_lower_bound = cluster_id;
        if (cluster_upper_bound < cluster_id + 1)
            cluster_upper_bound = cluster_id + 1;

        // Prepare inv_index for new dataset
        dataset_object* new_dataset = new dataset_object();
        new_dataset->dataset_id = dataset_id;
        new_dataset->weight = bin->weight;
        swap(new_dataset->features, bin->features);  // <---- better to copy

		// Adding into inverted index table
		inv_cache_hit[cluster_id] = true;
		actual_cluster_amount[cluster_id]++;
		inv_idx_data[cluster_id].push_back(new_dataset);
	}

    // Set dataset_size by finding max dataset_id
	if (dataset_size < dataset_id)
        dataset_size = dataset_id + 1;

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
    ofstream iv_header_File(run_param.invdef_path.c_str(), ios::binary);
    if (iv_header_File.is_open())
	{
	    // Save total_df
	    iv_header_File.write(reinterpret_cast<char*>(&total_df), sizeof(total_df));

	    // Save dataset_size
        iv_header_File.write(reinterpret_cast<char*>(&dataset_size), sizeof(dataset_size));

        // Save cluster_amount
        iv_header_File.write(reinterpret_cast<char*>(actual_cluster_amount), cluster_size * sizeof(*actual_cluster_amount));

        // Save idf
        iv_header_File.write(reinterpret_cast<char*>(idf), cluster_size * sizeof(*idf));

        // Save written_word
        iv_header_File.write(reinterpret_cast<char*>(written_word), cluster_size * sizeof(*written_word));

	    // Close file
		iv_header_File.close();
	}
}

void invert_index::load_header()
{
    // Load existing header
    ifstream iv_header_File (run_param.invdef_path.c_str(), ios::binary);
    if (iv_header_File)
    {
        // Load total_df
        iv_header_File.read((char*)(&total_df), sizeof(total_df));

        // Load dataset_size
        iv_header_File.read((char*)(&dataset_size), sizeof(dataset_size));

        // Load cluster_amount
        for(size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            iv_header_File.read((char*)(&actual_cluster_amount[cluster_id]), sizeof(actual_cluster_amount[cluster_id]));

        // Load idf
        for (size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            iv_header_File.read((char*)(&idf[cluster_id]), sizeof(idf[cluster_id]));

        // Load written_word
        for (size_t cluster_id = 0; cluster_id < cluster_size; cluster_id++)
            iv_header_File.read((char*)(&written_word[cluster_id]), sizeof(written_word[cluster_id]));

        // Close file
        iv_header_File.close();
    }

    // Sort DF for being used in loading top-x frequent cluster
    sort_cluster_df();
}

void invert_index::save()
{
    // Preparer partition directory
    if (!partition_ready)
        partition_init();

    // Time check
    timespec start_time = CurrentPreciseTime();

	// Write data
	for (size_t cluster_id = cluster_lower_bound; cluster_id < cluster_upper_bound; cluster_id++)
	{
	    // Skip writing this word_id if nothing here
	    if (!inv_cache_hit[cluster_id])
            continue;

		// Building data filename
		stringstream partition_path;
		stringstream data_path;
		// cluster_id = 234567
		// Dir = cluster_id / partition_size = 234567 / 10000 = 23.xxx = 23
		// Filename = cluster_id % partition_size = 234567 % 10000 = 4567
		// DigitMax = log10(num) + 1
		partition_path << run_param.inv_path << "/" << ZeroPadNumber(cluster_id / partition_size, partition_digit_length);
		data_path << partition_path.str() << "/" << ZeroPadNumber(cluster_id % partition_size, file_digit_length) << ".dat";

        // Prevent collision (debug)
		/*if (is_path_exist(data_path.str()))
		{
			cout << "Collision at: " << data_path.str() << endl;
			cout << "Cluster: " << cluster_id << endl;
			cout << "ClusterDir: " << cluster_id / partition_size << " " <<
					"partition_path: " << cluster_id % partition_size << endl;

			cout << "Skipping.." << endl;
			continue;
		}*/

        /// Preparing buffer
        size_t buffer_size = 0;
        vector<dataset_object*> dataset_list = inv_idx_data[cluster_id];
        size_t dataset_list_size = dataset_list.size();
        size_t head_size = SIFThesaff::GetSIFTHeadSize();
        for (size_t dataset_idx = 0; dataset_idx < dataset_list_size; dataset_idx++)
        {
            size_t feature_size = dataset_list[dataset_idx]->features.size();

            buffer_size += sizeof(size_t) + sizeof(float) + sizeof(size_t) +
                           feature_size * (sizeof(size_t) + head_size * sizeof(float));
                        // sizeof(dataset_id) + sizeof(weight) + sizeof(feature_size) +
                        // feature_size * (sizeof(feature_id) + head_size * sizeof(kp))
        }
        char* buffer = new char[buffer_size];
        char* buffer_ptr = buffer;

        // Put buffer
        // Save inv_idx_data
        // Using deque is not continuous memory, write it block by block
        // iv_data_File.write(reinterpret_cast<char*>(&*(inv_idx_data[cluster_id].begin())), inv_idx_data[cluster_id].size() * sizeof(*(inv_idx_data[cluster_id].begin()))); //works if vector
        for (size_t dataset_idx = 0; dataset_idx < dataset_list_size; dataset_idx++)
        {
            dataset_object* dataset = dataset_list[dataset_idx];
            size_t feature_size = dataset->features.size();

            // Put size_t dataset_id
            *((size_t*)buffer_ptr) = dataset->dataset_id;
            buffer_ptr += sizeof(size_t);
            // Put float weight
            *((float*)buffer_ptr) = dataset->weight;
            buffer_ptr += sizeof(float);
            // Put size_t feature_size
            *((size_t*)buffer_ptr) = feature_size;
            buffer_ptr += sizeof(size_t);
            // Put vector<feature_object*>

            for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
            {
                feature_object* feature = dataset->features[feature_idx];
                // Put size_t feature_id
                *((size_t*)buffer_ptr) = feature->feature_id;
                buffer_ptr += sizeof(size_t);
                // Put x y a b c
                *((float*)buffer_ptr) = feature->kp[0];
                buffer_ptr += sizeof(float);
                *((float*)buffer_ptr) = feature->kp[1];
                buffer_ptr += sizeof(float);
                *((float*)buffer_ptr) = feature->kp[2];
                buffer_ptr += sizeof(float);
                *((float*)buffer_ptr) = feature->kp[3];
                buffer_ptr += sizeof(float);
                *((float*)buffer_ptr) = feature->kp[4];
                buffer_ptr += sizeof(float);
            }
        }

		fstream iv_data_File;
        if (written_word[cluster_id])  // If already written, open file with append mode
            iv_data_File.open(data_path.str().c_str(), ios::binary | ios::out | ios::app);
        else
            iv_data_File.open(data_path.str().c_str(), ios::binary | ios::out);
        if (iv_data_File.is_open())
        {
            // Write buffer
            iv_data_File.write(buffer, buffer_size);

            // Close file
			iv_data_File.close();

			// Flag written_word
			written_word[cluster_id] = true;
        }

        // Release buffer
        delete[] buffer;

        //percentout(cluster_id - cluster_lower_bound, cluster_upper_bound - cluster_lower_bound, 10);
        percentout_timeleft(cluster_id - cluster_lower_bound, 0, cluster_upper_bound - cluster_lower_bound, start_time, 1000);
	}

    // Save header
    save_header();
}

void invert_index::load(int top)
{
	// Hybrid Memory Management
	top_cache = top;

	// Reset inv_cache_hit and iMemLookupTable
	for(size_t idx = 0; idx < cluster_size; idx++)
		inv_cache_hit[idx] = false;
	// Calc max index to load from ordered CLS
	size_t max_load_idx = cluster_size * top_cache / 100;

    // Load x percent of total cluster into memory, and flag at inv_cache_hit[cluster_id]
	for (size_t fetch_cluster_idx = 0; fetch_cluster_idx < max_load_idx; fetch_cluster_idx++)
	{

        // Load from higher frequent cluster to lower frequent cluster
		size_t cluster_id = cluster_amount_sorted[fetch_cluster_idx].first;

        // Load if this cluster_id not appear in stop list
        if (!stopword_list[cluster_id])
            cache_at(cluster_id);

        percentout(fetch_cluster_idx, max_load_idx, 10);
	}
}

void invert_index::cache_at(size_t cluster_id)
{
	stringstream partition_path;
	stringstream data_path;

    partition_path << run_param.inv_path << "/" << ZeroPadNumber(cluster_id / partition_size, partition_digit_length);
    data_path << partition_path.str() << "/" << ZeroPadNumber(cluster_id % partition_size, file_digit_length) << ".dat";
    //cout << "Data dir: " << partition_path.str() << endl;
    //cout << "Data path: " << data_path.str() << " Length: " << cluster_amount_sorted[cluster_id].second << endl;

	size_t cluster_count = cluster_amount_sorted[cluster_id].second;

    // Load inv_idx_data
    ifstream iv_data_File (data_path.str().c_str(), ios::binary);
    if (iv_data_File)
    {
        // Set Mem Mapped flag
        inv_cache_hit[cluster_id] = true;

        // Update cluster boundary (for faster access within boundary)
        if (cluster_lower_bound > cluster_id)
            cluster_lower_bound = cluster_id;
        if (cluster_upper_bound < cluster_id + 1)
            cluster_upper_bound = cluster_id + 1;

        // Read dataset data
        for (size_t dataset_index = 0; dataset_index < cluster_count; dataset_index++)
        {
            dataset_object* read_dataset = new dataset_object();
            // Read dataset_id
            iv_data_File.read((char*)(&(read_dataset->dataset_id)), sizeof(read_dataset->dataset_id));
            // Read weight
            iv_data_File.read((char*)(&(read_dataset->weight)), sizeof(read_dataset->weight));
            // Read feature_size
            size_t feature_size = 0;
            iv_data_File.read((char*)(&feature_size), sizeof(feature_size));
            for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
            {
                feature_object* read_feature = new feature_object();
                // Read feature_id
                iv_data_File.read((char*)(&(read_feature->feature_id)), sizeof(read_feature->feature_id));
                // Read x y a b c
                int head_size = SIFThesaff::GetSIFTHeadSize();
                read_feature->kp = new float[head_size];
                for (int head_idx = 0; head_idx < head_size; head_idx++)
                    iv_data_File.read((char*)(&(read_feature->kp[head_idx])), sizeof(*(read_feature->kp)));

                // Keep feature
                read_dataset->features.push_back(read_feature);
            }
            inv_idx_data[cluster_id].push_back(read_dataset);
        }

        // Close file
        iv_data_File.close();

        // Memory flag
        cache_free = false;
    }
}

void invert_index::partition_init()
{
    // Prepare partition directory
	/*
	for (size_t partition_idx = 0; partition_idx < DB_MAX_PARTITION; partition_idx++)
	{
		string partition_path;
		// run_param.inv_path/00
		partition_path = run_param.inv_path + "/" + ZeroPadNumber(partition_idx, partition_digit_length);
		make_dir_available(partition_path);
	}*/
	// Faster by mkdir 0{0..9} && mkdir {10..99}
	string cmd = "ssh `hostname` 'mkdir -m 755 -p " + run_param.inv_path + "/0{0..9}; mkdir -m 755 -p " + run_param.inv_path + "/{10..99}';" + COUT2NULL;
	//cout << "Running script.. " << cmd << endl;
	exec(cmd);
	//cin.get();

	partition_ready = true;
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
            idf[cluster_id] = log10(float(dataset_size) / cluster_amount);
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

int invert_index::set_stopword_list(int percent_top_stop_doc, int percent_bottom_stop_doc)
{
    // Return total number of stopped doc

    // Stop word
    // Ref http://lastlaugh.inf.cs.cmu.edu/alex/BagOfVisWords.MIR07.pdf saids no stopword for visual word

    size_t total_top_stop = total_df * percent_top_stop_doc / 100;
    size_t total_bottom_stop = total_df * percent_bottom_stop_doc / 100;
    // Top stop
    size_t curr_top_stopped = 0;
    size_t curr_bottom_stopped = 0;
    for (size_t cluster_idx = 0; cluster_idx < cluster_amount_sorted.size(); cluster_idx++)
    {
        // Enough stop
        if (curr_top_stopped >= total_top_stop)
            break;
        stopword_list[cluster_amount_sorted[cluster_idx].first] = true;
        curr_top_stopped +=  cluster_amount_sorted[cluster_idx].second;
    }
    // Bottom stop
    for (size_t cluster_idx = cluster_amount_sorted.size() - 1; cluster_idx >= 0; cluster_idx--)
    {
        // Enough stop
        if (curr_bottom_stopped >= total_bottom_stop)
            break;
        stopword_list[cluster_amount_sorted[cluster_idx].first] = true;
        curr_bottom_stopped +=  cluster_amount_sorted[cluster_idx].second;
    }

    return curr_top_stopped + curr_bottom_stopped;
}

// Memory management
void invert_index::update_cache_status(size_t index)
{
    // It's about
    // inv_cache_hit
    // mfu_cache_freq
}

// Retrieval
size_t invert_index::search(const vector<bow_bin_object*>& bow_sig, vector< pair<size_t, float> >& ret_ranklist, int sim_mode, int* sim_param)
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
	cout << "Preparing..";
	cout.flush();
	timespec prepareTime = CurrentPreciseTime();
	for (size_t index = 0; index < dataset_size ; index++)
		if(dataset_hit[index])
			ret_ranklist.push_back(pair<size_t, float>(index, dataset_score[index]));
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
	sort(ret_ranklist.begin(), ret_ranklist.end(), compare_pair_second<>());
	cout << "done! (in " << setprecision(2) << fixed << TimeElapse(sortTime) << " s)" << endl;

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
	int hitpoint = 0;
	int misspoint = 0;
    // bin cluster_id | weight
	size_t bow_sig_size = bow_sig.size();

	cout << "Non-zero bin count: " << bow_sig_size << endl;

	/*for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++)
    {
        cout << "cluster_id: " << bow_sig_it->cluster_id;
        cout << "weight: " << bow_sig_it->weight << endl;
    }*/

	// For each cluster_id in bow signature
    for (size_t bin_idx = 0; bin_idx != bow_sig_size; bin_idx++)
    {
        bow_bin_object* a_query_bin = bow_sig[bin_idx];
		size_t cluster_id = a_query_bin->cluster_id;

        // Skip on stop list
        if (stopword_list[cluster_id])
            continue;

		// a_query_bin->weight = Normalized tf-idf of this bin
		float query_weight = a_query_bin->weight;

        // Inverted histogram cache hit check
		if (!inv_cache_hit[cluster_id])
		{
			// Caching
			cache_at(cluster_id);
			misspoint++;
		}
		else
			hitpoint++;

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
			dataset_hit[dataset_id] = true;
			dataset_score[dataset_id] += intersected_weight; // accumulated weight

            // Dump matching if enable
			if (matching_dump)
			{
			    // for each feature in this bin
			    //      for each feature in this matched id of a dataset
                for (size_t query_feature_idx = 0; query_feature_idx < a_query_bin->features.size(); query_feature_idx++)
                    for (size_t dataset_feature_idx = 0; dataset_feature_idx < a_dataset->features.size(); dataset_feature_idx++)
                        feature_matching_dump(dataset_id, cluster_id, a_query_bin->weight, a_dataset->features[dataset_feature_idx]->kp, a_query_bin->features[query_feature_idx]->kp);
            }
		}
    }

    cout << "Total hit: " << hitpoint << " Total miss: " << misspoint << " HitRate: " << setprecision(2) << fixed << 100.0 * hitpoint / (hitpoint + misspoint) << "%" << endl;
}

void invert_index::similarity_gvp(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float dataset_score[], int* sim_param)
{
	int hitpoint = 0;
	int misspoint = 0;

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

        // Inverted histogram cache hit check
        if(!inv_cache_hit[cluster_id])
        {
            // Caching
            cache_at(cluster_id);
            misspoint++;
        }
        else
            hitpoint++;

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
                    if (matching_dump)
                        feature_matching_dump(dataset_id, cluster_id, a_query_bin->weight, a_dataset_feature->kp, a_query_feature->kp);
                }

                // Flag existing in dataset_hit
                dataset_hit[dataset_id] = true;
            }
        }
        //cout << "done";
    }
	//cout << "done!" << endl;
	cout << "Total hit: " << hitpoint << " Total miss: " << misspoint << " HitRate: " << setprecision(2) << fixed << 100.0 * hitpoint / (hitpoint + misspoint) << "%" << endl;


	cout << "Phase 2 of 2 | Calculating space score." << endl;
	// Phase 2
	// Calculate score
	//cout << "Phase 2, calculate space score.."; cout.flush();
	gvp_processor.get_score(dataset_score);
	//cout << "done!" << endl;
}

// Dump feature matching
void invert_index::start_matching_dump(const string& dataset_root_dir, const vector<string>& ImgParentPaths, const vector<size_t>& ImgParentsIdx, const vector<string>& ImgLists, const vector<size_t>& dump_list, const string& query_path)
{
    // Matching dump param
    matching_dump = true;
    _ImgParentPaths = ImgParentPaths;
    _ImgParentsIdx = ImgParentsIdx;
    _ImgLists = ImgLists;
    _query_path = query_path;

    // Get query path
    //_matching_path = get_directory(_query_path) + "/matches";
    _matching_path = _query_path +"_matches";

    make_dir_available(_matching_path, "777");

    if (dump_list.empty())  // Normal all dump
    {
        for (size_t dataset_id = 0; dataset_id < _ImgLists.size(); dataset_id++)
        {
            // File name
            string dump_name = _matching_path + "/" + get_filename(_query_path) + "_" + _ImgLists[dataset_id] + ".matches";

            // Header
            stringstream dump_header;
            dump_header << dataset_root_dir << "/" << _ImgParentPaths[_ImgParentsIdx[dataset_id]] << "/" << _ImgLists[dataset_id] << endl;  // Dataset path
            dump_header << query_path << endl;                                                                                              // Query path

            text_write(dump_name, dump_header.str(), false);
        }
    }
    else
    {
        for (size_t dump_id = 0; dump_id < dump_list.size(); dump_id++)
        {
            // File name
            size_t dataset_id = dump_list[dump_id];
            string dump_name = _matching_path + "/" + get_filename(_query_path) + "_" + _ImgLists[dataset_id] + ".matches";

            // Header
            stringstream dump_header;
            dump_header << dataset_root_dir << "/" << _ImgParentPaths[_ImgParentsIdx[dataset_id]] << "/" << _ImgLists[dataset_id] << endl;  // Dataset path
            dump_header << query_path << endl;                                                                                              // Query path

            text_write(dump_name, dump_header.str(), false);
        }
    }
}

void invert_index::feature_matching_dump(const size_t dataset_id, const size_t cluster_id, const float weight, const float* dataset_kp, const float* query_kp)
{
    string dump_name = _matching_path + "/" +  get_filename(_query_path) + "_" + _ImgLists[dataset_id] + ".matches";

    if (is_path_exist(dump_name))
    {
        // Match data
        // cluster_id, dataset xyabc, query xyabc
        stringstream dump_data;
        dump_data << cluster_id << " " << weight << " "
        << dataset_kp[0] << " " << dataset_kp[1] << " " << dataset_kp[2] << " " << dataset_kp[3] << " " << dataset_kp[4] << " "
        << query_kp[0] << " " << query_kp[1] << " " << query_kp[2] << " " << query_kp[3] << " " << query_kp[4] << endl;

        text_write(dump_name, dump_data.str(), true);
    }
}

void invert_index::stop_matching_dump()
{
    matching_dump = false;
}

void invert_index::release_cache(void)
{
    if (!cache_free)
    {
        timespec start_time = CurrentPreciseTime();
        for (size_t word_id = cluster_lower_bound; word_id < cluster_upper_bound; word_id++)
        {
            if (inv_cache_hit[word_id])
            {
                // Release Inverted index
                size_t doc_size = inv_idx_data[word_id].size();
                for (size_t doc_idx = 0; doc_idx < doc_size; doc_idx++)
                {
                    dataset_object* doc = inv_idx_data[word_id][doc_idx];
                    vector<feature_object*>& features = inv_idx_data[word_id][doc_idx]->features;
                    size_t feature_size = features.size();
                    for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
                    {
                        delete[] features[feature_idx]->kp;             // delete float* kp[]
                        delete features[feature_idx];                   // delete feature_object*
                    }
                    features.clear();
                    delete doc;                                         // delete dataset_object*
                }
                inv_idx_data[word_id].clear();

                // Clear cache flag
                inv_cache_hit[word_id] = false;
            }

            percentout_timeleft(word_id - cluster_lower_bound, 0, cluster_upper_bound - cluster_lower_bound, start_time, 1000);
        }

        // Reset cluster boundary
        cluster_lower_bound = cluster_size;
        cluster_upper_bound = 0;

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
        delete[] written_word;
        delete[] inv_idx_data;                                                                      // delete vector<dataset_object*>*
        delete[] stopword_list;
        delete[] inv_cache_hit;

        cluster_amount_sorted.clear();

        mem_free = true;
	}
}

}
//;)
