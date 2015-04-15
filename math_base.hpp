// *** ADDED BY HEADER FIXUP ***
#include <iostream>
// *** END ***
/* 
 * Declarations for base/interface classes for math.hpp, which may be used by
 * other files within the same library.
 * @NOTE: To avoid cyclic inclusion, this file should not include any other
 * headers of this project.
 * 
 * @author Merlin Zhang
 * @version 1.05, 2014.07.10
 */

#ifndef INS_MATH_BASE_HPP
#define	INS_MATH_BASE_HPP

#include "defs.hpp"
#include "utils_base.hpp" // Cloneable

#include <cstddef> // ptrdiff_t, size_t
#include <ostream>

namespace ins { namespace math {

/**
 * Interface for a geometric object.
 */
class GeometricObject {
public:
    /**
     * Returns the dimensionality of the object.
     * The return value can be -1, meaning unknown.
     * @return the dimensionality of the object
     */
    virtual ptrdiff_t dimensions() const = 0;
    
    virtual ~GeometricObject() {}
};

/**
 * Interface for a shape.
 */
class Shape : public GeometricObject {
public:
//    virtual bool isInside(const Point& p) const = 0;
};


using VectorPair = ins::utils::IndexedPair<size_t, compute_type, false>;

template <bool isConst>
using VectorIterator = typename ins::utils::InputIterator<VectorPair, isConst>;

/**
 * Interface for a high-dimension vector. The actual numeric type stored in the
 * vector can be different to the default one to save space.
 * 
 * Besides the pure virtual functions, all the implementing classes are expected
 * to implement the following:
 * 1. A convert constructor which takes a const Vector reference as input, i.e.,
 * explicit DerivedVector(const Vector& o);
 * 2. A hash function in std::hash, which can reuse the code form Vector, i.e.,
 * namespace std {
 * template <>
 * struct hash<DerivedVector> {
 *     size_t operator()(const DerivedVector& v) const {
 *         return hash<ins::geom::Vector>()(v); }
 * };
 * } // std::
 * @NOTE: Since std::hash is templated class, a definition for Vector can not
 * be applied to its sub-class.
 */
class Vector : public GeometricObject, public ins::utils::Cloneable<Vector> {
public:
    enum SkipType { SKIP_NONE, SKIP_1, SKIP_2, SKIP_AND, SKIP_OR };
    
    enum Sparsity { SPARSE, DENSE, UNKNOWN };
    
    /**
     * Determines the sparsity of the underlying data structure that stores the
     * vector. A data structure is SPARSE if it stores only non-zero components
     * (thus it is more efficient to be accessed from non-zero iterator); DENSE
     * if it stores all components, regardless of its value; UNKNOWN if it is
     * neither SPARSE nor DENSE.
     * @return the sparsity of the underlying data structure
     */
    virtual Sparsity sparsity() const = 0;
    
    /**
     * Returns the number of non-zero components.
     * @return the number of non-zero components
     */
    virtual size_t numNonZeros() const;
    
    /**
     * Returns the value for the given index/dimension. If the dimensionality is
     * unknown, this function return 0 except for any index >= 0, except for
     * non-zero components.
     * @param index the index
     * @return the value of the given index/dimension
     * @throws std::out_of_bound if the provided index is outside the possible
     * dimensions
     */
    virtual compute_type get(const size_t index) const = 0;
    
    /**
     * Sets the value for the given index/dimension.
     * @param index the index
     * @param value the value to be set
     */
    virtual void set(const size_t index, const compute_type value) = 0;
    
    /**
     * Compares this vector with another vector for equality. A vector is said
     * to be equal to another if they have same dimensionality (can be both
     * unknown) and all the corresponding components are of equal value.
     * @param rhs the other vector
     * @return true if this vector is equal to rhs, false otherwise
     */
    virtual bool operator ==(const Vector& rhs) const;
    
    /**
     * Compares this vector with another vector for inequality.
     * @param rhs the other vector
     * @return true if this vector is not equal to rhs, false otherwise
     */
    virtual bool operator !=(const Vector& rhs) const;
    
    /**
     * Adding another vector to this vector, by adding the corresponding
     * components.
     * Note that we assume two vectors have the same dimensionality, otherwise
     * the result is undefined.
     * @param rhs the vector to be added
     * @return this vector
     */
    virtual Vector& operator +=(const Vector& rhs);
    
