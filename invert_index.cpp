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
    reset();

	delete[] actual_cluster_amount;
	delete[] idf;
	delete[] inv_idx_data;
}

size_t invert_index::init(size_t cluster_size, const string& path)
{
    CLUSTER_SIZE = cluster_size;
    inv_file_path = path;

    cout << "Initializing memory...";
    cout << " " << path;
	cout.flush();

    // Initializing
    total_df = 0;
    actual_cluster_amount = new size_t[CLUSTER_SIZE];
    idf = new float[CLUSTER_SIZE];
    inv_idx_data = new deque<dataset_object>[CLUSTER_SIZE];

	for(size_t cluster_id = 0; cluster_id != CLUSTER_SIZE; cluster_id++)
	{
        stop_list[cluster_id] = false;
		inv_cache_hit[cluster_id] = false;
		actual_cluster_amount[cluster_id] = 0;
		idf[cluster_id] = 0.0f;
	}
	cout << "OK!" << endl;

    /// Load inverted file header
	// Check if exist inverted file
	if (is_path_exist((inv_file_path + "/invertedhist.def")))
	{
        // Load existing invert_index Def
        ifstream iv_header_File ((inv_file_path + "/invertedhist.def").c_str(), ios::binary);
        if (iv_header_File)
        {
            // Load dataset_size
            //dataset_size = 1000000;
            iv_header_File.read((char*)(&dataset_size), sizeof(dataset_size));

            // Load cluster_amount
            for(size_t cluster_id = 0; cluster_id < CLUSTER_SIZE; cluster_id++)
            {
                // Read cluster_amount
                size_t read_cluster_amount;
                iv_header_File.read((char*)(&read_cluster_amount), sizeof(read_cluster_amount));
                total_df += actual_cluster_amount[cluster_id] = read_cluster_amount;
                // Prepare cluster amount to be sorted for high occurance caching fist
                cluster_amount_sorted.push_back(pair<size_t, size_t>(cluster_id, read_cluster_amount));
            }
            // Sort asc cluster_amount
            sort(cluster_amount_sorted.begin(), cluster_amount_sorted.end(), compare_pair_second<>());

            // Stop word
            // Ref http://lastlaugh.inf.cs.cmu.edu/alex/BagOfVisWords.MIR07.pdf saids no stopword for visual word
            // Ref http://lastlaugh.inf.cs.cmu.edu/alex/BagOfVisWords.MIR07.pdf saids no stopword for visual word
            int top_stop = 0;
            int bottom_stop = 0;
            size_t total_top_stop = total_df * top_stop / 100;
            size_t total_bottom_stop = total_df * bottom_stop / 100;
            // Top stop
            size_t curr_top_stopped = 0;
            size_t curr_bottom_stopped = 0;
            for (size_t cluster_idx = 0; cluster_idx < cluster_amount_sorted.size(); cluster_idx++)
            {
                // Enough stop
                if (curr_top_stopped >= total_top_stop)
                    break;
                stop_list[cluster_amount_sorted[cluster_idx].first] = true;
                curr_top_stopped +=  cluster_amount_sorted[cluster_idx].second;
            }
            // Bottom stop
            for (size_t cluster_idx = cluster_amount_sorted.size() - 1; cluster_idx >= 0; cluster_idx--)
            {
                // Enough stop
                if (curr_bottom_stopped >= total_bottom_stop)
                    break;
                stop_list[cluster_amount_sorted[cluster_idx].first] = true;
                curr_bottom_stopped +=  cluster_amount_sorted[cluster_idx].second;
            }

            // Load idf
            for (size_t cluster_id = 0; cluster_id < CLUSTER_SIZE; cluster_id++)
            {
                // Read idf
                float read_idf;
                iv_header_File.read((char*)(&read_idf), sizeof(read_idf));
                idf[cluster_id] = read_idf;
            }

            // Close file
            iv_header_File.close();
        }
	}
	else
        dataset_size = 0;

    // Matching dump clear flag
    matching_dump = false;

	return dataset_size;
}

