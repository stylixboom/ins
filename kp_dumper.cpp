/*
 * kp_dumper.cpp
 *
 *  Created on: November 14, 2014
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
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>    // sort
#include <cstdlib>      // exit
#include <memory>       // uninitialized_fill_n
#include <omp.h>

// Siriwat's header
#include "../alphautils/alphautils.h"
#include "../alphautils/imtools.h"

#include "kp_dumper.h"

#include "version.h"

using namespace std;
using namespace alphautils;
using namespace alphautils::imtools;
using namespace ins;

namespace ins
{

kp_dumper::kp_dumper()
{

}

kp_dumper::~kp_dumper()
{
    reset();
}

// Get from one shot
vector<dump_object>& kp_dumper::get_singledump_by_idx(int idx)
{
    // Set reference from redirection idx to data_id
    return dump_space[_dump_ids[idx]];
}

/// Special usage for fetch the collected dataset information by dataset_id
/// application eg, dataset visualization, reranking, etc.
/// Note: dump_id == dataset_id
vector<dump_object>& kp_dumper::get_singledump_by_dump_id(size_t dump_id)
{
    return dump_space[dump_id];
}

// Get only a specified sequence_id
int kp_dumper::convert_imgfilename_to_sequenceid(int idx, const string& img_filename)
{
    vector<string>& single_dump_filename = _img_filenames[idx];
    for (size_t sequence_id = 0; sequence_id < single_dump_filename.size(); sequence_id++)
    {
        if (single_dump_filename[sequence_id] == img_filename)
            return sequence_id;
    }

    // img_filename not found
    cout << "Specified image filename \"" << img_filename << "\" for index " << idx << " not found" << endl;
    exit(-1);
}

void kp_dumper::get_singledump_with_filter(int idx, size_t sequence_id, vector<dump_object>& single_dump)
{
    vector<dump_object>& full_singledump = dump_space[_dump_ids[idx]];
    for (size_t singledump_idx = 0; singledump_idx < full_singledump.size(); singledump_idx++)
    {
        // Filtering with sequence_id
        if (full_singledump[singledump_idx].sequence_id == size_t(sequence_id))
            single_dump.push_back(full_singledump[singledump_idx]);
    }
}

string kp_dumper::get_full_imgpath(int idx, int sequence_id)
{
    string path = _img_roots[idx] + "/" + _img_filenames[idx][sequence_id];
    if (path[0] == '/') // Absolute path
        return path;
    else                // Relative path, assume to be from dataset root (please check consistency with ins_param.cpp)
        return resolve_path(string(getenv("HOME")) + "/webstylix/code/dataset/" + path);
}

void kp_dumper::collect_kp(size_t dataset_id, size_t cluster_id, float weight, bool fg, size_t sequence_id, float* kp)
{
    dump_space[dataset_id].push_back(dump_object{cluster_id, weight, fg, sequence_id, SIFT_Keypoint{kp[0], kp[1], kp[2], kp[3], kp[4]}});
}

void kp_dumper::collect_kp(size_t dataset_id, size_t cluster_id, float weight, bool fg, size_t sequence_id, SIFT_Keypoint kp)
{
    dump_space[dataset_id].push_back(dump_object{cluster_id, weight, fg, sequence_id, kp});
}

void kp_dumper::load(const string& in_path)
{
    ifstream InFile (in_path.c_str());
    if (InFile)
    {
        /// Template
        /*
        total_img
        rank_id|root_path|total_frame|frame1;frame2|total_kp|cluster_id,weight,fg,sequence_id,x,y,a,b,c;cluster_id,weight,fg,sequence_id,x,y,a,b,c
        */
        /// Separator '|' = variable, ';' = array, ',' = attribute

        // Read total image
        string read_line;
        getline(InFile, read_line);
        _total_img = atoi(read_line.c_str());
        // Read each image
        for (int img_idx = 0; img_idx < _total_img; img_idx++)
        {
            vector<string> variables;
            getline(InFile, read_line);
            StringExplode(read_line, "|", variables);

            // Read dataset_id
            size_t dataset_id = atoi(variables[0].c_str());
            _dump_ids.push_back(dataset_id);

            // Read img_path
            _img_roots.push_back(variables[1]);

            // Read image filenames
            //int total_frame = atoi(variables[2].c_str());
            _img_filenames.push_back(vector<string>());
            vector<string>& frame_filenames = _img_filenames.back();
            StringExplode(variables[3], ";", frame_filenames);

            // Read kp to dump object
            int total_kp = atoi(variables[4].c_str());
            vector<string> kps;
            StringExplode(variables[5], ";", kps);
            for (int kp_idx = 0; kp_idx < total_kp; kp_idx++)
            {
                /// cluster_id,weight,fg,sequence_id,x,y,a,b,c

                vector<string> vals;
                StringExplode(kps[kp_idx], ",", vals);

                collect_kp(dataset_id,                          // dataset_id
                           strtoull(vals[0].c_str(), NULL, 0),  // cluster_id
                           atof(vals[1].c_str()),               // weight
                           toBool(vals[2]),                     // fg
                           strtoull(vals[3].c_str(), NULL, 0),  // sequence_id
                           SIFT_Keypoint{float(atof(vals[4].c_str())),  // x
                                         float(atof(vals[5].c_str())),  // y
                                         float(atof(vals[6].c_str())),  // a
                                         float(atof(vals[7].c_str())),  // b
                                         float(atof(vals[8].c_str()))});// c
            }
        }

        InFile.close();
    }
}

