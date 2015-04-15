/**
 * Implementation of compatibility utilities.
 * 
 * @author Merlin Zhang
 * @version 1.06, 2014.07.11
 */

#include "compat.hpp"

#include "ins_param.h"

#include "defs.hpp"
#include "math.hpp"

#include <cstddef> // ptrdiff_t
#include <memory> // shared_ptr
#include <string>
#include <stdexcept> // runtime_error

namespace ins { namespace compat {

//void bow_sigToImage(const std::vector<bow_bin_object*>& bow_sig,
//        const ins_param& run_param,
//        ins::Image& image,
//        ins::utils::Index<ins::utils::IntId,
//        std::shared_ptr<ins::EclipseRoi> >& bowIndex) {
////    bowIndex.clear();
//    image.setFeatureVector(math::SparseVector<compute_type>(run_param.CLUSTER_SIZE));
//    auto& featureVector = image.featureVector();
//    // For each bin...
//    for (auto itHist = bow_sig.begin(); itHist != bow_sig.end(); itHist++) {
//        // Set the feature vector of image.
//        featureVector.set((*itHist)->cluster_id, (*itHist)->weight);
//        
//        const vector<feature_object*>& features = (*itHist)->features;
//        // For each feature in the bin...
//        for (auto itFeat = features.begin(); itFeat != features.end(); itFeat++) {
//            float* kp = (*itFeat)->kp;
////            // Add image ID to feature ID to make it more unique, the result is
////            // like: "[IMAGE_ID]/[FEATURE_ID]"
////            std::string featureId = image.id() + "/"
////                    + std::to_string((*itFeat)->image_id);
//            
//            auto roiPtr = std::make_shared<ins::EclipseRoi>(
//                    math::Eclipse<compute_type>(kp[0], kp[1], kp[2], kp[3], kp[4]));
//            
//            // Add to the image.
//            image.addFeature(roiPtr);
//            // Add to the index.
//            bowIndex.set(ins::utils::IntId((*itHist)->cluster_id), roiPtr);
//        }
//    }
//}
//
//void imageToBow_sig(const ins::Image& image,
//        const ins::utils::Index<ins::utils::IntId,
//        std::shared_ptr<ins::EclipseRoi> >& bowIndex,
//        std::vector<bow_bin_object*>& bow_sig) {
//    // For each bin...
//    for (auto itRowPtr = bowIndex.rows(); itRowPtr->hasNext(); ++*itRowPtr) {
//        // Create the bin.
//        bow_bin_object* bin = new bow_bin_object;
//        // Fill with data.
//        bin->cluster_id = **itRowPtr;
//        bin->weight = image.featureVector().get(**itRowPtr);
//        // For each feature in the bin...
//        for (auto itFeatPtr = bowIndex.rowIterator(**itRowPtr);
//                itFeatPtr->hasNext(); ++*itFeatPtr) {
//            const auto& roiPtr = (*itFeatPtr)->index();
//            
////            std::string featureId = roiPtr->id();
////            // Remove "[IMAGE_ID]/" from feature ID (if exist).
////            if (featureId.compare(0, image.id().length() + 1, image.id() + "/") == 0)
////                featureId.erase(0, image.id().length() + 1);
//            
//            // Create, fill, and add the feature to the bin.
//            feature_object* feature = new feature_object;
//            feature->image_id = std::stoul(image.id());
//            feature->sequence_id = 0;
//            feature->weight = 1;
//            feature->kp = new float[5];
//            feature->kp[0] = roiPtr->meta().x();
//            feature->kp[1] = roiPtr->meta().y();
//            feature->kp[2] = roiPtr->meta().a();
//            feature->kp[3] = roiPtr->meta().b();
//            feature->kp[4] = roiPtr->meta().c();
//            bin->features.push_back(feature);
//        }
//        // Add the bin to histogram.
//        bow_sig.push_back(bin);
//    }
//}

void bow_sigToFrame(const std::vector<bow_bin_object*>& bow_sig,
        const ins_param& run_param, ins::Frame& frame, EclipseRoiIndex& bowIndex) {
//    bowIndex.clear();
//    frame.clearFeatures();
    
    frame.setFeatureVector(math::SparseVector<compute_type>(run_param.CLUSTER_SIZE));
    auto& featureVector = frame.featureVector();
    
    // @NOTE: We didn't check if image_id and sequence_id for all the features are consistent
    // For each bin...
    for (auto itHist = bow_sig.begin(); itHist != bow_sig.end(); itHist++) {
        // Set the feature vector of image.
        size_t codewordId = (*itHist)->cluster_id;
        featureVector.set(codewordId, (*itHist)->weight);
        
        const vector<feature_object*>& features = (*itHist)->features;
        // For each feature in the bin...
        for (auto itFeat = features.begin(); itFeat != features.end(); itFeat++) {
            frame.id() = std::to_string((*itFeat)->image_id);
            frame.t() = (*itFeat)->sequence_id;
            // forget about the weight...
            float* kp = (*itFeat)->kp;
            
            auto roiPtr = std::make_shared<ins::EclipseRoi>(
                    math::Eclipse<compute_type>(kp[0], kp[1], kp[2], kp[3], kp[4]));
            
            // Add to the image.
            frame.addFeature(roiPtr);
            // Add to the index.
            bowIndex.add(codewordId, frame.t(), roiPtr);
        }
    }
}

void frameToBow_sig(const ins::Frame& frame, const EclipseRoiIndex& bowIndex,
        std::vector<bow_bin_object*>& bow_sig) {
    // For each bin...
    for (auto itRowPtr = bowIndex.rows(); itRowPtr->hasNext(); ++*itRowPtr) {
        // Create the bin.
        bow_bin_object* bin = new bow_bin_object;
        // Fill with data.
        bin->cluster_id = **itRowPtr;
        bin->weight = frame.featureVector().get(**itRowPtr);
        // For each feature in the bin...
        for (auto itFeatPtr = bowIndex.rowIterator(**itRowPtr);
                itFeatPtr->hasNext(); ++*itFeatPtr) {
            ptrdiff_t t = (*itFeatPtr)->index();
            if (frame.t() != t)
                throw std::runtime_error("Feature timestamp mismatch: "
                        + std::to_string(frame.t()) +  " vs. " + std::to_string(t));
            
            const auto& roiPtr = (*itFeatPtr)->get();
            
            // Create, fill, and add the feature to the bin.
            feature_object* feature = new feature_object;
            feature->image_id = std::stoul(frame.id());
            feature->sequence_id = t;
            feature->weight = 1;
            feature->kp = new float[5];
            feature->kp[0] = roiPtr->meta().x();
            feature->kp[1] = roiPtr->meta().y();
            feature->kp[2] = roiPtr->meta().a();
            feature->kp[3] = roiPtr->meta().b();
            feature->kp[4] = roiPtr->meta().c();
            bin->features.push_back(feature);
        }
        // Add the bin to histogram.
        bow_sig.push_back(bin);
    }
}

void multi_bowToVideo(const std::vector<std::vector<bow_bin_object*> >& multi_bow,
        const ins_param& run_param, ins::Video& video, EclipseRoiIndex& bowIndex) {
    for (size_t i = 0; i < multi_bow.size(); ++i) {
        auto framePtr = std::make_shared<Frame>();
        bow_sigToFrame(multi_bow[i], run_param, *framePtr, bowIndex);
        video.addWord(framePtr);
    }
}

void videoToBow_pool_sig(const ins::Video& video, const EclipseRoiIndex& bowIndex,
        std::vector<bow_bin_object*>& bow_pool_sig) {
    // For each bin...
    for (auto itRowPtr = bowIndex.rows(); itRowPtr->hasNext(); ++*itRowPtr) {
        // Create the bin.
        bow_bin_object* bin = new bow_bin_object;
        // Fill with data.
        bin->cluster_id = **itRowPtr;
        bin->weight = video.featureVector().get(**itRowPtr);
        // For each feature in the bin...
        for (auto itFeatPtr = bowIndex.rowIterator(**itRowPtr);
                itFeatPtr->hasNext(); ++*itFeatPtr) {
            ptrdiff_t t = (*itFeatPtr)->index();
            const auto& roiPtr = (*itFeatPtr)->get();
            
            // Create, fill, and add the feature to the bin.
            feature_object* feature = new feature_object;
            feature->image_id = std::stoul(video.wordAt(t)->id());
            feature->sequence_id = t;
            feature->weight = 1;
            feature->kp = new float[5];
            feature->kp[0] = roiPtr->meta().x();
            feature->kp[1] = roiPtr->meta().y();
            feature->kp[2] = roiPtr->meta().a();
            feature->kp[3] = roiPtr->meta().b();
            feature->kp[4] = roiPtr->meta().c();
            bin->features.push_back(feature);
        }
        // Add the bin to histogram.
        bow_pool_sig.push_back(bin);
    }
}


}} // ins::compat::