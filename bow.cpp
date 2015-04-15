/*
 * bow.cpp
 *
 *  Created on: May 12, 2014
 *              Siriwat Kasamwattanarote
 */
#include <ctime>
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
#include <algorithm>    // sort
#include <cstdlib>      // exit

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "../alphautils/imtools.h"
#include "../alphautils/hdf5_io.h"
#include "../sifthesaff/SIFThesaff.h"
#include "invert_index.h"
#include "ins_param.h"

// Merlin's header
#include "compat.hpp"
#include "retr.hpp"
#include "core.hpp"     // Mask
#include <memory>       // shared_ptr

#include "bow.h"

#include "version.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace alphautils::hdf5io;
using namespace ins;

namespace ins
{

bow::bow(void)
{

}

bow::~bow(void)
{
    reset_bow();
    reset_bow_pool();
}

void bow::init(const ins_param& param)
{
    run_param = param;
    bow_offset_ready = false;
    bow_pool_offset_ready = false;
}

// Tools
void bow::rooting_bow(vector<bow_bin_object*>& bow_sig)
{
    /// Traversing BOW
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        if (run_param.powerlaw_bgonly && bow_sig[bin_idx]->fg)
            continue;
        /// Power law on raw frequency
        bow_sig[bin_idx]->weight = sqrt(double(bow_sig[bin_idx]->weight));
    }
}

void bow::rooting_lastbow()
{
    /// RECOMMENDED to do this on raw frequency
    /// To lower the effect of peak
    /// by using powerlaw normalization
    vector<bow_bin_object*>& last_bow = multi_bow.back();

    /// Power law
    rooting_bow(last_bow);
}

int bow::masking_lastbow(const unique_ptr<Mask>& mask, const float width, const float height)
{
    vector<bow_bin_object*>& last_bow = multi_bow.back();

    int total_kp_pass = 0;

    /// Traversing BOW
    for (size_t bin_idx = 0; bin_idx < last_bow.size(); bin_idx++)
    {
        bow_bin_object* bin_obj = last_bow[bin_idx];

        if (!run_param.asymmetric_enable)
        {
            /// Clear weight
            bin_obj->weight = 0.0f;
        }

        /// Mask pass checking
        for (size_t feature_idx = 0; feature_idx < bin_obj->features.size(); feature_idx++)
        {
            // Checking bin_obj->features[feature_idx]
            // Check with different mask mode, and return result to kp_pass, with weight kp_weight
            float ptx = bin_obj->features[feature_idx]->kp[0];
            float pty = bin_obj->features[feature_idx]->kp[1];
            if (run_param.normpoint)
            {
                ptx *= width;
                pty *= height;
            }
            float mask_weight = mask->level(ptx, pty);

            /// Asymmetric mask mode
            if (run_param.asymmetric_enable)
            {
                if (mask_weight < MASK_THRESHOLD)     /// if not pass, flag as background
                {
                    //bin_obj->features[feature_idx]->weight = run_param.asym_bg_weight;
                }
                else                        /// if pass, flag as foreground
                {
                    //bin_obj->features[feature_idx]->weight = run_param.asym_fg_weight;
                    bin_obj->fg |= true;
                    total_kp_pass++;
                }
            }
            /// Normal mask mode
            else
            {
                if (mask_weight < MASK_THRESHOLD)
                {
                    /// If not pass, remove this feature
                    // Ref: http://www.cplusplus.com/forum/general/42121/
                    // if not pass, do fast removing vector element, the original data order will be lost
                    if (feature_idx < bin_obj->features.size() - 1)
                        swap(bin_obj->features[feature_idx], bin_obj->features.back());   // swap to back


                    // Release memory of this back() feature before being removed
                    delete[] bin_obj->features.back()->kp;
                    delete bin_obj->features.back();

                    bin_obj->features.pop_back();                                              // remove back

                    feature_idx--;  // step back, since this index was removed
                }
                else
                {
                    /// If pass, accumulating weight (raw weight without tf)
                    bin_obj->weight += 1.0f;
                    /// Mask foreground
                    bin_obj->fg |= true;
                    total_kp_pass++;
                }
            }
        }

        if (!run_param.asymmetric_enable)
        {
            /// If all features fail, remove this bin
            if (bin_obj->features.size() == 0)
            {
                if (bin_idx < last_bow.size() -1)    // check if this is not the last element, then allow to swap with last before being remove
                    swap(last_bow[bin_idx], last_bow.back());     // swap to back

                // Release memory of this bow_bin_object before being remove
                delete last_bow.back();

                last_bow.pop_back();                         // remove back

                bin_idx--;  // step back, since this index was removed
            }
        }
    }

    return total_kp_pass;
}

