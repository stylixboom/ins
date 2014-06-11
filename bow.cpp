/*
 * bow.cpp
 *
 *  Created on: May 12, 2014
 *              Siriwat Kasamwattanarote
 */
#include "bow.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace alphautils::hdf5io;
using namespace ins;
//using namespace geom;

namespace ins
{

bow::bow(ins_param param)
{
    run_param = param;
    bow_offset_ready = false;
    bow_pool_offset_ready = false;
}

bow::~bow(void)
{
    reset_bow();
    reset_bow_pool();
}

// Tools
void bow::masking(vector<bow_bin_object*>& bow_sig)
{
    /// Traversing BOW
    for (size_t bin_idx = 0; bin_idx < bow_sig.size(); bin_idx++)
    {
        /// Clear weight
        bow_sig[bin_idx]->weight = 0.0f;

        /// Mask pass checking
        for (size_t feature_idx = 0; feature_idx < bow_sig[bin_idx]->features.size(); feature_idx++)
        {
            // Checking bow_sig[bin_idx]->features[feature_idx]
            // Check with different mask mode, and return result to kp_pass, with weight kp_weight
            bool kp_pass = false;
            float kp_weight = 0.0f;

            if (!kp_pass)
            {
                /// If not pass, remove this feature
                // Ref: http://www.cplusplus.com/forum/general/42121/
                // if not pass, do fast removing vector element, the original data order will be lost
                swap(bow_sig[bin_idx]->features[feature_idx], bow_sig[bin_idx]->features.back());   // swap to back

                // Release memory of this back() feature before being removed
                delete[] bow_sig[bin_idx]->features.back()->kp;
                delete bow_sig[bin_idx]->features.back();

                bow_sig[bin_idx]->features.pop_back();                                              // remove back
            }
            else
            {
                /// If pass, accumulating weight (raw weight without tf)
                bow_sig[bin_idx]->weight += kp_weight;
            }
        }

        /// If all features fail, remove this bin
        if (bow_sig[bin_idx]->features.size() == 0)
        {
            swap(bow_sig[bin_idx], bow_sig.back());     // swap to back

            // Release memory of this bow_bin_object before being remove
            delete bow_sig.back();

            bow_sig.pop_back();                         // remove back
        }
    }
}

void bow::logtf_unitnormalize(vector<bow_bin_object*>& bow_sig)
{
    size_t bow_sig_size = bow_sig.size();

    /// tf
    for (size_t bin_idx = 0; bin_idx < bow_sig_size; bin_idx++)
        bow_sig[bin_idx]->weight = 1 + log10(bow_sig[bin_idx]->weight);   // tf = 1 + log10(weight)

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
        bow_sig[bin_idx]->weight = (1 + log10(bow_sig[bin_idx]->weight)) * idf[bow_sig[bin_idx]->cluster_id];   // tf-idf = (1 + log10(weight)) * idf[cluster_id]

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

// BoW (build per image)
void bow::build_bow(const int* quantized_indices, const vector<float*>& keypoints)
{
    /// Building Bow
    /*cout << "quantized_indices.size(): " << quantized_indices.size() << " keypoints.size(): " << keypoints.size() << endl;
    for (size_t kp_idx = 0; kp_idx < quantized_indices.size(); kp_idx++)
        cout << "quantized_indices[" << kp_idx << "]: " << quantized_indices[kp_idx] << endl;*/


    /// Initializing a sparse bow
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE];
    for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
        non_zero_bin[cluster_id] = false;
    unordered_map<size_t, vector<feature_object*> > curr_sparse_bow; // sparse of feature
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
        feature->feature_id  = feature_id;
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
    //cout << "multi_bow.size(): " << multi_bow.size() << endl;

    /// Release memory
    delete[] non_zero_bin;
    //curr_sparse_bow.clear();
}

void bow::get_bow(vector<bow_bin_object*>& bow_sig)
{
    // Pooling then return bow
    pooling(bow_sig);

    // Release memory
    reset_bow();
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

            // Dataset ID (write for referencing, but not use)
            OutFile.write(reinterpret_cast<char*>(&bow_idx), sizeof(bow_idx));
            curr_bow_offset += sizeof(bow_idx);

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
                    // Feature ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->feature_id)), sizeof(feature->feature_id));
                    curr_bow_offset += sizeof(feature->feature_id);
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

bool bow::load_specific_bow(size_t image_id, vector<bow_bin_object*>& bow_sig)
{
    if (!bow_offset_ready)
        load_bow_offset();

    ifstream InFile (run_param.bow_path.c_str(), ios::binary);
    if (InFile)
    {
        /// Skip to bow of specific image_id
        size_t curr_offset = bow_offset[image_id];
        InFile.seekg(curr_offset, InFile.beg);

        /// Bow hist
        // Dataset ID (read, but not use)
        size_t dataset_id_read;
        InFile.read((char*)(&dataset_id_read), sizeof(dataset_id_read));

        // Dataset bow
        vector<bow_bin_object*> read_bow;

        // Non-zero count
        size_t bin_count;
        InFile.read((char*)(&bin_count), sizeof(bin_count));

        // ClusterID and FeatureIDs
        for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
        {
            bow_bin_object* read_bin = new bow_bin_object();

            // Cluster ID
            InFile.read((char*)(&(read_bin->cluster_id)), sizeof(read_bin->cluster_id));

            // Weight
            InFile.read((char*)(&(read_bin->weight)), sizeof(read_bin->weight));

            // Feature count
            size_t feature_count;
            InFile.read((char*)(&feature_count), sizeof(feature_count));
            for (size_t bow_feature_id = 0; bow_feature_id < feature_count; bow_feature_id++)
            {
                feature_object* feature = new feature_object();

                // Feature ID
                InFile.read((char*)(&(feature->feature_id)), sizeof(feature->feature_id));
                // x y a b c
                int head_size = SIFThesaff::GetSIFTHeadSize();
                feature->kp = new float[head_size];
                for (int head_idx = 0; head_idx < head_size; head_idx++)
                    InFile.read((char*)(&(feature->kp[head_idx])), sizeof(*(feature->kp)));

                read_bin->features.push_back(feature);
            }

            // Keep bow
            read_bow.push_back(read_bin);
        }

        // Keep hist
        bow_sig.swap(read_bow);

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

void bow::reset_bow()
{
    if (multi_bow.size())
    {
        // Clear offset
        bow_offset_ready = false;
        bow_offset.clear();

        // Clear buffer
        for (size_t bow_idx = 0; bow_idx < multi_bow.size(); bow_idx++)
        {
            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
            {
                for (size_t feature_idx = 0; feature_idx < multi_bow[bow_idx][bin_idx]->features.size(); feature_idx++)
                {
                    delete[] multi_bow[bow_idx][bin_idx]->features[feature_idx]->kp;    // delete float* kp[]         // <---- External memory, but delete here, outside do not delete
                    delete multi_bow[bow_idx][bin_idx]->features[feature_idx];          // delete feature_object*
                }
                multi_bow[bow_idx][bin_idx]->features.clear();
                delete multi_bow[bow_idx][bin_idx];                                     // delete bow_bin_object*
            }
            multi_bow[bow_idx].clear();
        }
        multi_bow.clear();
    }
}

// Pooling, assuming the current multi_bow is of one pool
void bow::build_pool()
{
    // Create empty bow_pool_sig
    vector<bow_bin_object*> bow_pool_sig;

    // Pooling
    pooling(bow_pool_sig);

    // Keep into bow_pool
    multi_bow_pool.push_back(bow_pool_sig);
}

void bow::pooling(vector<bow_bin_object*>& bow_sig)
{
    // Create sparse bow space
    bool* non_zero_bin = new bool[run_param.CLUSTER_SIZE];
    for (size_t cluster_id = 0; cluster_id < run_param.CLUSTER_SIZE; cluster_id++)
        non_zero_bin[cluster_id] = false;

    vector<bow_bin_object*> curr_compact_bow;
    unordered_map<size_t, bow_bin_object*> curr_sparse_bow;

    if (run_param.pooling_mode == POOL_SUM)
    {
        /*Document vid;
        /// Merging sparse bow
        size_t curr_multi_bow_size = multi_bow.size();
        //cout << "multi_bow.size(): " << curr_multi_bow_size << endl;
        for (size_t bow_idx = 0; bow_idx < curr_multi_bow_size; bow_idx++)
        {
            Document img;
            img.setFeature(SparseVector(run_param.CLUSTER_SIZE));
            //cout << "multi_bow[" << bow_idx << "].size(): " << multi_bow[bow_idx].size() << endl;
            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
            {
                img.getFeature().set(multi_bow[bow_idx][bin_idx].cluster_id, multi_bow[bow_idx][bin_idx].weight);
            }
            vid.addFeature(img);
        }
        SumPooling pool;
        pool.pooling(vid, vid);
        auto it = vid.getFeature().nonZeroIterator();
        for (; it->isValid(); it->next()) {
            bow_bin_object curr_bow;
            curr_bow.cluster_id = it->index();
            curr_bow.weight = it->get();
            //curr_bow.features = it->....
            curr_compact_bow.push_back(curr_bow);
        }*/
    }
    else if (run_param.pooling_mode == POOL_AVG)
    {
        /// Merging sparse bow
        size_t curr_multi_bow_size = multi_bow.size();
        //cout << "multi_bow.size(): " << curr_multi_bow_size << endl;
        for (size_t bow_idx = 0; bow_idx < curr_multi_bow_size; bow_idx++)
        {
            //cout << "multi_bow[" << bow_idx << "].size(): " << multi_bow[bow_idx].size() << endl;
            for (size_t bin_idx = 0; bin_idx < multi_bow[bow_idx].size(); bin_idx++)
            {
                size_t cluster_id = multi_bow[bow_idx][bin_idx]->cluster_id;

                // Initial curr_sparse_bow
                if (!non_zero_bin[cluster_id])
                {
                    // Create new bin
                    bow_bin_object* new_bin = new bow_bin_object();
                    new_bin->cluster_id = cluster_id;
                    new_bin->weight = 0;
                    // Keep in curr_sparse_bow
                    curr_sparse_bow[cluster_id] = new_bin;
                    non_zero_bin[cluster_id] = true;
                }

                // Update curr_sparse_bow
                curr_sparse_bow[cluster_id]->weight += multi_bow[bow_idx][bin_idx]->weight;
                curr_sparse_bow[cluster_id]->features.insert(curr_sparse_bow[cluster_id]->features.end(), multi_bow[bow_idx][bin_idx]->features.begin(), multi_bow[bow_idx][bin_idx]->features.end());
            }
        }
        /// Building compact bow
        unordered_map<size_t, bow_bin_object*>::iterator spare_bow_it;
        for (spare_bow_it = curr_sparse_bow.begin(); spare_bow_it != curr_sparse_bow.end(); spare_bow_it++)
        {
            // Looking for non-zero bin of cluster,
            // then put that bin together in curr_compact_bow by specified cluster_id

            // Averaging
            spare_bow_it->second->weight /= curr_multi_bow_size;

            // Keep it back to curr_compact_bow
            curr_compact_bow.push_back(spare_bow_it->second);
        }

        // Swap to return
        bow_sig.swap(curr_compact_bow);
    }
    else if (run_param.pooling_mode == POOL_MAX)
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
                    // Feature ID
                    OutFile.write(reinterpret_cast<char*>(&(feature->feature_id)), sizeof(feature->feature_id));
                    curr_bow_pool_offset += sizeof(feature->feature_id);
                    // x
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

bool bow::load_specific_bow_pool(size_t pool_id, vector<bow_bin_object*>& bow_sig)  // <---- This will create new memory, please delete[] kp, delete feature_object, and delete bow_bin_object
{
    if (!bow_pool_offset_ready)
        load_bow_pool_offset();

    ifstream InFile (run_param.bow_pool_path.c_str(), ios::binary);
    if (InFile)
    {
        /// Skip to bow of specific pool_id
        size_t curr_offset = bow_pool_offset[pool_id];
        InFile.seekg(curr_offset, InFile.beg);

        /// Bow hist
        // Dataset ID (read, but not use)
        size_t dataset_id_read;
        InFile.read((char*)(&dataset_id_read), sizeof(dataset_id_read));

        // Dataset bow
        vector<bow_bin_object*> read_bow_pool;

        // Non-zero count
        size_t bin_count;
        InFile.read((char*)(&bin_count), sizeof(bin_count));

        // ClusterID and FeatureIDs
        for (size_t bin_idx = 0; bin_idx < bin_count; bin_idx++)
        {
            bow_bin_object* read_bin = new bow_bin_object();

            // Cluster ID
            InFile.read((char*)(&(read_bin->cluster_id)), sizeof(read_bin->cluster_id));

            // Weight
            InFile.read((char*)(&(read_bin->weight)), sizeof(read_bin->weight));

            // Feature count
            size_t feature_count;
            InFile.read((char*)(&feature_count), sizeof(feature_count));
            for (size_t bow_feature_id = 0; bow_feature_id < feature_count; bow_feature_id++)
            {
                feature_object* feature = new feature_object();

                // Feature ID
                InFile.read((char*)(&(feature->feature_id)), sizeof(feature->feature_id));
                // x y a b c
                int head_size = SIFThesaff::GetSIFTHeadSize();
                feature->kp = new float[head_size];
                for (int head_idx = 0; head_idx < head_size; head_idx++)
                    InFile.read((char*)(&(feature->kp[head_idx])), sizeof(*(feature->kp)));

                read_bin->features.push_back(feature);
            }

            // Keep bow
            read_bow_pool.push_back(read_bin);
        }

        // Keep hist
        bow_sig.swap(read_bow_pool);

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

void bow::reset_bow_pool()
{
    if (multi_bow_pool.size())
    {
        // Clear offset
        bow_pool_offset_ready = false;
        bow_pool_offset.clear();

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
                multi_bow_pool[bow_pool_idx][bin_idx]->features.clear();
                delete multi_bow_pool[bow_pool_idx][bin_idx];                                   // delete bow_bin_object*
            }
            multi_bow_pool[bow_pool_idx].clear();
        }
        multi_bow_pool.clear();
    }
}

}
//;)
