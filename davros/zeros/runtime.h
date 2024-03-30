#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "davros/common_runtime.h"
#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

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

#define DEFINE_PRIMITIVE_FIELD(cname, type)                                    \
  class cname##Field {                                                         \
  public:                                                                      \
    cname##Field() = default;                                                  \
    explicit cname##Field(uint32_t boff, uint32_t offset)                      \
        : source_offset_(boff), relative_binary_offset_(offset) {}             \
                                                                               \
    operator type() const {                                                    \
      return GetBuffer()->Get<type>(GetMessageBinaryStart() +                  \
                                    relative_binary_offset_);                  \
    }                                                                          \
                                                                               \
    cname##Field &operator=(type v) {                                          \
      GetBuffer()->Set(GetMessageBinaryStart() + relative_binary_offset_, v);  \
      return *this;                                                            \
    }                                                                          \
                                                                               \
    type Get() const {                                                         \
      return GetBuffer()->Get<type>(GetMessageBinaryStart() +                  \
                                    relative_binary_offset_);                  \
    }                                                                          \
                                                                               \
    void Set(type v) {                                                         \
      GetBuffer()->Set(GetMessageBinaryStart() + relative_binary_offset_, v);  \
    }                                                                          \
    BufferOffset BinaryEndOffset() const {                                     \
      return relative_binary_offset_ + sizeof(type);                           \
    }                                                                          \
    BufferOffset BinaryOffset() const { return relative_binary_offset_; }      \
    bool operator==(const cname##Field &other) const {                         \
      return Get() == other.Get();                                             \
    }                                                                          \
    bool operator!=(const cname##Field &other) const {                         \
      return !(*this == other);                                                \
    }                                                                          \
    size_t SerializedSize() const { return sizeof(type); }                     \
                                                                               \
  private:                                                                     \
    PayloadBuffer *GetBuffer() const {                                         \
      return Message::GetBuffer(this, source_offset_);                         \
    }                                                                          \
    BufferOffset GetMessageBinaryStart() const {                               \
      return Message::GetMessageBinaryStart(this, source_offset_);             \
    }                                                                          \
    uint32_t source_offset_;                                                   \
    BufferOffset relative_binary_offset_;                                      \
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

#undef DEFINE_PRIMITIVE_FIELD

class StringField {
public:
  StringField() = default;
  explicit StringField(uint32_t source_offset, uint32_t relative_binary_offset)
      : source_offset_(source_offset),
        relative_binary_offset_(relative_binary_offset) {}

  operator std::string_view() const {
    return GetBuffer()->GetStringView(GetMessageBinaryStart() +
                                      relative_binary_offset_);
  }

  StringField &operator=(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s,
                             GetMessageBinaryStart() + relative_binary_offset_);
    return *this;
  }

  std::string_view Get() const {
    return GetBuffer()->GetStringView(GetMessageBinaryStart() +
                                      relative_binary_offset_);
  }

  void Set(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s,
                             GetMessageBinaryStart() + relative_binary_offset_);
  }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(BufferOffset);
  }

  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const StringField &other) const {
    return Get() == other.Get();
  }
  bool operator!=(const StringField &other) const { return !(*this == other); }

  size_t size() const {
    return GetBuffer()->StringSize(GetMessageBinaryStart() +
                                   relative_binary_offset_);
  }

  const char *data() const {
    return GetBuffer()->StringData(GetMessageBinaryStart() +
                                   relative_binary_offset_);
  }

  size_t SerializedSize() const { return 4 + size(); }

private:
  template <int N> friend class StringArrayField;

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