void bow::logtf_unitnormalize(vector<bow_bin_object*>& bow_sig)
{
    size_t bow_sig_size = bow_sig.size();

    /// tf, better take log from double value
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        bow_sig[bin_idx]->weight = log10(1 + double(bow_sig[bin_idx]->weight));   // tf = log10(1+weight)

    /// Normalization
    // Unit length
    float sum_of_square = 0.0f;
    float unit_length = 0.0f;
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        sum_of_square += bow_sig[bin_idx]->weight * bow_sig[bin_idx]->weight;
    unit_length = sqrt(sum_of_square);

    // Normalizing
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        bow_sig[bin_idx]->weight = bow_sig[bin_idx]->weight / unit_length;
}

void bow::logtf_idf_unitnormalize(vector<bow_bin_object*>& bow_sig, const float* idf)
{
    size_t bow_sig_size = bow_sig.size();

    /// tf-idf
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        bow_sig[bin_idx]->weight = log10(1 + double(bow_sig[bin_idx]->weight)) * idf[bow_sig[bin_idx]->cluster_id];   // tf-idf = log10(1+weight) * idf[cluster_id]

    /// Normalization
    // Unit length
    float sum_of_square = 0.0f;
    float unit_length = 0.0f;
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        sum_of_square += bow_sig[bin_idx]->weight * bow_sig[bin_idx]->weight;
    unit_length = sqrt(sum_of_square);

    // Normalizing
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        bow_sig[bin_idx]->weight = bow_sig[bin_idx]->weight / unit_length;
}

void bow::bow_filter(vector<bow_bin_object*>& bow_sig, const size_t min_word, const size_t max_word)
{
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        bow_bin_object* bin_obj = bow_sig[bin_idx];
        // Check if this bin out of range
        if (bin_obj->cluster_id < min_word || bin_obj->cluster_id > max_word)
        {
            // Remove bin if cluster_id is out of range
            // 1. Removing features
            size_t feature_size = bin_obj->features.size();
            for (size_t feature_idx = 0; feature_idx < feature_size; feature_idx++)
            {
                delete[] bin_obj->features[feature_idx]->kp;        // delete[] kp*
                delete bin_obj->features[feature_idx];              // delete feature_object*
            }
            vector<feature_object*>().swap(bin_obj->features);
            // 2. Removing bin
            delete bin_obj;                                         // delete bow_bin_object*

            if (bin_idx < bow_sig.size() - 1)                       // check if this bin is not the last element, then allow to swap with last before being remove
                swap(bow_sig[bin_idx], bow_sig.back());             // swap to back

            bow_sig.pop_back();                                     // remove back

            bin_idx--;                                              // Step back, since last index was removed
        }
    }
}

bool bow::load_running_bows(const vector<size_t>& image_ids, vector< vector<bow_bin_object*> >& bow_sigs)
{
    if (run_param.pooling_enable)
        return load_specific_bow_pools(image_ids, bow_sigs);
    else
        return load_specific_bows(image_ids, bow_sigs);
}

size_t bow::get_running_bow_size()
{
    if (run_param.pooling_enable)
        return get_bow_pool_size();
    else
        return get_bow_size();
}

