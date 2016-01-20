/*
 * ins_param.cpp
 *
 *  Created on: October 28, 2013
 *      Author: Siriwat Kasamwattanarote
 */
#include <ctime>
#include <unistd.h>     // sysconf
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
#include <algorithm>    // sort
#include <cstdlib>      // exit
#include <sys/types.h>  // for checking exist file and dir
#include <sys/stat.h>

// Siriwat's header
#include "../alphautils/alphautils.h"

#include "ins_param.h"

#include "version.h"

using namespace std;
using namespace alphautils;

namespace ins
{

ins_param::ins_param(void)
{
    // Directory param
    home_path                   = getenv("HOME");
                            make_dir_available(home_path + "/webstylix/code");
    code_root_dir               = resolve_path(home_path + "/webstylix/code");
                            make_dir_available(home_path + "/webstylix/code/database/ins_offline");
    database_root_dir           = resolve_path(home_path + "/webstylix/code/database/ins_offline");
                            make_dir_available(home_path + "/webstylix/code/dataset");
    dataset_root_dir            = resolve_path(home_path + "/webstylix/code/dataset");
                            make_dir_available(home_path + "/webstylix/code/dataset_feature");
    dataset_feature_root_dir    = resolve_path(home_path + "/webstylix/code/dataset_feature");
    shm_root_dir                = "/dev/shm";
                            make_dir_available(shm_root_dir + "/query");
                        if (!is_path_exist(home_path + "/webstylix/code/ins_online/query"))
                            exec("ln -s " + shm_root_dir + "/query" + home_path + "/webstylix/code/ins_online/query");
    if (is_path_exist(home_path + "/webstylix/code/ins_online/query"))
        query_root_dir          = str_replace_first(resolve_path(home_path + "/webstylix/code/ins_online/query"), "run", "dev");
                                // Unify path from Ubuntu /run/shm to /dev/shm
    else
        cout << "Warning: Online query path not available!" << endl;

    dataset_preset_index        = 0;

    // Get total CPUs, then keep it to MAXCPU
    MAXCPU = sysconf( _SC_NPROCESSORS_ONLN );

    // Norm point
    normpoint = false;

    // Knn search param
    KDTREE = 16; // default

    // Powerlaw
    powerlaw_enable = false;
    powerlaw_bgonly = false;

    // Inverted index mode
    INV_mode = INV_BASIC;
    stopword_enable = false;

    // Similarity mode
    SIM_mode = SIM_L1;

    // Mask
    mask_enable = false;

    // Reuse bow_sig (for speed)
    reuse_bow_sig = false;

    // QE
    qe_enable = false;
    qe_ransac_enable = false;
    qe_ransac_adint_enable = false;
    qe_ransac_adint_manual = false;

    // QBmining
    qb_enable = false;
    qb_iteration = 0;
    qb_transpose_enable = false;

    // Multi-query
    earlyfusion_enable = false;             // Default disable all fusion
    earlyfusion_mode = EARLYFUSION_AVG;

    // Multi-query
    latefusion_enable = false;

    // Scale
    query_scale_enable = false;
    query_scale_restore_enable = false;

    // Noise
    query_noise_enable = false;

    // Pooling
    pooling_enable = false;
    pooling_mode = POOL_AVG;
}

ins_param::~ins_param(void)
{

}

/// Sample param
/// oxbuildings/105k,-1,oxbuildings105k_sifthesaff-rgb-norm-root_akm_1000000_kd3_16_gvp_10_2_mask_roi_qscale_80_qbootstrap_176
void ins_param::set_presetparam(const string& params_prefix)
{
    vector<string> params;
    StringExplode(params_prefix, "_", params);

    // Prepare dataset header
    /// Pattern datasetheader
    stringstream header_builder;
    header_builder << params[0] << "_" << params[1];

    // Feature extractor name
    /// Pattern sifthesaff-root-norm-rgb, sifthesaff-root-norm-lab, brisk
    stringstream feature_builder;
    feature_builder << params[1];
	// Feature type
	if (str_contains(params[1], "sifthesaff"))
        feature_type = FEAT_SIFTHESAFF;
	else if (str_contains(params[1], "orb"))
		feature_type = FEAT_ORB;
	else
	{
		cout << "Feature type incorrect" << endl;
		exit(EXIT_FAILURE);
	}
			
	// Color space
    if (str_contains(params[1], "rgb"))
        colorspace = RGB_SPACE;
    else if (str_contains(params[1], "irgb"))
        colorspace = IRGB_SPACE;
    else
        colorspace = LAB_SPACE;
	// Feature params
    if (str_contains(params[1], "norm"))
        normpoint = true;
    if (str_contains(params[1], "root"))
        rootsift = true;

    // Start checking after main header
    for (size_t param_idx = 2; param_idx < params.size();)
    {
        if (params[param_idx] == "akm")
        {
            /// Pattern clusteringmethod_size
            CLUSTER_SIZE = atoi(params[param_idx + 1].c_str());
            header_builder << "_akm_" << params[param_idx + 1];
            param_idx += 2;
        }
        else if (params[param_idx] == "kd3")
        {
            /// **Only for akm clustering
            /// Pattern kd3_size
            KDTREE = atoi(params[param_idx + 1].c_str());
            header_builder << "_kd3_" << params[param_idx + 1];
            param_idx += 2;
        }
        else if (params[param_idx] == "powerlaw")
        {
            /// Pattern powerlaw
            powerlaw_enable = true;

            /// Check top enable
            if (param_idx + 1 < params.size() && params[param_idx + 1] == "bgonly")
            {
                powerlaw_bgonly = true;
                param_idx += 1;
            }

            param_idx += 1;
        }
        else if (params[param_idx] == "gvp" || params[param_idx] == "dvp")
        {
            /// Pattern gvpmode_gvpsize_gvplength
            SIM_mode = SIM_GVP;
            if (params[param_idx] == "gvp") // GVP baseline
                GVP_mode = OFFSET;
            else                            // GVP degree
                GVP_mode = DEGREE;
            GVP_size = atoi(params[param_idx + 1].c_str());
            GVP_length = atoi(params[param_idx + 2].c_str());
            param_idx += 3;
        }
        else if (params[param_idx] == "inv") // INV mode
        {
            /// Pattern inv_0
            if (params[param_idx + 1] == "basic")       // Basic mode
                INV_mode = INV_BASIC;
            else if (params[param_idx + 1] == "0")      // with img_order, sequence_order
                INV_mode = INV_0;
            else if (params[param_idx + 1] == "2")      // with img_order, sequence_order, feature (x,y)
                INV_mode = INV_2;
            else if (params[param_idx + 1] == "5")      // with img_order, sequence_order, feature (x,y,a,b,c)
                INV_mode = INV_5;
            else
            {
                cout << "Inverted index mode not specified!" << endl;
                exit(-1);
            }
            param_idx += 2;
        }
        else if (params[param_idx] == "stopword") // INV mode
        {
            /// Pattern stopword_amount
            stopword_enable = true;
            stopword_amount = atoi(params[param_idx + 1].c_str());
            param_idx += 2;
        }
        else if (params[param_idx] == "mask")
        {
            /// Pattern maskenable_maskmode
            mask_enable = true;
            if (params[param_idx + 1] == "roi")         // Mask roi
                mask_mode = MASK_ROI;
            else if (params[param_idx + 1] == "img")    // Mask image
                mask_mode = MASK_IMG;
            else
                mask_mode = MASK_POLYGON;
            param_idx += 2;
        }
        else if (params[param_idx] == "qe") // Query Expansion
        {
            /// Pattern qe_topk_[ransac_6|ransac_adint_0.1]_poolavg_[mask]
            qe_enable = true;

            qe_topk = atoi(params[param_idx + 1].c_str());              // top-k

            if (params[param_idx + 2] == "ransac")                      // Enable ransac check for topk
            {
                qe_ransac_enable = true;

                if (params[param_idx + 3] == "adint")
                {
                    // Adaptive Inlier Threshold mode
                    qe_ransac_adint_enable = true;
                    qe_ransac_adint_snr_threshold = atof(params[param_idx + 4].c_str());   // RANSAC inlier frequency SNR threshold

                    param_idx += 3;
                }
                else
                {
                    // Fixed threshold mode
                    qe_ransac_threshold = atoi(params[param_idx + 3].c_str());

                    param_idx += 2;
                }
            }

            if (params[param_idx + 2] == "poolavg")         // QE pooling average
                qe_pooling_mode = QE_POOL_AVG;
            else if (params[param_idx + 2] == "poolsum")    // QE pooling sum
                qe_pooling_mode = QE_POOL_SUM;
            else if (params[param_idx + 2] == "poolmax")    // QE pooling max
                qe_pooling_mode = QE_POOL_MAX;

            if ((param_idx + 3 < params.size()) && (params[param_idx + 3] == "mask"))            // Constrain pooling with query mask
            {
                qe_query_mask_enable = true;
                param_idx += 1;
            }

            param_idx += 3;
        }
        else if (params[param_idx] == "qb") // Query Bootstrapping with FIM
        {
            /// Pattern qb_it_topk_[colotime_sec]_[ransac_6|ransac_adint_0.1]_[fim_[t]_minsup_maxsup]|[maxpat|maxbin]_poolavg_[mask]
            qb_enable = true;

            qb_iteration = atoi(params[param_idx + 1].c_str());         // iteration

            qb_topk = atoi(params[param_idx + 2].c_str());              // top-k

            if (params[param_idx + 3] == "colotime")                    // Time to cut colossal pattern
            {
                qb_colossal_time = atoi(params[param_idx + 4].c_str()); // in second

                param_idx += 2;
            }
            else
                qb_colossal_time = 30;                                  // default timer is 30 seconds

            if (params[param_idx + 3] == "ransac")                      // Enable ransac check for topk
            {
                qe_ransac_enable = true;

                if (params[param_idx + 4] == "adint")
                {
                    // Adaptive Inlier Threshold mode
                    qe_ransac_adint_enable = true;
                    qe_ransac_adint_snr_threshold = atof(params[param_idx + 5].c_str());   // RANSAC inlier frequency SNR threshold

                    param_idx += 3;
                }
                else
                {
                    // Fixed threshold mode
                    qe_ransac_threshold = atoi(params[param_idx + 4].c_str());

                    param_idx += 2;
                }
            }

            if (params[param_idx + 3] == "fim")                         // Original Frequent itemset mining
            {
                qb_mode = QB_FIM;
                if (params[param_idx + 4] == "t")                       // Enable mining with transpose mode
                {
                    qb_transpose_enable = true;
                    param_idx += 1;
                }

                qb_minsup = atoi(params[param_idx + 4].c_str());        // Minimum support
                qb_maxsup = atoi(params[param_idx + 5].c_str());        // Maximum support
                param_idx += 3;
            }
            else if (params[param_idx + 3] == "maxpat")                 // Tracing support to find the maximum pattern
            {
                qb_mode = QB_MAXPAT;
                qb_transpose_enable = true;
                param_idx += 1;
            }
            else if (params[param_idx + 3] == "maxbin")                 // Tracing support to find the maximum bin
            {
                qb_mode = QB_MAXBIN;
                qb_transpose_enable = true;
                param_idx += 1;
            }

            if (params[param_idx + 3] == "poolavg")                     // QB pooling average
                qb_pooling_mode = QB_POOL_AVG;
            else if (params[param_idx + 3] =="poolsum")                 // QB pooling sum
                qb_pooling_mode = QB_POOL_SUM;
            else if (params[param_idx + 3] =="poolmax")                 // QB pooling max
                qb_pooling_mode = QB_POOL_MAX;

            if ((param_idx + 4 < params.size()) && (params[param_idx + 4] == "mask"))            // Constrain pooling with query mask
            {
                qb_query_mask_enable = true;
                param_idx += 1;
            }

            param_idx += 4;
        }
        else if (params[param_idx] == "scan") // Scanning window
        {
            /// Pattern scan_width_height_shiftx_shifty
            scanning_window_enable = true;
            scanning_window_width = atoi(params[param_idx + 1].c_str());
            scanning_window_height = atoi(params[param_idx + 2].c_str());
            scanning_window_shift_x = atoi(params[param_idx + 3].c_str());
            scanning_window_shift_y = atoi(params[param_idx + 4].c_str());
            param_idx += 5;
            // Default method for fusion
            latefusion_enable = true;
            latefusion_mode = LATEFUSION_SUM;  // sum rank
        }
        else if (params[param_idx] == "earlyfusion")
        {
            /// Pattern earlyfusion_mode
            earlyfusion_enable = true;
            latefusion_enable = false;
            if (params[param_idx + 1] == "sum")
            {
                earlyfusion_mode = EARLYFUSION_SUM;
            }
            else if (params[param_idx + 1] == "max")
            {
                earlyfusion_mode = EARLYFUSION_MAX;
            }
            else if (params[param_idx + 1] == "avg")
            {
                earlyfusion_mode = EARLYFUSION_AVG;
            }
            else if (params[param_idx + 1] == "fim")
            {
                earlyfusion_mode = EARLYFUSION_FIM;
            }
            else
            {
                cout << "Late fusion mode not specified!" << endl;
                exit(-1);
            }
            param_idx += 2;
        }
        else if (params[param_idx] == "latefusion")
        {
            /// Pattern latefusion_mode
            earlyfusion_enable = false;
            latefusion_enable = true;
            if (params[param_idx + 1] == "sum")
            {
                latefusion_mode = LATEFUSION_SUM;
            }
            else if (params[param_idx + 1] == "max")
            {
                latefusion_mode = LATEFUSION_MAX;
            }
            else if (params[param_idx + 1] == "avg")
            {
                latefusion_mode = LATEFUSION_AVG;
            }
            else
            {
                cout << "Late fusion mode not specified!" << endl;
                exit(-1);
            }
            param_idx += 2;
        }
        else if (params[param_idx] == "qscale") // Query scale test
        {
            /// Pattern qscale_mv(m:mode v:value)_[restore]
            query_scale_enable = true;
            if (str_contains(params[param_idx + 1], "a"))       // absolute max size of width and height, eg a255 (255px)
            {
                query_scale_type = SCALE_ABS;
                query_scale = atoi(str_replace_first(params[param_idx + 1], "a", "").c_str());

                query_scale_postfix = "_scaled_a" + toString(query_scale) + ".jpg";

                param_idx += 2;
            }
            else if (str_contains(params[param_idx + 1], "r"))  // scale ratio, 20, 50, 100, 120 ..., eg r80 (80%)
            {
                query_scale_type = SCALE_RATIO;
                query_scale = atoi(str_replace_first(params[param_idx + 1], "r", "").c_str());

                query_scale_postfix = "_scaled_r" + toString(query_scale) + ".jpg";

                if ((param_idx + 2 < params.size()) && (params[param_idx + 2] == "restore"))    // Restore query to its original size
                {
                    query_scale_restore_enable = true;
                    param_idx += 1;
                }

                param_idx += 2;
            }
        }
        else if (params[param_idx] == "qnoise")
        {
            /// Pattern qnoise_amount
            query_noise_enable = true;
            query_noise_amount = atof(params[param_idx + 1].c_str());

            param_idx += 2;
        }
        else if (params[param_idx] == "pooling")        // Pooling mode
        {
            /// Pattern pooling_mode
            pooling_enable = true;
            if (params[param_idx + 1] == "sum")         // sum pooling
                pooling_mode = POOL_SUM;
            else if (params[param_idx + 1] == "max")    // max pooling
                pooling_mode = POOL_MAX;
            else if (params[param_idx + 1] == "avg")    // avg pooling
                pooling_mode = POOL_AVG;
            else
            {
                cout << "Pooling mode not specified!" << endl;
                exit(-1);
            }
            pooling_string = "pooling_" + params[param_idx + 1];
            param_idx += 2;
        }
        else if (params[param_idx] == "asym") // Asymmetric bg-fg
        {
            /// Pattern asym_fgweight_bgweight
            asymmetric_enable = true;
            asym_fg_weight = atof(params[param_idx + 1].c_str());
            asym_bg_weight = atof(params[param_idx + 2].c_str());
            param_idx += 3;
        }
        else if (params[param_idx] == "reusebow")
        {
            /// Pattern reusebow
            reuse_bow_sig = true;
            param_idx += 1;
        }
        else if (params[param_idx] == "report") // Report
        {
            /// Pattern report
            report_enable = true;
            param_idx += 1;
        }
        else if (params[param_idx] == "matchdump") // Visualize match
        {
            /// Pattern matchdump
            matching_dump_enable = true;
            param_idx += 1;
        }
        else if (params[param_idx] == "submit") // Submit
        {
            /// Pattern submit
            submit_enable = true;

            if (is_path_exist(home_path + "/webstylix/code/ins_online/trec_submit/siriwat"))
                trecsubmit_root_dir     = resolve_path(home_path + "/webstylix/code/ins_online/trec_submit/siriwat");
            else
                cout << "Warning: TREC submit path not available!" << endl;

            param_idx += 1;
        }
        else
        {
            /// Probably just an unimportant parameter
            param_idx += 1;
        }
    }

    // Set dataset header
    dataset_header = header_builder.str();

    // Set feature_name
    feature_name = feature_builder.str();

    // Remove un-necessary prefix
    if (reuse_bow_sig)
        dataset_prefix = str_replace_last(dataset_prefix, "_reusebow", "");
    if (report_enable)
        dataset_prefix = str_replace_last(dataset_prefix, "_report", "");
    if (matching_dump_enable)
        dataset_prefix = str_replace_last(dataset_prefix, "_matchdump", "");
    if (submit_enable)
        dataset_prefix = str_replace_last(dataset_prefix, "_submit", "");

    /// Initialize directory path
    // File param
    offline_working_path    = database_root_dir + "/" + dataset_header;
    cluster_path            = offline_working_path + "/cluster";
    dataset_basepath_path   = offline_working_path + "/dataset_basepath";
    dataset_filename_path   = offline_working_path + "/dataset_filename";
    feature_keypoint_path   = offline_working_path + "/feature_keypoint_" + feature_name;
    feature_descriptor_path = offline_working_path + "/feature_descriptor_" + feature_name;
    poolinfo_path           = offline_working_path + "/poolinfo";
    poolinfo_checkpoint_path= offline_working_path + "/poolinfo.chkpnt";
    quantized_path          = offline_working_path + "/quantized";
    quantized_offset_path   = offline_working_path + "/quantized_offset";
    searchindex_path        = offline_working_path + "/searchindex";
    bow_path                = offline_working_path + "/bow";
    bow_offset_path         = offline_working_path + "/bow_offset";
    inv_name                = "inv" + toString(INV_mode) + "_" + dataset_header;
    if (pooling_enable)
    {
        bow_pool_path           = offline_working_path + "/bow_" + pooling_string;
        bow_pool_offset_path    = offline_working_path + "/bow_" + pooling_string + "_offset";
        inv_name                = inv_name + "_" + pooling_string;
    }
    if (powerlaw_enable)
    {
        bow_path            = str_replace_first(bow_path, "bow", "bow_powerlaw");
        bow_offset_path     = str_replace_first(bow_offset_path, "bow", "bow_powerlaw");
        if (pooling_enable)
        {
            bow_pool_path        = str_replace_first(bow_pool_path, "bow", "bow_powerlaw");
            bow_pool_offset_path = str_replace_first(bow_pool_offset_path, "bow", "bow_powerlaw");
        }
        inv_name += "_powerlaw";
    }
    inv_header_path         = offline_working_path + "/" + inv_name + "_header";
    inv_data_path           = offline_working_path + "/" + inv_name + "_data";
    online_working_path     = query_root_dir + "/" + dataset_prefix;
    querylist_filename      = "query_list.txt";
    hist_postfix            = ".bowsig";
    histrequest_filename    = "hist_request.txt";
    histrequest_path        = online_working_path + "/" + histrequest_filename;
    dataset_path            = dataset_root_dir + "/" + path_from_dataset;
    feature_dir             = dataset_feature_root_dir + "/" + feature_name;
}

void ins_param::LoadPreset()
{
    cout << "== Dataset Preset ==" << endl;

    // Release mem
    dataset_preset.clear();

    // Experimental list
    string experiment_path = code_root_dir + "/experiments.txt";

    vector<string> params = text_readline2vector(experiment_path);

    // Read preset params
    for (size_t param_idx = 0; param_idx < params.size(); param_idx++)
    {
        if (str_contains(params[param_idx], "## "))
        {

            // Comment
            cout << greenc << str_replace_first(params[param_idx], "## ", "") << endc << endl;
        }
        else
        {
            vector<string> vals;
            StringExplode(params[param_idx], ",", vals);

            dataset_preset_object new_preset = {vals[0], 				// string path_from_dataset_root
												atoi(vals[1].c_str()),	// int group_at_pool_level;
												vals[2]};				// string database_params_prefix

            dataset_preset.push_back(new_preset);

            // Preset
            // Counting number of max digits (int)log10(params.size()) + 1) <-- it should be a maximum number of param without comment
            cout << "[" << bluec << CharPadString(toString(dataset_preset.size() - 1), ' ', (int)log10(params.size()) + 1) << endc << "] " << dataset_preset[dataset_preset.size() - 1].database_params_prefix << " [" << bluec << dataset_preset.size() - 1 << endc << "]" << endl;
        }
    }
    cout << "Select preset id: ";
    cin >> dataset_preset_index;

    // set as global prefix
    path_from_dataset = dataset_preset[dataset_preset_index].path_from_dataset_root;
    group_level = dataset_preset[dataset_preset_index].group_at_pool_level;
    dataset_prefix = dataset_preset[dataset_preset_index].database_params_prefix;

    // set dataset name
    vector<string> dataset_subpath;
    StringExplode(path_from_dataset, "/", dataset_subpath);
    dataset_name = dataset_subpath[0];

    // set parameter
    set_presetparam(dataset_preset[dataset_preset_index].database_params_prefix);

    stringstream tmp_strbuild;
    // Set full original param
    tmp_strbuild << "[" << CharPadString(toString(dataset_preset_index), ' ', (int)log10(dataset_preset.size()) + 1) << "] " << dataset_preset[dataset_preset_index].database_params_prefix;
    raw_param = tmp_strbuild.str();

    tmp_strbuild.str("");
    tmp_strbuild << "dataset_preset_index:" << dataset_preset_index << "#";
    tmp_strbuild << "database_root_dir:" << database_root_dir << "#";
    tmp_strbuild << "dataset_root_dir:" << dataset_root_dir << "#";
    tmp_strbuild << "query_root_dir:" << query_root_dir << "#";
    tmp_strbuild << "path_from_dataset:" << path_from_dataset << "#";
    tmp_strbuild << "group_level:" << group_level << "#";
    tmp_strbuild << "dataset_prefix:" << dataset_prefix << "#";
    tmp_strbuild << "MAXCPU:" << MAXCPU << "#";
    tmp_strbuild << "feature_name:" << feature_name << "#";
    tmp_strbuild << "normpoint:" << normpoint << "#";
    tmp_strbuild << "CLUSTER_SIZE:" << CLUSTER_SIZE << "#";
    tmp_strbuild << "KDTREE:" << KDTREE << "#";
    tmp_strbuild << "SIM_mode:" << SIM_mode << "#";
    tmp_strbuild << "mask_enable:" << boolalpha << mask_enable << "#";
    tmp_strbuild << "mask_mode:" << mask_mode << "#";
    tmp_strbuild << "report_enable:" << boolalpha << report_enable;
    detailed_param = tmp_strbuild.str();
}

}
//;)