void invert_index::add(size_t dataset_id, const vector<bow_bin_object>& bow_sig)
{
	vector<bow_bin_object>::const_iterator bow_sig_it;

	for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++)
	{
		size_t cluster_id = (*bow_sig_it).cluster_id;

        // Prepare inv_index for new dataset
        dataset_object new_dataset;
        new_dataset.dataset_id = dataset_id;
        new_dataset.weight = (*bow_sig_it).weight;
        new_dataset.features = (*bow_sig_it).features;

		// Adding into inverted index table
		inv_cache_hit[cluster_id] = true;
		actual_cluster_amount[cluster_id]++;
		inv_idx_data[cluster_id].push_back(new_dataset);
	}

	dataset_size++;
}

size_t invert_index::search(const vector<bow_bin_object>& bow_sig, vector< pair<size_t, float> >& ret_ranklist, int sim_mode, int* sim_param)
{
	int totalMatch = 0;
	// video id | similarity
	// Result

    // Query comparor
	bitset<MAX_DATASET_SIZE> dataset_hit; // MAX_DATASET_SIZE and dataset_size is the same
	float dataset_score[dataset_size];
	// Initialize
	for (size_t index = 0; index != dataset_size; index++)
	{
		dataset_hit[index] = false;
		dataset_score[index] = 0.0f;
	}

	/*cout << "Before search: ";
	for (size_t index = 0; index != dataset_size; index++)
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
	for (size_t index = 0; index != dataset_size; index++)
		cout << setprecision(4) << fixed << dataset_score[index] << " ";
    cout << endl;*/

	// Making vector< pair<size_t, float> > for sorting
	cout << "Preparing..";
	cout.flush();
	timespec prepareTime = CurrentPreciseTime();
	for (size_t index = 0; index != dataset_size ; index++)
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

	totalMatch = ret_ranklist.size();
	return totalMatch;
}

void invert_index::similarity_l1(const vector<bow_bin_object>& bow_sig, bitset<MAX_DATASET_SIZE>& dataset_hit, float dataset_score[])
{
	int hitpoint = 0;
	int misspoint = 0;
    // bin cluster_id | weight
	vector<bow_bin_object>::const_iterator bow_sig_it;

	cout << "Non-zero bin count: " << bow_sig.size() << endl;

	/*for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++)
    {
        cout << "cluster_id: " << bow_sig_it->cluster_id;
        cout << "weight: " << bow_sig_it->weight << endl;
    }*/

	// For each cluster_id in bow signature
    for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++)
    {
		size_t cluster_id = bow_sig_it->cluster_id;

        // Skip on stop list
        if (stop_list[cluster_id])
            continue;

		// bow_sig_it->weight = Normalized tf-idf of this bin
		float weight = bow_sig_it->weight;

        // Inverted histogram cache hit check
		if (!inv_cache_hit[cluster_id])
		{
			// Caching
			cache_dataset(cluster_id);
			misspoint++;
		}
		else
			hitpoint++;

		// cluster_id | (dataset id, weight)
		// foreach cluster_id match, accumulate similarity to each dataset
		deque<dataset_object>::iterator matched_cluster_it;
		for (matched_cluster_it = inv_idx_data[cluster_id].begin(); matched_cluster_it != inv_idx_data[cluster_id].end();  matched_cluster_it++)
		{
			// Look if exist video id in result
			size_t dataset_id = matched_cluster_it->dataset_id;
			// Histogram intersection
			float weight_intersected = weight;
			//cout << "bow weight: " << weight << " cluster_id: " << cluster_id << " dataset_id: " << dataset_id << " db weight: " << (*matched_cluster_it).weight << endl;
			// matched_cluster_it->weight = Normalized tf-idf of this bin
			if (weight_intersected > matched_cluster_it->weight)
				weight_intersected = matched_cluster_it->weight;
			dataset_hit[dataset_id] = true;
			dataset_score[dataset_id] += weight_intersected; // accumulated weight

			if (matching_dump)
			{
                vector<feature_object>::const_iterator query_feature_it;
                for (query_feature_it = bow_sig_it->features.begin(); query_feature_it != bow_sig_it->features.end(); query_feature_it++)
                {
                    vector<feature_object>::iterator dataset_feature_it; // feature from inverted_index
                    for (dataset_feature_it = matched_cluster_it->features.begin(); dataset_feature_it != matched_cluster_it->features.end(); dataset_feature_it++) // foreach feature in a bin
                    {
                        // Dump matching if enable
                        feature_matching_dump(dataset_id, cluster_id, bow_sig_it->weight, dataset_feature_it->x, dataset_feature_it->y, dataset_feature_it->a, dataset_feature_it->b, dataset_feature_it->c, query_feature_it->x, query_feature_it->y, query_feature_it->a, query_feature_it->b, query_feature_it->c);
                    }
                }
            }
		}
    }

    cout << "Total hit: " << hitpoint << " Total miss: " << misspoint << " HitRate: " << setprecision(2) << fixed << 100.0 * hitpoint / (hitpoint + misspoint) << "%" << endl;
}

