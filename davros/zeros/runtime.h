#pragma once

#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace davros::zeros {

// All field offsets are relative to the start of the message in the buffer, not
// the start of the buffer.  The Message contains the offset to the start
// of the message, from the buffer.

#define DEFINE_PRIMITIVE_FIELD(cname, type)                                    \
  class cname##Field {                                                         \
  public:                                                                      \
    explicit cname##Field(uint32_t boff, uint32_t offset)                      \
        : buffer_offset_(boff), field_offset_(offset) {}                       \
                                                                               \
    operator type() {                                                          \
      return GetBuffer()->Get<type>(GetMessageStart() + field_offset_);        \
    }                                                                          \
                                                                               \
    cname##Field &operator=(type i) {                                          \
      GetBuffer()->Set(GetMessageStart() + field_offset_, i);                  \
      return *this;                                                            \
    }                                                                          \
                                                                               \
  private:                                                                     \
    PayloadBuffer *GetBuffer() {                                               \
      return Message::GetBuffer(this, buffer_offset_);                         \
    }                                                                          \
    BufferOffset GetMessageStart() {                                           \
      return Message::GetMessageStart(this, buffer_offset_);                   \
    }                                                                          \
    uint32_t buffer_offset_;                                                   \
    BufferOffset field_offset_;                                                \
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

class StringField {
public:
  explicit StringField(uint32_t boff, uint32_t offset)
      : buffer_offset_(boff), field_offset_(offset) {}

  operator std::string_view() {
    return GetBuffer()->GetStringView(GetMessageStart() + field_offset_);
  }

  StringField &operator=(const std::string &s) {
    PayloadBuffer::SetString(GetBufferAddr(), s,
                             GetMessageStart() + field_offset_);
    return *this;
  }

private:
  PayloadBuffer *GetBuffer() {
    return Message::GetBuffer(this, buffer_offset_);
  }

  PayloadBuffer **GetBufferAddr() {
    return Message::GetBufferAddr(this, buffer_offset_);
  }
  BufferOffset GetMessageStart() {
    return Message::GetMessageStart(this, buffer_offset_);
  }

  uint32_t buffer_offset_;
  BufferOffset field_offset_;
};

template <typename MessageType> class MessageField {
public:
  MessageField(std::shared_ptr<PayloadBuffer*> buffer, BufferOffset msg_offset, BufferOffset field_offset)
      : msg_(buffer, Message::GetMessageStart(this, msg_offset) + field_offset) {}

  operator MessageType &() { return msg_; }

  MessageType* operator->() {
    return &msg_;
  }

private:
  PayloadBuffer *GetBuffer() { return msg_.GetBuffer(); }

  PayloadBuffer **GetBufferAddr() { return msg_.GetBufferAddr(); }

  MessageType msg_;
};

#if 0
template <typename T> class ArrayField {
public:
  explicit ArrayField(uint32_t boff, uint32_t offset)
      : buffer_offset_(boff), field_offset_(offset) {}

  PayloadBuffer *GetBuffer() {
    return *reinterpret_cast<PayloadBuffer **>(reinterpret_cast<char *>(this) -
                                               buffer_offset_);
  }
  PayloadBuffer **GetBufferAddr() {
    return reinterpret_cast<PayloadBuffer **>(reinterpret_cast<char *>(this) -
                                              buffer_offset_);
  }

  BufferOffset *GetOffset() {
    return reinterpret_cast<BufferOffset *>(reinterpret_cast<char *>(this) -
                                            buffer_offset_ + field_offset_);
  }

  uint32_t buffer_offset_;
  BufferOffset field_offset_;
};
#endif
}