template <typename Enum> class EnumField {
public:
  using T = typename std::underlying_type<Enum>::type;
  EnumField() = default;
  explicit EnumField(uint32_t boff, uint32_t offset)
      : source_offset_(boff), relative_binary_offset_(offset) {}

  operator Enum() const {
    return static_cast<Enum>(
        GetBuffer()->template Get<typename std::underlying_type<Enum>::type>(
            GetMessageBinaryStart() + relative_binary_offset_));
  }

  operator T() const {
    return GetBuffer()->template Get<typename std::underlying_type<Enum>::type>(
        GetMessageBinaryStart() + relative_binary_offset_);
  }

  EnumField &operator=(Enum e) {
    GetBuffer()->Set(GetMessageBinaryStart() + relative_binary_offset_,
                     static_cast<typename std::underlying_type<Enum>::type>(e));
    return *this;
  }

  Enum Get() const {
    return static_cast<Enum>(
        GetBuffer()->template Get<typename std::underlying_type<Enum>::type>(
            GetMessageBinaryStart() + relative_binary_offset_));
  }

  void Set(Enum e) {
    GetBuffer()->Set(GetMessageBinaryStart() + relative_binary_offset_,
                     static_cast<typename std::underlying_type<Enum>::type>(e));
  }

  void Set(T e) {
    GetBuffer()->Set(GetMessageBinaryStart() + relative_binary_offset_, e);
  }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ +
           sizeof(typename std::underlying_type<Enum>::type);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  bool operator==(const EnumField &other) const {
    return static_cast<Enum>(*this) == static_cast<Enum>(other);
  }
  bool operator!=(const EnumField &other) const { return !(*this == other); }

  size_t SerializedSize() const { return sizeof(T); }

private:
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }
  BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }
  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
};

// A message field enapsulates a message that is held inline in the
// parent message, both in the source message and the
// binary message.
//
// The embedded message has its absolute_binary_offset set to the absolute offet
// into the payload buffer.
template <typename MessageType> class MessageField {
public:
  MessageField() = default;
  MessageField(std::shared_ptr<PayloadBuffer *> buffer,
               BufferOffset source_offset, BufferOffset relative_binary_offset)
      : relative_binary_offset_(relative_binary_offset),
        msg_(buffer, Message::GetMessageBinaryStart(this, source_offset) +
                         relative_binary_offset) {}

  operator MessageType &() { return msg_; }
  MessageType &operator*() { return msg_; }
  MessageType *operator->() { return &msg_; }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + MessageType::BinarySize();
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  absl::Status SerializeToBuffer(Buffer &buffer) const {
    return msg_.SerializeToBuffer(buffer);
  }

  absl::Status DeserializeFromBuffer(Buffer &buffer) {
    return msg_.DeserializeFromBuffer(buffer);
  }

  size_t SerializedSize() const { return msg_.SerializedSize(); }

  bool operator==(const MessageField<MessageType> &m) const {
    return msg_ == m.msg_;
  }

  bool operator!=(const MessageField<MessageType> &m) const {
    return msg_ != m.msg_;
  }

private:
  template <typename T> friend class MessageVectorField;
  BufferOffset relative_binary_offset_;
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

template <typename Field, typename T> struct EnumFieldIterator {
  EnumFieldIterator(Field *f, BufferOffset o) : field(f), offset(o) {}

  EnumFieldIterator &operator++() {
    offset += sizeof(T);
    return *this;
  }
  EnumFieldIterator &operator--() {
    offset -= sizeof(T);
    return *this;
  }
  EnumFieldIterator operator+(size_t i) {
    return iterator(field, field->BaseOffset() +
                               i * sizeof(std::underlying_type<T>::type));
  }
  EnumFieldIterator operator-(size_t i) {
    return iterator(field, field->BaseOffset() -
                               i * sizeof(std::underlying_type<T>::type));
  }
  T &operator*() const {
    using U = typename std::underlying_type<T>::type;
    U *addr = field->GetBuffer()->template ToAddress<U>(offset);
    return *reinterpret_cast<T *>(addr);
  }

  bool operator==(const EnumFieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const EnumFieldIterator &it) const { return !operator==(it); }

  Field *field;
  BufferOffset offset;
};

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

  std::array<T, N> Get() const {
    std::array<T, N> v;
    for (size_t i = 0; i < N; i++) {
      v[i] = (*this)[i];
    }
    return v;
  }
  using iterator = FieldIterator<PrimitiveArrayField, T>;
  using const_iterator = FieldIterator<PrimitiveArrayField, const T>;

  iterator begin() { return iterator(this, BaseOffset()); }
  const_iterator begin() const { return const_iterator(this, BaseOffset()); }
  iterator end() { return iterator(this, BaseOffset() + N * sizeof(T)); }
  const_iterator end() const {
    return const_iterator(this, BaseOffset() + N * sizeof(T));
  }

  size_t size() const { return N; }
  T *data() const { return GetBuffer()->template ToAddress<T>(BaseOffset()); }

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

  using iterator = EnumFieldIterator<EnumArrayField, Enum>;
  using const_iterator = EnumFieldIterator<EnumArrayField, const Enum>;

  iterator begin() { return iterator(this, BaseOffset()); }
  const_iterator begin() const { return const_iterator(this, BaseOffset()); }
  iterator end() { return iterator(this, BaseOffset() + N * sizeof(T)); }
  const_iterator end() const {
    return const_iterator(this, BaseOffset() + N * sizeof(T));
  }

  size_t size() const { return N; }
  Enum *data() const { GetBuffer()->template ToAddress<Enum>(BaseOffset()); }

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

  using iterator = typename std::array<T, N>::iterator;
  using const_iterator = typename std::array<T, N>::const_iterator;

  iterator begin() { return msgs_.begin(); }
  iterator end() { return msgs_.end(); }
  const_iterator begin() const { return msgs_.begin(); }
  const_iterator end() const { return msgs_.end(); }

  size_t size() const { return N; }
  T *data() const { msgs_.data(); }

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
                                            sizeof(BufferOffset) * i -
                                            GetMessageBinaryStart();
    }
  }

  StringField &operator[](int index) { return strings_[index]; }

  using iterator = typename std::array<StringField, N>::iterator;
  using const_iterator = typename std::array<StringField, N>::const_iterator;

  iterator begin() { return strings_.begin(); }
  iterator end() { return strings_.end(); }
  const_iterator begin() const { return strings_.begin(); }
  const_iterator end() const { return strings_.end(); }

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

