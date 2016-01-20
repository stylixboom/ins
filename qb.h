/*
 * qb.h
 *
 *  Created on: July 30, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
#include <bitset>

namespace ins
{
    // Anything shared with ins namespace here

class qb
{
    // Private variables and functions here
    ins_param run_param;
    const float* _idf;
    vector< vector<bow_bin_object*> > _multi_bow;
    #define BIT1K bitset<1000>
    #define BIT1M bitset<1000000>
    vector<BIT1M> _multi_bow_flag;
    BIT1M _query_fg_mask;
    string _working_dir;
    string _query_name;
	
	/// Sequence id for image/frame of video
	int sequence_offset = 1000;

    /// Top-k Homography Pass
    bool ransac_check;
    vector<bool> topk_inlier;
    vector<int> topk_ransac_inlier;
    vector<double> topk_ransac_score;
    int actual_topk;

    /// Bow tools
    bow bow_tools;

public:
    // Public variables and functions here
	qb(const ins_param& param, const float* idf, const string& query_name, const string& working_dir = "");
	~qb(void);

    // QB I/O
    void add_bow(const vector<bow_bin_object*>& bow_sig);
	void add_bow_from_rank(const vector<result_object>& result, const int top_k);
	void set_multi_bow(vector< vector<bow_bin_object*> >& multi_bow);
	void build_transaction(const string& out, const vector< vector<bow_bin_object*> >& multi_bow, bool transpose = false);
	size_t import_fim2checkbin(const string& in, bool transpose = false);
	void import_fim2itemset(const string& in, unordered_set<size_t>& item_set, unordered_set<int>& transaction_set, bool transpose = false);
	void import_fim2itemweight(const string& in, unordered_map<size_t, float>& item_weight, bool transpose = false);

    // Top-k Homography check with RANSAC
    void topk_ransac_check(const vector<bow_bin_object*>& query_bow);

	// Mining                                                // Query Expansion basic, run with ransac
	void mining_fim_bow(const int minsup, const int maxsup, vector<bow_bin_object*>& mined_bow_sig);        // Original FIM, with specified minsup, maxsup, transpose
	void mining_maxpat_bow(vector<bow_bin_object*>& mined_bow_sig);                                         // Tracing support to find the maximum number of pattern
	void mining_maxbin_bow(vector<bow_bin_object*>& mined_bow_sig);                                         // Tracing support to find the maximum number of bin

	// Pooling
    void set_query_fg(const vector<bow_bin_object*>& query_bow);
	void build_fim_pool(vector< vector<bow_bin_object*> >& multi_bow, const unordered_set<size_t>& item_set, const unordered_set<int>& transaction_set, vector<bow_bin_object*>& bow_sig, bool transpose = false);

	// Memory management
	void reset_multi_bow();                                                                          // Clear bow_pool memory

};

};
//;)
