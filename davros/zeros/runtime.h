#pragma once

#include "davros/zeros/fields.h"
#include "davros/zeros/iterators.h"
#include "davros/zeros/arrays.h"
#include "davros/zeros/vectors.h"

namespace davros::zeros {

class Buffer;

// There are two message formats for zero copy ROS:
// 1. Source format - as seen by the programmer - a C++ class
// 2. Binary format - sent over the wire in binary
//
// Source messages are used in the program to access the fields by the
// program.  Binary messages are held in a PayloadBuffer and contain the
// actual data.  Accessing a source message field results in the data being
// written or read in the binary message.  The source message does not
// contain the values of the fields - just information about where to find
// the values in the binary.
//
// All fields know both the source and binary offsets.  The source offset
// is a positive integer containing the number of bytes from the source field
// to the start of the enclosing message.  This is used to get access to the
// PayloadBuffer containing the binary mesage.
//
// The binary offset in the field is the offset from the start of the
// enclosing binary message to the field in the PayloadBuffer.

template <typename T> constexpr size_t AlignedOffset(size_t offset) {
  return (offset + sizeof(T) - 1) & ~(sizeof(T) - 1);
}

} // namespace davros::zeros
