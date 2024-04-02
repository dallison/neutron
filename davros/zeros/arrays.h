#pragma once

// Array fields.

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "davros/common_runtime.h"
#include "davros/zeros/fields.h"
#include "davros/zeros/iterators.h"
#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace davros::zeros {

// vtype: value type
// itype: iterator type
// ctype: const iterator type
// utype: underlying type
#define DECLARE_ZERO_COPY_ARRAY_BITS(vtype, itype, ctype, utype)               \
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
  iterator end() { return iterator(this, BaseOffset() + N * sizeof(utype)); }  \
  reverse_iterator rbegin() {                                                  \
    return reverse_iterator(this, BaseOffset(), true);                         \
  }                                                                            \
  reverse_iterator rend() {                                                    \
    return reverse_iterator(this, BaseOffset() + N * sizeof(utype), true);     \
  }                                                                            \
  const_iterator begin() const { return const_iterator(this, BaseOffset()); }  \
  const_iterator rbegin() const {                                              \
    return const_reverse_iterator(this, BaseOffset(), true);                   \
  }                                                                            \
  const_iterator cbegin() const { return const_iterator(this, BaseOffset()); } \
  const_reverse_iterator crbegin() const {                                     \
    return const_reverse_iterator(this, BaseOffset(), true);                   \
  }                                                                            \
  const_iterator end() const {                                                 \
    return const_iterator(this, BaseOffset() + N * sizeof(utype));             \
  }                                                                            \
  const_iterator cend() const {                                                \
    return const_iterator(this, BaseOffset() + N * sizeof(utype));             \
  }                                                                            \
  const_iterator rend() const {                                                \
    return const_reverse_iterator(this, BaseOffset() + N * sizeof(utype),      \
                                  true);                                       \
  }                                                                            \
  const_iterator crend() const {                                               \
    return const_reverse_iterator(this, BaseOffset() + N * sizeof(utype),      \
                                  true);                                       \
  }

// vtype: value type
// rtype: relay type (like std::array<T,N>)
// relay: member to relay through
#define DECLARE_RELAY_ARRAY_BITS(vtype, rtype, relay)                          \
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
  const_reverse_iterator rbegin() const { return relay.rbegin(); }             \
  const_reverse_iterator rend() const { return relay.rend(); }                 \
  const_iterator cbegin() const { return relay.cbegin(); }                     \
  const_iterator cend() const { return relay.cend(); }                         \
  const_reverse_iterator crbegin() const { return relay.crbegin(); }           \
  const_reverse_iterator crend() const { return relay.crend(); }

