#pragma once

#include "gemini/zeros/arrays.h"
#include "gemini/zeros/fields.h"
#include "gemini/zeros/iterators.h"
#include "gemini/zeros/vectors.h"

namespace gemini::zeros {

class Buffer;

// There are two message formats for zero copy ROS:
// 1. Source format - as seen by the programmer - a C++ class
// 2. Binary format - sent over the wire in binary
//
// Source messages are used in the program to access the fields by the
// program.  Binary messages are held in a toolbelt::PayloadBuffer and contain the
// actual data.  Accessing a source message field results in the data being
// written or read in the binary message.  The source message does not
// contain the values of the fields - just information about where to find
// the values in the binary.
//
// All fields know both the source and binary offsets.  The source offset
// is a positive integer containing the number of bytes from the source field
// to the start of the enclosing message.  This is used to get access to the
// toolbelt::PayloadBuffer containing the binary mesage.
//
// The binary offset in the field is the offset from the start of the
// enclosing binary message to the field in the toolbelt::PayloadBuffer.
//
// The toolbelt::PayloadBuffer is organized as a header (of type toolbelt::PayloadBuffer)
// and a memory allocation heap that allows variable sized memory blocks
// to be allocated, reallocated and freed.  Everything in the buffer
// is relocateable and can be moved around or sent to another process.
//
//        +-------------------+
//        |                   |
//        |  toolbelt::PayloadBuffer    |
//        |                   |
//        +-------------------+
//        |                   |
//        |                   |
//        |                   |
//        |                   |
//        |    Buffer heap    |
//        |                   |
//        |                   |
//        |                   |
//        |                   |
//        |                   |
//        +-------------------+
//
// Primitive fields
// ----------------
// These are things like integers, floating point numbers, enums and
// Time and Duration types.  They are stored inline in the buffer in their
// native format and endianness, aligned to their appropriate alignment.
// For example, a 64 bit floating point number is stored in IEEE 754
// binary format.
//
// String fields
// --------------
// These are stored as a 32-bit offset (absolute from the start of the
// buffer) to the string data, which is a 32-bit length followed by the
// string bytes.  In the following diagram, O is the offset to the string
// data; L is the length of the string data (in bytes, 32-bit native
// format) and "str" is the string bytes.  The bytes are NOT terminated
// by a NUL byte.  The offset and the length are aligned to a 4 byte
// boundary.
//
//         +----------------+
//         |                |
//         |                |
//         |      +-----+   |
//         |      |  O  |------+
//         |      +-----+   |  |
//         |                |  |
//         |  +----------------+
//         |  V             |
//         |  +---+-------+ |
//         |  | L | "str" | |
//         |  +---+-------+ |
//         |                |
//         |                |
//         |                |
//         |                |
//         |                |
//         +----------------+
//
// Message fields
// --------------
// A message field is stored inline in the current message, aligned to an 8 byte
// boundary.  The length is aligned to 4 bytes.
//
// Fixed size primitive array fields
// ----------------------------------
// These are stored as inline arrays of fixed size.  They are aligned to the
// native alignment of the type.
//
// Fixed size string array fields.
// -------------------------------
// These are a fixed size set of offsets to string data, aligned to 4 bytes.  Each
// string is stored as a regular string field, with the offset referring to its
// length.
//
// Fixed size message array fields
// -------------------------------
// These are stored inline as a fixed size array of messages.  The array is
// aligned to 8 bytes.
//
// Variable size primitive fields
// ------------------------------
// This is stored as an 8-byte toolbelt::VectorHeader consisting of:
// 1. uint32_t num_elements
// 2. toolbelt::BufferOffset offset
//
// The offset refers to an allocated block of memory (in the buffer) that starts
// out with sufficient space for 2 elements (2 * sizeof(T) bytes) and grows
// exponentially as the space is used up.  The capacity of this block is
// held in a 32-bit little endian word immediately preceding the memory
// block that specifies the number of bytes occupied by the block (not number
// of elements).
//
// The num_elements member is the number of used elements in the field, which
// is always less than or equal to the capacity divided by the element size.
//
//          +--------------+
//          | num_elements |
//          +--------------+
//          | offset       |-----+
//          +--------------+     |
//                               |
//          +--------------------+
//          |
//          |
//          |    +---------------+
//          |    | block size    |  <-- in bytes (capacity * sizeof(T))
//          +--->+---------------+
//               |               |
//               |  array        |
//               |  data         |
//               |               |  <--- contents of vector
//               |               |
//               |               |
//               |               |
//               +---------------+
//
// Variable size string fields
// ---------------------------
// As variable sized primitive fields with each element being an offset
// to the string length and data
//
// Variable size message fields.
// ----------------------------
// As variable sized primitive fields with each element being an offset
// to a message allocated from a block of memory.

template <typename T>
constexpr size_t AlignedOffset(size_t offset) {
  return (offset + sizeof(T) - 1) & ~(sizeof(T) - 1);
}

#define DEFINE_PRIMITIVE_FIELD_STREAMER(cname, type)           \
  inline std::ostream &operator<<(std::ostream &os,            \
                                  const cname##Field &field) { \
    os << field.Get() << std::endl;                            \
    return os;                                                 \
  }

DEFINE_PRIMITIVE_FIELD_STREAMER(Int8, int8_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Uint8, uint8_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Int16, int16_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Uint16, uint16_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Int32, int32_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Uint32, uint32_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Int64, int64_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Uint64, uint64_t)
DEFINE_PRIMITIVE_FIELD_STREAMER(Float32, float)
DEFINE_PRIMITIVE_FIELD_STREAMER(Float64, double)
DEFINE_PRIMITIVE_FIELD_STREAMER(Bool, bool)
DEFINE_PRIMITIVE_FIELD_STREAMER(Duration, gemini::Duration)
DEFINE_PRIMITIVE_FIELD_STREAMER(Time, gemini::Time)

inline std::ostream &operator<<(std::ostream &os, const StringField &field) {
  os << field.Get() << std::endl;
  return os;
}

inline std::ostream &operator<<(std::ostream &os,
                                const NonEmbeddedStringField &field) {
  os << field.Get() << std::endl;
  return os;
}

template <typename Enum>
inline std::ostream &operator<<(std::ostream &os,
                                const EnumField<Enum> &field) {
  os << field.Get() << std::endl;
  return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const MessageField<T> &field) {
  os << field.Get() << std::endl;
  return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const NonEmbeddedMessageField<T> &field) {
  os << field.Get() << std::endl;
  return os;
}

// Arrays
template <typename T, int N>
inline std::ostream &operator<<(std::ostream &os,
                                const PrimitiveArrayField<T, N> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <typename Enum, int N>
inline std::ostream &operator<<(std::ostream &os,
                                const EnumArrayField<Enum, N> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <int N>
inline std::ostream &operator<<(std::ostream &os,
                                const StringArrayField<N> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <typename T, int N>
inline std::ostream &operator<<(std::ostream &os,
                                const MessageArrayField<T, N> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const PrimitiveVectorField<T> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <typename Enum>
inline std::ostream &operator<<(std::ostream &os,
                                const EnumVectorField<Enum> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os,
                                const StringVectorField &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const MessageVectorField<T> &field) {
  for (auto &v : field.Get()) {
    os << v << std::endl;
  }
  return os;
}

#undef DEFINE_PRIMITIVE_FIELD_STREAMER

}  // namespace gemini::zeros
