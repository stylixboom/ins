/*
 * homography.h
 *
 *  Created on: November 27, 2014
 *      Author: Siriwat Kasamwattanarote
 */

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
#pragma once
#include <bitset>

using namespace std;

namespace ins
{

class homography
{
    // Private variables and functions here
    ins_param run_param;

    #define BIT1M bitset<1000000>

    BIT1M src_mask;                                 // Mask of cluster_ids of query
    unordered_map<size_t, Point2f> src_pts_map;     // Map cluster_id -> point

    // RANSAC variables
    vector<double*> u_pack;                         // vector[[x1, y1, z1, x2, y2, z2] * total_match]
    vector< vector<size_t> > ref_cluster_ids_pack;  // vector[vector[cls1]]
    vector<int> match_size_pack;
    vector<int> inlier_count_pack;
    vector<double> ransac_score_pack;
    vector<BIT1M> inlier_bin_pack;

    // RANSAC inlier
    int inlier_thre;

public:
	homography(const ins_param& param);
	~homography(void);
	// Functions
	void set_src_2dpts(const vector<Point2f>& src_2dpts, const vector<size_t>& cluster_ids);
	void add_ref_2dpts(const vector<Point2f>& ref_2dpts, const vector<size_t>& cluster_ids);
	void calc_homo();
	BIT1M& get_inliers_at(const size_t ref_idx);
	void get_inliers_pack(vector<BIT1M>& inlier_pack);
	void get_inlier_count_pack(vector<int>& inlier_count);
	void get_ransac_score_pack(vector<double>& ransac_score);
	int calc_adint();                                               /// Calculate Adaptive Inlier Threshold
	void verify_topk(vector<bool>& topk_inlier, int inl_th = 5);    /// Verify topk, return is vector<bool> topk_inlier
	// Tools
	void normalise2dpts(vector<Point2f>& pts);
	void reset(void);
};

};
//;)
