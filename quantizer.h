/*
 * quantizer.h
 *
 *  Created on: June 6, 2014
 *              Siriwat Kasamwattanarote
 */
#pragma once
#include <flann/flann.hpp>

using namespace ::flann;

namespace ins
{
    // Anything shared with ins namespace here

class quantizer
{
    // Private variables and functions here
    ins_param run_param;
    timespec startTime;

    // Cluster file
    bool cluster_ready;
    int data_dimension;
    Matrix<float> cluster;

    // Quantizer, default 16 parallel tree
    SearchParams sparams;
    Index<::flann::L2<float>> flann_search_index{KDTreeIndexParams(16)}; // C++11 initialize class member

public:
    // Public variables and functions here
	quantizer(void);
	~quantizer(void);
    size_t knn;

    void init(ins_param param);
    void quantize(const Matrix<float>& data, const int data_size, Matrix<int>& quantized_index, Matrix<float>& quantized_dist);
	void load_cluster();
	void load_savedindex();
	void build_index();
};

};
//;)
