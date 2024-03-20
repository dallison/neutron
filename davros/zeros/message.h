#pragma once

#include <stdint.h>

namespace davros::zeros {

constexpr int32_t kMessageMagicVirtual = 0x80000001;
constexpr int32_t kMessageMagicProxy = 0x80000002;

struct VirtualMessage {};
struct StaticMessage {};
struct ProxyMessage {};

struct Message;

struct StaticHeader {
  StaticHeader() = default;
  StaticHeader(int32_t pb_offset) : magic_or_offset(pb_offset) {}
  int32_t magic_or_offset;
};

struct VirtualHeader {
  VirtualHeader() : magic_or_offset(kMessageMagicVirtual) {}
  VirtualHeader(Message* m) : magic_or_offset(kMessageMagicVirtual), msg(m) {}
  int32_t magic_or_offset;
  Message *msg;
};

struct ProxyHeader {
  ProxyHeader() : magic_or_offset(kMessageMagicProxy) {}
  int32_t magic_or_offset;
  Message *msg;
};

struct Offset {
  int32_t value;
};

struct Message {
  char header[16];

  // Message(StaticMessage) { header.static_hdr = StaticHeader(); }
  // Message(StaticMessage, int offset) {
  //   header.static_hdr = StaticHeader(offset);
  // }
  // Message(VirtualMessage) { header.virtual_hdr = VirtualHeader(); }
  // Message(ProxyMessage) { header.proxy_hdr = ProxyHeader(); }

  // // Empty message on the stack or heap.
  // Message() : Message(VirtualMessage{}) {}

  // // Empty message at an offset into a buffer.
  // Message(Offset offset) : Message(StaticMessage{}, offset.value) {}
};

} // namespace davros::zeros
