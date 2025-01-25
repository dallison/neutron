#pragma once

// Vector fields.

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "neutron/common_runtime.h"
#include "neutron/zeros/fields.h"
#include "neutron/zeros/iterators.h"
#include "neutron/zeros/message.h"
#include "toolbelt/payload_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace neutron::zeros {

#define DECLARE_ZERO_COPY_VECTOR_BITS(vtype, itype, ctype, utype)              \
  using value_type = vtype;                                                    \
  using reference = value_type &;                                              \
  using const_reference = value_type &;                                        \
  using pointer = value_type *;                                                \
  using const_pointer = const value_type *;                                    \
  using size_type = size_t;                                                    \
  using difference_type = ptrdiff_t;                                           \
                                                                               \
  using iterator = itype;                                                      \
  using const_iterator = ctype;                                                \
  using reverse_iterator = itype;                                              \
  using const_reverse_iterator = ctype;                                        \
                                                                               \
  iterator begin() { return iterator(this, BaseOffset()); }                    \
  iterator end() {                                                             \
    return iterator(this, BaseOffset() + NumElements() * sizeof(value_type));  \
  }                                                                            \
  const_iterator begin() const { return const_iterator(this, BaseOffset()); }  \
  const_iterator end() const {                                                 \
    return const_iterator(this,                                                \
                          BaseOffset() + NumElements() * sizeof(value_type));  \
  }                                                                            \
  const_iterator cbegin() const { return const_iterator(this, BaseOffset()); } \
  const_iterator cend() const {                                                \
    return const_iterator(this,                                                \
                          BaseOffset() + NumElements() * sizeof(value_type));  \
  }                                                                            \
                                                                               \
  reverse_iterator rbegin() {                                                  \
    return reverse_iterator(this, BaseOffset(), true);                         \
  }                                                                            \
  reverse_iterator rend() {                                                    \
    return reverse_iterator(                                                   \
        this, BaseOffset() + NumElements() * sizeof(value_type), true);        \
  }                                                                            \
  const_reverse_iterator rbegin() const {                                      \
    return const_reverse_iterator(this, BaseOffset(), true);                   \
  }                                                                            \
  const_reverse_iterator rend() const {                                        \
    return const_reverse_iterator(                                             \
        this, BaseOffset() + NumElements() * sizeof(value_type), true);        \
  }                                                                            \
  const_reverse_iterator crbegin() const {                                     \
    return const_reverse_iterator(this, BaseOffset(), true);                   \
  }                                                                            \
  const_reverse_iterator crend() const {                                       \
    return const_reverse_iterator(                                             \
        this, BaseOffset() + NumElements() * sizeof(value_type), true);        \
  }

// vtype: value type
// rtype: relay type (like std::array<T,N>)
// relay: member to relay through
#define DECLARE_VECTOR_ARRAY_BITS(vtype, rtype, relay)                         \
  using value_type = vtype;                                                    \
  using reference = value_type &;                                              \
  using const_reference = value_type &;                                        \
  using pointer = value_type *;                                                \
  using const_pointer = const value_type *;                                    \
  using size_type = size_t;                                                    \
  using difference_type = ptrdiff_t;                                           \
                                                                               \
  using iterator = typename rtype::iterator;                                   \
  using const_iterator = typename rtype::const_iterator;                       \
  using reverse_iterator = typename rtype::reverse_iterator;                   \
  using const_reverse_iterator = typename rtype::const_reverse_iterator;       \
                                                                               \
  iterator begin() { return relay.begin(); }                                   \
  iterator end() { return relay.end(); }                                       \
  reverse_iterator rbegin() { return relay.rbegin(); }                         \
  reverse_iterator rend() { return relay.rend(); }                             \
  const_iterator begin() const { return relay.begin(); }                       \
  const_iterator end() const { return relay.end(); }                           \
  const_iterator cbegin() const { return relay.begin(); }                      \
  const_iterator cend() const { return relay.end(); }                          \
  const_reverse_iterator rbegin() const { return relay.rbegin(); }             \
  const_reverse_iterator rend() const { return relay.rend(); }                 \
  const_reverse_iterator crbegin() const { return relay.crbegin(); }           \
  const_reverse_iterator crend() const { return relay.crend(); }

