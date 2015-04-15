// *** ADDED BY HEADER FIXUP ***
#include <iostream>
// *** END ***
/* 
 * Declarations for base/interface classes for utils.hpp, which may be used by
 * other files within the same library.
 * @NOTE: To avoid cyclic inclusion, this file should not include any other
 * headers of this project.
 * 
 * @author Merlin Zhang
 * @version 1.07, 2014.06.27
 */

#ifndef INS_UTILS_BASE_HPP
#define	INS_UTILS_BASE_HPP

#include <cstddef> // size_t
#include <memory> // shared_ptr
#include <ostream>
//#include <iterator>
#include <type_traits> // conditional

namespace ins { namespace utils {
// Some const definitions for returning by reference.
const bool TRUE_ = true;
const bool FALSE_ = false;

/**
 * An interface that indicates the class can make a new copy of itself.
 */
template <class T>
class Cloneable {
public:
    /**
     * Returns a new copy of itself.
     * Note that it is the caller's responsibility to delete the new copy after
     * use.
     * @return a new copy of itself
     */
    virtual T* clone() const = 0;
};

template <typename TValue, bool isConst>
class InputIterator : public Cloneable<InputIterator<TValue, isConst> > {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef TValue value_type;
    typedef ptrdiff_t difference_type;
    typedef typename std::conditional<isConst,
            const TValue*, TValue*>::type pointer;
    typedef typename std::conditional<isConst,
            const TValue&, TValue&>::type reference;
    typedef const TValue* const_pointer;
    typedef const TValue& const_reference;
    
    virtual bool operator ==(const InputIterator<value_type, isConst>& other) const = 0;
    
    virtual bool operator !=(const InputIterator<value_type, isConst>& other) const {
        return !operator ==(other); }
    
    virtual pointer operator ->() = 0;
    
    virtual const_pointer operator ->() const = 0;
    
    virtual reference operator *() = 0;
    
    virtual const_reference operator *() const = 0;
    
    virtual InputIterator<value_type, isConst>& operator ++() = 0;
    
    virtual void operator ++(int) { operator ++(); }
    
    virtual bool hasNext() const = 0;
    
//    virtual std::shared_ptr<InputIterator<value_type, isConst> > getEnd() const = 0;
    
    virtual ~InputIterator() {}
};

template <typename TIndex, typename TValue, bool byRef>
class IndexedPair {
public:
    typedef typename std::conditional<byRef, const TIndex&, TIndex>::type 
    index_out;
    typedef typename std::conditional<byRef, const TValue&, TValue>::type
    value_out;
    typedef const TValue& value_in;
    
    virtual index_out index() const = 0;
    
    virtual value_out get() const = 0;
    
    virtual void set(value_in value) = 0;
    
    virtual bool operator < (const IndexedPair<TIndex, TValue, byRef>& rhs) {
        return this->index() < rhs.index(); }
};

template <typename TIndex, typename TValue, bool byRef>
std::ostream& operator <<(std::ostream& os, const IndexedPair<TIndex, TValue,
        byRef>& obj) {
    os << "(" << obj.index() << "):" << obj.get();
    return os;
}

template <typename TIndex, typename TValue, bool isConst>
using IndexIterator = InputIterator<IndexedPair<TIndex, TValue, true>, isConst>;

template <typename TRow, typename TColumn, typename TValue = bool>
class Index {
public:
    virtual ~Index() {}
    
    virtual bool contains(const TRow& row, const TColumn& col) const = 0;
    
    virtual const TValue& get(const TRow& row, const TColumn& col) const = 0;
    
    virtual void set(const TRow& row, const TColumn& col, const TValue& value) = 0;
    
    virtual void remove(const TRow& row, const TColumn& col) = 0;
    
    virtual void clear() = 0;
    
    virtual size_t numRows() const = 0;
    
    virtual std::shared_ptr<InputIterator<TRow, true> > rows() const = 0;
    
    virtual size_t rowSize(const TRow& row) const = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, TValue, false> >
    rowIterator(const TRow& row) = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, TValue, true> >
    rowIterator(const TRow& row) const = 0;
    
    virtual void rowRemove(const TRow& row) = 0;
};

template <typename TRow, typename TColumn>
class Index<TRow, TColumn, bool> {
public:
    virtual ~Index() {}
    
    virtual bool contains(const TRow& row, const TColumn& col) const = 0;
    
