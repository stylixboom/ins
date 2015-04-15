/**
 * Utility classes/functions that are used in different part of the project.
 * 
 * @author Merlin Zhang
 * @version 1.12, 2014.07.03
 */

#ifndef INS_UTILS_HPP
#define	INS_UTILS_HPP

#include "utils_base.hpp"

#include "defs.hpp"
#include "math_base.hpp" // Vector

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cstdint> // uintptr_t
#include <functional> // reference_wrapper
#include <algorithm> // sort

#include "utils/iterator.hpp"
#include "utils/index.hpp"
#include "utils/io.hpp"

#include <iostream> // @TODO: DEBUG

namespace ins { namespace utils {

/**
 * Split the string into tokens with respective delimiters.
 * @param s the input string
 * @param delimiters the delimiters
 * @param result [out] the collection to hold the split results
 * @param allowEmpty whether to elide empty tokens
 * @return a reference of result
 */
template <class TContainer>
TContainer& split(const typename TContainer::value_type& s,
        const typename TContainer::value_type& delimiters,
        TContainer& result, bool allowEmpty = true);

/**
 * Split the string into tokens with respective delimiters.
 * Note that while this function save the caller from creating the result
 * collection, extra copying of the return value might result in worse
 * efficiency.
 * @param s the input string
 * @param delimiters the delimiters
 * @param allowEmpty whether to elide empty tokens
 * @return a vector containing the tokens
 */
template <class TString>
typename std::vector<TString> split(const TString& s, const TString& delimiters,
        bool allowEmpty = true);

/**
 * Trims left side of white space and returns a copy of the input string.
 * @param s the input string
 * @param delimiters the delimiters
 * @return a copy of input string with left side of white spaces trimmed
 */
template <class TString>
TString ltrim(const TString& s, const TString& delimiters = " \f\n\r\t\v");

/**
 * Trims right side of white space and returns a copy of the input string.
 * @param s the input string
 * @param delimiters the delimiters
 * @return a copy of input string with right side of white spaces trimmed
 */
template <class TString>
TString rtrim(const TString& s, const TString& delimiters = " \f\n\r\t\v");

/**
 * Trims both sides of white space and returns a copy of the input string.
 * @param s the input string
 * @param delimiters the delimiters
 * @return a copy of input string with both sides of white spaces trimmed
 */
template <class TString>
TString trim(const TString& s, const TString& delimiters = " \f\n\r\t\v");

//template <typename TValue, bool isConst>
//typename std::vector<std::reference_wrapper<TValue> >
//sort(const InputIterator<TValue, isConst>& it) {
//    typename std::vector<std::reference_wrapper<TValue> > vs;
//    auto itPtr = it.clone();
//    for (; itPtr->hasNext(); ++*itPtr)
//        vs.push_back(**itPtr);
//    delete itPtr;
//    std::sort(vs.begin(), vs.end());
//    return vs;
//}

template <typename TIndex, typename TValue, bool byRef, bool isConst>
typename std::vector<std::reference_wrapper<IndexedPair<TIndex, TValue, byRef> > >
sort(std::shared_ptr<InputIterator<IndexedPair<TIndex, TValue, byRef>, isConst> > itPtr) {
    typename std::vector<std::reference_wrapper<IndexedPair<TIndex, TValue, byRef> > > vs;
//    auto itPtr = it.clone();
    for (; itPtr->hasNext(); ++*itPtr) {
        std::cout << **itPtr << std::endl;
        vs.push_back(**itPtr);
    }
    std::sort(vs.begin(), vs.end(), []
    (IndexedPair<TIndex, TValue, byRef>& a, IndexedPair<TIndex, TValue, byRef>& b) {
        return a.index() < b.index(); });
//    delete itPtr;
        std::cout << vs.size() << std::endl;
    return vs;
}

/**
 * The class that stores key -> value string pair of properties.
 * @param TString the type of string
 */
template <class TString>
class Properties {
public:
    bool contains(const TString& key) { return contents.count(key) > 0; }

    void put(const TString& key, const TString& value);
    
    template <typename T>
    void put(const TString& key, const T& value);

    int remove(const TString& key);

    const TString& get(const TString& key) const;
    
