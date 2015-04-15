/**
 * Implementation of utility class/functions.
 * 
 * @author Merlin Zhang
 * @version 1.03, 2014.06.26
 */

#include "utils.hpp"

#include <type_traits> // conditonal

namespace ins { namespace utils {

using ins::math::VectorIterator;
using ins::math::VectorPair;

template <bool isConst>
class IdNonZeroIterator : public VectorIterator<isConst>,
        public VectorPair {
public:
    typedef typename VectorIterator<isConst>::iterator_category iterator_category;
    typedef typename VectorIterator<isConst>::value_type value_type;
    typedef typename VectorIterator<isConst>::difference_type difference_type;
    typedef typename VectorIterator<isConst>::pointer pointer;
    typedef typename VectorIterator<isConst>::reference reference;
    typedef typename VectorIterator<isConst>::const_pointer const_pointer;
    typedef typename VectorIterator<isConst>::const_reference const_reference;
    
    typedef VectorPair::index_out index_out;
    typedef VectorPair::value_out value_out;
    typedef VectorPair::value_in value_in;
    
    typedef typename std::conditional<isConst, const IntId, IntId>::type inside_type;
    
    virtual bool operator ==(const InputIterator<value_type, isConst>& other) const {
        try {
            return id == dynamic_cast<decltype(*this)>(other).id;
        } catch (std::bad_cast e) {
            return false;
        }
    }
    
    virtual pointer operator ->() { return this; }
    
    virtual const_pointer operator ->() const { return this; }
    
    virtual reference operator *() { return *this; }
    
    virtual const_reference operator *() const { return *this; }
    
    virtual IdNonZeroIterator<isConst>& operator ++() { id = nullptr; return *this; }
    
    virtual bool hasNext() const { return id != nullptr; }
    
//    virtual std::shared_ptr<InputIterator<value_type, isConst> > getEnd() const {
//        return std::shared_ptr<InputIterator<value_type, isConst> >(
//                new IdNonZeroIterator());
//    }
    
    virtual index_out index() const { return id->id(); } // We are lazy to return something new for invalid cases.
    
    virtual value_out get() const { return 1; }
    
    virtual void set(value_in value) {
        throw std::runtime_error("Not supported");
    }
    
    virtual IdNonZeroIterator<isConst>* clone() const {
        return new IdNonZeroIterator<isConst>(*this); }
    
    IdNonZeroIterator(inside_type& id) : id(&id) {}
    
    IdNonZeroIterator() : id(nullptr) {}
    
protected:
    inside_type* id;
};

IntId::IntId(const Vector& o) {
    auto itPtr = o.nonZeroIterator();
    if (itPtr->hasNext()) {
        id_ = (*itPtr)->index();
        ++*itPtr;
        if (itPtr->hasNext())
            throw std::out_of_range("more than 1 non-zero component");
    } else
        throw std::out_of_range("0 non-zero component");
}

compute_type IntId::get(size_t index) const {
    if (index == id_) return 1;
    else if (index >= 0) return 0;
    else throw std::out_of_range("index < 0");
}

void IntId::set(size_t index, const compute_type value) {
    throw std::runtime_error("Not supported");
}

std::shared_ptr<VectorIterator<false> > IntId::nonZeroIterator() {
    return std::shared_ptr<VectorIterator<false> >(
            new IdNonZeroIterator<false>(*this));
}

std::shared_ptr<VectorIterator<true> > IntId::nonZeroIterator() const {
    return std::shared_ptr<VectorIterator<true> >(
            new IdNonZeroIterator<true>(*this));
}

//std::shared_ptr<VectorIterator<false> > IntId::nonZeroEnd() {
//    return std::shared_ptr<VectorIterator<false> >(
//            new IdNonZeroIterator<false>());
//}
//
//std::shared_ptr<VectorIterator<true> > IntId::nonZeroEnd() const {
//    return std::shared_ptr<VectorIterator<true> >(
//            new IdNonZeroIterator<true>());
//}

}} // ins::utils::
