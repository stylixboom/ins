/*
 * kp_dumper.h
 *
 *  Created on: November 14, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
#include <unordered_map>

// Siriwat's header
#include "../alphautils/imtools.h"

using namespace alphautils::imtools;

namespace ins
{
    // Anything shared with ins namespace here
    typedef struct _dump_object{ size_t cluster_id; float weight; bool fg; size_t sequence_id; SIFT_Keypoint kp; } dump_object;

class kp_dumper
{
    // Private variables and functions here
    bool is_load = false;
    int _total_img = 0;
    vector<size_t> _dump_ids;

    // dump space
    unordered_map<size_t, vector<dump_object> > dump_space;                 // dump_id, dump_object
public:
    // Public variables and functions here
	kp_dumper();
	~kp_dumper();

    vector<string> _img_roots;
    vector< vector<string> > _img_filenames;

    vector<dump_object>& get_singledump_by_idx(int idx);
    vector<dump_object>& get_singledump_by_dump_id(size_t dump_id);
    int convert_imgfilename_to_sequenceid(int idx, const string& img_filename);
    void get_singledump_with_filter(int idx, size_t sequence_id, vector<dump_object>& single_dump);
    string get_full_imgpath(int idx, int sequence_id);
	void collect_kp(size_t dataset_id, size_t cluster_id, float weight, bool fg, size_t sequence_id, float* kp);
	void collect_kp(size_t dataset_id, size_t cluster_id, float weight, bool fg, size_t sequence_id, SIFT_Keypoint kp);
	void load(const string& in_path);
	void dump(const string& out_path, const vector<size_t>& dump_ids, const vector<string>& img_roots, const vector< vector<string> >& img_filenames);
	void reset();
};

};
//;)
