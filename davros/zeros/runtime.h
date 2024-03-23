#pragma once

#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include "davros/common_runtime.h"
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace davros::zeros {

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

#define DEFINE_PRIMITIVE_FIELD(cname, type)                                    \
  class cname##Field {                                                         \
  public:                                                                      \
    explicit cname##Field(uint32_t boff, uint32_t offset)                      \
        : source_offset_(boff), binary_offset_(offset) {}                      \
                                                                               \
    operator type() {                                                          \
      return GetBuffer()->Get<type>(GetMessageStart() + binary_offset_);       \
    }                                                                          \
                                                                               \
    cname##Field &operator=(type i) {                                          \
      GetBuffer()->Set(GetMessageStart() + binary_offset_, i);                 \
      return *this;                                                            \
    }                                                                          \
                                                                               \
  private:                                                                     \
    PayloadBuffer *GetBuffer() const {                                               \
      return Message::GetBuffer(this, source_offset_);                         \
    }                                                                          \
    BufferOffset GetMessageStart() const {                                           \
      return Message::GetMessageStart(this, source_offset_);                   \
    }                                                                          \
    uint32_t source_offset_;                                                   \
    BufferOffset binary_offset_;                                               \
  };

DEFINE_PRIMITIVE_FIELD(Int8, int8_t)
DEFINE_PRIMITIVE_FIELD(Uint8, uint8_t)
DEFINE_PRIMITIVE_FIELD(Int16, int16_t)
DEFINE_PRIMITIVE_FIELD(Uint16, uint16_t)
DEFINE_PRIMITIVE_FIELD(Int32, int32_t)
DEFINE_PRIMITIVE_FIELD(Uint32, uint32_t)
DEFINE_PRIMITIVE_FIELD(Int64, int64_t)
DEFINE_PRIMITIVE_FIELD(Uint64, uint64_t)
DEFINE_PRIMITIVE_FIELD(Float32, float)
DEFINE_PRIMITIVE_FIELD(Float64, double)
DEFINE_PRIMITIVE_FIELD(Bool, bool)
DEFINE_PRIMITIVE_FIELD(Duration, davros::Duration)
DEFINE_PRIMITIVE_FIELD(Time, davros::Time)

class StringField {
public:
  explicit StringField(uint32_t source_offset, uint32_t binary_offset)
      : source_offset_(source_offset), binary_offset_(binary_offset) {}

  operator std::string_view() {
    return GetBuffer()->GetStringView(GetMessageStart() + binary_offset_);
  }

  StringField &operator=(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s,
                             GetMessageStart() + binary_offset_);
    return *this;
  }

private:
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageStart() const {
    return Message::GetMessageStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset binary_offset_;
};

// A message field enapsulates a message that is held inline in the
// parent message, both in the source message and the
// binary message.
template <typename MessageType> class MessageField {
public:
  MessageField(std::shared_ptr<PayloadBuffer *> buffer,
               BufferOffset source_offset, BufferOffset binary_offset)
      : msg_(buffer,
             Message::GetMessageStart(this, source_offset) + binary_offset) {}

  operator MessageType &() { return msg_; }
  MessageType &operator*() { return msg_; }
  MessageType *operator->() { return &msg_; }

private:
  MessageType msg_;
};

template <typename Field, typename T> struct FieldIterator {
  FieldIterator(Field *f, BufferOffset o) : field(f), offset(o) {
    std::cout << "iterator at offset " << std::hex << o << std::endl;
  }

  FieldIterator &operator++() {
    offset += sizeof(T);
    return *this;
  }
  FieldIterator &operator--() {
    offset -= sizeof(T);
    return *this;
  }
  FieldIterator operator+(size_t i) {
    return iterator(field, field->BaseOffset() + i * sizeof(T));
  }
  FieldIterator operator-(size_t i) {
    return iterator(field, field->BaseOffset() - i * sizeof(T));
  }
  T &operator*() const {
    T *addr = field->GetBuffer()->template ToAddress<T>(offset);
    std::cout << "offset " << offset << " addr " << addr 
              << std::endl;
    return *addr;
  }

  bool operator==(const FieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const FieldIterator &it) const { return !operator==(it); }

  Field *field;
  BufferOffset offset;
};

