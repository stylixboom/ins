// *** ADDED BY HEADER FIXUP ***
#include <ctime>
// *** END ***
/*
 * quantizer.cpp
 *
 *  Created on: June 6, 2014
 *              Siriwat Kasamwattanarote
 */
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

#include <flann/flann.hpp>

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "../alphautils/hdf5_io.h"
#include "ins_param.h"

#include "quantizer.h"

using namespace std;
using namespace ::flann;
using namespace alphautils;
using namespace alphautils::hdf5io;
using namespace ins;

namespace ins
{

quantizer::quantizer(void)
{

}

quantizer::~quantizer(void)
{
    // Release memory
    delete[] cluster.ptr();
}

void quantizer::init(ins_param param)
{
    run_param = param;

    // Initial variable
    cluster_ready = false;

    /// Search index
    if (!is_path_exist(run_param.searchindex_path))
        build_index();
    else
        load_savedindex();

    // Search param
    sparams = SearchParams();
    //sparams.checks = FLANN_CHECKS_AUTOTUNED;
    //sparams.checks = FLANN_CHECKS_UNLIMITED; // for only one tree
    sparams.checks = 512;               // Higher is better, also slower
    sparams.cores = run_param.MAXCPU;   // Take max CPU cores to run
    knn = 1;                            // 1 for hard assignment
}

void quantizer::quantize(const Matrix<float>& data, const int data_size, Matrix<int>& quantized_index, Matrix<float>& quantized_dist)
{
    quantized_index = Matrix<int>(new int[data_size * knn], data_size, knn); // size = feature_amount x knn
    quantized_dist = Matrix<float>(new float[data_size * knn], data_size, knn);

    flann_search_index.knnSearch(data, quantized_index, quantized_dist, knn, sparams);
}

void quantizer::load_cluster()
{
    cout << "Load cluster..";
    cout.flush();
    startTime = CurrentPreciseTime();
    // Release memory
    delete[] cluster.ptr();

    size_t cluster_amount;      // Cluster size
    size_t cluster_dimension;   // Feature dimension

    // Get HDF5 header
    HDF_get_2Ddimension(run_param.cluster_path, "clusters", cluster_amount, cluster_dimension);
    cout << "[" << cluster_dimension << ", " << cluster_amount << "].."; cout.flush();

    // Wrap data to maxrix for flann knn searching
    float* empty_cluster;   // will be replaced by read HDF5
    // float* empty_cluster = new float[cluster_amount * cluster_dimension];

    // Read from HDF5
    HDF_read_2DFLOAT(run_param.cluster_path, "clusters", empty_cluster, cluster_amount, cluster_dimension);

    Matrix<float> read_cluster(empty_cluster, cluster_amount, cluster_dimension);

    // Keep cluster
    cluster = read_cluster;
    data_dimension = cluster_dimension;
    cluster_ready = true;

    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(startTime) << " s)" << endl;
}

void quantizer::load_savedindex()
{
    if (!cluster_ready)
        load_cluster();
    cout << "Load FLANN search index..";
    cout.flush();
    startTime = CurrentPreciseTime();
    Index< ::flann::L2<float> > search_index(cluster, SavedIndexParams(run_param.searchindex_path)); // load index with provided dataset
    flann_search_index = search_index;
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(startTime) << " s)" << endl;
}

void quantizer::build_index()
{
    if (!cluster_ready)
        load_cluster();
    // Building search index
    cout << "Build FLANN search index..";
    cout.flush();
    startTime = CurrentPreciseTime();
    Index< ::flann::L2<float> > search_index(KDTreeIndexParams((int)run_param.KDTREE));
    search_index.buildIndex(cluster);
    flann_search_index = search_index;
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(startTime) << " s)" << endl;

    // Saving search index
    cout << "Saving index..";
    cout.flush();
    startTime = CurrentPreciseTime();
    flann_search_index.save(run_param.searchindex_path);
    cout << "done! (in " << setprecision(2) << fixed << TimeElapse(startTime) << " s)" << endl;
}

}
//;)
