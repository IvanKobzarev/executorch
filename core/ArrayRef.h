//===--- ArrayRef.h - Array Reference Wrapper -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// ATen: modified from llvm::ArrayRef.
// removed llvm-specific functionality
// removed some implicit const -> non-const conversions that rely on
// complicated std::enable_if meta-programming
// removed a bunch of slice variants for simplicity...

#pragma once

#include <array>
#include <iterator>
#include <vector>


namespace torch {
namespace executor {

/// ArrayRef - Represent a constant reference to an array (0 or more elements
/// consecutively in memory), i.e. a start pointer and a length.  It allows
/// various APIs to take consecutive elements easily and conveniently.
///
/// This class does not own the underlying data, it is expected to be used in
/// situations where the data resides in some other buffer, whose lifetime
/// extends past that of the ArrayRef. For this reason, it is not in general
/// safe to store an ArrayRef.
///
/// This is intended to be trivially copyable, so it should be passed by
/// value.
template <typename T>
class ArrayRef final {
 public:
  using iterator = const T*;
  using const_iterator = const T*;
  using size_type = size_t;
  using value_type = T;

  using reverse_iterator = std::reverse_iterator<iterator>;

 private:
  /// The start of the array, in an external buffer.
  const T* Data;

  /// The number of elements.
  size_type Length;

 public:
  /// @name Constructors
  /// @{

  /// Construct an empty ArrayRef.
  /* implicit */ constexpr ArrayRef() : Data(nullptr), Length(0) {}

  /// Construct an ArrayRef from a single element.
  // TODO Make this explicit
  constexpr ArrayRef(const T& OneElt) : Data(&OneElt), Length(1) {}

  /// Construct an ArrayRef from a pointer and length.
  constexpr ArrayRef(const T* data, size_t length)
      : Data(data), Length(length) {
    assert(Data != nullptr || Length == 0);
  }

  /// Construct an ArrayRef from a range.
   ArrayRef(const T* begin, const T* end)
      : Data(begin), Length(end - begin) {
    assert(Data != nullptr || Length == 0);
  }


  /// Construct an ArrayRef from a std::vector.
  // The enable_if stuff here makes sure that this isn't used for
  // std::vector<bool>, because ArrayRef can't work on a std::vector<bool>
  // bitfield.
  template <typename A>
  /* implicit */ ArrayRef(const std::vector<T, A>& Vec)
      : Data(Vec.data()), Length(Vec.size()) {
    static_assert(
        !std::is_same<T, bool>::value,
        "ArrayRef<bool> cannot be constructed from a std::vector<bool> bitfield.");
  }

  /// Construct an ArrayRef from a std::array
  template <size_t N>
  /* implicit */ constexpr ArrayRef(const std::array<T, N>& Arr)
      : Data(Arr.data()), Length(N) {}

  /// Construct an ArrayRef from a C array.
  template <size_t N>
  /* implicit */ constexpr ArrayRef(const T (&Arr)[N]) : Data(Arr), Length(N) {}

  /// Construct an ArrayRef from a std::initializer_list.
  /* implicit */ constexpr ArrayRef(const std::initializer_list<T>& Vec)
      : Data(
            std::begin(Vec) == std::end(Vec) ? static_cast<T*>(nullptr)
                                             : std::begin(Vec)),
        Length(Vec.size()) {}

  /// @}
  /// @name Simple Operations
  /// @{

  constexpr iterator begin() const {
    return Data;
  }
  constexpr iterator end() const {
    return Data + Length;
  }

  // These are actually the same as iterator, since ArrayRef only
  // gives you const iterators.
  constexpr const_iterator cbegin() const {
    return Data;
  }
  constexpr const_iterator cend() const {
    return Data + Length;
  }

  constexpr reverse_iterator rbegin() const {
    return reverse_iterator(end());
  }
  constexpr reverse_iterator rend() const {
    return reverse_iterator(begin());
  }

  /// empty - Check if the array is empty.
  constexpr bool empty() const {
    return Length == 0;
  }

  constexpr const T* data() const {
    return Data;
  }

  /// size - Get the array size.
  constexpr size_t size() const {
    return Length;
  }

  /// front - Get the first element.
   const T& front() const {
    // ArrayRef: attempted to access front() of empty list
    assert(!empty());
    return Data[0];
  }

