#pragma once

#include "davros/zeros/payload_buffer.h"
#include <stdint.h>

namespace davros::zeros {

struct Message {
  Message(PayloadBuffer* pb) : buffer(pb) {}
  PayloadBuffer *buffer;

  static PayloadBuffer *GetBuffer(void *field, uint32_t offset) {
    return *reinterpret_cast<PayloadBuffer **>(reinterpret_cast<char *>(field) -
                                               offset);
  }
  static PayloadBuffer **GetBufferAddr(void *field,uint32_t offset) {
    return reinterpret_cast<PayloadBuffer **>(reinterpret_cast<char *>(field) -
                                              offset);
  }

};

} // namespace davros::zeros
