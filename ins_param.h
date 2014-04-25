/*
 * ins_param.h
 *
 *  Created on: October 28, 2013
 *      Author: Siriwat Kasamwattanarote
 */
#pragma once
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
#include <tr1/unordered_map>
#include <math.h>
#include <algorithm> // sort
#include <stdlib.h> // exit

#include <sys/types.h> // for checking exist file and dir
#include <sys/stat.h>

#include "../alphautils/alphautils.h"

using namespace std;
using namespace tr1;
using namespace alphautils;

namespace ins
{
    // Color space
    const static int RGB_SPACE = 0;
    const static int IRGB_SPACE = 1;
    const static int LAB_SPACE = 2;

    // Similarity mode
	const static int SIM_L1 = 0;
	const static int SIM_GVP = 1;

	// GVP Space mode
	const static int OFFSET = 0;
	const static int DEGREE = 1;

	// Mask mode
	const static int MASK_ROI = 0;
	const static int MASK_POLYGON = 1;

    // Querybootstrap min_bow_type
    const static int MIN_BIN = 0;
	const static int MIN_FEATURE = 1;
	const static int MIN_IDF = 2;

	// Multirank mode
	const static int ADD_COMBINATION = 0;
	const static int MEAN_COMBINATION = 1;
	const static int NORM_ADD_COMBINATION = 2;
	const static int NORM_MEAN_COMBINATION = 3;

	// Query scale
	const static int SCALE_ABS = 0;
	const static int SCALE_RATIO = 1;

class ins_param
{
    typedef struct _dataset_preset_object{ string path_from_dataset_root; int group_at_pool_level; string database_params_prefix; } dataset_preset_object;

public:
    vector<dataset_preset_object> dataset_preset;
    int dataset_preset_index;

    // Raw param
    string raw_param;
    string detailed_param;

    // Summarized param
    string code_root_dir;
    string database_root_dir;
    string dataset_root_dir;
    string query_root_dir;

    string path_from_dataset;
    int group_level;
    string dataset_prefix;
    string dataset_header;

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

    // Similarity
    int SIM_mode;

    // GVP
    int GVP_mode;
    int GVP_size;
    int GVP_length;

    // Mask
    bool mask_enable;
    int mask_mode;

    // http://en.wikipedia.org/wiki/Bootstrapping
    // Query bootstrap
    bool query_bootstrap1_enable;
    bool query_bootstrap2_enable;
    int query_bootstrap_rankcheck;
    int query_bootstrap_minbow_type;
    int query_bootstrap_minbow;
    int query_bootstrap_patch;
    int query_bootstrap_clusterit;
    bool query_bootstrap_mask_enable;
    // Frequent Item Set Mining for qb
    bool qbmining_enable;
    int qbmining_minsup;

    // Scanning windows
    bool scanning_window_enable;
    int scanning_window_width;
    int scanning_window_height;
    int scanning_window_shift_x;
    int scanning_window_shift_y;

    // Multirank
    int multirank_mode;

    // Query scale
    bool query_scale_enable;
    int query_scale_type;
    int query_scale;

    // Misc
    bool report_enable;

	ins_param(void);
	~ins_param(void);
	void set_presetparam(const string& params_prefix);
    void LoadPreset();
};

};
//;)
