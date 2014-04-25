/*
 * homography.h
 *
 *  Created on: January 14, 2013
 *      Author: Siriwat Kasamwattanarote
 */
#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h> // usleep
#include "opencv2/core/core.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include "../alphautils/alphautils.h"

using namespace std;
using namespace cv;
using namespace alphautils;

namespace ins
{

class homography
{
	vector< pair <size_t, int> > lookupResult; // index, miss match
	vector< vector < DMatch > > rawResult;
	string TempDir;
	string MatchImgDir;

    // Comparision
    template<template <typename> class P = std::greater >
    struct compare_pair_second {
        template<class T1, class T2> bool operator()(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right) {
            return P<T2>()(left.second, right.second);
        }
    };

public:
	homography(void);
	~homography(void);
	void Process(const string& queryfile, const string& frameroot, const vector<string>& framedir, const vector<string>& framename, const vector< pair<size_t, float> >& rank, const size_t maxLimit);
	void GetReRanked(vector< pair <size_t, int> >& result);
	void Reset(void);
};

};
