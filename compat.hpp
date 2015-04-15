/**
 * Compatibility utilities: convert data/functions between different
 * implementations.
 * 
 * @author Merlin Zhang
 * @version 1.04, 2014.07.11
 */

#ifndef INS_COMPAT_HPP
#define	INS_COMPAT_HPP

#include "invert_index.h" // bow_bin_object

//#include "utils.hpp" // IntId
#include "core.hpp" // Frame
#include "retr.hpp" // FeatureIndex

#include <vector>

// Forward declaration...
namespace ins { // ins_param.hpp
class ins_param;
}

namespace ins { namespace compat {

///**
// * Converts the format of image Bag of Visual Words from Siriwat's to Merlin's.
// * Note that it is the caller's reponsibility to ensure the output is empty when
// * passing into this function.
// * @param bow_sig [in] Siriwat's bag of words signature
// * @param run_param [in] Siriwat's run parameter (size of codebook:
// * CLUSTER_SIZE)
// * @param image [out] Merlin's image object
// * @param bowIndex [out] Merlin's index (IntId -> EclipseRoi)
// */
//void bow_sigToImage(const std::vector<bow_bin_object*>& bow_sig,
//        const ins_param& run_param, ins::Image& image,
//        ins::utils::Index<ins::utils::IntId,
//        std::shared_ptr<ins::EclipseRoi> >& bowIndex);
//
///**
// * Converts the format of image Bag of Visual Words from Merlin's to Siriwat's.
// * Note that it is the caller's reponsibility to ensure the output is empty when
// * passing into this function.
// * @param image [in] Merlin's image object
// * @param bowIndex [in] Merlin's index (IntId -> EclipseRoi)
// * @param bow_sig [out] Siriwat's bag of words signature
// */
//void imageToBow_sig(const ins::Image& image,
//        const ins::utils::Index<ins::utils::IntId,
//        std::shared_ptr<ins::EclipseRoi> >& bowIndex,
//        std::vector<bow_bin_object*>& bow_sig);

/**
 * Converts the format of frame Bag of Visual Words from Siriwat's to Merlin's.
 * Note that it is the caller's reponsibility to ensure the output is empty when
 * passing into this function.
 * @param bow_sig [in] Siriwat's bag of words signature
 * @param run_param [in] Siriwat's run parameter (size of codebook:
 * CLUSTER_SIZE)
 * @param frame [out] Merlin's Frame object
 * @param bowIndex [out] Merlin's MultiIndex object (codeword ID, timestamp, ROI)
 */
void bow_sigToFrame(const std::vector<bow_bin_object*>& bow_sig,
        const ins_param& run_param, ins::Frame& frame, EclipseRoiIndex& bowIndex);

/**
 * Converts the format of frame Bag of Visual Words from Merlin's to Siriwat's.
 * Note that it is the caller's reponsibility to ensure the output is empty when
 * passing into this function.
 * @param frame [in] Merlin's Frame object
 * @param bowIndex [in] Merlin's MultiIndex object (codeword ID, timestamp, ROI)
 * @param bow_sig [out] Siriwat's bag of words signature
 */
void frameToBow_sig(const ins::Frame& frame, const EclipseRoiIndex& bowIndex,
        std::vector<bow_bin_object*>& bow_sig);

/**
 * Converts the format of video Bag of Visual Words from Siriwat's to Merlin's.
 * Note that it is the caller's reponsibility to ensure the output is empty when
 * passing into this function.
 * @param bow_sig [in] Siriwat's multi bag of words signature
 * @param run_param [in] Siriwat's run parameter (size of codebook:
 * CLUSTER_SIZE)
 * @param video [out] Merlin's Video object
 * @param bowIndex [out] Merlin's MultiIndex object (codeword ID, timestamp, ROI)
 */
void multi_bowToVideo(const std::vector<std::vector<bow_bin_object*> >& multi_bow,
        const ins_param& run_param, ins::Video& video, EclipseRoiIndex& bowIndex);

/**
 * Converts the format of video Bag of Visual Words from Merlin's to Siriwat's.
 * Note that it is the caller's reponsibility to ensure the output is empty when
 * passing into this function.
 * @NOTE: This function assumes the timestamps stored on the bowIndex correspond
 * to video's frame index (i.e. video.wordAt(t)->t() == t, where t is the
 * timestamp obtained from bowIndex). This is NOT guaranteed, especially when
 * the timestamp information is not available (therefore t = -1).
 * @param video [in] Merlin's Video object
 * @param bowIndex [in] Merlin's MultiIndex object (codeword ID, timestamp, ROI)
 * @param bow_pool_sig [out] Siriwat's bag of words signature for pool (video)
 */
void videoToBow_pool_sig(const ins::Video& video, const EclipseRoiIndex& bowIndex,
        std::vector<bow_bin_object*>& bow_pool_sig);

}} // ins::compat::


#endif	/* INS_COMPAT_HPP */

