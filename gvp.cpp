/*
 * gvp.cpp
 *
 *  Created on: September 25, 2013
 *      Author: Siriwat Kasamwattanarote
 */

#include "gvp.h"

using namespace std;
using namespace alphautils;

namespace ins
{

gvp_space::gvp_space(void)
{
}

gvp_space::~gvp_space(void)
{
    // Release mem
    // Space IDF
    space_idf_iter space_idf_it;
    for (space_idf_it = space_idf.begin(); space_idf_it != space_idf.end(); space_idf_it++)
        delete[] space_idf_it->second;
    space_idf.clear();

    // Space Freq
    space_freq_iter space_freq_it;
    for (space_freq_it = space_freq.begin(); space_freq_it != space_freq.end(); space_freq_it++)
        delete[] space_freq_it->second;
    space_freq.clear();
}

void gvp_space::init_space(int space_mode, int space_size, int gvp_length)
{
    // Initial parameters
    // GVP parameters
    _space_mode = space_mode;
    _space_size = space_size;
    _gvp_length = gvp_length;

    // Sector parameter
    if (_space_mode == OFFSET)  // Offset space
    {
        _size_per_distance_x = 1.0f / _space_size; // x quantize
        _size_per_distance_y = 1.0f / _space_size; // y quantize
    }
    else                        // Degree space
    {
        _size_per_degree = 360.0f / (_space_size * 2);  // rotation quantize
        _size_per_distance = 1.4142135623730950488016887242097f / _space_size;        // distance quantize
    }

    // Space parameter
    if (_space_mode == OFFSET)  // Offset space
        total_space = (_space_size * 2) * (_space_size * 2);    // offset 4 quadrants -> +,+ -,+ -,- +,-
    else                        // Degree space
        total_space = _space_size * _space_size;          // Orientation is wider than distance
}

void gvp_space::calc_motion(size_t dataset_id, float tfidf_in, float idf_in, float dataset_x, float dataset_y, float query_x, float query_y)
{
    // Initial space idf and score if no dataset before
    if (space_freq.find(dataset_id) == space_freq.end())
    {
        // Create new space for new corresponding dataset
        space_tf[dataset_id] = new tf_score[total_space];
        space_idf[dataset_id] = new idf_score[total_space];
        space_freq[dataset_id] = new ncr_score[total_space];
        for (int space_id = 0; space_id < total_space; space_id++)
        {
            space_tf[dataset_id][space_id] = 0.0f;
            space_idf[dataset_id][space_id] = 0.0f;
            space_freq[dataset_id][space_id] = 0;
        }
    }

    if (_space_mode == OFFSET)
    {
        // Finding an offset from it's center
        int quant_offset_x = ((int)(dataset_x * _space_size) - (int)(query_x * _space_size) + _space_size);
        int quant_offset_y = ((int)(dataset_y * _space_size) - (int)(query_y * _space_size) + _space_size);

        /*
        cout << "Dataset point [" << dataset_id << "]" << endl << " ";
        for (int col = 0; col < _space_size; col++)
            cout << " " << CharPadString(toString(col), ' ', (int)ceil(log10(_space_size)));
        cout << endl;
        for (int row = 0; row < _space_size; row++)
        {
            cout << CharPadString(toString(row), ' ', (int)ceil(log10(_space_size)));
            for (int col = 0; col < _space_size; col++)
            {
                if (row == (int)(dataset_x * _space_size) && col == (int)(dataset_y * _space_size))
                    cout << " " << CharPadString("x", ' ', (int)ceil(log10(_space_size)));
                else
                    cout << " " << CharPadString("-", ' ', (int)ceil(log10(_space_size)));
            }
            cout << endl;
        }
        cout << (int)(dataset_x * _space_size) << "," << (int)(dataset_y * _space_size) << endl;

        cout << "Query point" << endl << " ";
        for (int col = 0; col < _space_size; col++)
            cout << " " << CharPadString(toString(col), ' ', (int)ceil(log10(_space_size)));
        cout << endl;
        for (int row = 0; row < _space_size; row++)
        {
            cout << CharPadString(toString(row), ' ', (int)ceil(log10(_space_size)));
            for (int col = 0; col < _space_size; col++)
            {
                if (row == (int)(query_x * _space_size) && col == (int)(query_y * _space_size))
                    cout << " " << CharPadString("x", ' ', (int)ceil(log10(_space_size)));
                else
                    cout << " " << CharPadString("-", ' ', (int)ceil(log10(_space_size)));
            }
            cout << endl;
        }
        cout << (int)(query_x * _space_size) << "," << (int)(query_y * _space_size) << endl;

        cout << "Offset point" << endl << "  ";
        for (int col = 0; col < _space_size * 2; col++)
            cout << " " << CharPadString(toString(col), ' ', (int)ceil(log10(_space_size * 2)));
        cout << endl;
        for (int row = 0; row < _space_size * 2; row++)
        {
            cout << CharPadString(toString(row), ' ', (int)ceil(log10(_space_size * 2)));
            for (int col = 0; col < _space_size * 2; col++)
            {
                if (row == quant_offset_x && col == quant_offset_y)
                    cout << " " << CharPadString("x", ' ', (int)ceil(log10(_space_size * 2)));
                else
                    cout << " " << CharPadString("-", ' ', (int)ceil(log10(_space_size * 2)));
            }
            cout << endl;
        }
        cout << quant_offset_x << "," << quant_offset_y << " = " << (int)(dataset_x * _space_size) - (int)(query_x * _space_size) << "," << (int)(dataset_y * _space_size) - (int)(query_y * _space_size) << endl;

        cout << "paused.." << endl;
        char tmp;
        cin >> tmp;
        */

        // Crop outbound, fall if bug exist
        if (quant_offset_x < 0)
            quant_offset_x = 0;
        if (quant_offset_y < 0)
            quant_offset_y = 0;
        if (quant_offset_x > (_space_size * 2) - 1)
            quant_offset_x = (_space_size * 2) - 1;
        if (quant_offset_y > (_space_size * 2) - 1)
            quant_offset_y = (_space_size * 2) - 1;

        /*if(quant_offset_x < 0 || quant_offset_y < 0 || quant_offset_x > _space_size - 1 || quant_offset_y > _space_size - 1)
        {
            cout << "over block xy " << quant_offset_x << " " << quant_offset_y << endl;
            exit(0);
        }

        if(quant_offset_y * _space_size + quant_offset_x < 0 || quant_offset_y * _space_size + quant_offset_x > _space_size * _space_size - 1)
        {
            cout << "over block id " << quant_offset_y * _space_size + quant_offset_x << endl;
            exit(0);
        }*/

        int space_id = quant_offset_y * (_space_size * 2) + quant_offset_x;

        //cout << "quant_offset_x:" << quant_offset_x << " quant_offset_y:" << quant_offset_y << " _size_per_sector_x:" << _size_per_sector_x << " _size_per_sector_y:" << _size_per_sector_y << endl;
        //cout << "feature_id:" << feature_id << " dy:" << dy << " dx:" << dx << " space size:" << _space_size * _space_size << " space_id:" << quant_offset_y * _space_size + quant_offset_x <<  endl;
        //cout << "space.size() = " << space.size() << endl;
        //cout << "quant_offset_y * _space_size + quant_offset_x = " << quant_offset_y << " * " << _space_size << " + " << quant_offset_x << " = " << quant_offset_y * _space_size + quant_offset_x << endl;
        /// Voting with keeping idf
        space_tf[dataset_id][space_id] += tfidf_in;
        space_idf[dataset_id][space_id] += idf_in;
        space_freq[dataset_id][space_id]++;
    }
    else // DEGREE mode
    {
        float dx = dataset_x - query_x;
        float dy = dataset_y - query_y;
        float raw_degree;
        float raw_distance;
        int quant_degree;
        int quant_distance;

        // Calculate degree
        if (dy == 0 && dx ==0)
            raw_degree = 0.0f;
        else
            raw_degree = atan2(dy, dx) * 180 / PI; // avoid nan (div-by-zero), by above special if case0;

        if ((int)raw_degree <=  -1)
            raw_degree += 360;

        // Calculate distance
        raw_distance = sqrt(dx * dx + dy * dy);

        // Quantization
        quant_degree = (int)(_space_size * raw_degree / 360);
        quant_distance = (int)(_space_size * raw_distance / 1.4142135623730950488016887242097f);

        // Calculate space id
        int space_id = quant_degree * _space_size + quant_distance;
        //cout << "feature_id:" << feature_id << " dy:" << dy << " dx:" << dx << " space_id:" << space_id <<  endl;
        /// Voting with keep idf
        if (space_id >= total_space || space_id < 0)
            cout << "wrong space_id " << space_id << endl;
        space_tf[dataset_id][space_id] += tfidf_in;
        space_idf[dataset_id][space_id] += idf_in;
        space_freq[dataset_id][space_id]++;
    }
}

void gvp_space::get_score(float dataset_score[])
{
    // For each dataset
    for (space_freq_iter space_freq_it = space_freq.begin(); space_freq_it != space_freq.end(); space_freq_it++)
    {
        size_t dataset_id = space_freq_it->first;
        float score = 0.0f;

        // Prepare score space and initialize
        float* score_list = new float[total_space];
        for (int score_id = 0; score_id < total_space; score_id++)
            score_list[score_id] = 0.0f;

        /// Accumulate score from each space
        for (int space_id = 0; space_id < total_space; space_id++)
        {
            ncr_score space_bin_score = space_freq[dataset_id][space_id];
            if (space_bin_score >= (ncr_score)_gvp_length)
            {
                //cout << "space[space_id]:" << space[space_id] << " space_id:" << space_id << " _gvp_length:" << _gvp_length << endl;
                ///-- tf for frequency is necessary for too big value, wtf = 1 + log10(freq)
                ///-- idf is necessary for emphasizing the important word
                //score_list[space_id] = (1 + log10(nCr(space_bin_score - 1, _gvp_length - 1))) * space_idf[dataset_id][space_id];   // tf-idf with ncr minus 1
                //score_list[space_id] = (1 + log10(nCr(space_b in_score - 1, _gvp_length - 1))) * space_idf[dataset_id][space_id];   // tf-idf
                //score_list[space_id] = nCr(space_bin_score, _gvp_length) * space_idf[dataset_id][space_id];   // idf
                //score_list[space_id] = 1 + log10(nCr(space_bin_score, _gvp_length));   // tf
                score_list[space_id] = log10(nCr(space_bin_score, _gvp_length) * space_idf[dataset_id][space_id]);   // tf-idf
                //score_list[space_id] = log10(nCr(space_bin_score, _gvp_length));   // tf
            }
        }


        /// Normalization
        // Unit length
        float sum_of_square = 0.0f;
        float unit_length = 0.0f;
        for (int score_id = 0; score_id < total_space; score_id++)
            sum_of_square += score_list[score_id] * score_list[score_id];
        unit_length = sqrt(sum_of_square);

        // Normalizing
        for (int score_id = 0; score_id < total_space; score_id++)
            score_list[score_id] = score_list[score_id] / unit_length;


        /// Calculate total score
        for (int score_id = 0; score_id < total_space; score_id++)
            score += score_list[score_id];


        // Release memory
        delete[] score_list;

        //cout << "score:" << score << endl;
        dataset_score[dataset_id] = score;
    }
}
};
//;)