// This is a variable length vector of T.  It looks like a std::vector<T>.
// The binary message contains a VectorHeader at the binary offset.  This
// contains the number of elements and the base offset for the data.
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

  std::vector<T> Get() const {
    std::vector<T> v;
    size_t n = size();
    for (size_t i = 0; i < n; i++) {
      v.push_back((*this)[i]);
    }
    return v;
  }
  using iterator = FieldIterator<PrimitiveVectorField, T>;
  using const_iterator = FieldIterator<PrimitiveVectorField, const T>;

  iterator begin() { return iterator(this, BaseOffset()); }
  iterator end() {
    return iterator(this, BaseOffset() + NumElements() * sizeof(T));
  }
  const_iterator begin() const { return const_iterator(this, BaseOffset()); }
  const_iterator end() const {
    return const_iterator(this, BaseOffset() + NumElements() * sizeof(T));
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
  T *data() const { return GetBuffer()->template ToAddress<T>(BaseOffset()); }

  size_t capacity() const {
    VectorHeader *hdr = Header();
    BufferOffset *addr =
        GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(T);
  }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(VectorHeader);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

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

  size_t SerializedSize() const { return 4 + size() * sizeof(T); }

private:
  friend FieldIterator<PrimitiveVectorField, T>;
  VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

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

  using iterator = EnumFieldIterator<EnumVectorField, Enum>;
  using const_iterator = EnumFieldIterator<EnumVectorField, const Enum>;

  iterator begin() { return iterator(this, BaseOffset()); }
  iterator end() {
    return iterator(this, BaseOffset() + NumElements() * sizeof(T));
  }

  const_iterator begin() const { return const_iterator(this, BaseOffset()); }
  const_iterator end() const {
    return const_iterator(this, BaseOffset() + NumElements() * sizeof(T));
  }

  void push_back(const Enum &v) {
    PayloadBuffer::VectorPush<T>(GetBufferAddr(), Header(), static_cast<T>(v));
  }

  void reserve(size_t n) {
    PayloadBuffer::VectorReserve<T>(GetBufferAddr(), Header(), n);
  }

  void resize(size_t n) {
    PayloadBuffer::VectorResize<T>(GetBufferAddr(), Header(), n);
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  Enum *data() const { GetBuffer()->template ToAddress<Enum>(BaseOffset()); }

  size_t capacity() const {
    VectorHeader *hdr = Header();
    BufferOffset *addr =
        GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(T);
  }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(VectorHeader);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

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
  VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

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

// A message field enapsulates a message that is inline in the
// source message but at a fixed location in the binary.
template <typename MessageType> class NonEmbeddedMessageField {
public:
  NonEmbeddedMessageField() = default;
  NonEmbeddedMessageField(std::shared_ptr<PayloadBuffer *> buffer,
                          BufferOffset absolute_binary_offset)
      : msg_(buffer, absolute_binary_offset) {}

  operator MessageType &() { return msg_; }
  MessageType &operator*() { return msg_; }
  MessageType *operator->() { return &msg_; }

  MessageType &Get() { return msg_; }

  BufferOffset BinaryEndOffset() const {
    return msg_.absolute_binary_offset + MessageType::BinarySize();
  }
  BufferOffset BinaryOffset() const { return msg_.absolute_binary_offset; }

  bool operator==(const NonEmbeddedMessageField<MessageType> &other) const {
    return msg_ != other.msg_;
  }
  bool operator!=(const NonEmbeddedMessageField<MessageType> &other) const {
    return !(*this == other);
  }

  size_t SerializedSize() const { return msg_.SerializedSize(); }

private:
  template <typename T> friend class MessageVectorField;
  MessageType msg_;
};

// The vector contains a set of BufferOffsets allocated in the buffer,
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
    VectorHeader *hdr = Header();
    BufferOffset *data =
        GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
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

  using iterator = typename std::vector<NonEmbeddedMessageField<T>>::iterator;
  using const_iterator =
      typename std::vector<NonEmbeddedMessageField<T>>::const_iterator;

  iterator begin() { return msgs_.begin(); }
  iterator end() { return msgs_.end(); }
  const_iterator begin() const { return msgs_.begin(); }
  const_iterator end() const { return msgs_.end(); }

  void push_back(const T &v) {
    BufferOffset offset = v.absolute_binary_offset;
    PayloadBuffer::VectorPush<BufferOffset>(GetBufferAddr(), Header(), offset);
    NonEmbeddedMessageField<T> field(GetSharedBuffer(), offset);
    field.msg_ = v;
    msgs_.push_back(std::move(field));
  }

  size_t capacity() const {
    VectorHeader *hdr = Header();
    BufferOffset *addr =
        GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
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
        BufferOffset absolute_binary_offset = GetBuffer()->ToOffset(binary);
        auto &m = msgs_[i];
        m.msg_.buffer = GetSharedBuffer();
        m.msg_.absolute_binary_offset = absolute_binary_offset;
      }
    }
  }

  void clear() { Header()->num_elements = 0; }

  size_t size() const { return Header()->num_elements; }
  T *data() { GetBuffer()->template ToAddress<T>(BaseOffset()); }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(VectorHeader);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

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

