/*
 * bow.h
 *
 *  Created on: May 12, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
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
#include <algorithm> // sort
#include <cstdlib> // exit

#include "../alphautils/alphautils.h"
#include "../alphautils/imtools.h"
#include "../alphautils/hdf5_io.h"
#include "../sifthesaff/SIFThesaff.h"
#include "ins_param.h"
#include "invert_index.h"
#include "core.hpp"
#include "represent.hpp"
#include "geom.hpp"

#include "version.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace alphautils::hdf5io;
using namespace ins;
//using namespace geom;

namespace ins
{
    // Anything shared with ins namespace here

class bow
{
    // Private variables and functions here
    ins_param run_param;

    // Bow
    bool bow_offset_ready;
    vector<size_t> bow_offset;
    vector< vector<bow_bin_object*> > multi_bow;
    // Pooling
    bool bow_pool_offset_ready;
    vector<size_t> bow_pool_offset;
    vector< vector<bow_bin_object*> > multi_bow_pool;
public:
    // Public variables and functions here
	bow(ins_param param);
	~bow(void);
	// Tools
	void masking(vector<bow_bin_object*>& bow_sig);                                                 // Masking /*to be implement*/
	void logtf_unitnormalize(vector<bow_bin_object*>& bow_sig);                                     // 1+log(tf), normalize         // Use for offline dataset image indexing
	void logtf_idf_unitnormalize(vector<bow_bin_object*>& bow_sig, const float* idf);               // (1+log(tf))*idf, normalize   // Use for online query image bow extractor
	// BoW
	void build_bow(const int* quantized_indices, const vector<float*>& keypoints);                  // Building Bow to multi_bow
	void get_bow(vector<bow_bin_object*>& bow_sig);                                                 // Pooling current multi_bow then return one bow_sig an clear memory
	void flush_bow(bool append);                                                                    // Flush bow to disk
	void load_bow_offset();                                                                         // Load bow_offset from disk, use once before calling load_specific_bow
	bool load_specific_bow(size_t image_id, vector<bow_bin_object*>& bow_sig);                      // Load specific bow from disk
	void reset_bow();                                                                               // Clear bow memory
	// Pooling
	void build_pool();                                                                              // Building multi_bow_pool from current multi_bow
	void pooling(vector<bow_bin_object*>& bow_sig);                                                 // Pooling bow_sig from current multi_bow
	void flush_bow_pool(bool append);                                                               // Flush bow_pool to disk
	void load_bow_pool_offset();                                                                    // Load bow_offset from disk, use once before calling load_specific_bow_pool
	bool load_specific_bow_pool(size_t pool_id, vector<bow_bin_object*>& bow_sig);                  // Load specific bow from disk
	void reset_bow_pool();                                                                          // Clear bow_pool memory
};

};
//;)