  /// back - Get the last element.
   const T& back() const {
    // ArrayRef: attempted to access back() of empty list
    assert(!empty());
    return Data[Length - 1];
  }

  /// equals - Check for element-wise equality.
  constexpr bool equals(ArrayRef RHS) const {
    return Length == RHS.Length && std::equal(begin(), end(), RHS.begin());
  }

  /// slice(n, m) - Take M elements of the array starting at element N
   ArrayRef<T> slice(size_t N, size_t M)
      const {
    // cant slice longer then the array
    assert(N + M <= size());
    return ArrayRef<T>(data() + N, M);
  }

  /// slice(n) - Chop off the first N elements of the array.
  constexpr ArrayRef<T> slice(size_t N) const {
    return slice(N, size() - N);
  }

  /// @}
  /// @name Operator Overloads
  /// @{
  constexpr const T& operator[](size_t Index) const {
    return Data[Index];
  }

  /// Vector compatibility
   const T& at(size_t Index) const {
    // invalid index
    assert(Index < Length);
    return Data[Index];
  }

  /// Disallow accidental assignment from a temporary.
  ///
  /// The declaration here is extra complicated so that "arrayRef = {}"
  /// continues to select the move assignment operator.
  template <typename U>
  typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type&
  operator=(U&& Temporary) = delete;

  /// Disallow accidental assignment from a temporary.
  ///
  /// The declaration here is extra complicated so that "arrayRef = {}"
  /// continues to select the move assignment operator.
  template <typename U>
  typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type&
  operator=(std::initializer_list<U>) = delete;

  /// @}
  /// @name Expensive Operations
  /// @{
  std::vector<T> vec() const {
    return std::vector<T>(Data, Data + Length);
  }

  /// @}
};

/// @name ArrayRef Convenience constructors
/// @{

/// Construct an ArrayRef from a single element.
template <typename T>
ArrayRef<T> makeArrayRef(const T& OneElt) {
  return OneElt;
}

/// Construct an ArrayRef from a pointer and length.
template <typename T>
ArrayRef<T> makeArrayRef(const T* data, size_t length) {
  return ArrayRef<T>(data, length);
}

/// Construct an ArrayRef from a range.
template <typename T>
ArrayRef<T> makeArrayRef(const T* begin, const T* end) {
  return ArrayRef<T>(begin, end);
}

/// Construct an ArrayRef from a std::vector.
template <typename T>
ArrayRef<T> makeArrayRef(const std::vector<T>& Vec) {
  return Vec;
}

/// Construct an ArrayRef from a std::array.
template <typename T, std::size_t N>
ArrayRef<T> makeArrayRef(const std::array<T, N>& Arr) {
  return Arr;
}

/// Construct an ArrayRef from an ArrayRef (no-op) (const)
template <typename T>
ArrayRef<T> makeArrayRef(const ArrayRef<T>& Vec) {
  return Vec;
}

/// Construct an ArrayRef from an ArrayRef (no-op)
template <typename T>
ArrayRef<T>& makeArrayRef(ArrayRef<T>& Vec) {
  return Vec;
}

/// Construct an ArrayRef from a C array.
template <typename T, size_t N>
ArrayRef<T> makeArrayRef(const T (&Arr)[N]) {
  return ArrayRef<T>(Arr);
}

// WARNING: Template instantiation will NOT be willing to do an implicit
// conversions to get you to an ArrayRef, which is why we need so
// many overloads.

template <typename T>
bool operator==(ArrayRef<T> a1, ArrayRef<T> a2) {
  return a1.equals(a2);
}

template <typename T>
bool operator!=(ArrayRef<T> a1, ArrayRef<T> a2) {
  return !a1.equals(a2);
}

template <typename T>
bool operator==(const std::vector<T>& a1, ArrayRef<T> a2) {
  return ArrayRef<T>(a1).equals(a2);
}

template <typename T>
bool operator!=(const std::vector<T>& a1, ArrayRef<T> a2) {
  return !ArrayRef<T>(a1).equals(a2);
}

template <typename T>
bool operator==(ArrayRef<T> a1, const std::vector<T>& a2) {
  return a1.equals(ArrayRef<T>(a2));
}

template <typename T>
bool operator!=(ArrayRef<T> a1, const std::vector<T>& a2) {
  return !a1.equals(ArrayRef<T>(a2));
}

using IntArrayRef = ArrayRef<int64_t>;

} // executor
} // torch