// BoW (build per image)
void bow::build_bow(const int* quantized_indices, const vector<float*>& keypoints, const size_t image_id)
{
    /// Building Bow
    /*cout << "quantized_indices.size(): " << quantized_indices.size() << " keypoints.size(): " << keypoints.size() << endl;
    for (size_t kp_idx = 0; kp_idx < quantized_indices.size(); kp_idx++)
        cout << "quantized_indices[" << kp_idx << "]: " << quantized_indices[kp_idx] << endl;*/


    /// Initializing a sparse bow
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE];
    fill_n(non_zero_bin, run_param.CLUSTER_SIZE, false);

    unordered_map<size_t, vector<feature_object*> > curr_sparse_bow; // sparse of feature cluster_id -> vector<feature>
    size_t current_feature_amount = keypoints.size();
    //cout << "current_feature_amount: " << current_feature_amount << " ";
    for (size_t feature_id = 0; feature_id < current_feature_amount; feature_id++)
    {
        // Get cluster from quantized index of this feature_id
        size_t cluster_id = quantized_indices[feature_id];

        // Mask non-zero
        non_zero_bin[cluster_id] = true;

        // Create new feature object with feature_id and spatial information, x,y,a,b,c
        feature_object* feature = new feature_object();
        feature->image_id  = image_id;              // image_id or pool_id, depends on sender
        feature->sequence_id = multi_bow.size();    // an order of frame, frame number, or called sequence number
        feature->kp = keypoints[feature_id]; // pointer to x y a b c    // <---- Memory create externally

        // Keep new feature into its corresponding bin (cluster_id)
        curr_sparse_bow[cluster_id].push_back(feature);
    }

    /// Building a compact bow
    vector<bow_bin_object*> curr_compact_bow;
    unordered_map<size_t, vector<feature_object*> >::iterator spare_bow_it;
    for (spare_bow_it = curr_sparse_bow.begin(); spare_bow_it != curr_sparse_bow.end(); spare_bow_it++)
    {
        // For each non-zero bin of cluster,
        // Create new bin with cluster_id, frequency, and its features
        bow_bin_object* bow_bin = new bow_bin_object();
        bow_bin->cluster_id = spare_bow_it->first;

        // Raw weight (without tf)
        bow_bin->weight = spare_bow_it->second.size();
        bow_bin->features.swap(spare_bow_it->second);

        // Keep new bin into compact_bow
        curr_compact_bow.push_back(bow_bin);
    }
    //cout << "curr_compact_bow.size(): " << curr_compact_bow.size() << " ";

    /// Keep compact bow together in multi_bow
    multi_bow.push_back(curr_compact_bow);
    /// Keep image_id
    multi_bow_image_id.push_back(image_id);
    //cout << "multi_bow.size(): " << multi_bow.size() << endl;

    /// Release memory
    delete[] non_zero_bin;
    //curr_sparse_bow.clear();
}

vector<bow_bin_object*>& bow::finalize_bow()
{
    if (run_param.earlyfusion_enable)
    {
        build_pool(run_param.earlyfusion_mode);             // if earlyfusion_enable, do pooling
        return multi_bow_pool[multi_bow_pool.size() - 1];   // then, reference to the last multi_bow_pool
    }
    else
        return multi_bow[multi_bow.size() - 1];             // otherwise, just reference to the last multi_bow result
}

vector<bow_bin_object*>& bow::get_bow_at(const size_t idx)
{
    return multi_bow[idx];
}

vector<bow_bin_object*>& bow::get_last_bow()
{
    return multi_bow[multi_bow.size() - 1];
}

vector<bow_bin_object*>& bow::get_bow_pool_at(const size_t idx)
{
    return multi_bow_pool[idx];
}

void bow::flush_bow(bool append)
{
    size_t bow_count;
    size_t curr_bow_offset;

    fstream OutFile;
    if (append)
        OutFile.open(run_param.bow_path.c_str(), ios::binary | ios::in | ios::out);
    else
        OutFile.open(run_param.bow_path.c_str(), ios::binary | ios::out);
    if (OutFile.is_open())
    {
        // Bow count
        // If appending, read existing bow_count from its header
        if (append)
        {
            // Read current bow_count
            OutFile.seekg(0, OutFile.beg);
            OutFile.read((char*)(&bow_count), sizeof(bow_count));

            // Update bow_count for append mode
            OutFile.seekp(0, OutFile.beg);
            bow_count += multi_bow.size();
            OutFile.write(reinterpret_cast<char*>(&bow_count), sizeof(bow_count));

            // Go to the end of stream
            OutFile.seekp(0, OutFile.end);
            curr_bow_offset = OutFile.tellp();
        }
        else
        {
            // Write at the beginning of stream for normal mode
            bow_count = multi_bow.size();
            OutFile.write(reinterpret_cast<char*>(&bow_count), sizeof(bow_count));

            // Start after read bow_count
            curr_bow_offset = sizeof(bow_count);
        }

        // Bow hist
        for (size_t bow_idx = 0; bow_idx < multi_bow.size(); bow_idx++)
        {
            // Keep offset
            bow_offset.push_back(curr_bow_offset);

            // Image ID (write for reference, but not use)
            OutFile.write(reinterpret_cast<char*>(&multi_bow_image_id[bow_idx]), sizeof(multi_bow_image_id[bow_idx]));
            curr_bow_offset += sizeof(multi_bow_image_id[bow_idx]);

            // Non-zero count
            size_t bin_count = multi_bow[bow_idx].size();
            OutFile.write(reinterpret_cast<char*>(&bin_count), sizeof(bin_count));
            curr_bow_offset += sizeof(bin_count);

            // Bin
            for (size_t bin_id = 0; bin_id < bin_count; bin_id++)
            {
                // Cluster ID
                size_t cluster_id = multi_bow[bow_idx][bin_id]->cluster_id;
                OutFile.write(reinterpret_cast<char*>(&cluster_id), sizeof(cluster_id));
                curr_bow_offset += sizeof(cluster_id);

                // Weight
                float weight = multi_bow[bow_idx][bin_id]->weight;
                OutFile.write(reinterpret_cast<char*>(&weight), sizeof(weight));
                curr_bow_offset += sizeof(weight);

                // Feature Count
                size_t feature_count = multi_bow[bow_idx][bin_id]->features.size();
                OutFile.write(reinterpret_cast<char*>(&feature_count), sizeof(feature_count));
                curr_bow_offset += sizeof(feature_count);
                for (size_t bow_feature_id = 0; bow_feature_id < feature_count; bow_feature_id++)
                {
                    // Write all features from bin
                    feature_object* feature = multi_bow[bow_idx][bin_id]->features[bow_feature_id];
                    // Image ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->image_id)), sizeof(feature->image_id));
                    curr_bow_offset += sizeof(feature->image_id);
                    // Sequence ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->sequence_id)), sizeof(feature->sequence_id));
                    curr_bow_offset += sizeof(feature->sequence_id);
                    // x y a b c
                    int head_size = SIFThesaff::GetSIFTHeadSize();
                    OutFile.write(reinterpret_cast<char*>(feature->kp), head_size * sizeof(*(feature->kp)));
                    curr_bow_offset += head_size * sizeof(*(feature->kp));
                }
            }
        }

        // Write offset
        bin_write_vector_SIZET(run_param.bow_offset_path, bow_offset, append);

        // Close file
        OutFile.close();
    }
}

