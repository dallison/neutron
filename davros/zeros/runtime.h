#pragma once

#include "davros/common_runtime.h"
#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
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
    PayloadBuffer *GetBuffer() const {                                         \
      return Message::GetBuffer(this, source_offset_);                         \
    }                                                                          \
    BufferOffset GetMessageStart() const {                                     \
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
  StringField() = default;
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
  template <int N> friend class StringArrayField;

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
  MessageField() = default;
  MessageField(std::shared_ptr<PayloadBuffer *> buffer,
               BufferOffset source_offset, BufferOffset binary_offset)
      : msg_(buffer,
             Message::GetMessageStart(this, source_offset) + binary_offset) {}

  operator MessageType &() { return msg_; }
  MessageType &operator*() { return msg_; }
  MessageType *operator->() { return &msg_; }

private:
  template <typename T> friend class MessageVectorField;
  MessageType msg_;
};

template <typename Field, typename T> struct FieldIterator {
  FieldIterator(Field *f, BufferOffset o) : field(f), offset(o) {}

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
    return *addr;
  }

  bool operator==(const FieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const FieldIterator &it) const { return !operator==(it); }

  Field *field;
  BufferOffset offset;
};

template <typename Field> struct StringFieldIterator {
  StringFieldIterator(Field *f, BufferOffset o) : field(f), offset(o) {}

  StringFieldIterator &operator++() {
    offset += sizeof(BufferOffset);
    return *this;
  }
  StringFieldIterator &operator--() {
    offset -= sizeof(BufferOffset);
    return *this;
  }
  StringFieldIterator operator+(size_t i) {
    return iterator(field, field->BaseOffset() + i * sizeof(BufferOffset));
  }
  StringFieldIterator operator-(size_t i) {
    return iterator(field, field->BaseOffset() - i * sizeof(BufferOffset));
  }
  std::string_view operator*() const {
    return field->GetBuffer()->GetStringView(field->BaseOffset() + offset);
  }

  bool operator==(const StringFieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const StringFieldIterator &it) const {
    return !operator==(it);
  }

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
template <typename T, int N> class MessageArrayField {
public:
  explicit MessageArrayField(uint32_t source_offset, uint32_t binary_offset) {
    std::shared_ptr<PayloadBuffer *> buffer =
        Message::GetSharedBuffer(this, source_offset);
    // Construct the embedded messages.
    size_t index = 0;
    for (auto &msg : msgs_) {
      msg.buffer = buffer;
      BufferOffset offset = binary_offset + T::BinarySize() * index;
      msg.start_offset = offset;
      index++;
    }
  }

  T &operator[](int index) { return msgs_[index]; }

  operator std::array<T, N> &() { return msgs_; }

  using iterator = typename std::array<T, N>::iterator;

  iterator begin() { return msgs_.begin(); }
  iterator end() { return msgs_.end(); }

  size_t size() const { return N; }
  T *data() const { msgs_.data(); }

private:
  std::array<T, N> msgs_;
};

// This is a fixed length array of StringField.
// It looks like a std::array<StringField,N>.
template <int N> class StringArrayField {
public:
  explicit StringArrayField(uint32_t source_offset, uint32_t binary_offset) {
    for (size_t i = 0; i < N; i++) {
      strings_[i].source_offset_ = source_offset + sizeof(StringField) * i;
      strings_[i].binary_offset_ = binary_offset + sizeof(BufferOffset) * i;
    }
  }

  StringField &operator[](int index) { return strings_[index]; }

  using iterator = typename std::array<StringField, N>::iterator;

  iterator begin() { return strings_.begin(); }
  iterator end() { return strings_.end(); }

  size_t size() const { return N; }
  StringField *data() const { return strings_.data(); }

private:
  std::array<StringField, N> strings_;
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
    return base[index];
  }

  using iterator = FieldIterator<PrimitiveVectorField, T>;

  iterator begin() { return iterator(this, BaseOffset()); }
  iterator end() {
    return iterator(this, BaseOffset() + NumElements() * sizeof(T));
  }

  void push_back(const T &v) {
    PayloadBuffer::VectorPush<T>(GetBufferAddr(), Header(), v);
  }

  void reserve(size_t n) {
    PayloadBuffer::VectorReserve<T>(GetBufferAddr(), Header(), n);
  }

  void resize(size_t n) {
    PayloadBuffer::VectorResize<T>(GetBufferAddr(), Header(), n);
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  T *data() const { GetBuffer()->template ToAddress<T>(BaseOffset()); }

 size_t capacity() const {
    VectorHeader* hdr = Header();
    BufferOffset* addr = GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(T);
  }
  
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

template <typename T> class MessageVectorField {
public:
  explicit MessageVectorField(uint32_t source_offset, uint32_t binary_offset)
      : source_offset_(source_offset), binary_offset_(binary_offset) {
    // Populate the msgs vector with MessageField objects referring to the
    // binary messages.
    VectorHeader *hdr = Header();
    for (uint32_t i = 0; i < hdr->num_elements; i++) {
      if (hdr->data == 0) {
        // If the vector says there's a message at this index but
        // the data is 0 it shows corruption in the binary message.
        // TODO: How do we deal with this?
        // abort for now
        std::cerr << "Invalid message vector entry at index " << i << std::endl;
        abort();
      }
      MessageField<T> field(
          GetSharedBuffer(),
          source_offset + msgs_.size() * sizeof(MessageField<T>), hdr->data);
      msgs_.push_back(std::move(field));
    }
  }

  MessageField<T> &operator[](int index) { return msgs_[index]; }

  using iterator = typename std::vector<MessageField<T>>::iterator;

  iterator begin() { return msgs_.begin(); }
  iterator end() { return msgs_.end(); }

  void push_back(const T &v) {
    BufferOffset offset = v.start_offset;
    MessageField<T> field(GetSharedBuffer(),
                          msgs_.size() * sizeof(MessageField<T>),
                          binary_offset_ + msgs_.size() * sizeof(BufferOffset));
    field.msg_ = v;
    msgs_.push_back(std::move(field));
    PayloadBuffer::VectorPush<BufferOffset>(GetBufferAddr(), Header(), offset);
  }

  size_t capacity() const {
    VectorHeader* hdr = Header();
    BufferOffset* addr = GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(BufferOffset);
  }

  void reserve(size_t n) {
    PayloadBuffer::VectorReserve<BufferOffset>(GetBufferAddr(), Header(), n);
    msgs_.reserve(n);
  }

  void resize(size_t n) {
    VectorHeader *hdr = Header();
    uint32_t current_size = hdr->num_elements;

    // Resize the vector data in the binary.  This contains BufferOffets.
    PayloadBuffer::VectorResize<BufferOffset>(GetBufferAddr(), Header(), n);
    msgs_.resize(n);

    // If the size has increased, allocate messages for the new entries and set
    // the offsets in the MessageFields in the source message.
    if (n > current_size) {
      for (uint32_t i = current_size; i < uint32_t(n); i++) {
        void *binary =
            PayloadBuffer::Allocate(GetBufferAddr(), T::BinarySize(), 8, true);
        BufferOffset binary_offset = GetBuffer()->ToOffset(binary);
        auto &m = msgs_[i];
        m.msg_.buffer = GetSharedBuffer();
        m.msg_.start_offset = binary_offset;
      }
    }
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  T *data() const { GetBuffer()->template ToAddress<T>(BaseOffset()); }

private:
  friend FieldIterator<MessageVectorField, T>;
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

  std::shared_ptr<PayloadBuffer *> GetSharedBuffer() {
    return Message::GetSharedBuffer(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset binary_offset_;
  std::vector<MessageField<T>> msgs_;
};
}
