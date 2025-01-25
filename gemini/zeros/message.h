#pragma once

#include "absl/status/statusor.h"
#include "toolbelt/payload_buffer.h"
#include <memory>
#include <stdint.h>

namespace gemini::zeros {

// Payload buffers can move. All messages in a message tree must all use the
// same payload buffer. We hold a shared pointer to a pointer to the payload
// buffer.
//
//            +-------+
//            |       |
//            V       |
// +---------------+  |
// |               |  |
// | toolbelt::PayloadBuffer |  |
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
  Message(std::shared_ptr<toolbelt::PayloadBuffer *> pb,
          toolbelt::BufferOffset start)
      : buffer(pb), absolute_binary_offset(start) {}
  std::shared_ptr<toolbelt::PayloadBuffer *> buffer;
  toolbelt::BufferOffset absolute_binary_offset;

  // 'field' is the offset from the start of the message to the field (positive)
  // Subtract the field offset from the field to get the address of the
  // std::shared_ptr to the pointer to the toolbelt::PayloadBuffer.
  static toolbelt::PayloadBuffer *GetBuffer(const void *field,
                                            uint32_t offset) {
    const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return *msg->buffer;
  }

  static toolbelt::PayloadBuffer **GetBufferAddr(const void *field,
                                                 uint32_t offset) {
                                                      const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return msg->buffer.get();
  }

  static std::shared_ptr<toolbelt::PayloadBuffer *>
  GetSharedBuffer(void *field, uint32_t offset) {
        const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return msg->buffer;
  }

  static toolbelt::BufferOffset GetMessageBinaryStart(const void *field,
                                                      uint32_t offset) {
    const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return msg->absolute_binary_offset;
  }

  size_t Size() const { return (*buffer)->Size();}
  uint64_t ByteSizeLong() const { return (*buffer)->Size();}
  uint32_t ByteSize() const { return uint32_t((*buffer)->Size());}
  void* Data() const { return reinterpret_cast<void*>(buffer.get()); }
};

inline absl::StatusOr<::toolbelt::PayloadBuffer *> NewDynamicBuffer(
    size_t initial_size, std::function<absl::StatusOr<void *>(size_t)> alloc,
    std::function<absl::StatusOr<void *>(void *, size_t, size_t)> realloc) {
  absl::StatusOr<void *> buffer = alloc(initial_size);
  if (!buffer.ok()) {
    return buffer.status();
  }
  ::toolbelt::PayloadBuffer *pb =
      new (*buffer)::toolbelt::PayloadBuffer(initial_size, [
        initial_size, realloc = std::move(realloc)
      ](::toolbelt::PayloadBuffer * *p, size_t old_size, size_t new_size) {
        absl::StatusOr<void *> r = realloc(*p, old_size, new_size);
        if (!r.ok()) {
          std::cerr << "Failed to resize PayloadBuffer from " << initial_size
                    << " to " << new_size << std::endl;
          abort();
        }
        *p = reinterpret_cast<::toolbelt::PayloadBuffer *>(*r);
      });
  return pb;
}

inline ::toolbelt::PayloadBuffer *NewDynamicBuffer(size_t initial_size) {
  absl::StatusOr<::toolbelt::PayloadBuffer *> r = NewDynamicBuffer(
      initial_size, [](size_t size) -> void * { return ::malloc(size); },
      [](void *p, size_t old_size, size_t new_size) -> void * {
        return ::realloc(p, new_size);
      });
  if (!r.ok()) {
    std::cerr << "Failed to allocate PayloadBuffer of size " << initial_size
              << std::endl;
    abort();
  }
  return *r;
}

} // namespace gemini::zeros
