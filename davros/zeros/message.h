#pragma once

#include "davros/zeros/payload_buffer.h"
#include <memory>
#include <stdint.h>

namespace davros::zeros {

// Payload buffers can move. All messages in a message tree must all use the
// same payload buffer. We hold a shared pointer to a pointer to the payload
// buffer.
//
//            +-------+
//            |       |
//            V       |
// +---------------+  |
// |               |  |
// | PayloadBuffer |  |
// |               |  |
// +---------------+  |
//                    |
//                    |
// +---------------+  |
// |     *         +--+
// +---------------+
//       ^ ^
//       | |
//       | +--------------------------+
//       +------------+   +--------+  |
//                    |   |        V  |
// +---------------+  |   |      +---+--------+
// |    buffer     +--+   |      |   buffer    |
// +---------------+      |      +-------------+
// |               |      |      |             |
// |   Message     |      |      |  Message    |
// |               |      |      |  Field      |
// |               +------+      |             |
// +---------------+             +-------------+

struct Message {
  Message() = default;
  Message(std::shared_ptr<PayloadBuffer *> pb, BufferOffset start)
      : buffer(pb), start_offset(start) {}
  std::shared_ptr<PayloadBuffer *> buffer;
  BufferOffset start_offset;

  // 'field' is the offset from the start of the message to the field (positive)
  // Subtract the field offset from the field to get the address of the
  // std::shared_ptr to the pointer to the PayloadBuffer.
  static PayloadBuffer *GetBuffer(const void *field, uint32_t offset) {
    const std::shared_ptr<PayloadBuffer *> *pb =
        reinterpret_cast<const std::shared_ptr<PayloadBuffer *> *>(
            reinterpret_cast<const char *>(field) - offset);
    return *pb->get();
  }

  static PayloadBuffer **GetBufferAddr(const void *field, uint32_t offset) {
    const std::shared_ptr<PayloadBuffer *> *pb =
        reinterpret_cast<const std::shared_ptr<PayloadBuffer *> *>(
            reinterpret_cast<const char *>(field) - offset);
    return pb->get();
  }

 static std::shared_ptr<PayloadBuffer *> GetSharedBuffer(void *field, uint32_t offset) {
    std::shared_ptr<PayloadBuffer *> *pb =
        reinterpret_cast<std::shared_ptr<PayloadBuffer *> *>(
            reinterpret_cast<char *>(field) - offset);
    return *pb;
  }
  
  static BufferOffset GetMessageStart(const void *field, uint32_t offset) {
    const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return msg->start_offset;
  }
};

} // namespace davros::zeros
