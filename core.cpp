/**
 * The implementation of core classes/functions that are used in different part
 * of the project.
 * 
 * @author Merlin Zhang
 * @version 1.04, 2014.06.13
 */

#include <stdexcept>

#include "core.hpp"

#include "utils.hpp"
#include "math.hpp"

namespace ins {

ptrdiff_t FeatureCollection::featureDimensions() const {
    if (vs.empty()) return -1;

    const std::shared_ptr<FeatureBased>& f = vs[0];
    if (!f->hasFeatureVector()) return -1;
    
    ptrdiff_t dims = f->featureVector().dimensions();

    for (auto it = vs.begin(); it != vs.end(); it++) {
        if (!(*it)->hasFeatureVector()
                || (*it)->featureVector().dimensions() != dims)
            return -1;
    }

    return dims;
}

math::Vector::Sparsity FeatureCollection::featureSparsity() const {
    using math::Vector;
    
    if (vs.empty()) return Vector::UNKNOWN;

    const std::shared_ptr<FeatureBased>& f = vs[0];
    if (!f->hasFeatureVector()
            || f->featureVector().sparsity() == Vector::UNKNOWN)
        return Vector::UNKNOWN;
    
    math::Vector::Sparsity sp = f->featureVector().sparsity();

    for (auto it = vs.begin(); it != vs.end(); it++) {
        if (!(*it)->hasFeatureVector()
                || (*it)->featureVector().sparsity() != sp)
            return sp;
    }

    return sp;
}

//
//FeatureCollection::~FeatureCollection() {
//    for (std::vector<FeatureBased*>::iterator it = vs.begin();
//            it != vs.end(); it++) {
//        delete *it;
//    }
//}
//
//FeatureCollection::FeatureCollection(const FeatureCollection& o) {
//    for (std::vector<FeatureBased*>::const_iterator it = o.vs.begin();
//            it != o.vs.end(); it++) {
//        vs.push_back((*it)->clone());
//    }
//}
//
//FeatureCollection& FeatureCollection::operator =(
//        const FeatureCollection& rhs) {
//    // Check for self-assignment!
//    if (this == &rhs) // Same object?
//        return *this; // Yes, so skip assignment, and just return *this.
//    // Deallocate, allocate new space, copy values...
//    for (std::vector<FeatureBased*>::iterator it = vs.begin();
//            it != vs.end(); it++) {
//        delete *it;
//    }
//    vs.clear();
//    for (std::vector<FeatureBased*>::const_iterator it = rhs.vs.begin();
//            it != rhs.vs.end(); it++) {
//        vs.push_back((*it)->clone());
//    }
//    return *this;
//}

}