void invert_index::similarity_gvp(const vector<bow_bin_object>& bow_sig, bitset<MAX_DATASET_SIZE>& dataset_hit, float dataset_score[], int* sim_param)
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
	vector<bow_bin_object>::const_iterator bow_sig_it;
	for (bow_sig_it = bow_sig.begin(); bow_sig_it != bow_sig.end(); bow_sig_it++) // foreach bin
    {
        size_t cluster_id = bow_sig_it->cluster_id;

        // Skip on stop list
        if (stop_list[cluster_id])
            continue;

        // Inverted histogram cache hit check
        if(!inv_cache_hit[cluster_id])
        {
            // Caching
            cache_dataset(cluster_id);
            misspoint++;
        }
        else
            hitpoint++;

        // Calculating many-to-many feature from query to matched dataset
        // From query
        vector<feature_object>::const_iterator query_feature_it;
        for (query_feature_it = bow_sig_it->features.begin(); query_feature_it != bow_sig_it->features.end(); query_feature_it++)
        {
            // To matched dataset
            deque<dataset_object>::iterator matched_cluster_it;
            // in the matched cluster
            for (matched_cluster_it = inv_idx_data[cluster_id].begin(); matched_cluster_it != inv_idx_data[cluster_id].end();  matched_cluster_it++)
            {
                // to all features of each dataset
                size_t dataset_id = matched_cluster_it->dataset_id;
                vector<feature_object>::iterator dataset_feature_it; // feature from inverted_index
                for (dataset_feature_it = matched_cluster_it->features.begin(); dataset_feature_it != matched_cluster_it->features.end(); dataset_feature_it++) // foreach feature in a bin
                {
                    gvp_processor.calc_motion(dataset_id, bow_sig_it->weight, idf[cluster_id], dataset_feature_it->x, dataset_feature_it->y, query_feature_it->x, query_feature_it->y);

                    // Dump matching if enable
                    if (matching_dump)
                        feature_matching_dump(dataset_id, cluster_id, bow_sig_it->weight, dataset_feature_it->x, dataset_feature_it->y, dataset_feature_it->a, dataset_feature_it->b, dataset_feature_it->c, query_feature_it->x, query_feature_it->y, query_feature_it->a, query_feature_it->b, query_feature_it->c);
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

void invert_index::feature_matching_dump(size_t dataset_id, size_t cluster_id, float weight, float dataset_kp_x, float dataset_kp_y, float dataset_kp_a, float dataset_kp_b, float dataset_kp_c, float query_kp_x, float query_kp_y, float query_kp_a, float query_kp_b, float query_kp_c)
{
    string dump_name = _matching_path + "/" +  get_filename(_query_path) + "_" + _ImgLists[dataset_id] + ".matches";

    if (is_path_exist(dump_name))
    {
        // Match data
        // cluster_id, dataset xyabc, query xyabc
        stringstream dump_data;
        dump_data << cluster_id << " " << weight << " "
        << dataset_kp_x << " " << dataset_kp_y << " " << dataset_kp_a << " " << dataset_kp_b << " " << dataset_kp_c << " "
        << query_kp_x << " " << query_kp_y << " " << query_kp_a << " " << query_kp_b << " " << query_kp_c << endl;

        text_write(dump_name, dump_data.str(), true);
    }
}

void invert_index::stop_matching_dump()
{
    matching_dump = false;
}

void invert_index::update_idf()
{
    // idf
    for (size_t cluster_id = 0; cluster_id < CLUSTER_SIZE; cluster_id++)
    {
        // If df != 0
        if (actual_cluster_amount[cluster_id])
            idf[cluster_id] = log10((float)dataset_size / actual_cluster_amount[cluster_id]);
        else
            idf[cluster_id] = 0;
    }
}

float invert_index::get_idf(size_t cluster_id)
{
    return idf[cluster_id];
}

void invert_index::get_idf_ref(float *&ref_idf)
{
    ref_idf = idf;
}

void invert_index::save_invfile()
{
	// Save invert_index Header
	ofstream iv_header_File ((inv_file_path + "/invertedhist.def").c_str(), ios::binary);
	if (iv_header_File.is_open())
	{
        // Save dataset_size
        iv_header_File.write(reinterpret_cast<char*>(&dataset_size), sizeof(dataset_size));

		// Save cluster_amount
		iv_header_File.write(reinterpret_cast<char*>(&actual_cluster_amount[0]), CLUSTER_SIZE * sizeof(actual_cluster_amount[0]));

		// Save idf
		iv_header_File.write(reinterpret_cast<char*>(&idf[0]), CLUSTER_SIZE * sizeof(idf[0]));

		// Close file
		iv_header_File.close();
	}

	// Save invert_index Data

	// Partitionning
	size_t partition_size = CLUSTER_SIZE / DB_MAX_PARTITION;
	int partition_digit_length = (int)log10(DB_MAX_PARTITION);
	int file_digit_length = (int)ceil(log10(partition_size));

	// Prepare partition directory
	/*
	for (size_t partition_idx = 0; partition_idx < DB_MAX_PARTITION; partition_idx++)
	{
		string partition_path;
		// inv_file_path/00
		partition_path = inv_file_path + "/" + ZeroPadNumber(partition_idx, partition_digit_length);
		make_dir_available(partition_path);
	}*/
	// Faster by mkdir 0{0..9} && mkdir {10..99}
	string cmd = "ssh `hostname` \"mkdir -m 755 -p " + inv_file_path + "/0{0..9}\"";
	//cout << "Running script.. " << cmd << endl;
	exec(cmd);
	cmd = "ssh `hostname` \"mkdir -m 755 -p " + inv_file_path + "/{10..99}\"";
	//cout << "Running script.. " << cmd << endl;
	exec(cmd);
	//cin.get();

	// Write data
	for (size_t cluster_id = 0; cluster_id != CLUSTER_SIZE; cluster_id++)
	{
		// Calc cluster size
		stringstream partition_path;
		stringstream data_path;
		// cluster_id = 234567
		// Dir = cluster_id / partition_size = 234567 / 10000 = 23.xxx = 23
		// Filename = cluster_id % partition_size = 234567 % 10000 = 4567
		// DigitMax = log10(num) + 1
		partition_path << inv_file_path << "/" << ZeroPadNumber(cluster_id / partition_size, partition_digit_length);
		data_path << partition_path.str() << "/" << ZeroPadNumber(cluster_id % partition_size, file_digit_length) << ".dat";

        // Prevent collision (debug)
		if (is_path_exist(data_path.str()))
		{
			cout << "Collision at: " << data_path.str() << endl;
			cout << "Cluster: " << cluster_id << endl;
			cout << "ClusterDir: " << cluster_id / partition_size << " " <<
					"partition_path: " << cluster_id % partition_size << endl;

			cout << "Skipping.." << endl;
			continue;
		}

		// Create data file
		ofstream iv_data_File (data_path.str().c_str(), ios::binary);
		if (iv_data_File.is_open())
		{
			// Save inv_idx_data
			// Using deque is not contineous memory, write it block by block
			// iv_data_File.write(reinterpret_cast<char*>(&*(inv_idx_data[cluster_id].begin())), inv_idx_data[cluster_id].size() * sizeof(*(inv_idx_data[cluster_id].begin()))); //works if vector
			deque<dataset_object>::iterator inv_idx_data_it;
			for (inv_idx_data_it = inv_idx_data[cluster_id].begin(); inv_idx_data_it != inv_idx_data[cluster_id].end(); inv_idx_data_it++)
			{

				// Write int dataset_id
				iv_data_File.write(reinterpret_cast<char*>(&(inv_idx_data_it->dataset_id)), sizeof(inv_idx_data_it->dataset_id));
				// Write float weight
				iv_data_File.write(reinterpret_cast<char*>(&(inv_idx_data_it->weight)), sizeof(inv_idx_data_it->weight));
				// Write feature_amount
				size_t feature_amount = inv_idx_data_it->features.size();
				iv_data_File.write(reinterpret_cast<char*>(&feature_amount), sizeof(feature_amount));
				// Write vector<feature_object>
				vector<feature_object>::iterator feature_it;
				for (feature_it = inv_idx_data_it->features.begin(); feature_it != inv_idx_data_it->features.end(); feature_it++)
				{
                    // Write feature_id
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->feature_id)), sizeof(feature_it->feature_id));
                    // Write x
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->x)), sizeof(feature_it->x));
                    // Write y
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->y)), sizeof(feature_it->y));
                    // Write a
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->a)), sizeof(feature_it->a));
                    // Write b
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->b)), sizeof(feature_it->b));
                    // Write c
                    iv_data_File.write(reinterpret_cast<char*>(&(feature_it->c)), sizeof(feature_it->c));
                }
			}
			// Close file
			iv_data_File.close();

            percentout(cluster_id, CLUSTER_SIZE, 10);
		}
	}
}