    virtual const bool& get(const TRow& row, const TColumn& col) const = 0;
    
    virtual void set(const TRow& row, const TColumn& col, const bool& value = true) = 0;
    
    virtual void remove(const TRow& row, const TColumn& col) = 0;
    
    virtual void clear() = 0;
    
    virtual size_t numRows() const = 0;
    
    virtual std::shared_ptr<InputIterator<TRow, true> > rows() const = 0;
    
    virtual size_t rowSize(const TRow& row) const = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, bool, false> >
    rowIterator(const TRow& row) = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, bool, true> >
    rowIterator(const TRow& row) const = 0;
    
    virtual void rowRemove(const TRow& row) = 0;
};

template <typename TRow, typename TColumn, typename TValue>
std::ostream& operator <<(std::ostream& os,
        const Index<TRow, TColumn, TValue>& obj) {
    size_t entries = 0;
    for (auto itPtr = obj.rows(); itPtr->hasNext(); ++*itPtr) {
        entries += obj.rowSize(**itPtr);
    }
    os << "Index: #rows=" << obj.numRows() << ", #entries=" << entries;
    return os;
}

//template <typename TRow, typename TColumn, typename TValue = bool>
//class BiIndex : public Index<TRow, TColumn, TValue> {
//public:
//    virtual std::shared_ptr<InputIterator<TColumn, true> > columnIterator() const = 0;
//    
//    virtual size_t columnSize(const TColumn& col) const = 0;
//    
//    virtual std::shared_ptr<IndexIterator<TRow, TValue, false> >
//    columnIterator(const TColumn& col) = 0;
//    
//    virtual std::shared_ptr<IndexIterator<TRow, TValue, true> >
//    columnIterator(const TColumn& col) const = 0;
//    
//    virtual void columnRemove(const TColumn& col) = 0;
//};
//
//template <typename TRow, typename TColumn>
//class BiIndex<TRow, TColumn, bool> : public Index<TRow, TColumn, bool> {
//public:
//    virtual std::shared_ptr<InputIterator<TColumn, true> > columnIterator() const = 0;
//    
//    virtual size_t columnSize(const TColumn& col) const = 0;
//    
//    virtual std::shared_ptr<IndexIterator<TRow, bool, false> >
//    columnIterator(const TColumn& col) = 0;
//    
//    virtual std::shared_ptr<IndexIterator<TRow, bool, true> >
//    columnIterator(const TColumn& col) const = 0;
//    
//    virtual void columnRemove(const TColumn& col) = 0;
//};

template <typename TRow, typename TColumn, typename TValue>
class MultiIndex {
public:
    virtual ~MultiIndex() {}
    
    virtual size_t count(const TRow& row, const TColumn& col) const = 0;
    
    virtual const TValue& get(const TRow& row, const TColumn& col, size_t idx) const = 0;
    
    virtual void add(const TRow& row, const TColumn& col, const TValue& value) = 0;
    
    virtual void remove(const TRow& row, const TColumn& col, size_t idx) = 0;
    
    virtual void clear() = 0;
    
    virtual size_t numRows() const = 0;
    
    virtual std::shared_ptr<InputIterator<TRow, true> > rows() const = 0;
    
    virtual size_t rowSize(const TRow& row) const = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, TValue, false> >
    rowIterator(const TRow& row) = 0;
    
    virtual std::shared_ptr<IndexIterator<TColumn, TValue, true> >
    rowIterator(const TRow& row) const = 0;
    
    virtual void rowRemove(const TRow& row) = 0;
    
    virtual std::shared_ptr<InputIterator<TValue, false> >
    cellIterator(const TRow& row, const TColumn& col) = 0;
    
    virtual std::shared_ptr<InputIterator<TValue, true> >
    cellIterator(const TRow& row, const TColumn& col) const = 0;
    
    virtual void cellRemove(const TRow& row, const TColumn& col) = 0;
};

template <typename TRow, typename TColumn, typename TValue>
std::ostream& operator <<(std::ostream& os,
        const MultiIndex<TRow, TColumn, TValue>& obj) {
    size_t entries = 0;
    for (auto itPtr = obj.rows(); itPtr->hasNext(); ++*itPtr) {
        entries += obj.rowSize(**itPtr);
    }
    os << "Index: #rows=" << obj.numRows() << ", #entries=" << entries;
    return os;
}

}} // ins::utils::

#endif	/* INS_UTILS_UTILS_BASE_HPP */

