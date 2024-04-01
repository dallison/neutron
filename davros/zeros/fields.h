#pragma once

// Single value fields.

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

  StringField &operator=(const char *s) {
    PayloadBuffer::SetString(GetBufferAddr(), s, strlen(s),
                             GetMessageBinaryStart() + relative_binary_offset_);
    return *this;
  }

  StringField &operator=(std::string_view s) {
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
  bool empty() const { return size() == 0; }

  size_t SerializedSize() const { return 4 + size(); }

private:
  template <int N> friend class StringArrayField;

  PayloadBuffer *GetBuffer() const { return *buffer_; }

  PayloadBuffer **GetBufferAddr() const { return buffer_.get(); }

  std::shared_ptr<PayloadBuffer *> buffer_;
  BufferOffset
      relative_binary_offset_; // Offset into PayloadBuffer of StringHeader
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
} // namespace davros::zeros