void bow::load_bow_offset()
{
    cout << "Loading BOW offset..."; cout.flush();
    if (!bin_read_vector_SIZET(run_param.bow_offset_path, bow_offset))
    {
        cout << "BOW offset file does not exits, (" << run_param.bow_offset_path << ")" << endl;
        exit(-1);
    }
    cout << "done!" << endl;

    bow_offset_ready = true;
}

bool bow::load_specific_bow(const size_t image_id, vector<bow_bin_object*>& bow_sig)
{
    // Transform one image to multiple image function
    vector<size_t> one_image;
    one_image.push_back(image_id);
    vector< vector<bow_bin_object*> > one_bow_sig(1);
    // Load
    bool status = load_specific_bows(one_image, one_bow_sig);
    // Keep
    if (status)
        bow_sig.swap(one_bow_sig[0]);

    return status;
}

bool bow::load_specific_bows(const vector<size_t>& image_ids, vector< vector<bow_bin_object*> >& bow_sigs)
{
    if (!bow_offset_ready)
        load_bow_offset();

    ifstream InFile (run_param.bow_path.c_str(), ios::binary);
    if (InFile)
    {
        size_t image_size = image_ids.size();
        for (size_t image_idx = 0; image_idx < image_size; image_idx++)
        {
            // Current pool_id
            size_t image_id = image_ids[image_idx];

            /// Prepare bow buffer
            size_t curr_offset = bow_offset[image_id];
            size_t buffer_size = 0;
            if (image_id < bow_offset.size() - 1)
                buffer_size = bow_offset[image_id + 1] - curr_offset;
            else
            {
                InFile.seekg(0, InFile.end);
                buffer_size = InFile.tellg();
                buffer_size -= curr_offset;
            }
            char* bow_buffer = new char[buffer_size];
            char* bow_buffer_ptr = bow_buffer;
            InFile.seekg(curr_offset, InFile.beg);
            InFile.read(bow_buffer, buffer_size);

            /// Bow hist
            // Dataset ID (read, but not use) (will be assigned at inverted_index.add())
            size_t dataset_id_read = *((size_t*)bow_buffer_ptr);
            bow_buffer_ptr += sizeof(dataset_id_read);

            // Non-zero count
            size_t bin_count = *((size_t*)bow_buffer_ptr);
            bow_buffer_ptr += sizeof(bin_count);

            // ClusterID and ImageIDs
            int head_size = SIFThesaff::GetSIFTHeadSize();
            for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
            {
                // Cluster ID
                size_t cluster_id = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(cluster_id);

                // Create bin_obj
                bow_bin_object* read_bin = new bow_bin_object();

                // Set cluster_id
                read_bin->cluster_id = cluster_id;

                // Weight
                read_bin->weight = *((float*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(read_bin->weight);

                // Feature count
                size_t feature_count;
                feature_count = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(feature_count);
                for (size_t bow_feature_id = 0; bow_feature_id < feature_count; bow_feature_id++)
                {
                    feature_object* feature = new feature_object();

                    // Image ID
                    feature->image_id = *((size_t*)bow_buffer_ptr);
                    bow_buffer_ptr += sizeof(feature->image_id);
                    // Sequence ID
                    feature->sequence_id = *((size_t*)bow_buffer_ptr);
                    bow_buffer_ptr += sizeof(feature->sequence_id);
                    // x y a b c
                    feature->kp = new float[head_size];
                    for (int head_idx = 0; head_idx < head_size; head_idx++)
                    {
                        feature->kp[head_idx] = *((float*)bow_buffer_ptr);
                        bow_buffer_ptr += sizeof(feature->kp[head_idx]);
                    }

                    read_bin->features.push_back(feature);
                }

                // Keep bow
                bow_sigs[image_idx].push_back(read_bin);
            }

            // Release bow_buffer
            delete[] bow_buffer;
        }

        // Close file
        InFile.close();

        return true;
    }
    else
    {
        cout << "Bow path incorrect! : " << run_param.bow_path << endl;
        return false;
    }
}

size_t bow::get_bow_size()
{
    if (!bow_offset_ready)
        load_bow_offset();

    return bow_offset.size();
}

void bow::reset_bow()
{
    if (multi_bow.size())
    {
        // Clear offset
        bow_offset_ready = false;
        vector<size_t>().swap(bow_offset);

        // Clear buffer
        for (size_t bow_idx = 0; bow_idx < multi_bow.size(); bow_idx++)
        {
            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
            {
                for (size_t feature_idx = 0; feature_idx < multi_bow[bow_idx][bin_idx]->features.size(); feature_idx++)
                {
                    delete[] multi_bow[bow_idx][bin_idx]->features[feature_idx]->kp;// delete float* kp[]         // <---- External memory, but delete here, outside do not delete
                    delete multi_bow[bow_idx][bin_idx]->features[feature_idx];          // delete feature_object*
                }
                vector<feature_object*>().swap(multi_bow[bow_idx][bin_idx]->features);
                delete multi_bow[bow_idx][bin_idx];                                     // delete bow_bin_object*
            }
            vector<bow_bin_object*>().swap(multi_bow[bow_idx]);
        }
        vector< vector<bow_bin_object*> >().swap(multi_bow);
        // Clear image_id
        vector<size_t>().swap(multi_bow_image_id);
    }
}

// Pooling, assuming the current multi_bow is of one pool
void bow::build_pool(const int pooling_mode)
{
    // Bypass mode
    //multi_bow_pool.push_back(multi_bow[0]);
    //return;
    // Create empty bow_pool_sig

    // Keep into bow_pool
    multi_bow_pool.push_back(vector<bow_bin_object*>());
    vector<bow_bin_object*>& bow_pool_sig = multi_bow_pool.back();

    // Create sparse bow space
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE];
    fill_n(non_zero_bin, run_param.CLUSTER_SIZE, false);

    unordered_map<size_t, bow_bin_object*> curr_sparse_bow;

    if (pooling_mode == EARLYFUSION_SUM || pooling_mode == POOL_SUM)
    {
        Video video;
        EclipseRoiIndex bowIndex;
        compat::multi_bowToVideo(multi_bow, run_param, video, bowIndex);

        SumPooling pooling;
        pooling.pooling(video, video);

        compat::videoToBow_pool_sig(video, bowIndex, bow_pool_sig);
    }
    else if (pooling_mode == EARLYFUSION_AVG || pooling_mode == POOL_AVG)
    {
        /*Video video;
        EclipseRoiIndex bowIndex;
        compat::multi_bowToVideo(multi_bow, run_param, video, bowIndex);

        AveragePooling pooling;
        pooling.pooling(video, video);

        compat::videoToBow_pool_sig(video, bowIndex, bow_pool_sig);*/
        /// Merging sparse bow
        size_t curr_multi_bow_size = multi_bow.size();
        //cout << "multi_bow.size(): " << curr_multi_bow_size << endl;
        for (size_t bow_idx = 0; bow_idx < curr_multi_bow_size; bow_idx++)
        {
            // Normalization before use
            logtf_unitnormalize(multi_bow[bow_idx]);

            //cout << "multi_bow[" << bow_idx << "].size(): " << multi_bow[bow_idx].size() << endl;
            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
            {
                size_t cluster_id = multi_bow[bow_idx][bin_idx]->cluster_id;
                bool fg = multi_bow[bow_idx][bin_idx]->fg;

                // Initial curr_sparse_bow
                if (!non_zero_bin[cluster_id])
                {
                    // Create new bin
                    bow_bin_object* new_bin = new bow_bin_object();     // <-- New memory create here
                    new_bin->cluster_id = cluster_id;
                    new_bin->weight = 0.0f;
                    new_bin->fg = false;
                    // Keep in curr_sparse_bow
                    curr_sparse_bow[cluster_id] = new_bin;
                    non_zero_bin[cluster_id] = true;
                }

                // Update curr_sparse_bow
                curr_sparse_bow[cluster_id]->fg |= fg;
                curr_sparse_bow[cluster_id]->weight += multi_bow[bow_idx][bin_idx]->weight;     // Reuse memory from outside
                curr_sparse_bow[cluster_id]->features.insert(curr_sparse_bow[cluster_id]->features.end(), multi_bow[bow_idx][bin_idx]->features.begin(), multi_bow[bow_idx][bin_idx]->features.end());
            }
        }
        /// Building compact bow
        unordered_map<size_t, bow_bin_object*>::iterator sparse_bow_it;
        for (sparse_bow_it = curr_sparse_bow.begin(); sparse_bow_it != curr_sparse_bow.end(); sparse_bow_it++)
        {
            // Looking for non-zero bin of cluster,
            // then put that bin together in bow_pool_sig by specified cluster_id

            // Averaging
            sparse_bow_it->second->weight /= double(curr_multi_bow_size);

            // Keep it back to bow_pool_sig
            bow_pool_sig.push_back(sparse_bow_it->second);
        }
    }
    else if (pooling_mode == EARLYFUSION_MAX || pooling_mode == POOL_MAX)
    {

    }
    else if (pooling_mode == EARLYFUSION_FIM)
    {

    }

    // Release memory
    delete[] non_zero_bin;
}

void bow::flush_bow_pool(bool append)
{
    size_t bow_pool_count;
    size_t curr_bow_pool_offset;

    fstream OutFile;
    if (append)
        OutFile.open(run_param.bow_pool_path.c_str(), ios::binary | ios::in | ios::out);
    else
        OutFile.open(run_param.bow_pool_path.c_str(), ios::binary | ios::out);
    if (OutFile.is_open())
    {
        // Bow pool count
        if (append)
        {
            // Read current bow_pool_count
            // If appending, read existing bow_pool_count from its header
            OutFile.seekg(0, OutFile.beg);
            OutFile.read((char*)(&bow_pool_count), sizeof(bow_pool_count));

            // Update bow_pool_count for append mode
            OutFile.seekp(0, OutFile.beg);
            bow_pool_count += multi_bow_pool.size();
            OutFile.write(reinterpret_cast<char*>(&bow_pool_count), sizeof(bow_pool_count));

            // Go to the end of stream
            OutFile.seekp(0, OutFile.end);
            curr_bow_pool_offset = OutFile.tellp();
        }
        else
        {
            // Write at the beginning of stream for normal mode
            bow_pool_count = multi_bow_pool.size();
            OutFile.write(reinterpret_cast<char*>(&bow_pool_count), sizeof(bow_pool_count));

            // Start after read bow_pool_count
            curr_bow_pool_offset = sizeof(bow_pool_count);
        }

        // Bow hist
        for (size_t bow_pool_idx = 0; bow_pool_idx < multi_bow_pool.size(); bow_pool_idx++)
        {
            // Keep offset
            bow_pool_offset.push_back(curr_bow_pool_offset);

            // Dataset ID (write for referencing, but not use)
            OutFile.write(reinterpret_cast<char*>(&bow_pool_idx), sizeof(bow_pool_idx));
            curr_bow_pool_offset += sizeof(bow_pool_idx);

            // Non-zero count
            size_t bin_count = multi_bow_pool[bow_pool_idx].size();
            OutFile.write(reinterpret_cast<char*>(&bin_count), sizeof(bin_count));
            curr_bow_pool_offset += sizeof(bin_count);

            // Bin
            for (size_t bin_id = 0; bin_id < bin_count; bin_id++)
            {
                // Cluster ID
                size_t cluster_id = multi_bow_pool[bow_pool_idx][bin_id]->cluster_id;
                OutFile.write(reinterpret_cast<char*>(&cluster_id), sizeof(cluster_id));
                curr_bow_pool_offset += sizeof(cluster_id);

                // Weight
                float weight = multi_bow_pool[bow_pool_idx][bin_id]->weight;
                OutFile.write(reinterpret_cast<char*>(&weight), sizeof(weight));
                curr_bow_pool_offset += sizeof(weight);

                // Feature Count
                size_t feature_count = multi_bow_pool[bow_pool_idx][bin_id]->features.size();
                OutFile.write(reinterpret_cast<char*>(&feature_count), sizeof(feature_count));
                curr_bow_pool_offset += sizeof(feature_count);
                for (size_t bow_pool_feature_id = 0; bow_pool_feature_id < feature_count; bow_pool_feature_id++)
                {
                    // Write all features from bin
                    feature_object* feature = multi_bow_pool[bow_pool_idx][bin_id]->features[bow_pool_feature_id];
                    // Image ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->image_id)), sizeof(feature->image_id));
                    curr_bow_pool_offset += sizeof(feature->image_id);
                    // Sequence ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->sequence_id)), sizeof(feature->sequence_id));
                    curr_bow_pool_offset += sizeof(feature->sequence_id);
                    // x y a b c
                    int head_size = SIFThesaff::GetSIFTHeadSize();
                    OutFile.write(reinterpret_cast<char*>(feature->kp), head_size * sizeof(*(feature->kp)));
                    curr_bow_pool_offset += head_size * sizeof(*(feature->kp));
                }
            }
        }

        // Write offset
        bin_write_vector_SIZET(run_param.bow_pool_offset_path, bow_pool_offset, append);

        // Close file
        OutFile.close();
    }
}

void bow::load_bow_pool_offset()
{
    cout << "Loading BOW offset..."; cout.flush();
    if (!bin_read_vector_SIZET(run_param.bow_pool_offset_path, bow_pool_offset))
    {
        cout << "BOW pool offset file does not exits, (" << run_param.bow_pool_offset_path << ")" << endl;
        exit(-1);
    }
    cout << "done!" << endl;

    bow_pool_offset_ready = true;
}

bool bow::load_specific_bow_pool(const size_t pool_id, vector<bow_bin_object*>& bow_sig)
{
    // Transform one pool to multiple pool function
    vector<size_t> one_pool;
    one_pool.push_back(pool_id);
    vector< vector<bow_bin_object*> > one_bow_sig(1);
    // Load
    bool status = load_specific_bow_pools(one_pool, one_bow_sig);
    // Keep
    if (status)
        bow_sig.swap(one_bow_sig[0]);

    return status;
}

bool bow::load_specific_bow_pools(const vector<size_t>& pool_ids, vector< vector<bow_bin_object*> >& bow_sigs)  // <---- This will create new memory, please delete[] kp, delete feature_object, and delete bow_bin_object
{
    if (!bow_pool_offset_ready)
        load_bow_pool_offset();

    ifstream InFile (run_param.bow_pool_path.c_str(), ios::binary);
    if (InFile)
    {
        size_t pool_size = pool_ids.size();
        for (size_t pool_idx = 0; pool_idx < pool_size; pool_idx++)
        {
            // Current pool_id
            size_t pool_id = pool_ids[pool_idx];

            /// Prepare buffer
            size_t curr_offset = bow_pool_offset[pool_id];
            size_t buffer_size = 0;
            if (pool_id < bow_pool_offset.size() - 1)
                buffer_size = bow_pool_offset[pool_id + 1] - curr_offset;
            else
            {
                InFile.seekg(0, InFile.end);
                buffer_size = InFile.tellg();
                buffer_size -= curr_offset;
            }
            char* bow_buffer = new char[buffer_size];
            char* bow_buffer_ptr = bow_buffer;
            InFile.seekg(curr_offset, InFile.beg);
            InFile.read(bow_buffer, buffer_size);

            /// Bow hist
            // Dataset ID (read, but not use) (will be assigned at inverted_index.add())
            size_t dataset_id_read = *((size_t*)bow_buffer_ptr);
            bow_buffer_ptr += sizeof(dataset_id_read);

            // Non-zero count
            size_t bin_count = *((size_t*)bow_buffer_ptr);
            bow_buffer_ptr += sizeof(bin_count);

            // ClusterID and FeatureIDs
            int head_size = SIFThesaff::GetSIFTHeadSize();
            for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
            {
                // Cluster ID
                size_t cluster_id = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(cluster_id);

                // Create bin_obj
                bow_bin_object* read_bin = new bow_bin_object();

                // Set cluster_id
                read_bin->cluster_id = cluster_id;

                // Weight
                read_bin->weight = *((float*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(read_bin->weight);

                // Feature count
                size_t feature_count;
                feature_count = *((size_t*)bow_buffer_ptr);
                bow_buffer_ptr += sizeof(feature_count);
                for (size_t bow_feature_id = 0; bow_feature_id < feature_count; bow_feature_id++)
                {
                    feature_object* feature = new feature_object();

                    // Image ID
                    feature->image_id = *((size_t*)bow_buffer_ptr);
                    bow_buffer_ptr += sizeof(feature->image_id);
                    // Sequence ID
                    feature->sequence_id = *((size_t*)bow_buffer_ptr);
                    bow_buffer_ptr += sizeof(feature->sequence_id);
                    // x y a b c
                    feature->kp = new float[head_size];
                    for (int head_idx = 0; head_idx < head_size; head_idx++)
                    {
                        feature->kp[head_idx] = *((float*)bow_buffer_ptr);
                        bow_buffer_ptr += sizeof(feature->kp[head_idx]);
                    }

                    read_bin->features.push_back(feature);
                }

                // Keep bow
                bow_sigs[pool_idx].push_back(read_bin);
            }

            // Release bow_buffer
            delete[] bow_buffer;
        }

        // Close file
        InFile.close();

        return true;
    }
    else
    {
        cout << "Bow pool path incorrect! : " << run_param.bow_pool_path << endl;
        return false;
    }
}

size_t bow::get_bow_pool_size()
{
    if (!bow_pool_offset_ready)
        load_bow_pool_offset();

    return bow_pool_offset.size();
}

void bow::reset_bow_pool()
{
    if (multi_bow_pool.size())
    {
        // Clear offset
        bow_pool_offset_ready = false;
        vector<size_t>().swap(bow_pool_offset);

        // Clear buffer
        for (size_t bow_pool_idx = 0; bow_pool_idx < multi_bow_pool.size(); bow_pool_idx++)
        {
            for (size_t bin_idx = 0; bin_idx < multi_bow_pool[bow_pool_idx].size(); bin_idx++)
            {
                /*for (size_t feature_idx = 0; feature_idx < multi_bow_pool[bow_pool_idx][bin_idx]->features.size(); feature_idx++)
                {
                    //delete[] multi_bow_pool[bow_pool_idx][bin_idx]->features[feature_idx]->kp;  // delete float* kp[]     <---- deleted by reset_bow()
                    //delete multi_bow_pool[bow_pool_idx][bin_idx]->features[feature_idx];        // delete feature_object* <---- deleted by reset_bow()
                }*/
                vector<feature_object*>().swap(multi_bow_pool[bow_pool_idx][bin_idx]->features);
                delete multi_bow_pool[bow_pool_idx][bin_idx];                                   // delete bow_bin_object*
            }
            vector<bow_bin_object*>().swap(multi_bow_pool[bow_pool_idx]);
        }
        vector< vector<bow_bin_object*> >().swap(multi_bow_pool);
    }
}

// Misc
string bow::print(const vector<bow_bin_object*>& bow_sig, const bool print_feature)
{
    stringstream txt_out;
    size_t feature_size_print = 0;
    txt_out << "Bag-of-word size = " << bow_sig.size();
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        txt_out << "bin# " << bin_idx << "\tcluster_id: " << bow_sig[bin_idx]->cluster_id << "\tweight: " << bow_sig[bin_idx]->weight << "\tfeature_size: " << bow_sig[bin_idx]->features.size() << "\n";
        feature_size_print += bow_sig[bin_idx]->features.size();
        if (print_feature)
        {
            for (size_t feature_idx = 0; feature_idx < bow_sig[bin_idx]->features.size(); feature_idx++)
            {
                txt_out << "  feature# " << feature_idx <<
                "\timage_id: " << bow_sig[bin_idx]->features[feature_idx]->image_id <<
                "\tsequence_id: " << bow_sig[bin_idx]->features[feature_idx]->sequence_id <<
                "\t[" << bow_sig[bin_idx]->features[feature_idx]->kp[0] <<
                " " << bow_sig[bin_idx]->features[feature_idx]->kp[1] <<
                " " << bow_sig[bin_idx]->features[feature_idx]->kp[2] <<
                " " << bow_sig[bin_idx]->features[feature_idx]->kp[3] <<
                " " << bow_sig[bin_idx]->features[feature_idx]->kp[4] << "]\n";
            }
        }
    }
    txt_out << "total_feature: " << feature_size_print << "\n";

    return txt_out.str();
}

void bow::release_bowsig(vector<bow_bin_object*>& bow_sig)
{
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        for (size_t feature_idx = 0; feature_idx < bow_sig[bin_idx]->features.size(); feature_idx++)
        {
            delete[] bow_sig[bin_idx]->features[feature_idx]->kp;  // delete float* kp[]
            delete bow_sig[bin_idx]->features[feature_idx];        // delete feature_object*
        }
        vector<feature_object*>().swap(bow_sig[bin_idx]->features);
        delete bow_sig[bin_idx];                                   // delete bow_bin_object*
    }
    vector<bow_bin_object*>().swap(bow_sig);
}

}
//;)