void invert_index::load_invfile(int top)
{
	// Hybrid Memory Management
	top_cache = top;

	// Reset inv_cache_hit and iMemLookupTable
	for(size_t idx = 0; idx < CLUSTER_SIZE; idx++)
		inv_cache_hit[idx] = false;
	// Calc max index to load from ordered CLS
	size_t max_load_idx = CLUSTER_SIZE * top_cache / 100;

	// Load x percent of class into memory, and flag on iLookupFlag and inv_cache_hit
	size_t partition_size = CLUSTER_SIZE / DB_MAX_PARTITION;
	int partition_digit_length = (int)log10(DB_MAX_PARTITION);
	int file_digit_length = (int)ceil(log10(partition_size));

	for (size_t fetch_cluster_idx = 0; fetch_cluster_idx < max_load_idx; fetch_cluster_idx++)
	{
		stringstream partition_path;
		stringstream data_path;

		size_t cluster_id = cluster_amount_sorted[fetch_cluster_idx].first;
		size_t cluster_count = cluster_amount_sorted[fetch_cluster_idx].second;

        // Skip on stop list
        if (stop_list[cluster_id])
            continue;

		partition_path << inv_file_path << "/" << ZeroPadNumber(cluster_id / partition_size, partition_digit_length);
		data_path << partition_path.str() << "/" << ZeroPadNumber(cluster_id % partition_size, file_digit_length) << ".dat";
		//cout << "Data dir: " << partition_path.str() << endl;
		//cout << "Data path: " << data_path.str() << " Length: " << cluster_amount_sorted[fetch_cluster_idx].second << endl;

		// Load inv_idx_data
		ifstream iv_data_File (data_path.str().c_str(), ios::binary);
		if (iv_data_File)
		{
			// Set Mem Mapped flag
			inv_cache_hit[cluster_id] = true;

            // Read dataset data
			for (size_t dataset_index = 0; dataset_index < cluster_count; dataset_index++)
			{
                dataset_object read_dataset;
                // Read dataset_id
				iv_data_File.read((char*)(&(read_dataset.dataset_id)), sizeof(read_dataset.dataset_id));
				// Read weight
				iv_data_File.read((char*)(&(read_dataset.weight)), sizeof(read_dataset.weight));
				// Read feature_amount
				size_t feature_amount = 0;
				iv_data_File.read((char*)(&feature_amount), sizeof(feature_amount));
				for (size_t feature_index = 0; feature_index < feature_amount; feature_index++)
				{
                    feature_object read_feature;
                    // Read feature_id
                    iv_data_File.read((char*)(&(read_feature.feature_id)), sizeof(read_feature.feature_id));
                    // Read x
                    iv_data_File.read((char*)(&(read_feature.x)), sizeof(read_feature.x));
                    // Read y
                    iv_data_File.read((char*)(&(read_feature.y)), sizeof(read_feature.y));
                    // Read a
                    iv_data_File.read((char*)(&(read_feature.a)), sizeof(read_feature.a));
                    // Read b
                    iv_data_File.read((char*)(&(read_feature.b)), sizeof(read_feature.b));
                    // Read c
                    iv_data_File.read((char*)(&(read_feature.c)), sizeof(read_feature.c));

                    // Keep feature
                    read_dataset.features.push_back(read_feature);
				}
				inv_idx_data[cluster_id].push_back(read_dataset);
			}

			// Close file
			iv_data_File.close();
		}

        percentout(fetch_cluster_idx, max_load_idx, 10);
	}
}