    const size_t size() const { return contents.size(); }

    /**
     * Gets the iterator range of the properties with the given key prefix.
     * @param key the key prefix
     * @return a pair of iterator (begin, end), where begin points to the first
     * entry with given prefix, end points to the first entry after the given
     * prefix
     */
    const typename std::pair<typename std::map<TString, TString>::iterator,
            typename std::map<TString, TString>::iterator>
    findPrefix(const TString& key) const;
    
    Properties() {}
    
    Properties(std::map<TString, TString> contents) : contents(std::move(contents)) {}

    static std::map<TString, TString> load(const std::string& path);

    void save(const std::string& path) const;
    
private:
    typename std::map<TString, TString> contents;
};


class UniqueId {
public:
    virtual const uintptr_t id() const { return reinterpret_cast<uintptr_t>(this); }
    
    virtual bool operator ==(const UniqueId& rhs) const { return this == &rhs; }
    
    virtual bool operator !=(const UniqueId& rhs) const {
        return !this->operator ==(rhs); }
};

template <typename T>
class AbstractId {
public:
    virtual T& id() { return id_; }
    
    virtual const T& id() const { return id_; }
    
    virtual bool operator ==(const AbstractId<T>& rhs) const {
        if (this == &rhs) return true;
        return id_ == rhs.id_;
    }
    
    virtual bool operator !=(const AbstractId<T>& rhs) const {
        return !this->operator ==(rhs);
    }
    
    explicit AbstractId(const T id) : id_(std::move(id)) {}
    
    AbstractId() {}

    virtual ~AbstractId() {}
    
protected:
    T id_;
};

class StringId : public AbstractId<std::string> {
public:
    StringId(const std::string id) : AbstractId<std::string>(std::move(id)) {}
    
    explicit StringId(const size_t id) : AbstractId<std::string>(std::to_string(id)) {}
    
    operator std::string() { return id_; }
    
    StringId() {}
};

class IntId : public AbstractId<size_t>, public ins::math::Vector {
public:
    /**
     * Constructs a term ID.
     * @param id the id of the term
     */
    IntId(const size_t id) : AbstractId<size_t>(id) {}
    
    operator size_t() const { return id_; }
    
    /**
     * Constructs a term ID from another vector.
     * @param o the vector to be converted
     * @throws std::out_of_range if o has 0 or more than 1 non-zero components
     */
    explicit IntId(const Vector& o);
    
    IntId() {}
    
    virtual bool operator ==(const IntId& rhs) const {
        // Inherited from both Id and Vector, use Id's version.
        return AbstractId<size_t>::operator ==(rhs); }
    
    virtual bool operator !=(const IntId& rhs) const {
        // Inherited from both Id and Vector, use Id's version.
        return AbstractId<size_t>::operator !=(rhs); }
    
    virtual Sparsity sparsity() const { return SPARSE; }
    
    virtual ptrdiff_t dimensions() const { return -1; }

    virtual compute_type get(size_t index) const;

    virtual void set(size_t index, const compute_type value);
    
    virtual IntId* clone() const { return new IntId(*this); }

    virtual std::shared_ptr<ins::math::VectorIterator<false> > nonZeroIterator();
    
    virtual std::shared_ptr<ins::math::VectorIterator<true> > nonZeroIterator() const;
};

}} // ins::utils::

// Hash functions...
namespace std {
template <>
struct hash<ins::utils::UniqueId> {
    size_t operator ()(const ins::utils::UniqueId& id) const {
        return hash<ins::utils::UniqueId*>()(const_cast<ins::utils::UniqueId*>(&id));
    }
};

template <typename T>
struct hash<typename ins::utils::AbstractId<T> > {
    size_t operator ()(const typename ins::utils::AbstractId<T>& id) const {
        return hash<T>()(id.id());
    }
};

template <>
struct hash<ins::utils::StringId> {
    size_t operator ()(const ins::utils::StringId& id) const {
        return hash<std::string>()(id.id());
    }
};

template <>
struct hash<ins::utils::IntId> {
    size_t operator ()(const ins::utils::IntId& id) const {
        return id.id();
    }
};
} // std::

#include "utils.hxx"

#endif	/* INS_UTILS_HPP */

