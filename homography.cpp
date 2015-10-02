/*
 * homography.cpp
 *
 *  Created on: November 27, 2014
 *      Author: Siriwat Kasamwattanarote
 */
#include <ctime>
#include <bitset>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>     // usleep
#include <memory>       // uninitialized_fill_n
#include <omp.h>
#include <map>

#include "opencv2/core/core.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"

// RANSAC is C code, not C++
extern "C" {
#include "../ransac/ransac.h"
}

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "invert_index.h"

#include "homography.h"

#include "version.h"

using namespace std;
using namespace cv;
using namespace alphautils;

namespace ins
{

homography::homography(const ins_param& param)
{
    // Initialize
    run_param = param;
    src_mask.reset();
}

homography::~homography(void)
{
	reset();
}

void homography::set_src_2dpts(const vector<Point2f>& src_2dpts, const vector<size_t>& cluster_ids)
{
    /// Keep all point to source, ready to use
    size_t total_point = src_2dpts.size();
    for (size_t pts_idx = 0; pts_idx < total_point; pts_idx++)
    {
        // Mask non_zero
        src_mask.set(cluster_ids[pts_idx]);
        // Map cluster_id -> Point2f
        src_pts_map[cluster_ids[pts_idx]] = src_2dpts[pts_idx];
    }
}

// Functions
void homography::add_ref_2dpts(const vector<Point2f>& ref_2dpts, const vector<size_t>& cluster_ids)
{
    size_t total_point = ref_2dpts.size();
    int total_match = 0;

    /// Find only the matched point (same cluster_id as source), to make U
    vector<Point2f> query_match_pts;
    vector<Point2f> ref_match_pts;
    // Cluster id pack
    ref_cluster_ids_pack.push_back(vector<size_t>());
    vector<size_t>& _cluster_ids = ref_cluster_ids_pack.back();
    for (size_t pts_idx = 0; pts_idx < total_point; pts_idx++)
    {
        size_t cluster_id = cluster_ids[pts_idx];
        // Check if match to src
        if (src_mask[cluster_id])
        {
            query_match_pts.push_back(src_pts_map[cluster_id]);
            ref_match_pts.push_back(ref_2dpts[pts_idx]);
            _cluster_ids.push_back(cluster_id);

            total_match++;
        }
    }

    /// Normalize 2D Points
	normalise2dpts(query_match_pts);
	normalise2dpts(ref_match_pts);

    /// Create new memory here, PLEASE take care!!
    int udim = 6;
    double* u = new double[udim * total_match];            // [x1, y1, z1, x2, y2, z2] * total_match
    for (int match_idx = 0; match_idx < total_match; match_idx++)
    {
        /// Pack source point x1, y1, z1 and reference point x2, y2, z2 together
        int offset = match_idx * udim;
        u[offset + 0] = query_match_pts[match_idx].x;       // x1
        u[offset + 1] = query_match_pts[match_idx].y;       // y1
        u[offset + 2] = 1;                                  // z1
        u[offset + 3] = ref_match_pts[match_idx].x;         // x2
        u[offset + 4] = ref_match_pts[match_idx].y;         // y2
        u[offset + 5] = 1;                                  // z2
    }
    /// Keep to pack
    u_pack.push_back(u);
    match_size_pack.push_back(total_match);

}

void homography::calc_homo()
{
    /// HOMOGRAPHY parameters

    /* Caizhi original

	TC_PAIRS = double([X1;X2]);
    THRESHOLD = 0.001;
    LO = int32(1);
    CONF = .95;
    INL_LIMIT = 28; (doc 49 or 28)
    MAX_SAMPLES = 1000;
    [H,inliers] = loransacH(TC_PAIRS, THRESHOLD, LO, CONF, INL_LIMIT, MAX_SAMPLES);
	*/

    double ERR_THRESHOLD = 0.001f; /*px (points have gaussian noise with sigma = 0.1 px)*/
	double CONF = .95f;
	int INL_LIMIT = 28;
	int MAX_SAMPLES = 3000;
	int DO_LO = 1;

	size_t total_u_size = u_pack.size();

	/// Initial result space
	inlier_bin_pack.resize(total_u_size);
	inlier_count_pack.resize(total_u_size, 0);
	ransac_score_pack.resize(total_u_size, 0.0f);

    /// Process RANSAC
    //cout << "inlier: ";
    #pragma omp parallel shared(total_u_size)
    {
        #pragma omp for schedule(dynamic,1)
        for (size_t ref_idx = 0; ref_idx < total_u_size; ref_idx++)
        {
            double* u = u_pack[ref_idx];
            int match_size = match_size_pack[ref_idx];

            // Homography can be checked for at least 4 points
            if (match_size <= 3)
               continue;

            uchar* inlier = new uchar[match_size];
            // might not necessary fill_n(inlier, match_size, 0);

            double H[3*3];
            Score scr;

            scr = ransacH(u, match_size, ERR_THRESHOLD, CONF, MAX_SAMPLES, H, inlier, DO_LO, INL_LIMIT);

            /// Count inlier and set inlier bin in BIT1M
            int inlier_count = 0;
            BIT1M& inlier_bin = inlier_bin_pack[ref_idx];
            for (int inlier_idx = 0; inlier_idx < match_size; inlier_idx++)
            {
                /// Inlier check for each point
                if (inlier[inlier_idx])
                {
                    inlier_count++;
                    inlier_bin.set(ref_cluster_ids_pack[ref_idx][inlier_idx]);
                }
            }

            /// Keep to pack
            inlier_count_pack[ref_idx] = inlier_count;  // same as scr.I
            ransac_score_pack[ref_idx] = scr.J;

            //cout << match_size << "->"  << inlier_count << ", ";

            // Release memory
            delete[] inlier;
        }
    }
    //cout << endl;

}

BIT1M& homography::get_inliers_at(const size_t ref_idx)
{
    return inlier_bin_pack[ref_idx];
}

void homography::get_inliers_pack(vector<BIT1M>& inlier_pack)
{
    inlier_pack.swap(inlier_bin_pack);
}

// Not return just reference because it will be used far away
void homography::get_inlier_count_pack(vector<int>& inlier_count)
{
    inlier_count.swap(inlier_count_pack);
}

void homography::get_ransac_score_pack(vector<double>& ransac_score)
{
    ransac_score.swap(ransac_score_pack);
}

/// The Official Adaptive Inlier Threshold algorithm
/// the return value thre(int) is to separate between outlier and inlier
/// So, if you want inlier, use this condition if(inlier_val >= thre) --> then do stuff as inlier
int homography::calc_adint()
{
    cout << "Threshold mode: " << bluec << "ADINT (Adaptive Inlier Threshold)" << endc << endl;

    // Total running topk
    size_t total_u_size = u_pack.size();

    /// Calculate auto threshold
    /*
    1. Build inlier histogram
    2. Sort and find max of inlier freq
    3. Auto threshold is the first freq lower than SNR threshold from its maximum freq
    */

    // Build inlier histogram
    map<int, int> inlier_histrogram;
    for (size_t bow_idx = 0; bow_idx < total_u_size; bow_idx++)
    {
        int inlier = inlier_count_pack[bow_idx];
        if (inlier_histrogram.find(inlier) == inlier_histrogram.end())
            inlier_histrogram[inlier] = 0;

        inlier_histrogram[inlier]++;
    }
    cout << "Ransac inlier" << endl;
    for (size_t bow_idx = 0; bow_idx < total_u_size; bow_idx++)
    {
        cout << inlier_count_pack[bow_idx] << endl;
    }

    // Convert MAP to vector
    vector< pair<int, int> > inlier_hist_space;
    for (auto hist_it = inlier_histrogram.begin(); hist_it != inlier_histrogram.end(); hist_it++)
        inlier_hist_space.push_back(pair<int, int>(hist_it->first, hist_it->second));

    int adint = 0;
    /// Not possible to calculate ADINT
    if (inlier_histrogram.size() <= 3)
    {
        // return highest threshold to fixed 2 top-k as inliers
        return inlier_histrogram[inlier_histrogram.size() - 1];
    }
    // ADINT Calculation
    else
    {
        // Find max
        size_t inlier_max_idx = 0;
        int inlier_max = 0;
        int highest_freq = 0;
        int inlier_min = 0;
        int lowest_freq = 100000;   // Set low to very high at first
        cout << "Inlier histogram label -> freq:" << endl;
        // Find MAX
        for (size_t hist_idx = 0; hist_idx < inlier_hist_space.size(); hist_idx++)
        {
            // Printing label
            cout << inlier_hist_space[hist_idx].first << "\t-> " << inlier_hist_space[hist_idx].second << endl;

            if (highest_freq < inlier_hist_space[hist_idx].second)
            {
                inlier_max_idx = hist_idx;
                inlier_max = inlier_hist_space[hist_idx].first;
                highest_freq = inlier_hist_space[hist_idx].second;
            }
        }
        // Find MIN, which is lowest next to max
        for (size_t hist_idx = inlier_max_idx + 1; hist_idx < inlier_hist_space.size(); hist_idx++)
        {
            if (lowest_freq > inlier_hist_space[hist_idx].second)
            {
                inlier_min = inlier_hist_space[hist_idx].first;
                lowest_freq = inlier_hist_space[hist_idx].second;
            }
        }
        cout << "Peak: " << inlier_max << " freq: " << highest_freq << endl;
        cout << "Valley: " << inlier_min << " freq: " << lowest_freq << endl;

        // Set threshold
        adint = inlier_min;
        // Finding the first higher inlier that has radius distance more than SNR
        // a = highest_freq - curr_freq
        // b = inlier_curr - inlier_max
        // a^2 + b^2 = c^2
        // c = sqrt(a^2 + b^2)
        // if (highest_freq * (1 - thre) <= c)
        //      if (found next peak inside the radius)
        //          ** The peak inside the radius is the inlier freq that has step > the average step inside the radius
        float curr_step = 0.0f;
        float next_step = 0.0f;
        float average_step = 0.0f;
        float sum_step = 0.0f;
        int step_count = 0;
        // Start search next after max
        for (size_t running_adint_idx = inlier_max_idx + 1; running_adint_idx < inlier_hist_space.size() - 2; running_adint_idx++)
        {
            float a = highest_freq - inlier_hist_space[running_adint_idx].second;
            float b = inlier_hist_space[running_adint_idx].first - inlier_max;
            float c = sqrt(a * a + b * b);
            cout << "SNR " << highest_freq * (1 - run_param.qe_ransac_adint_snr_threshold) << " <= " << c << " ";

            // Calculate step
            curr_step = inlier_hist_space[running_adint_idx].first - inlier_hist_space[running_adint_idx - 1].first;
            sum_step += curr_step;
            step_count++;
            cout << "\tstep: " << curr_step << endl;;

            if (b > 0 &&
                highest_freq * (1 - run_param.qe_ransac_adint_snr_threshold) <= c)
            {
                cout << "Found ADINT at " << inlier_hist_space[running_adint_idx].first << " -> " << inlier_hist_space[running_adint_idx].second << endl;
                cout << "ADINT SNR is " << highest_freq * (1 - run_param.qe_ransac_adint_snr_threshold) << " <= " << c << endl;
                adint = inlier_hist_space[running_adint_idx].first;

                // Average step
                average_step = sum_step / step_count;
                // Check big step in front
                next_step = inlier_hist_space[running_adint_idx + 1].first - inlier_hist_space[running_adint_idx].first;
                cout << "Next step: " << next_step << " > " << average_step << endl;
                if (next_step > average_step)
                {
                    cout << "Found big step next after ADINT" << endl
                        << "step: " << inlier_hist_space[running_adint_idx].first << " -> " << inlier_hist_space[running_adint_idx + 1].first << " "
                        << "avg: " << next_step << " > " << average_step << endl;
                    adint = inlier_hist_space[running_adint_idx + 1].first;
                    cout << "Suggested next ADINT: " << redc << adint << endc << endl;
                }

                break;
            }
        }
        // Force ADINT lower bound
        if (adint <= 6)
        {
            cout << "Fixed ADINT lower bound to 6." << endl;
            adint = 6;
        }
    }
    cout << "Auto threshold: " << greenc << adint << endc << endl;

    if (run_param.qe_ransac_adint_manual)
    {
        cout << "Select manual threshold (found ADINT: " << adint << ") : "; cout.flush();
        cin >> adint;
    }

    return adint;
}
/*
int homography::calc_adint()
{
    cout << "Threshold mode: " << bluec << "ADINT (Adaptive Inlier Threshold)" << endc << endl;

    // Total running topk
    size_t total_u_size = u_pack.size();

    /// Calculate auto threshold

    // 1. Find mode of inlier count
    // 2. Auto threshold is one step higher than mode

    map<int, int> inlier_histrogram;
    // Counting
    for (size_t bow_idx = 0; bow_idx < total_u_size; bow_idx++)
    {
        int inlier = inlier_count_pack[bow_idx];
        if (inlier_histrogram.find(inlier) == inlier_histrogram.end())
            inlier_histrogram[inlier] = 0;

        inlier_histrogram[inlier]++;
    }
    cout << "Ransac inlier" << endl;
    for (size_t bow_idx = 0; bow_idx < total_u_size; bow_idx++)
    {
        cout << inlier_count_pack[bow_idx] << endl;
    }
    // Find mode
    int inlier_mode1 = 0;
    int inlier_mode2 = 0;
    int highest_freq1 = 0;
    int highest_freq2 = 0;
    cout << "Inlier histogram label:" << endl;
    for (auto hist_it = inlier_histrogram.begin(); hist_it != inlier_histrogram.end(); hist_it++)
    {
        cout << hist_it->first << endl;
        if (highest_freq1 < hist_it->second)
        {
            inlier_mode1 = hist_it->first;
            highest_freq1 = hist_it->second;
        }
    }
    cout << "Inlier histogram freq:" << endl;
    for (auto hist_it = inlier_histrogram.begin(); hist_it != inlier_histrogram.end(); hist_it++)
    {
        cout << hist_it->second << endl;
        if (highest_freq2 < hist_it->second && inlier_mode1 < hist_it->first)
        {
            inlier_mode2 = hist_it->first;
            highest_freq2 = hist_it->second;
        }
    }
    // Set threshold
    cout << "First mode: " << inlier_mode1 << " freq: " << highest_freq1 << endl;
    cout << "Second mode: " << inlier_mode2 << " freq: " << highest_freq2 << endl;
    /// Adaptive Inlier Threshold
    int adint = inlier_mode1 + 1;
    // Checking if signal-to-noise (SNR) (inl2/inl1) is higher than SNR ratio
    // if higher, this signal is also noise
    // if lower, this is a good signal. Then keep signal, remove noise
    if (float(highest_freq2) / highest_freq1 > run_param.qe_ransac_adint_snr_threshold)
    {
        cout << "This inlier " << inlier_mode2 << " still contains noise (" << float(highest_freq2) / highest_freq1 << " > " << run_param.qe_ransac_adint_snr_threshold << "), use next threshold step" << endl;
        adint = inlier_mode2 + 1;
    }
    cout << "Auto threshold: " << greenc << adint << endc << endl;
    if (run_param.qe_ransac_adint_manual)
    {
        cout << "Select threshold (auto: " << adint << ") : "; cout.flush();
        cin >> adint;
    }

    return adint;
}
*/

void homography::verify_topk(vector<bool>& topk_inlier, int inl_th)
{
    // Total running topk
    size_t total_u_size = u_pack.size();

    /// Check ADINT mode
    if (!inl_th)
        inl_th = calc_adint();  /// Calculate Adaptive Inlier Threshold

    cout << "Verifying topk.. " << bluec << "thre" << endc << " = " << redc << inl_th << endc << endl;

    /// 8. Counting topk inlier
    topk_inlier.resize(total_u_size, false);       // Prepare space for topk inlier flag
    int actual_topk = 0;                                // Actual topk, to be counted from inlier
    for (size_t bow_idx = 0; bow_idx < total_u_size; bow_idx++)
    {
        /// Verifying inlier count for each topk
        if (inlier_count_pack[bow_idx] >= inl_th)
        {
            topk_inlier[bow_idx] = true;
            actual_topk++;
        }
    }
    cout << "Actual top-k transaction: " << redc << total_u_size << endc << "->" << bluec << actual_topk << endc << endl;

    if (run_param.qb_enable && actual_topk == 0)
    {
        cout << "No top-k pass the threshold (" << inl_th << ")" << endl;
        cout << redc << "Assumed the first 2 top-ks are inlier." << endc << endl;
        topk_inlier[0] = true;
        topk_inlier[1] = true;
        actual_topk = 2;
    }
}

// Tools
void homography::normalise2dpts(vector<Point2f>& pts)
{
    // normalises 2D homogeneous points
    // Source MATLAB code ref: http://www.csse.uwa.edu.au/~pk/research/matlabfns/Projective/normalise2dpts.m
	Point2f sum(0, 0);
	double cenX = 0, cenY = 0;
	double meanDist = 0, sumDist = 0;

	// Centroid
	size_t in_pts_size = pts.size();
	for(size_t idx = 0; idx < in_pts_size; idx++)
	{
		sum.x += pts[idx].x;
		sum.y += pts[idx].y;
	}
	cenX = sum.x / in_pts_size;
	cenY = sum.y / in_pts_size;

	// Mean Distance
	Point2f* diff = new Point2f[in_pts_size];
	for(size_t idx = 0; idx < in_pts_size; idx++)
	{
		diff[idx].x = pts[idx].x - cenX;
		diff[idx].y = pts[idx].y - cenY;
		sumDist += sqrt(diff[idx].x * diff[idx].x + diff[idx].y * diff[idx].y);
	}
	meanDist = sumDist / in_pts_size;

	// Scale
	double scale = 1.414213562373f / meanDist;

	// Mean dist and Scale and Norm
	for(size_t idx = 0; idx < in_pts_size; idx++)
	{
		// Norm
		pts[idx].x = diff[idx].x * scale;
		pts[idx].y = diff[idx].y * scale;
	}

	// Release memory
	delete[] diff;
}

void homography::reset(void)
{
    for (size_t ref_idx = 0; ref_idx < u_pack.size(); ref_idx++)
    {
        delete[] u_pack[ref_idx];
        vector<size_t>().swap(ref_cluster_ids_pack[ref_idx]);
    }
    vector<double*>().swap(u_pack);
    vector< vector<size_t> >().swap(ref_cluster_ids_pack);
}

}
//;)
