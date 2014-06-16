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
#include "../sifthesaff/SIFThesaff.h"
#include "ins_param.h"
#include "gvp.h"

#include "version.h"

using namespace std;
using namespace alphautils;

namespace ins
{
// feature_id is the index relative to its image
// weight is for using with mask
typedef struct _feature_object{ size_t feature_id; float weight; float* kp; } feature_object;
// weight is for each bin
typedef struct _bow_bin_object{ size_t cluster_id; float weight; vector<feature_object*> features; } bow_bin_object;
typedef struct _dataset_object{ size_t dataset_id; float weight; vector<feature_object*> features; } dataset_object;
// bow_sig = vector<bow_bin_object>
// invert_data = array[vector<dataset_object>]

class invert_index
{
    // Run param
    ins_param run_param;

    bool mem_free;
    bool cache_free;

    size_t cluster_size;
	static const size_t DB_MAX_PARTITION = 100;
	string inv_file_path;
	size_t dataset_size;
	size_t cluster_lower_bound;
	size_t cluster_upper_bound;
	// Inverted Index Data
	size_t total_df;
	vector<dataset_object*>* inv_idx_data;
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
	bool* stopword_list;
	// df
	size_t* actual_cluster_amount;
	float* idf;
	bool* written_word;
	// Hybrid Memory Management
	int top_cache;
	bool* inv_cache_hit;
	vector< pair<size_t, size_t> > cluster_amount_sorted;
	unordered_map<size_t, size_t> mfu_cache_freq; // Most frequently mapped

    // Feature matching dump
    bool matching_dump;
    vector<string> _ImgParentPaths;
    vector<size_t> _ImgParentsIdx;
    vector<string> _ImgLists;
    string _query_path;
    string _matching_path;

    // Partitioning
    bool partition_ready;
    size_t partition_size;
	int partition_digit_length;
	int file_digit_length;
public:
	invert_index(void);
	~invert_index(void);
	// Inverted index construction
	void init(const ins_param& run_param, bool resume = false);
	bool build_header_from_bow_file();
	void add(const size_t dataset_id, const vector<bow_bin_object*>& bow_sig);
	// Inverted index IO
	void flush();
	void save_header();
	void save();
	void load_header();
	void load(int top);
	void cache_at(size_t index);
	void partition_init();
	// Inverted index meta-data
	void sort_cluster_df();
	void recalculate_idf();
	float get_idf_at(size_t cluster_id);
	float* get_idf();
	int set_stopword_list(int percent_top_stop_doc, int percent_bottom_stop_doc);
	// Memory management
	void update_cache_status(size_t index);
	// Retrieval
	size_t search(const vector<bow_bin_object*>& bow_sig, vector< pair<size_t, float> >& ret_ranklist, int sim_mode = SIM_L1, int* sim_param = NULL);
	void similarity_l1(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float* dataset_score);
	void similarity_gvp(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float* dataset_score, int* sim_param);
	// Tools
	void start_matching_dump(const string& dataset_root_dir, const vector<string>& ImgParentPaths, const vector<size_t>& ImgParentsIdx, const vector<string>& ImgLists, const vector<size_t>& dump_list, const string& query_path);
	void feature_matching_dump(const size_t dataset_id, const size_t cluster_id, const float weight, const float* dataset_kp, const float* query_kp);
	void stop_matching_dump();
	// Release memory
	void release_cache(void);
	void release_mem(void);
};

};
//;)