void invert_index::cache_dataset(size_t cluster_id)
{
	size_t partition_size = CLUSTER_SIZE / DB_MAX_PARTITION;
	int partition_digit_length = (int)log10(DB_MAX_PARTITION);
	int file_digit_length = (int)ceil(log10(partition_size));

	stringstream partition_path;
	stringstream data_path;

	size_t cluster_count = actual_cluster_amount[cluster_id];

	partition_path << inv_file_path << "/" << ZeroPadNumber(cluster_id / partition_size, partition_digit_length);
	data_path << partition_path.str() << "/" << ZeroPadNumber(cluster_id % partition_size, file_digit_length) << ".dat";
	//cout << "Data dir: " << partition_path.str() << endl;
	//cout << "Data path: " << data_path.str() << endl;

	// Load inv_idx_data
	ifstream iv_data_File (data_path.str().c_str(), ios::binary);
    if (iv_data_File)
    {
        // Set Mem Mapped flag
        inv_cache_hit[cluster_id] = true;

        // Read dataset data
        for (size_t dataset_index = 0; dataset_index < cluster_count; dataset_index++)
        {
            dataset_object read_dataset;
            // Read dataset_id
            iv_data_File.read((char*)(&(read_dataset.dataset_id)), sizeof(read_dataset.dataset_id));
            // Read weight
            iv_data_File.read((char*)(&(read_dataset.weight)), sizeof(read_dataset.weight));
            // Read feature_amount
            size_t feature_amount = 0;
            iv_data_File.read((char*)(&feature_amount), sizeof(feature_amount));
            for (size_t feature_index = 0; feature_index < feature_amount; feature_index++)
            {
                feature_object read_feature;
                // Read feature_id
                iv_data_File.read((char*)(&(read_feature.feature_id)), sizeof(read_feature.feature_id));
                // Read x
                iv_data_File.read((char*)(&(read_feature.x)), sizeof(read_feature.x));
                // Read y
                iv_data_File.read((char*)(&(read_feature.y)), sizeof(read_feature.y));
                // Read a
                iv_data_File.read((char*)(&(read_feature.a)), sizeof(read_feature.a));
                // Read b
                iv_data_File.read((char*)(&(read_feature.b)), sizeof(read_feature.b));
                // Read c
                iv_data_File.read((char*)(&(read_feature.c)), sizeof(read_feature.c));

                // Keep feature
                read_dataset.features.push_back(read_feature);
            }
            inv_idx_data[cluster_id].push_back(read_dataset);
        }

        // Close file
        iv_data_File.close();
    }
}

void invert_index::update_cache_status(size_t index)
{
    // It's about
    // inv_cache_hit
    // mfu_cache_freq
}

void invert_index::reset(void)
{
    if (cluster_amount_sorted.size() > 0)
    {
        dataset_size = 0;
        for (size_t cluster_id = 0; cluster_id < CLUSTER_SIZE; cluster_id++)
        {
            inv_cache_hit[cluster_id] = false;
            actual_cluster_amount[cluster_id] = 0;
            idf[cluster_id] = 0.0f;
            for (size_t dataset_object_id = 0; dataset_object_id < inv_idx_data[cluster_id].size(); dataset_object_id++)
                inv_idx_data[cluster_id][dataset_object_id].features.clear();
            inv_idx_data[cluster_id].clear();
        }
        cluster_amount_sorted.clear();
	}
}

}
//;)
