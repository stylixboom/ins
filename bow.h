/*
 * bow.h
 *
 *  Created on: May 12, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
// Merlin's header
#include "core.hpp"     // Mask

namespace ins
{
    // Anything shared with ins namespace here

class bow
{
    // Private variables and functions here
    ins_param run_param;

    const float MASK_THRESHOLD = 0.5f;

    // Bow
    bool bow_offset_ready;
    vector<size_t> bow_offset;
    vector< vector<bow_bin_object*> > multi_bow;
    vector<size_t> multi_bow_image_id;
    // Pooling
    bool bow_pool_offset_ready;
    vector<size_t> bow_pool_offset;
    vector< vector<bow_bin_object*> > multi_bow_pool;
public:
    // Public variables and functions here
	bow(void);
	~bow(void);
	void init(const ins_param& param);
	// Tools
	void rooting_bow(vector<bow_bin_object*>& bow_sig);                                             // Sebastien recommended to do power law
	void rooting_lastbow();                                                                         // Sebastien recommended to do power law
	int masking_lastbow(const unique_ptr<Mask>& mask, const float width = 0.0f, const float height = 0.0f);// Masking
	void logtf_unitnormalize(vector<bow_bin_object*>& bow_sig);                                     // 1+log(tf), normalize         // Use for offline dataset image indexing
	void logtf_idf_unitnormalize(vector<bow_bin_object*>& bow_sig, const float* idf);               // (1+log(tf))*idf, normalize   // Use for online query image bow extractor
	void bow_filter(vector<bow_bin_object*>& bow_sig, const size_t min_word, const size_t max_word);// Keep only wanted cluster_id
	bool load_running_bows(const vector<size_t>& image_ids, vector< vector<bow_bin_object*> >& bow_sigs);       // Load specific bow from disk, auto select between bow or bow_pool
	size_t get_running_bow_size();                                                                  // Get current running bow or bow_pool size
	vector<bow_bin_object*>& finalize_bow();                                                        // Get the reference to current multi_bow or current multi_bow_pool after finalized
	vector<bow_bin_object*>& get_bow_at(const size_t idx);                                          // Get the reference to current multi_bow
	vector<bow_bin_object*>& get_last_bow();                                                        // Get the reference to last multi_bow
	vector<bow_bin_object*>& get_bow_pool_at(const size_t idx);                                     // Get the reference to current multi_bow_pool
	// BoW
	void build_bow(const int* quantized_indices, const vector<float*>& keypoints, const size_t image_id);   // Building Bow to multi_bow
	void flush_bow(bool append);                                                                    // Flush bow to disk
	void load_bow_offset();                                                                         // Load bow_offset from disk, use once before calling load_specific_bow
	bool load_specific_bow(const size_t image_id, vector<bow_bin_object*>& bow_sig);                // Load specific bow from disk
	bool load_specific_bows(const vector<size_t>& image_ids, vector< vector<bow_bin_object*> >& bow_sigs);
	size_t get_bow_size();                                                                          // Get bow size
	void reset_bow();                                                                               // Clear bow memory
	// Pooling
	void build_pool(const int pooling_mode);                                                        // Building multi_bow_pool from current multi_bow
	void flush_bow_pool(bool append);                                                               // Flush bow_pool to disk
	void load_bow_pool_offset();                                                                    // Load bow_offset from disk, use once before calling load_specific_bow_pool
	bool load_specific_bow_pool(const size_t pool_id, vector<bow_bin_object*>& bow_sig);            // Load specific bow from disk
	bool load_specific_bow_pools(const vector<size_t>& pool_ids, vector< vector<bow_bin_object*> >& bow_sigs);
	size_t get_bow_pool_size();                                                                     // Get bow_pool size
	void reset_bow_pool();                                                                          // Clear bow_pool memory
	// Misc
	//bool export_bow();
	//bool import_bow();
	static string print(const vector<bow_bin_object*>& bow_sig, const bool print_feature = false);  // bow to string printer
	static void release_bowsig(vector<bow_bin_object*>& bow_sig);                                   // Release a specified bow_sig
};

};
//;)
