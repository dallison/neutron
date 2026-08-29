#pragma once

#include "absl/status/statusor.h"
#include "toolbelt/payload_buffer.h"
#include <memory>
#include <stdint.h>
#include <string_view>

namespace neutron::zeros {

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
  // When set, bounds checking for readonly payloads uses this received size
  // instead of in-buffer full_size.  Shared across the whole message tree.
  std::shared_ptr<const size_t> readonly_size;
  toolbelt::BufferOffset absolute_binary_offset;

  static const Message *GetMessage(const void *field, uint32_t offset) {
    return reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
  }

  // 'field' is the offset from the start of the message to the field (positive)
  // Subtract the field offset from the field to get the address of the
  // std::shared_ptr to the pointer to the toolbelt::PayloadBuffer.
  static toolbelt::PayloadBuffer *GetBuffer(const void *field,
                                            uint32_t offset) {
    const Message *msg = GetMessage(field, offset);
    return msg->buffer != nullptr ? *msg->buffer : nullptr;
  }

  static toolbelt::PayloadBuffer **GetBufferAddr(const void *field,
                                                 uint32_t offset) {
    const Message *msg = GetMessage(field, offset);
    return msg->buffer.get();
  }

  static std::shared_ptr<toolbelt::PayloadBuffer *>
  GetSharedBuffer(void *field, uint32_t offset) {
    const Message *msg = GetMessage(field, offset);
    return msg->buffer;
  }

  static std::shared_ptr<const size_t> GetReadonlySize(const void *field,
                                                       uint32_t offset) {
    return GetMessage(field, offset)->readonly_size;
  }

  static toolbelt::BufferOffset GetMessageBinaryStart(const void *field,
                                                      uint32_t offset) {
    return GetMessage(field, offset)->absolute_binary_offset;
  }

  // Number of bytes trusted to be available.  For readonly messages this is
  // the size passed to CreateReadonly; for mutable/owned buffers it is full_size.
  static size_t TrustedSize(const void *field, uint32_t offset) {
    const Message *msg = GetMessage(field, offset);
    if (msg->readonly_size != nullptr) {
      return *msg->readonly_size;
    }
    toolbelt::PayloadBuffer *pb = msg->buffer != nullptr ? *msg->buffer : nullptr;
    return pb != nullptr ? size_t(pb->full_size) : 0;
  }

  // Size passed to PayloadBuffer::ToAddress.  Nonzero only for readonly payloads.
  static size_t TrustedBufferSize(const void *field, uint32_t offset) {
    const Message *msg = GetMessage(field, offset);
    return msg->readonly_size != nullptr ? *msg->readonly_size : 0;
  }

  template <typename T>
  static T *ToAddress(const void *field, uint32_t src_off,
                      toolbelt::BufferOffset off) {
    toolbelt::PayloadBuffer *pb = GetBuffer(field, src_off);
    if (pb == nullptr) {
      return nullptr;
    }
    return pb->ToAddress<T>(off, TrustedBufferSize(field, src_off));
  }

  static std::string_view GetStringView(const void *field, uint32_t src_off,
                                        toolbelt::BufferOffset hdr_off) {
    toolbelt::PayloadBuffer *pb = GetBuffer(field, src_off);
    if (pb == nullptr) {
      return {};
    }
    return pb->GetStringView(hdr_off, TrustedBufferSize(field, src_off));
  }

  static size_t StringSize(const void *field, uint32_t src_off,
                           toolbelt::BufferOffset hdr_off) {
    toolbelt::PayloadBuffer *pb = GetBuffer(field, src_off);
    if (pb == nullptr) {
      return 0;
    }
    return pb->StringSize(hdr_off, TrustedBufferSize(field, src_off));
  }

  static const char *StringData(const void *field, uint32_t src_off,
                                toolbelt::BufferOffset hdr_off) {
    toolbelt::PayloadBuffer *pb = GetBuffer(field, src_off);
    if (pb == nullptr) {
      return nullptr;
    }
    return pb->StringData(hdr_off, TrustedBufferSize(field, src_off));
  }

  // Clamps an attacker-influenced element count to what fits in the buffer.
  static size_t ClampElementCount(const void *field, uint32_t src_off,
                                  toolbelt::BufferOffset data_offset,
                                  size_t claimed, size_t elem_size) {
    if (data_offset == 0 || elem_size == 0) {
      return 0;
    }
    const size_t limit = TrustedSize(field, src_off);
    if (data_offset >= limit) {
      return 0;
    }
    const size_t avail = limit - data_offset;
    if (elem_size > avail) {
      return 0;
    }
    return avail / elem_size < claimed ? avail / elem_size : claimed;
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

} // namespace neutron::zeros
