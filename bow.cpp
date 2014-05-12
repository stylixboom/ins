/*
 * bow.cpp
 *
 *  Created on: May 12, 2014
 *      Author: Melin
 *              Siriwat Kasamwattanarote
 */
#include "bow.h"

using namespace std;
using namespace tr1;
using namespace alphautils;

namespace ins
{

bow::bow(ins_param param, const vector<int>& quantized_indices, const vector< vector<float> >& keypoints)
{
    run_param = param;
}

bow::~bow(void)
{

}

void bow::set_mask(/*param*/)
{

}

void bow::get_bow(vector<bow_bin_object>& bow_sig)
{

}

}