private:
  friend FieldIterator<MessageVectorField, T>;
  VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }

  BufferOffset BaseOffset() const { return Header()->data; }

  size_t NumElements() const { return Header()->num_elements; }

  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  BufferOffset GetMessageBinaryStart() const {
    return Message::GetMessageBinaryStart(this, source_offset_);
  }

  std::shared_ptr<PayloadBuffer *> GetSharedBuffer() {
    return Message::GetSharedBuffer(this, source_offset_);
  }

  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
  std::vector<NonEmbeddedMessageField<T>> msgs_;
};

// This is a string field that is not embedded inside a message.  These will be
// allocated from the heap, as is the case when used in a std::vector.  They
// store the std::shared_ptr to the PayloadBuffer pointer instead of an offset
// to the start of the message.
class NonEmbeddedStringField {
public:
  NonEmbeddedStringField() = default;
  explicit NonEmbeddedStringField(std::shared_ptr<PayloadBuffer *> buffer,
                                  uint32_t relative_binary_offset)
      : buffer_(buffer), relative_binary_offset_(relative_binary_offset) {}

  operator std::string_view() const {
    return GetBuffer()->GetStringView(relative_binary_offset_);
  }

  NonEmbeddedStringField &operator=(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s, relative_binary_offset_);
    return *this;
  }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(BufferOffset);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

  std::string_view Get() const {
    return GetBuffer()->GetStringView(relative_binary_offset_);
  }

  void Set(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s, relative_binary_offset_);
  }

  bool operator==(const NonEmbeddedStringField &other) const {
    return Get() == other.Get();
  }
  bool operator!=(const NonEmbeddedStringField &other) const {
    return !(*this == other);
  }

  size_t size() const {
    return GetBuffer()->StringSize(relative_binary_offset_);
  }

  const char *data() const {
    return GetBuffer()->StringData(relative_binary_offset_);
  }

  size_t SerializedSize() const { return 4 + size(); }