    /**
     * Subtract another vector from this vector, by subtracting the
     * corresponding components.
     * Note that we assume two vectors have the same dimensionality, otherwise
     * the result is undefined.
     * @param rhs the vector to be subtracted
     * @return this vector
     */
    virtual Vector& operator -=(const Vector& rhs);
    
    /**
     * Multiplies this vector by a number, every components will be multiplied.
     * @param rhs the multiplier
     * @return this vector
     */
    virtual Vector& operator *=(const compute_type rhs);
    
    /**
     * Divides this vector by a number, every components will be divided.
     * @param rhs the dividend
     * @return this vector
     */
    virtual Vector& operator /=(const compute_type rhs);
    
    /**
     * Applies a unary function to every components of this vector.
     * @param op the function, can be a binary function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components of 0 can be skipped
     * @return this vector
     */
    template <typename TUnaryOp>
    Vector& apply(TUnaryOp op, SkipType skipZero = SKIP_NONE) {
        return transform(*this, *this, op, skipZero);
    }
    
    /**
     * Applies a binary function to every corresponding components of this and
     * another vector, and assigns the results to this vector.
     * This function is a short hand of Vector::transform(*this, rhs, *this, op,
     * skipZero).
     * @param rhs the vector which components to used on the right hand side of
     * the function
     * @param op the function, can be a binary function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components with left input as 0 can be skipped, [SKIP_2]
     * components with right input as 0 can be skipped, [SKIP_OR] components
     * with left OR right input as 0 can be skipped, [SKIP_AND] components with
     * left AND right input as 0 can be skipped
     * @return this vector
     */
    template <typename TBinaryOp>
    Vector& apply(const Vector& rhs, TBinaryOp op, SkipType skipZero = SKIP_NONE) {
        return transform(*this, rhs, *this, op, skipZero);
    }
    
    /**
     * Applies a binary function to every components of this vector and a
     * number, and assigns the results to this vector.
     * This function is a short hand of Vector::transform(*this, rhs, *this, op,
     * skipZero).
     * @param rhs the number to used on the right hand side of the function
     * @param func the function, can be a binary function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components with left input as 0 can be skipped, [SKIP_2]
     * components with right input as 0 can be skipped, [SKIP_OR] components
     * with left OR right input as 0 can be skipped, [SKIP_AND] components with
     * left AND right input as 0 can be skipped
     * @return this vector
     */
    template <typename TBinaryOp>
    Vector& apply(const compute_type rhs, TBinaryOp op, SkipType skipZero = SKIP_NONE) {
        return transform(*this, rhs, *this, op, skipZero);
    }
    
    /*
     * @NOTE[Merlin]: The intention here is to have all Vector implementations
     * return some kind of non-zero iterator. For example, the DenseVector and
     * Point should return a DefaultNonZeroIterator, while SparseVector should
     * utilize its map structure and return an iterator based on it.
     * 
     * The implementation currently is very Java style. It seems that we can not
     * return value or reference instead of pointer (to a new object) in this
     * scenario. While a similar effect is usually implemented in C++ with
     * traits and tag class, it doesn't utilize the polymorphism, and I'm not
     * very comfortable with that style.
     */
    /**
     * Creates a new non-zero iterator pointing to the first non-zero position
     * (if exist).
     * @NOTE Any sub-class that implements a sparse vector should override this
     * function, since the default iterator can not work with unknown
     * dimensionality.
     * @return a pointer to the newly created non-zero iterator
     */
    virtual std::shared_ptr<VectorIterator<false> > nonZeroIterator();
    
    
    /**
     * Creates a new const non-zero iterator pointing to the first non-zero
     * position (if exist).
     * @NOTE Any sub-class that implements a sparse vector should override this
     * function, since the default iterator can not work with unknown
     * dimensionality.
     * @return a pointer to the newly created non-zero iterator
     */
    virtual std::shared_ptr<VectorIterator<true> > nonZeroIterator() const;
    
    // Static functions...
    /**
     * Applies a unary function to every components of a vector, and stores the
     * output to another vector.
     * Let the dimensionality of input and output to be d_i, d_o. Note that:
     * 1. This function allows for the output to be the same as input to make
     * the operation in-place.
     * 2. While usually the dimensionality of input and output should be the
     * same (i.e., d_i == d_o), it is not a requirement. However, the output
     * vector should be able to store the results (i.e., d_o >= d_i).
     * 3. The output range will be up to input's dimensionality (i.e.,
     * [0, d_i)).
     * 4. This function only guarantees the results of the components that
     * applied the function. It is the caller's responsibility to ensure the
     * output vector is "clean" (i.e., contains all 0) before use.
     * @param input the input vector
     * @param output the output vector
     * @param op the unary function, can be a function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components with input as 0 can be skipped
     * @return the output vector
     * @throws std::out_of_range if nonZeroOnly is false, and input is a sparse
     * vector of unknown dimensionality, since the output dimensionality can not
     * be decided
     * @throws std::runtime_error if skipZero is not SKIP_NONE or SKIP_1.
     */
    template <typename TUnaryOp>
    static Vector& transform(const Vector& input, Vector& output, TUnaryOp op,
    SkipType skipZero = SKIP_NONE);
    