void kp_dumper::dump(const string& out_path, const vector<size_t>& dump_ids, const vector<string>& img_roots, const vector< vector<string> >& img_filenames)
{
    size_t total_img = dump_ids.size();
    if (total_img > img_filenames.size())
        total_img = img_filenames.size();

    ofstream OutFile(out_path.c_str());
    if (OutFile.is_open())
    {
        /// Template
        /*
        total_img
        rank_id|root_path|total_frame|frame1;frame2|total_kp|cluster_id,weight,fg,sequence_id,x,y,a,b,c;cluster_id,weight,fg,sequence_id,x,y,a,b,c
        */
        /// Separator '|' = variable, ';' = array, ',' = attribute
        // total_img
        OutFile << total_img << endl;

        // match_object
        for (size_t img_idx = 0; img_idx < total_img; img_idx++)
        {
            // dataset_id
            OutFile << dump_ids[img_idx];
            // Image root_path
            OutFile << "|" << img_roots[img_idx];
            // total_frame
            OutFile << "|" << img_filenames[img_idx].size();
            for (size_t frame_idx = 0; frame_idx < img_filenames[img_idx].size(); frame_idx++)
            {
                // filename
                if (frame_idx == 0)
                    OutFile << "|" << img_filenames[img_idx][frame_idx];
                else
                    OutFile << ";" << img_filenames[img_idx][frame_idx];
            }
            // kp
            vector<dump_object>& curr_dump_object = dump_space[dump_ids[img_idx]];
            // total_kp
            OutFile << "|" << curr_dump_object.size();
            for (auto curr_it = curr_dump_object.begin(); curr_it != curr_dump_object.end(); curr_it++)
            {
                if (curr_it == curr_dump_object.begin())
                    OutFile << "|";
                else
                    OutFile << ";";

                // cluster_id
                OutFile << curr_it->cluster_id << ",";
                // weight
                OutFile << curr_it->weight << ",";
                // fg
                OutFile << curr_it->fg << ",";
                // sequence_id
                OutFile << curr_it->sequence_id << ",";
                // x
                OutFile << curr_it->kp.x << ",";
                // y
                OutFile << curr_it->kp.y << ",";
                // a
                OutFile << curr_it->kp.a << ",";
                // b
                OutFile << curr_it->kp.b << ",";
                // c
                OutFile << curr_it->kp.c;
            }
            OutFile << endl;
        }

        OutFile.close();
    }

    // Release memory
    for (auto dump_space_it = dump_space.begin(); dump_space_it != dump_space.end(); dump_space_it++)
        vector<dump_object>().swap(dump_space_it->second);
    unordered_map<size_t, vector<dump_object> >().swap(dump_space);
}

}

void kp_dumper::reset()
{
    // Use with float* after load() (but now changed to struct SIFTKeypoint)
    // Release memory if load dumper from file (has new kp*)
    /*if (is_load)
    {
        for (size_t dump_idx = 0; dump_idx < _dump_ids.size(); dump_idx++)
        {
            // delete kp
            vector<dump_object>& curr_dataset_dump = dump_space[dump_idx];
            for (size_t dump_object_idx = 0; dump_object_idx < curr_dataset_dump.size(); dump_object_idx++)
                delete[] curr_dataset_dump[dump_object_idx].kp;
            // Release memory of dump_object vector
            vector<dump_object>().swap(dump_space[dump_idx]);
        }
        // Release memory of dump_space
        unordered_map<size_t, vector<dump_object>>().swap(dump_space);
    }*/
}
//;)
