/*
 * invert_index.h
 *
 *  Created on: June 16, 2012
 *      Author: Siriwat Kasamwattanarote
 */
#pragma once
// Siriwat's header
#include "ins_param.h"
#include "kp_dumper.h"

namespace ins
{
// feature_id is the index relative to its image
// weight is for using with mask
typedef struct _feature_object{ size_t image_id; size_t sequence_id; float weight; float* kp; } feature_object;
// weight is for each bin
typedef struct _bow_bin_object{ size_t cluster_id; float weight; bool fg = false; vector<feature_object*> features; } bow_bin_object;
typedef struct _dataset_object{ size_t dataset_id; float weight; vector<feature_object*> features; } dataset_object;
// bow_sig = vector<bow_bin_object>
// invert_data = array[vector<dataset_object>]
typedef struct _result_object{ size_t dataset_id; float score; string info; } result_object;

class invert_index
{
    // Run param
    ins_param run_param;

    bool mem_free;
    bool cache_free;

    size_t cluster_size;
    size_t min_cluster_id;
    size_t max_cluster_id;
	// Inverted Index Header
	size_t total_df;
	size_t dataset_size;
	size_t* actual_cluster_amount;  // df
	float* idf;                     // idf
	size_t* cluster_offset;
	// Inverted Index Data
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
	// Hybrid Memory Management
	int top_cache;
	vector< pair<size_t, size_t> > cluster_amount_sorted;                   // vector<cluster_id, freq>
	unordered_map<size_t, int> mfu_cache_freq; // Most frequently mapped    // cluster_id, visited

    // Dumper
	kp_dumper dumper;
public:
	invert_index(void);
	~invert_index(void);
	// Inverted index construction
	void init(const ins_param& run_param, bool resume = true);
	bool build_header_from_bow_file();
	void add(const size_t dataset_id, const vector<bow_bin_object*>& bow_sig);
	// Inverted index IO
	void flush();
	void save_header();
	void save();
	void load_header();
	void load(int top);
	void cache_broker(const vector<bow_bin_object*>& bow_sig);
	void cache_worker(const map<size_t, size_t>& cache_list);
	// Inverted index meta-data
	void sort_cluster_df();
	void recalculate_idf();
	float get_idf_at(size_t cluster_id);
	float* get_idf();
	void set_stopword_peak(int bin_amount);
	int set_stopword_list(int percent_top_stop_doc, int percent_bottom_stop_doc);
	// Memory management
	void update_cache_status(size_t index);
	// Retrieval
	size_t search(const vector<bow_bin_object*>& bow_sig, vector<result_object>& ret_ranklist, int sim_mode = SIM_L1, int* sim_param = NULL);
	void similarity_l1(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float* dataset_score);
	void similarity_gvp(const vector<bow_bin_object*>& bow_sig, bool* dataset_hit, float* dataset_score, int* sim_param);
	// Tools
	// kp dumper
	void dump(const string& out_path, const vector<size_t>& dump_ids, const vector<string>& img_roots, const vector< vector<string> >& img_paths);
	// Release memory
	void release_cache(void);
	void release_mem(void);
};

};
//;)
