/*
 * invert_index.h
 *
 *  Created on: June 16, 2012
 *      Author: Siriwat Kasamwattanarote
 */
#pragma once
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
#include <algorithm> // sort
#include <cstdlib> // exit

#include "../alphautils/alphautils.h"
#include "ins_param.h"
#include "gvp.h"

#include "version.h"

using namespace std;
using namespace alphautils;

namespace ins
{
// feature_id is the index relative to its image
// weight is for using with mask
typedef struct _feature_object{ size_t feature_id; float weight; float x; float y; float a; float b; float c; } feature_object;
// weight is for each bin
typedef struct _bow_bin_object{ size_t cluster_id; float weight; vector<feature_object> features; } bow_bin_object;
typedef struct _dataset_object{ size_t dataset_id; float weight; vector<feature_object> features; } dataset_object;
// bow_sig = vector<bow_bin_object>
// invert_data = array[vector<dataset_object>]

class invert_index
{
    size_t CLUSTER_SIZE;
    static const size_t STATIC_CLUSTER_SIZE = 1000000;   // set to highest as possible, for bitset<STATIC_CLUSTER_SIZE>
    static const size_t MAX_DATASET_SIZE = 300000;
	static const size_t DB_MAX_PARTITION = 100;
	string inv_file_path;
	size_t dataset_size;
	// Inverted Index Data
	size_t total_df;
	size_t* actual_cluster_amount; // df
	float* idf;
	deque<dataset_object>* inv_idx_data;
	/// inv_data[
	///     cluster_id 0: deque[
	///         0 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         1 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         2 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         ...
    ///     ]
	///     cluster_id 1: deque[
	///         0 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         1 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         2 [dataset_id, weight, vector[ [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], [feature_id, x, y, a, b, c], ... ]],
	///         ...
    ///     ]
    ///     ...
	///
	/// ]
	///
	// Stop word
	bitset<STATIC_CLUSTER_SIZE> stop_list;
	// Hybrid Memory Management
	int top_cache;
	bitset<STATIC_CLUSTER_SIZE> inv_cache_hit;
	deque< pair<size_t, size_t> > cluster_amount_sorted;
	unordered_map<size_t, size_t> mfu_cache_freq; // Most frequently mapped

    // Feature matching dump
    bool matching_dump;
    vector<string> _ImgParentPaths;
    vector<size_t> _ImgParentsIdx;
    vector<string> _ImgLists;
    string _query_path;
    string _matching_path;
public:
	invert_index(void);
	~invert_index(void);
	size_t init(size_t cluster_size, const string& path);
	void add(size_t dataset_id, const vector<bow_bin_object>& bow_sig);
	size_t search(const vector<bow_bin_object>& bow_sig, vector< pair<size_t, float> >& ret_ranklist, int sim_mode = SIM_L1, int* sim_param = NULL);
	void similarity_l1(const vector<bow_bin_object>& bow_sig, bitset<MAX_DATASET_SIZE>& dataset_hit, float dataset_score[]);
	void similarity_gvp(const vector<bow_bin_object>& bow_sig, bitset<MAX_DATASET_SIZE>& dataset_hit, float dataset_score[], int* sim_param);
	void start_matching_dump(const string& dataset_root_dir, const vector<string>& ImgParentPaths, const vector<size_t>& ImgParentsIdx, const vector<string>& ImgLists, const vector<size_t>& dump_list, const string& query_path);
	void feature_matching_dump(size_t dataset_id, size_t cluster_id, float weight, float dataset_kp_x, float dataset_kp_y, float dataset_kp_a, float dataset_kp_b, float dataset_kp_c, float query_kp_x, float query_kp_y, float query_kp_a, float query_kp_b, float query_kp_c);
	void stop_matching_dump();
	void update_idf();
	float get_idf(size_t cluster_id);
	void get_idf_ref(float *&ref_idf);
	void save_invfile();
	void load_invfile(int top);
	void cache_dataset(size_t index);
	void update_cache_status(size_t index);
	void reset(void);
};

};
//;)