    /**
     * Applies a binary function to every components of two vectors, and stores
     * the output to another vector.
     * Let the dimensionality of input and output to be d_i1, d_i2, d_o. Note
     * that:
     * 1. This function allows for the output to be the same as input to make
     * the operation in-place.
     * 2. While usually the dimensionality of input and output should be the
     * same (i.e., d_i1 == d_i2 == d_o), it is not a requirement. However, the
     * output vector should be able to store the results (i.e., d_o >=
     * max(d_i1, d_i2)).
     * 3. The function will be applied between every corresponding components of
     * the input vectors. If they are of different size, the shorter input
     * vector will be treated as zero-padded.
     * 4. The output range will be up to input's dimensionality (i.e., [0,
     * max(d_i1, d_i2)).
     * 5. This function only guarantees the results of the components that
     * applied the function. It is the caller's responsibility to ensure the
     * output vector is "clean" (i.e., contains all 0) before use.
     * @param input1 the left-hand side input vector
     * @param input2 the right-hand side input vector
     * @param output the output vector
     * @param op the binary function, can be a function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components with left input as 0 can be skipped, [SKIP_2]
     * components with right input as 0 can be skipped, [SKIP_OR] components
     * with left OR right input as 0 can be skipped, [SKIP_AND] components with
     * left AND right input as 0 can be skipped
     * @return the output vector
     * @throws std::out_of_range if nonZeroOnly is false, and EITHER input1 OR
     * input2 is a sparse vector of unknown dimensionality, since the output
     * dimensionality can not be decided
     */
    template <typename TBinaryOp>
    static Vector& transform(const Vector& input1, const Vector& input2,
    Vector& output, TBinaryOp op, SkipType skipZero = SKIP_NONE);
    
    /**
     * Applies a binary function to every components of a vector and a number,
     * and stores the output to another vector.
     * Let the dimensionality of input1 and output to be d_i1, d_o. Note that:
     * 1. This function allows for the output to be the same as input1 to make
     * the operation in-place.
     * 2. While usually the dimensionality of input1 and output should be the
     * same (i.e., d_i1 == d_o), it is not a requirement. However, the output
     * vector should be able to store the results (i.e., d_o >= d_i1).
     * 3. The output range will be up to input's dimensionality (i.e., [0,
     * d_i1)).
     * 4. This function only guarantees the results of the components that
     * applied the function. It is the caller's responsibility to ensure the
     * output vector is "clean" (i.e., contains all 0) before use.
     * @param input1 the left-hand side input vector
     * @param input2 the right-hand side input number
     * @param output the output vector
     * @param op the binary function, can be a function object or pointer
     * @param skipZero whether the function can skip zero components for
     * possible optimization: [SKIP_NONE] NONE of the components can be skipped,
     * [SKIP_1] components with left input as 0 can be skipped, [SKIP_2]
     * components with right input as 0 can be skipped, [SKIP_OR] components
     * with left OR right input as 0 can be skipped, [SKIP_AND] components with
     * left AND right input as 0 can be skipped
     * @return the output vector
     * @throws std::out_of_range if nonZeroOnly is false, and input1 is a sparse
     * vector of unknown dimensionality, since the output dimensionality can not
     * be decided
     */
    template <typename TBinaryOp>
    static Vector& transform(const Vector& input1, const compute_type input2,
    Vector& output, TBinaryOp op, SkipType skipZero = SKIP_NONE);
};

/**
 * Append the vector to an output stream.
 * @param os the output stream
 * @param obj the vector to be appended
 * @return the output stream
 */
std::ostream& operator <<(std::ostream& os, const Vector& obj);

}} // ins::math::

// Hash functions...
namespace std {

template <>
struct hash<ins::math::Vector> {
    size_t operator()(const ins::math::Vector& v) const;
};

} // std::

#endif	/* INS_MATH_BASE_HPP */

