/*
 * bow.h
 *
 *  Created on: May 12, 2014
 *      Author: Melin
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
#include <tr1/unordered_map>
#include <math.h>
#include <algorithm> // sort
#include <stdlib.h> // exit

#include "../alphautils/alphautils.h"
#include "ins_param.h"
#include "invert_index.h"

#include "version.h"

using namespace std;
using namespace tr1;
using namespace alphautils;

namespace ins
{
    // Anything shared with ins namespace here

class bow
{
    // Private variables and functions here
    ins_param run_param;
public:
    // Public variables and functions here
	bow(ins_param param);
	~bow(void);
	void set_mask(/*param*/);
	void get_bow(vector<bow_bin_object>& bow_sig);

};

};
//;)
