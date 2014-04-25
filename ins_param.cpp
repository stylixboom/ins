/*
 * ins_param.cpp
 *
 *  Created on: October 28, 2013
 *      Author: Siriwat Kasamwattanarote
 */

#include "ins_param.h"

using namespace std;
using namespace tr1;
using namespace alphautils;

namespace ins
{

ins_param::ins_param(void)
{
    code_root_dir = "/home/stylix/webstylix/code";
    database_root_dir = "/home/stylix/webstylix/code/database/ins_offline";
    dataset_root_dir = "/home/stylix/webstylix/code/dataset";
    query_root_dir = "/home/stylix/webstylix/code/ins_online/query";
    dataset_preset_index = 0;

    // Get total CPUs, then keep it to MAXCPU
    MAXCPU = sysconf( _SC_NPROCESSORS_ONLN );

    // Norm point
    normpoint = false;

    // Knn search param
    KDTREE = 16; // default

    // Similarity mode
    SIM_mode = SIM_L1;

    // Mask
    mask_enable = false;

    // Boostrap
    query_bootstrap1_enable = false;
    query_bootstrap2_enable = false;
    query_bootstrap_mask_enable = false;
    qbmining_enable = false;

    // Scale
    query_scale_enable = false;
}

ins_param::~ins_param(void)
{

}

/// Sample param
/// oxbuildings/105k,-1,oxbuildings105k_sifthesaff-rgb-norm-root_akm_1000000_kd3_16_gvp_10_2_mask_roi_qscale_80_qbootstrap_176
void ins_param::set_presetparam(const string& params_prefix)
{
    vector<string> params;
    const char* delims = "_";

    string_splitter(params_prefix, delims, params);

    // Prepare dataset header
    stringstream header_builder;
    header_builder << params[0] << "_" << params[1];

    // Feature extractor name
    stringstream feature_builder;
    feature_builder << params[1];   // sifthesaff-root-norm-rgb, sifthesaff-root-norm-lab, brisk
    if (str_contains(params[1], "rgb"))
        colorspace = RGB_SPACE;
    else if (str_contains(params[1], "irgb"))
        colorspace = IRGB_SPACE;
    else
        colorspace = LAB_SPACE;
    if (str_contains(params[1], "norm"))
        normpoint = true;
    if (str_contains(params[1], "root"))
        rootsift = true;

    // Start checking after main header
    for (size_t param_idx = 2; param_idx < params.size(); param_idx++)
    {
        if (params[param_idx] == "akm")
        {
            CLUSTER_SIZE = atoi(params[param_idx + 1].c_str());
            header_builder << "_akm_" << params[param_idx + 1];
            param_idx += 1;
        }
        else if (params[param_idx] == "kd3")
        {
            KDTREE = atoi(params[param_idx + 1].c_str());
            header_builder << "_kd3_" << params[param_idx + 1];
            param_idx += 1;
        }
        else if (params[param_idx] == "gvp" || params[param_idx] == "dvp")
        {
            SIM_mode = SIM_GVP;
            if (params[param_idx] == "gvp") // GVP baseline
                GVP_mode = OFFSET;
            else                            // GVP degree
                GVP_mode = DEGREE;
            GVP_size = atoi(params[param_idx + 1].c_str());
            GVP_length = atoi(params[param_idx + 2].c_str());
            param_idx += 2;
        }
        else if (params[param_idx] == "mask")
        {
            mask_enable = true;
            if (params[param_idx + 1] == "roi") // Mask roi
                mask_mode = MASK_ROI;
            else                                // Mask polygon
                mask_mode = MASK_POLYGON;
            param_idx += 1;
        }
        else if (params[param_idx] == "qbootstrap1") // Query bootstraping v1
        {
            query_bootstrap1_enable = true;
            query_bootstrap_rankcheck = atoi(params[param_idx + 1].c_str());
            if (str_contains(params[param_idx + 2], "b"))
            {
                query_bootstrap_minbow_type = MIN_BIN;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "b", "").c_str());
            }
            else if (str_contains(params[param_idx + 2], "f"))
            {
                query_bootstrap_minbow_type = MIN_FEATURE;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "f", "").c_str());
            }
            else
            {
                query_bootstrap_minbow_type = MIN_IDF;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "i", "").c_str());
            }
            query_bootstrap_patch = atoi(params[param_idx + 3].c_str());
            if (str_contains(params[param_idx + 4], "m"))
            {
                // Focus mask
                query_bootstrap_mask_enable = true;
                query_bootstrap_clusterit = atoi(str_replace_first(params[param_idx + 4], "m", "").c_str());
            }
            else
                query_bootstrap_clusterit = atoi(params[param_idx + 4].c_str());
            param_idx += 4;
            // Default method for multirank
            multirank_mode = ADD_COMBINATION;  // Add rank
        }
        else if (params[param_idx] == "qbootstrap2") // Query bootstraping v2
        {
            query_bootstrap2_enable = true;
            query_bootstrap_rankcheck = atoi(params[param_idx + 1].c_str());
            if (str_contains(params[param_idx + 2], "b"))
            {
                query_bootstrap_minbow_type = MIN_BIN;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "b", "").c_str());
            }
            else if (str_contains(params[param_idx + 2], "f"))
            {
                query_bootstrap_minbow_type = MIN_FEATURE;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "f", "").c_str());
            }
            else
            {
                query_bootstrap_minbow_type = MIN_IDF;
                query_bootstrap_minbow = atoi(str_replace_first(params[param_idx + 2], "i", "").c_str());
            }
            param_idx += 2;
        }
        else if (params[param_idx] == "qbmining") // Frequent Item Set Mining for qb
        {
            qbmining_enable = true;
            qbmining_minsup = atoi(params[param_idx + 1].c_str());
            if (query_bootstrap_minbow_type != MIN_IDF) // default to keep idf 100% if disable MIN_IDF
                query_bootstrap_minbow = 100;
            param_idx += 1;
        }
        else if (params[param_idx] == "scan") // Scanning window
        {
            scanning_window_enable = true;
            scanning_window_width = atoi(params[param_idx + 1].c_str());
            scanning_window_height = atoi(params[param_idx + 2].c_str());
            scanning_window_shift_x = atoi(params[param_idx + 3].c_str());
            scanning_window_shift_y = atoi(params[param_idx + 4].c_str());
            param_idx += 4;
            // Default method for multirank
            multirank_mode = ADD_COMBINATION;  // Add rank
        }
        else if (params[param_idx] == "meanrank") // Multirank Combination
        {
            multirank_mode = MEAN_COMBINATION; // Mean rank
        }
        else if (params[param_idx] == "normaddrank") // Multirank Combination
        {
            multirank_mode = NORM_ADD_COMBINATION; // Norm Add rank
        }
        else if (params[param_idx] == "normmeanrank") // Multirank Combination
        {
            multirank_mode = NORM_MEAN_COMBINATION; // Norm Mean rank
        }
        else if (params[param_idx] == "qscale") // Query scale test
        {
            query_scale_enable = true;
            if (str_contains(params[param_idx + 1], "a"))   // absolute max size of width and height
            {
                query_scale_type = SCALE_ABS;
                query_scale = atoi(str_replace_first(params[param_idx + 1], "a", "").c_str());
            }
            else                                            // scale ratio, 20, 50, 100, 120 ...
            {
                query_scale_type = SCALE_RATIO;
                query_scale = atoi(str_replace_first(params[param_idx + 1], "r", "").c_str());
            }
            param_idx += 1;
        }
        else if (params[param_idx] == "report") // Report
        {
            report_enable = true;
        }
    }

    // Set dataset header
    dataset_header = header_builder.str();

    // Set feature_name
    feature_name = feature_builder.str();

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
            char const* delims = ",";

            vector<string> vals;

            string_splitter(params[param_idx], delims, vals);

                                              //string path_from_dataset_root; int group_at_pool_level; string database_params_prefix
            dataset_preset_object new_preset = {vals[0], atoi(vals[1].c_str()), vals[2]};

            dataset_preset.push_back(new_preset);

            // Preset
            // Counting number of max digits (int)log10(params.size()) + 1) <-- it should be a maximum number of param without comment
            cout << "[" << bluec << CharPadString(toString(dataset_preset.size() - 1), ' ', (int)log10(params.size()) + 1) << endc << "] " << dataset_preset[dataset_preset.size() - 1].database_params_prefix << " [" << bluec << dataset_preset.size() - 1 << endc << "]" << endl;
        }
    }
    cout << "Select preset id: ";
    cin >> dataset_preset_index;

    // set parameter
    set_presetparam(dataset_preset[dataset_preset_index].database_params_prefix);

    // set as global prefix
    path_from_dataset = dataset_preset[dataset_preset_index].path_from_dataset_root;
    group_level = dataset_preset[dataset_preset_index].group_at_pool_level;
    dataset_prefix = dataset_preset[dataset_preset_index].database_params_prefix;

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
    tmp_strbuild << "GVP_mode:" << GVP_mode << "#";
    tmp_strbuild << "GVP_size:" << GVP_size << "#";
    tmp_strbuild << "GVP_length:" << GVP_length << "#";
    tmp_strbuild << "mask_enable:" << boolalpha << mask_enable << "#";
    tmp_strbuild << "mask_mode:" << mask_mode << "#";
    tmp_strbuild << "query_bootstrap1_enable:" << boolalpha << query_bootstrap1_enable << "#";
    tmp_strbuild << "query_bootstrap2_enable:" << boolalpha << query_bootstrap2_enable << "#";
    tmp_strbuild << "query_bootstrap_rankcheck:" << query_bootstrap_rankcheck << "#";
    tmp_strbuild << "query_bootstrap_minbow_type:" << query_bootstrap_minbow_type << "#";
    tmp_strbuild << "query_bootstrap_minbow:" << query_bootstrap_minbow << "#";
    tmp_strbuild << "query_bootstrap_patch:" << query_bootstrap_patch << "#";
    tmp_strbuild << "query_bootstrap_clusterit:" << query_bootstrap_clusterit << "#";
    tmp_strbuild << "multirank_mode:" << multirank_mode << "#";
    tmp_strbuild << "query_scale_enable:" << boolalpha << query_scale_enable << "#";
    tmp_strbuild << "query_scale:" << query_scale;
    tmp_strbuild << "report_enable:" << boolalpha << report_enable;
    detailed_param = tmp_strbuild.str();
}

}
