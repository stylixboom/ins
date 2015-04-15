/* 
 * Declarations for base/interface classes for retr.hpp, which may be used by
 * other files within the same library.
 * @NOTE: To avoid cyclic inclusion, this file should not include any other
 * headers of this project.
 * 
 * @author Merlin Zhang
 * @version 1.01, 2014.06.27
 */

#ifndef INS_RETR_BASE_HPP
#define	INS_RETR_BASE_HPP

#include <cstddef>

#include "core/feature.hpp" // size_t

// Forward declaration...
namespace ins { // core.hpp
class FeatureBased;
class FeatureCollection;
}

namespace ins {

/**
 * The interface for a coding algorithm.
 */
class Coding {
public:
    /**
     * Transforms one feature vector into another feature vector.
     * The implementation class may have certain requirements on the provided
     * arguments (e.g., input needs to be a feature point, thus the x and y
     * coordinates is available). It is the caller's responsibility to ensure
     * such requirements are met, otherwise exceptions might be thrown.
     * @param src [in] a single feature
     * @param dst [out] a single feature
     */
    virtual void coding(FeatureBased& src, FeatureBased& dst) = 0;
    
    /**
     * Transforms one feature vector in place.
     * This is a shorthand of: coding(feature, feature);
     * The implementation class may have certain requirements on the provided
     * arguments (e.g., input needs to be a feature point, thus the x and y
     * coordinates is available). It is the caller's responsibility to ensure
     * such requirements are met, otherwise exceptions might be thrown.
     * @param feature [in & out] a single feature
     */
    virtual void coding(FeatureBased& feature) { coding(feature, feature); }
        
    /**
     * Transforms a collection of feature vectors in place.
     * The implementation class may have certain requirements on the provided
     * arguments (e.g., input needs to be a feature point, thus the x and y
     * coordinates is available). It is the caller's responsibility to ensure
     * such requirements are met, otherwise exceptions might be thrown.
     * @param features [in & out] a collection of features
     */
    virtual void coding(FeatureCollection& features) {
        for (size_t i = 0; i < features.featureSize(); i++)
            coding(*features.featureAt(i)); }
};

/**
 * The interface for a pooling algorithm.
 */
class Pooling {
public:
    /**
     * Aggregates multiple feature vectors into one feature vector.
     * The implementation class may have certain requirements on the provided
     * arguments (e.g., input needs to be a video, thus timestamps of frames is
     * available). It is the caller's responsibility to ensure such requirements
     * are met, otherwise exceptions might be thrown.
     * @param src [in] a collection of features
     * @param dst [out] a single feature
     */
    virtual void pooling(FeatureCollection& src, FeatureBased& dst) = 0;
};

} // ins::

#endif	/* INS_RETR_BASE_HPP */

