#pragma once

#include "davros/zeros/arrays.h"
#include "davros/zeros/fields.h"
#include "davros/zeros/iterators.h"
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

#define DEFINE_PRIMITIVE_FIELD_STREAMER(cname, type)                           \
  inline std::ostream &operator<<(std::ostream &os,                            \
                                  const cname##Field &field) {                 \
    os << field.Get() << std::endl;                                            \
    return os;                                                                 \
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
DEFINE_PRIMITIVE_FIELD_STREAMER(Duration, davros::Duration)
DEFINE_PRIMITIVE_FIELD_STREAMER(Time, davros::Time)

inline std::ostream &operator<<(std::ostream &os, const StringField &field) {
  os << field.Get() << std::endl;
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const NonEmbeddedStringField &field) {
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
} // namespace davros::zeros