// This is a fixed length array of T.  It looks like a std::array<T,N>.
template <typename T, int N> class PrimitiveArrayField {
public:
  PrimitiveArrayField() = default;
  explicit PrimitiveArrayField(uint32_t source_offset,
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

  PrimitiveArrayField &operator=(const PrimitiveArrayField &other) {
    for (size_t i = 0; i < N; i++) {
      (*this)[i] = other[i];
    }
    return *this;
  }

  T front() { return (*this)[0]; }
  const T front() const { return (*this)[0]; }
  T back() { return (*this)[N - 1]; }
  const T back() const { return (*this)[N - 1]; }

  std::array<T, N> Get() const {
    std::array<T, N> v;
    for (size_t i = 0; i < N; i++) {
      v[i] = (*this)[i];
    }
    return v;
  }
#define ITYPE FieldIterator<PrimitiveArrayField, value_type>
#define CTYPE FieldIterator<PrimitiveArrayField, const value_type>
  DECLARE_ZERO_COPY_ARRAY_BITS(T, ITYPE, CTYPE, T)
#undef ITYPE
#undef CTYPE

  size_t size() const { return N; }
  T *data() const { return GetBuffer()->template ToAddress<T>(BaseOffset()); }
  bool empty() const { return N == 0; }
  size_t max_size() const { return N; }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(T) * N;
  }

  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const PrimitiveArrayField<T, N> &other) const {
    for (size_t i = 0; i < N; i++) {
      if ((*this)[i] != other[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const PrimitiveArrayField<T, N> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const { return sizeof(T) * N; }

private:
  friend FieldIterator<PrimitiveArrayField, T>;
  friend FieldIterator<PrimitiveArrayField, const T>;
  BufferOffset BaseOffset() const {
    return Message::GetMessageBinaryStart(this, source_offset_) +
           relative_binary_offset_;
  }
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
};

// This is a fixed length array of T.  It looks like a std::array<T,N>.
template <typename Enum, int N> class EnumArrayField {
public:
  using T = typename std::underlying_type<Enum>::type;
  EnumArrayField() = default;
  explicit EnumArrayField(uint32_t source_offset,
                          uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {}

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
  Enum back() { return (*this)[N - 1]; }
  const Enum back() const { return (*this)[N - 1]; }

  const std::array<Enum, N> Get() const {
    std::array<Enum, N> r;
    for (size_t i = 0; i < N; i++) {
      r[i] = (*this)[i];
    }
    return r;
  }

#define ITYPE EnumFieldIterator<EnumArrayField, value_type>
#define CTYPE EnumFieldIterator<EnumArrayField, const value_type>
  DECLARE_ZERO_COPY_ARRAY_BITS(Enum, ITYPE, CTYPE, T)
#undef ITYPE
#undef CTYPE

  size_t size() const { return N; }
  Enum *data() const { GetBuffer()->template ToAddress<Enum>(BaseOffset()); }
  bool empty() const { return N == 0; }
  size_t max_size() const { return N; }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(T) * N;
  }

  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const EnumArrayField<Enum, N> &other) const {
    for (size_t i = 0; i < N; i++) {
      if ((*this)[i] != other[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const EnumArrayField<Enum, N> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const { return sizeof(Enum) * N; }

private:
  friend EnumFieldIterator<EnumArrayField, Enum>;
  friend EnumFieldIterator<EnumArrayField, const Enum>;
  BufferOffset BaseOffset() const {
    return Message::GetMessageBinaryStart(this, source_offset_) +
           relative_binary_offset_;
  }
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
};

// This is a fixed array of messages.  T must be derived from davros::Message.
template <typename T, int N> class MessageArrayField {
public:
  MessageArrayField() = default;
  explicit MessageArrayField(uint32_t source_offset,
                             uint32_t relative_binary_offset)
      : relative_binary_offset_(relative_binary_offset) {
    std::shared_ptr<PayloadBuffer *> buffer =
        Message::GetSharedBuffer(this, source_offset);
    // Construct the embedded messages.
    size_t index = 0;
    for (auto &msg : msgs_) {
      msg.buffer = buffer;
      BufferOffset offset =
          Message::GetMessageBinaryStart(this, source_offset) +
          relative_binary_offset + T::BinarySize() * index;
      msg.absolute_binary_offset = offset;
      index++;
    }
  }

  T &operator[](int index) { return msgs_[index]; }

  operator std::array<T, N> &() { return msgs_; }
  std::array<T, N> &Get() { return msgs_; }
  const std::array<T, N> &Get() const { return msgs_; }

#define RTYPE std::array<T, N>
  DECLARE_RELAY_ARRAY_BITS(T, RTYPE, msgs_)
#undef RTYPE

  size_t size() const { return N; }
  T *data() const { msgs_.data(); }
  bool empty() const { return N == 0; }
  size_t max_size() const { return N; }

  T& front() { return msgs_.front(); }
  const T& front() const { return msgs_.front(); }
  T& back() { return msgs_.back(); }
  const T& back() const { return msgs_.back(); }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + T::BinarySize() * N;
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const MessageArrayField<T, N> &other) const {
    return msgs_ != other.msgs_;
  }
  bool operator!=(const MessageArrayField<T, N> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const {
    size_t n = 0;
    for (auto &msg : msgs_) {
      n += msg.SerializedSize();
    }
    return n;
  }

private:
  BufferOffset relative_binary_offset_;
  std::array<T, N> msgs_;
};

// This is a fixed length array of StringField.
// It looks like a std::array<StringField,N>.
template <int N> class StringArrayField {
public:
  StringArrayField() = default;
  explicit StringArrayField(uint32_t source_offset,
                            uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {
    for (size_t i = 0; i < N; i++) {
      strings_[i].source_offset_ = source_offset + sizeof(uint32_t) +
                                   sizeof(BufferOffset) +
                                   sizeof(StringField) * i;
      // Binary offset in the StringField is relative to the start of the
      // message, not the buffer start.
      strings_[i].relative_binary_offset_ = relative_binary_offset +
                                            sizeof(BufferOffset) * i;
    }
  }

  StringField &operator[](int index) { return strings_[index]; }

  StringField& front() { return strings_.front(); }
  const StringField& front() const { return strings_.front(); }
  StringField& back() { return strings_.back(); }
  const StringField& back() const { return strings_.back(); }

  std::array<StringField, N> &Get() { return strings_; }
  const std::array<StringField, N> &Get() const { return strings_; }

#define RTYPE std::array<StringField, N>
  DECLARE_RELAY_ARRAY_BITS(StringField, RTYPE, strings_)
#undef RTYPE

  size_t size() const { return N; }
  StringField *data() const { return strings_.data(); }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(BufferOffset) * N;
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const StringArrayField<N> &other) const {
    return strings_ != other.strings_;
  }
  bool operator!=(const StringArrayField<N> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const {
    size_t n = 0;
    for (auto &s : strings_) {
      n += s.SerializedSize();
    }
    return n;
  }

private:
  BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
  std::array<StringField, N> strings_;
};

#undef DECLARE_ZERO_COPY_ARRAY_BITS
#undef DECLARE_RELAY_ARRAY_BITS
}