// This is a variable length vector of T.  It looks like a std::vector<T>.
// The binary message contains a toolbelt::VectorHeader at the binary offset.
// This contains the number of elements and the base offset for the data.
template <typename T> class PrimitiveVectorField {
public:
  PrimitiveVectorField() = default;
  explicit PrimitiveVectorField(uint32_t source_offset,
                                uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {}

  T &operator[](int index) {
    T *base = GetBuffer()->template ToAddress<T>(BaseOffset());
    return base[index];
  }

  T operator[](int index) const {
    T *base = GetBuffer()->template ToAddress<T>(BaseOffset());
    return base[index];
  }

  T front() { return (*this)[0]; }
  const T front() const { return (*this)[0]; }
  T back() { return (*this)[size() - 1]; }
  const T back() const { return (*this)[size() - 1]; }

  std::vector<T> Get() const {
    std::vector<T> v;
    size_t n = size();
    for (size_t i = 0; i < n; i++) {
      v.push_back((*this)[i]);
    }
    return v;
  }

#define ITYPE FieldIterator<PrimitiveVectorField, value_type>
#define CTYPE FieldIterator<PrimitiveVectorField, const value_type>
  DECLARE_ZERO_COPY_VECTOR_BITS(T, ITYPE, CTYPE, T)
#undef ITYPE
#undef CTYPE

  void push_back(const T &v) {
    toolbelt::PayloadBuffer::VectorPush<T>(GetBufferAddr(), Header(), v);
  }

  void reserve(size_t n) {
    toolbelt::PayloadBuffer::VectorReserve<T>(GetBufferAddr(), Header(), n);
  }

  void resize(size_t n) {
    toolbelt::PayloadBuffer::VectorResize<T>(GetBufferAddr(), Header(), n);
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  T *data() const { return GetBuffer()->template ToAddress<T>(BaseOffset()); }

  bool empty() const { return size() == 0; }

  size_t capacity() const {
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *addr =
        GetBuffer()->template ToAddress<toolbelt::BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(value_type);
  }

  toolbelt::BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(toolbelt::VectorHeader);
  }
  toolbelt::BufferOffset BinaryOffset() const {
    return relative_binary_offset_;
  }

  bool operator==(const PrimitiveVectorField<T> &other) const {
    size_t n = size();
    for (size_t i = 0; i < n; i++) {
      if ((*this)[i] != other[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const PrimitiveVectorField<T> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const { return 4 + size() * sizeof(value_type); }

private:
  friend FieldIterator<PrimitiveVectorField, T>;
  friend FieldIterator<PrimitiveVectorField, const T>;
  toolbelt::VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<toolbelt::VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  toolbelt::BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

  toolbelt::PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  toolbelt::PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  toolbelt::BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  uint32_t source_offset_;
  toolbelt::BufferOffset relative_binary_offset_;
};

template <typename Enum> class EnumVectorField {
public:
  EnumVectorField() = default;
  explicit EnumVectorField(uint32_t source_offset,
                           uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {}

  using T = typename std::underlying_type<Enum>::type;

  Enum &operator[](int index) {
    T *base = GetBuffer()->template ToAddress<T>(BaseOffset());
    return *reinterpret_cast<Enum *>(&base[index]);
  }

  const Enum &operator[](int index) const {
    const T *base = GetBuffer()->template ToAddress<const T>(BaseOffset());
    return *reinterpret_cast<const Enum *>(&base[index]);
  }

  Enum front() { return (*this)[0]; }
  const Enum front() const { return (*this)[0]; }
  Enum back() { return (*this)[size() - 1]; }
  const Enum back() const { return (*this)[size() - 1]; }

  const std::vector<Enum> Get() const {
    size_t n = size();
    std::vector<Enum> r;
    for (size_t i = 0; i < n; i++) {
      r[i] = (*this)[i];
    }
    return r;
  }

#define ITYPE EnumFieldIterator<EnumVectorField, value_type>
#define CTYPE EnumFieldIterator<EnumVectorField, const value_type>
  DECLARE_ZERO_COPY_VECTOR_BITS(Enum, ITYPE, CTYPE, T)
#undef ITYPE
#undef CTYPE

  void push_back(const Enum &v) {
    toolbelt::PayloadBuffer::VectorPush<T>(GetBufferAddr(), Header(),
                                           static_cast<T>(v));
  }

  void reserve(size_t n) {
    toolbelt::PayloadBuffer::VectorReserve<T>(GetBufferAddr(), Header(), n);
  }

  void resize(size_t n) {
    toolbelt::PayloadBuffer::VectorResize<T>(GetBufferAddr(), Header(), n);
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  Enum *data() const { GetBuffer()->template ToAddress<Enum>(BaseOffset()); }
  bool empty() const { return size() == 0; }

  size_t capacity() const {
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *addr =
        GetBuffer()->template ToAddress<toolbelt::BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(T);
  }

  toolbelt::BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(toolbelt::VectorHeader);
  }
  toolbelt::BufferOffset BinaryOffset() const {
    return relative_binary_offset_;
  }

  bool operator==(const EnumVectorField<Enum> &other) const {
    size_t n = size();
    for (size_t i = 0; i < n; i++) {
      if ((*this)[i] != other[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const EnumVectorField<Enum> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const { return 4 + size() * sizeof(T); }

private:
  friend EnumFieldIterator<EnumVectorField, Enum>;
  friend EnumFieldIterator<EnumVectorField, const Enum>;
  toolbelt::VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<toolbelt::VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  toolbelt::BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

  toolbelt::PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  toolbelt::PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  toolbelt::BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  uint32_t source_offset_;
  toolbelt::BufferOffset relative_binary_offset_;
};

// The vector contains a set of toolbelt::BufferOffsets allocated in the buffer,
// each of which contains the absolute offset of the message.
template <typename T> class MessageVectorField {
public:
  MessageVectorField() = default;
  explicit MessageVectorField(uint32_t source_offset,
                              uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {
    // Populate the msgs vector with MessageField objects referring to the
    // binary messages.
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *data =
        GetBuffer()->template ToAddress<toolbelt::BufferOffset>(hdr->data);
    for (uint32_t i = 0; i < hdr->num_elements; i++) {
      if (data[i] == 0) {
        // If the vector says there's a message at this index but
        // the data is 0 it shows corruption in the binary message.
        // TODO: How do we deal with this?
        // abort for now
        std::cerr << "Invalid message vector entry at index " << i << std::endl;
        abort();
      }
      NonEmbeddedMessageField<T> field(GetSharedBuffer(), data[i]);
      msgs_.push_back(std::move(field));
    }
  }

  NonEmbeddedMessageField<T> &operator[](int index) { return msgs_[index]; }

  NonEmbeddedMessageField<T> &front() { return msgs_.front(); }
  const NonEmbeddedMessageField<T> &front() const { return msgs_.front(); }
  NonEmbeddedMessageField<T> &back() { return msgs_.back(); }
  const NonEmbeddedMessageField<T> &back() const { return msgs_.back(); }

#define RTYPE std::vector<NonEmbeddedMessageField<T>>
  DECLARE_VECTOR_ARRAY_BITS(NonEmbeddedMessageField<T>, RTYPE, msgs_)
#undef RTYPE

  void push_back(const T &v) {
    toolbelt::BufferOffset offset = v.absolute_binary_offset;
    toolbelt::PayloadBuffer::VectorPush<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), offset);
    NonEmbeddedMessageField<T> field(GetSharedBuffer(), offset);
    field.msg_ = v;
    msgs_.push_back(std::move(field));
  }

  size_t capacity() const {
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *addr =
        GetBuffer()->template ToAddress<toolbelt::BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(toolbelt::BufferOffset);
  }

  void reserve(size_t n) {
    toolbelt::PayloadBuffer::VectorReserve<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), n);
    msgs_.reserve(n);
  }

  void resize(size_t n) {
    toolbelt::VectorHeader *hdr = Header();
    uint32_t current_size = hdr->num_elements;

    // Resize the vector data in the binary.  This contains BufferOffets.
    toolbelt::PayloadBuffer::VectorResize<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), n);
    msgs_.resize(n);

    // If the size has increased, allocate messages for the new entries and set
    // the offsets in the MessageFields in the source message.
    if (n > current_size) {
      for (uint32_t i = current_size; i < uint32_t(n); i++) {
        void *binary = toolbelt::PayloadBuffer::Allocate(
            GetBufferAddr(), T::BinarySize(), 8, true);
        toolbelt::BufferOffset absolute_binary_offset =
            GetBuffer()->ToOffset(binary);
        auto &m = msgs_[i];
        m.msg_.buffer = GetSharedBuffer();
        m.msg_.absolute_binary_offset = absolute_binary_offset;
      }
    }
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  T *data() { GetBuffer()->template ToAddress<T>(BaseOffset()); }
  bool empty() const { return size() == 0; }

  toolbelt::BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(toolbelt::VectorHeader);
  }
  toolbelt::BufferOffset BinaryOffset() const {
    return relative_binary_offset_;
  }

  bool operator==(const MessageVectorField<T> &other) const {
    return msgs_ != other.msgs_;
  }
  bool operator!=(const MessageVectorField<T> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const {
    size_t n = 4;
    for (auto &msg : msgs_) {
      n += msg.SerializedSize();
    }
    return n;
  }

  std::vector<NonEmbeddedMessageField<T>> &Get() { return msgs_; }

  const std::vector<NonEmbeddedMessageField<T>> &Get() const { return msgs_; }

private:
  friend FieldIterator<MessageVectorField, T>;
  friend FieldIterator<MessageVectorField, const T>;
  toolbelt::VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<toolbelt::VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  toolbelt::BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

  toolbelt::PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  toolbelt::PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  toolbelt::BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  std::shared_ptr<toolbelt::PayloadBuffer *> GetSharedBuffer() {
    return Message::GetSharedBuffer(this, source_offset_);
  }

  uint32_t source_offset_;
  toolbelt::BufferOffset relative_binary_offset_;
  std::vector<NonEmbeddedMessageField<T>> msgs_;
};

// This is a little more complex.  The binary vector contains a set of
// toolbelt::BufferOffsets each of which contains the offset into the
// toolbelt::PayloadBuffer of a toolbelt::StringHeader.  Each
// toolbelt::StringHeader contains the offset of the string data which is a
// length followed by the string contents.
//
// +-----------+
// |           |
// +-----------+         +----------+
// |           +-------->|    hdr   +------->+-------------+
// +-----------+         +----------+        |   length    |
// |    ...    |                             +-------------+
// +-----------+                             |  "string"   |
// |           |                             |   "data"    |
// +-----------+                             +-------------+
class StringVectorField {
public:
  StringVectorField() = default;
  explicit StringVectorField(uint32_t source_offset,
                             uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *data =
        GetBuffer()->ToAddress<toolbelt::BufferOffset>(hdr->data);
    for (uint32_t i = 0; i < hdr->num_elements; i++) {
      if (data[i] == 0) {
        // If the vector says there's a string at this index but
        // the data is 0 it shows corruption in the binary message.
        // TODO: How do we deal with this?
        // abort for now
        std::cerr << "Invalid string vector entry at index " << i << std::endl;
        abort();
      }
      NonEmbeddedStringField field(
          Message::GetSharedBuffer(this, source_offset), data[i]);
      strings_.push_back(std::move(field));
    }
  }

  NonEmbeddedStringField &operator[](int index) { return strings_[index]; }

#define RTYPE std::vector<NonEmbeddedStringField>
  DECLARE_VECTOR_ARRAY_BITS(NonEmbeddedStringField, RTYPE, strings_)
#undef RTYPE

  size_t size() const { return strings_.size(); }
  NonEmbeddedStringField *data() { return strings_.data(); }
  bool empty() const { return size() == 0; }

  NonEmbeddedStringField &front() { return strings_.front(); }
  const NonEmbeddedStringField &front() const { return strings_.front(); }
  NonEmbeddedStringField &back() { return strings_.back(); }
  const NonEmbeddedStringField &back() const { return strings_.back(); }

  std::vector<NonEmbeddedStringField> &Get() { return strings_; }
  const std::vector<NonEmbeddedStringField> &Get() const { return strings_; }

  void push_back(const std::string &s) {
    // Allocate string header in buffer.
    void *str_hdr = toolbelt::PayloadBuffer::Allocate(
        GetBufferAddr(), sizeof(toolbelt::StringHeader), 4);
    toolbelt::BufferOffset hdr_offset = GetBuffer()->ToOffset(str_hdr);
    toolbelt::PayloadBuffer::SetString(GetBufferAddr(), s, hdr_offset);

    // Add an offset for the new string to the binary.
    toolbelt::PayloadBuffer::VectorPush<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), hdr_offset);

    // Add a source string field.
    NonEmbeddedStringField field(Message::GetSharedBuffer(this, source_offset_),
                                 hdr_offset);
    strings_.push_back(std::move(field));
  }

  size_t capacity() const {
    toolbelt::VectorHeader *hdr = Header();
    toolbelt::BufferOffset *addr =
        GetBuffer()->template ToAddress<toolbelt::BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    return toolbelt::PayloadBuffer::DecodeSize(addr) /
           sizeof(toolbelt::BufferOffset);
  }

  void reserve(size_t n) {
    toolbelt::PayloadBuffer::VectorReserve<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), n);
    strings_.reserve(n);
  }

  void resize(size_t n) {
    toolbelt::VectorHeader *hdr = Header();
    uint32_t current_size = hdr->num_elements;

    // Resize the vector data in the binary.  This contains BufferOffets.
    toolbelt::PayloadBuffer::VectorResize<toolbelt::BufferOffset>(
        GetBufferAddr(), Header(), n);
    strings_.resize(n);

    // If the size has increased, allocate string headers for the new entries
    if (n > current_size) {
      for (uint32_t i = current_size; i < uint32_t(n); i++) {
        void *str_hdr = toolbelt::PayloadBuffer::Allocate(
            GetBufferAddr(), sizeof(toolbelt::StringHeader), 4);
        toolbelt::BufferOffset hdr_offset = GetBuffer()->ToOffset(str_hdr);
        NonEmbeddedStringField field(
            Message::GetSharedBuffer(this, source_offset_), hdr_offset);
        strings_[i] = std::move(field);
      }
    }
  }

  void clear() { Header()->num_elements = 0; }

  toolbelt::BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(toolbelt::VectorHeader);
  }
  toolbelt::BufferOffset BinaryOffset() const {
    return relative_binary_offset_;
  }

  bool operator==(const StringVectorField &other) const {
    return strings_ == other.strings_;
  }
  bool operator!=(const StringVectorField &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const {
    size_t n = 4;
    for (auto &s : strings_) {
      n += s.SerializedSize();
    }
    return n;
  }

private:
  toolbelt::VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<toolbelt::VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }
  toolbelt::PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  toolbelt::PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  uint32_t source_offset_;
  toolbelt::BufferOffset relative_binary_offset_;
  std::vector<NonEmbeddedStringField> strings_;
};
#undef DECLARE_ZERO_COPY_VECTOR_BITS
#undef DECLARE_RELAY_VECTOR_BITS
}
