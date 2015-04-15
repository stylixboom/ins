/*
 * ins_param.h
 *
 *  Created on: October 28, 2013
 *      Author: Siriwat Kasamwattanarote
 */
#pragma once
#include <vector>
#include <string>

using namespace std;

namespace ins
{
    // Color space
    const static int RGB_SPACE          = 10;       // PLEASE SYNCHRONIZE VALUE WITH SIFThesaff.h!!!
    const static int IRGB_SPACE         = 11;       //                          AND imtools.h!!!!!
    const static int LAB_SPACE          = 12;

    // Inverted index mode
    const static int INV_BASIC          = -1;       // Don't change, these number is to identify size
    const static int INV_0              = 0;        // "
    const static int INV_2              = 2;        // "
    const static int INV_5              = 5;        // "

    // Similarity mode
	const static int SIM_L1             = 20;
	const static int SIM_GVP            = 21;

	// GVP Space mode
	const static int OFFSET             = 211;
	const static int DEGREE             = 212;

	// Mask mode
	const static int MASK_ROI           = 30;
	const static int MASK_IMG           = 31;
	const static int MASK_POLYGON       = 32;

    // Caller mode, // Combined with boolean operator, // Must be to a power of 2 for AND operator
    const static char CALLER_NONE       = 1;    // Must be started from 1, since 0 will clear everything during AND operation
    const static char CALLER_QB         = 2;
    const static char CALLER_QE         = 4;
	// const static char CALLER_...        = 8;

	// Query Expansion
    const static int QE                 = 40;
    const static int QE_POOL_SUM        = 401;      // QE pooling bow with sum
    const static int QE_POOL_MAX        = 402;      // QE pooling bow with max
    const static int QE_POOL_AVG        = 403;      // QE pooling bow with average


    // QBmining type
    const static int QB_FIM             = 42;       // Original Frequent itemset mining
    const static int QB_MAXPAT          = 43;       // Transpose transaction, then find the highest pattern during support tracing (enable transpose mode implicitly)
    const static int QB_MAXBIN          = 44;       // Transpose transaction, then find the highest bin during support tracing (enable transpose mode implicitly)
    const static int QB_PREFIM          = 45;       // Take pre-calculate FIM pattern
    const static int QB_FIX             = 46;       // Specify minsup
    const static int QB_GLOSD           = 47;       // Baseline, auto minsup by average global SD to centroid without co-occurrence (trick)
    const static int QB_LOCSD           = 48;       // Auto minsup by average local compactness to centroid without co-occurrence (trick)
    const static int QB_POOL_SUM        = 441;      // QB pooling bow with sum
    const static int QB_POOL_MAX        = 442;      // QB pooling bow with max
    const static int QB_POOL_AVG        = 443;      // QB pooling bow with average

    // Multi query mode
    const static int EARLYFUSION_SUM    = 50;
    const static int EARLYFUSION_MAX    = 51;
    const static int EARLYFUSION_AVG    = 52;
    const static int EARLYFUSION_FIM    = 53;       // Frequent itemset mining, Use with early fusion only

    // Multi query mode
    const static int LATEFUSION_SUM     = 54;
    const static int LATEFUSION_MAX     = 55;
    const static int LATEFUSION_AVG     = 56;

	// Query scale
	const static int SCALE_ABS          = 60;
	const static int SCALE_RATIO        = 61;

	// Pooling
	const static int POOL_SUM           = 70;
	const static int POOL_MAX           = 71;
	const static int POOL_AVG           = 72;

	// Asym identify
	const static int ASYM_FG_BIN        = 80;
	const static int ASYM_BG_BIN        = 81;

	// Signal
	const static string SIGQUIT = "SIGQUIT";

class ins_param
{
    typedef struct _dataset_preset_object{ string path_from_dataset_root; int group_at_pool_level; string database_params_prefix; } dataset_preset_object;

public:
    vector<dataset_preset_object> dataset_preset;
    int dataset_preset_index;

    // Raw param
    string raw_param;
    string detailed_param;

    // File param
    string offline_working_path;
    string cluster_path;
    string dataset_basepath_path;
    string dataset_filename_path;
    string feature_keypoint_path;
    string feature_descriptor_path;
    string poolinfo_path;
    string poolinfo_checkpoint_path;
    string quantized_path;
    string quantized_offset_path;
    string searchindex_path;
    string bow_offset_path;
    string bow_pool_offset_path;
    string bow_path;
    string bow_pool_path;
    string inv_name;
    string inv_header_path;
    string inv_data_path;
    string online_working_path;
    string hist_postfix;
    string querylist_filename;
    string histrequest_filename;
    string histrequest_path;

    // Directory param
    string home_path;
    string code_root_dir;
    string database_root_dir;
    string dataset_root_dir;
    string dataset_feature_root_dir;
    string feature_dir;
    string query_root_dir;
    string trecsubmit_root_dir;
    string shm_root_dir;

    string path_from_dataset;
    string dataset_name;
    string dataset_path;
    int group_level;
    string dataset_prefix;
    string dataset_header;
    string pooling_string;

    // Hardware Param
    int MAXCPU;

    // Feature extractor
    string feature_name;
    int colorspace;
    bool normpoint;
    bool rootsift;

    // Cluster param
    size_t CLUSTER_SIZE;

    // Knn search param
    int KDTREE;

    // Powerlaw
    bool powerlaw_enable;
    bool powerlaw_bgonly;

    // Inverted index param
    int INV_mode;
    bool stopword_enable;
    int stopword_amount;

    // Similarity
    int SIM_mode;

    // GVP
    int GVP_mode;
    int GVP_size;
    int GVP_length;

    // Mask
    bool mask_enable;
    int mask_mode;

    // RANSAC
    bool qe_ransac_enable;
    bool qe_ransac_adint_enable;
    int qe_ransac_threshold;
    float qe_ransac_adint_snr_threshold;
    bool qe_ransac_adint_manual;    // Debug

    // Query Expansion
    bool qe_enable;
    int qe_topk;
    int qe_pooling_mode;
    bool qe_query_mask_enable;

    // Query bootstrap (QB)
    // Frequent Item Set Mining for qb
    bool qb_enable;
    int qb_iteration;
    int qb_topk;
    int qb_mode;
    int qb_colossal_time;
    bool qb_transpose_enable;
    int qb_minsup;
    int qb_maxsup;
    int qb_pooling_mode;
    bool qb_query_mask_enable;

    // Scanning windows
    bool scanning_window_enable;
    int scanning_window_width;
    int scanning_window_height;
    int scanning_window_shift_x;
    int scanning_window_shift_y;

    // Multi-query
    bool earlyfusion_enable;
    int earlyfusion_mode;

    // Multi-rank
    bool latefusion_enable;
    int latefusion_mode;

    // Query scale
    bool query_scale_enable;
    int query_scale_type;
    int query_scale;
    string query_scale_postfix;
    bool query_scale_restore_enable;

    // Add Noise
    bool query_noise_enable;
    float query_noise_amount;

    // Pooling
    bool pooling_enable;
    int pooling_mode;

    // Asymmetric query mode
    bool asymmetric_enable;
    float asym_fg_weight;
    float asym_bg_weight;

    // Misc
    bool reuse_bow_sig;
    bool report_enable;
    bool matching_dump_enable;
    bool submit_enable;

	ins_param(void);
	~ins_param(void);
	void set_presetparam(const string& params_prefix);
    void LoadPreset();
};

};
//;)
