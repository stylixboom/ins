/*
 * qe.h
 *
 *  Created on: November 27, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
#include <bitset>

namespace ins
{
    // Anything shared with ins namespace here

class qe
{
    // Private variables and functions here
    ins_param run_param;
    const float* _idf;
    vector< vector<bow_bin_object*> > _multi_bow;
    #define BIT1M bitset<1000000>
    vector<BIT1M> _multi_bow_flag;
    BIT1M _query_fg_mask;

    /// Top-k Homography Pass
    vector<bool> topk_inlier;

    /// Bow tools
    bow bow_tools;

public:
    // Public variables and functions here
	qe(const ins_param& param, const float* idf);
	~qe(void);

	// QE I/O
	void add_bow(const vector<bow_bin_object*>& bow_sig);
	void add_bow_from_rank(const vector<result_object>& result, const int top_k);
	void set_multi_bow(vector< vector<bow_bin_object*> >& multi_bow);

    // QE,  return is inlier count
    void qe_basic(const vector<bow_bin_object*>& query_bow, vector<bow_bin_object*>& qe_bow_sig, vector<int>& inlier_count, vector<double>& ransac_score);

	// Pooling
    void set_query_fg(const vector<bow_bin_object*>& query_bow);
    void build_qe_pool(vector< vector<bow_bin_object*> >& multi_bow, vector<BIT1M>& inliers_pack, vector<bow_bin_object*>& bow_sig);

    // Memory management
	void reset_multi_bow();                                                                          // Clear bow_pool memory

};

};
//;)