// This is a fixed length array of T.  It looks like a std::array<T,N>.
template <typename T, int N> class PrimitiveArrayField {
public:
  explicit PrimitiveArrayField(uint32_t source_offset, uint32_t binary_offset)
      : source_offset_(source_offset), binary_offset_(binary_offset) {}

  T &operator[](int index) {
    T *base = GetBuffer()->template ToAddress<T>(BaseOffset());
    return base[index];
  }

  using iterator = FieldIterator<PrimitiveArrayField, T>;

  iterator begin() { return iterator(this, BaseOffset()); }
  iterator end() { return iterator(this, BaseOffset() + N * sizeof(T)); }

  size_t size() const { return N; }
  T *data() const { GetBuffer()->template ToAddress<T>(BaseOffset()); }

private:
  friend FieldIterator<PrimitiveArrayField, T>;
  BufferOffset BaseOffset() const {
    return Message::GetMessageStart(this, source_offset_) + binary_offset_;
  }
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageStart() const {
    return Message::GetMessageStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset binary_offset_;
};

// This is a fixed array of messages.  T must be derived from davros::Message.
template <typename T, int N> class MessageArrayField  {
public:
  explicit MessageArrayField(uint32_t source_offset, uint32_t binary_offset)
       {
        std::shared_ptr<PayloadBuffer*> buffer = Message::GetSharedBuffer(this, source_offset);
        // Construct the embedded messages.
        size_t index = 0;
        for (auto& msg : msgs_) {
          msg.buffer = buffer;
          BufferOffset offset = binary_offset + T::BinarySize()* index;
          msg.start_offset = offset;
          index++;
        }
      }

  T &operator[](int index) {
    return msgs_[index];
  }

  operator std::array<T,N>&() {
    return msgs_;
  }

  using iterator = typename std::array<T,N>::iterator;

  iterator begin() { return msgs_.begin(); }
  iterator end() { return msgs_.end(); }

  size_t size() const { return N; }
  T *data() const { msgs_.data(); }

private:
  std::array<T, N> msgs_;
};


// This is a variable length vector of T.  It looks like a std::vector<T>.
// The binary message contains a VectorHeader at the binary offset.  This
// contains the number of elements and the base offset for the data.
template <typename T> class PrimitiveVectorField {
public:
  explicit PrimitiveVectorField(uint32_t source_offset, uint32_t binary_offset)
      : source_offset_(source_offset), binary_offset_(binary_offset) {}

  T &operator[](int index) {
    T *base = GetBuffer()->template ToAddress<T>(BaseOffset());
    std::cout << "base at " << base << std::endl;
    return base[index];
  }

  using iterator = FieldIterator<PrimitiveVectorField, T>;

  iterator begin() { return iterator(this, BaseOffset()); }
  iterator end() {
    return iterator(this, BaseOffset() + NumElements() * sizeof(T));
  }

  void push_back(const T& v) {
    PayloadBuffer::VectorPush<T>(GetBufferAddr(), Header(), v);
  }

  void reserve(size_t n) {
    PayloadBuffer::VectorReserve<T>(GetBufferAddr(), Header(), n);
  }

  void resize(size_t n) {
    PayloadBuffer::VectorResize<T>(GetBufferAddr(), Header(), n);
  }

  void clear() {
    Header()->num_elements = 0;
  }

  size_t size() const { return Header()->num_elements; }
  T *data() const { GetBuffer()->template ToAddress<T>(BaseOffset()); }

private:
  friend FieldIterator<PrimitiveVectorField, T>;
  VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<VectorHeader>(
        Message::GetMessageStart(this, source_offset_) + binary_offset_);
  }

  BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageStart() const {
    return Message::GetMessageStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset binary_offset_;
};
}
