/*
 * dataset_info.h
 *
 *  Created on: July 9, 2012
 *      Author: Siriwat Kasamwattanarote
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

#include <sys/types.h> // for checking exist file and dir
#include <sys/stat.h>

#include "../alphautils/alphautils.h"

using namespace std;
using namespace alphautils;

namespace ins
{

class dataset_info
{
    string ins_root;
    struct ShotInfo{
        int cfid;
        int year;
        int month;
        int day;
        int hour;
        int cStart;
        int cEnd;
        int sStart;
        int sEnd;
        int freq;
        string channel;
        string path;
    };
    size_t _dataset_size;
    vector<ShotInfo> _dataset_description;
    vector<string> _dataset_frames_dirname;
    vector<string> _dataset_frames_filename;
public:
	dataset_info(void);
	~dataset_info(void);
	size_t dataset_lookup_table_init(const string& ins_root_path);
    size_t dataset_cf_lookup_table_init(const string& ins_root_path);
    void commercial_init();
    void GenFrameFileName(vector<string>& ReturnPaths, const ShotInfo& shot);
    string GenNCheckMiddleFrameFileName(const ShotInfo& shot);
    ShotInfo ShotInfoParse(const string& path);
    const size_t& dataset_size()  const { return _dataset_size; }
    const vector<ShotInfo>& dataset_description() const { return _dataset_description; }
    const vector<string>& dataset_frames_dirname() const { return _dataset_frames_dirname; }
    const vector<string>& dataset_frames_filename() const { return _dataset_frames_filename; }
    void dataset_lookup_table_destroy();
};

};