private:
  template <int N> friend class StringArrayField;

  PayloadBuffer *GetBuffer() const { return *buffer_; }

  PayloadBuffer **GetBufferAddr() const { return buffer_.get(); }

  std::shared_ptr<PayloadBuffer *> buffer_;
  BufferOffset
      relative_binary_offset_; // Offset into PayloadBuffer of StringHeader
};

// This is a little more complex.  The binary vector contains a set of
// BufferOffsets each of which contains the offset into the PayloadBuffer of a
// StringHeader.  Each StringHeader contains the offset of the string data which
// is a length followed by the string contents.
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
    VectorHeader *hdr = Header();
    BufferOffset *data = GetBuffer()->ToAddress<BufferOffset>(hdr->data);
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

  using iterator = typename std::vector<NonEmbeddedStringField>::iterator;
  using const_iterator =
      typename std::vector<NonEmbeddedStringField>::const_iterator;

  iterator begin() { return strings_.begin(); }
  iterator end() { return strings_.end(); }

  const_iterator begin() const { return strings_.begin(); }
  const_iterator end() const { return strings_.end(); }

  size_t size() const { return strings_.size(); }
  NonEmbeddedStringField *data() { return strings_.data(); }

  void push_back(const std::string &s) {
    // Allocate string header in buffer.
    void *str_hdr =
        PayloadBuffer::Allocate(GetBufferAddr(), sizeof(StringHeader), 4);
    BufferOffset hdr_offset = GetBuffer()->ToOffset(str_hdr);
    PayloadBuffer::SetString(GetBufferAddr(), s, hdr_offset);

    // Add an offset for the new string to the binary.
    PayloadBuffer::VectorPush<BufferOffset>(GetBufferAddr(), Header(),
                                            hdr_offset);

    // Add a source string field.
    NonEmbeddedStringField field(Message::GetSharedBuffer(this, source_offset_),
                                 hdr_offset);
    strings_.push_back(std::move(field));
  }

  size_t capacity() const {
    VectorHeader *hdr = Header();
    BufferOffset *addr =
        GetBuffer()->template ToAddress<BufferOffset>(hdr->data);
    if (addr == nullptr) {
      return 0;
    }
    // Word before memory is size of memory in bytes.
    return addr[-1] / sizeof(BufferOffset);
  }

  void reserve(size_t n) {
    PayloadBuffer::VectorReserve<BufferOffset>(GetBufferAddr(), Header(), n);
    strings_.reserve(n);
  }

  void resize(size_t n) {
    VectorHeader *hdr = Header();
    uint32_t current_size = hdr->num_elements;

    // Resize the vector data in the binary.  This contains BufferOffets.
    PayloadBuffer::VectorResize<BufferOffset>(GetBufferAddr(), Header(), n);
    strings_.resize(n);

    // If the size has increased, allocate string headers for the new entries
    if (n > current_size) {
      for (uint32_t i = current_size; i < uint32_t(n); i++) {
        void *str_hdr =
            PayloadBuffer::Allocate(GetBufferAddr(), sizeof(StringHeader), 4);
        BufferOffset hdr_offset = GetBuffer()->ToOffset(str_hdr);
        NonEmbeddedStringField field(
            Message::GetSharedBuffer(this, source_offset_), hdr_offset);
        strings_[i] = std::move(field);
      }
    }
  }

  void clear() { Header()->num_elements = 0; }

  BufferOffset BinaryEndOffset() const {
    return relative_binary_offset_ + sizeof(VectorHeader);
  }
  BufferOffset BinaryOffset() const { return relative_binary_offset_; }

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
  VectorHeader *Header() const {
    return GetBuffer()->template ToAddress<VectorHeader>(
        Message::GetMessageBinaryStart(this, source_offset_) +
        relative_binary_offset_);
  }
  PayloadBuffer *GetBuffer() const {
    return Message::GetBuffer(this, source_offset_);
  }

  PayloadBuffer **GetBufferAddr() const {
    return Message::GetBufferAddr(this, source_offset_);
  }
  uint32_t source_offset_;
  BufferOffset relative_binary_offset_;
  std::vector<NonEmbeddedStringField> strings_;
};
} // namespace davros::zeros
