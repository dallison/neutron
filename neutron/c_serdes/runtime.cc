#include "neutron/c_serdes/runtime.h"

#if defined(__cplusplus)
extern "C" {
#endif
void NeutronBufferInit(NeutronBuffer *buffer, char *addr, size_t size) {
  buffer->start = addr;
  buffer->addr = addr;
  buffer->size = size;
  buffer->num_zeroes = 0;
}

size_t NeutronBufferSize(NeutronBuffer *buffer) {
  return buffer->addr - buffer->start;
}

size_t NeutronBufferRemaining(NeutronBuffer *buffer) {
  return buffer->size - NeutronBufferSize(buffer);
}

bool NeutronBufferCheckAtEnd(NeutronBuffer *buffer) {
  return buffer->addr == buffer->start + buffer->size;
}

bool NeutronBufferHasSpaceFor(NeutronBuffer *buffer, size_t n) {
  char *next = buffer->addr + n;
  // Off-by-one complexity here.  The end is one past the end of the buffer.
  if (next > buffer->start + buffer->size) {
    return false;
  }
  return true;
}

#define PUT_INT(type, type_name)                                               \
  bool NeutronBufferWrite##type_name##Field(NeutronBuffer *buffer, type v) {   \
    if (!NeutronBufferHasSpaceFor(buffer, sizeof(v))) {                        \
      return false;                                                            \
    }                                                                          \
    *(type *)buffer->addr = v;                                                 \
    buffer->addr += sizeof(v);                                                 \
    return true;                                                               \
  }

PUT_INT(uint8_t, Uint8)
PUT_INT(uint16_t, Uint16)
PUT_INT(uint32_t, Uint32)
PUT_INT(int8_t, Int8)
PUT_INT(int16_t, Int16)
PUT_INT(int32_t, Int32)
PUT_INT(bool, Bool)
PUT_INT(NeutronTime, Time)
PUT_INT(NeutronDuration, Duration)
#if __ARM_ARCH != 7
PUT_INT(uint64_t, Uint64)
PUT_INT(int64_t, Int64)
#endif
#undef PUT_INT

#if __ARM_ARCH == 7
// 64 bit for unaligned on armv7 needs to to split into two 32 bit writes.
bool NeutronBufferWriteInt64Field(NeutronBuffer *buffer, int64_t v) {
  if (!NeutronBufferHasSpaceFor(buffer, sizeof(v))) {
    return false;
  }
  *(int32_t *)buffer->addr = v & 0xffffffffLL;
  *(int32_t *)(buffer->addr + 4) = (v >> 32) & 0xffffffffLL;
  buffer->addr += sizeof(v);
  return true;
}

bool NeutronBufferWriteUint64Field(NeutronBuffer *buffer, uint64_t v) {
  if (!NeutronBufferHasSpaceFor(buffer, sizeof(v))) {
    return false;
  }
  *(uint32_t *)buffer->addr = v & 0xffffffffULL;
  *(uint32_t *)(buffer->addr + 4) = v >> 32;
  buffer->addr += sizeof(v);
  return true;
}
#endif

// Get an int.
#define GET_INT(type, type_name)                                               \
  bool NeutronBufferRead##type_name##Field(NeutronBuffer *buffer, type *v) {   \
    if (!NeutronBufferHasSpaceFor(buffer, sizeof(*v))) {                       \
      return false;                                                            \
    }                                                                          \
    *v = *(type *)buffer->addr;                                                \
    buffer->addr += sizeof(*v);                                                \
    return true;                                                               \
  }
GET_INT(uint8_t, Uint8)
GET_INT(uint16_t, Uint16)
GET_INT(uint32_t, Uint32)
GET_INT(int8_t, Int8)
GET_INT(int16_t, Int16)
GET_INT(int32_t, Int32)
GET_INT(bool, Bool)
GET_INT(NeutronTime, Time)
GET_INT(NeutronDuration, Duration)

#if __ARM_ARCH != 7
GET_INT(uint64_t, Uint64)
GET_INT(int64_t, Int64)
#endif

#undef GET_INT

#if __ARM_ARCH == 7
// 64 bit for unaligned on armv7 needs to to split into two 32 bit reads.
bool NeutronBufferReadInt64Field(NeutronBuffer *buffer, int64_t *v) {
  if (!NeutronBufferHasSpaceFor(buffer, sizeof(*v))) {
    return false;
  }
  *(int32_t *)v = *(int32_t *)buffer->addr;
  *(int32_t *)((char *)v + 4) = *(int32_t *)(buffer->addr + 4);
  buffer->addr += sizeof(*v);
  return true;
}
bool NeutronBufferReadUint64Field(NeutronBuffer *buffer, uint64_t *v) {
  if (!NeutronBufferHasSpaceFor(buffer, sizeof(*v))) {
    return false;
  }
  *(uint32_t *)v = *(uint32_t *)buffer->addr;
  *(uint32_t *)((char *)v + 4) = *(uint32_t *)(buffer->addr + 4);
  buffer->addr += sizeof(*v);
  return true;
}
#endif

// Possible alignment issues with float and double so we alias them to ints.
bool NeutronBufferWriteFloatField(NeutronBuffer *buffer, float v) {
  if (!NeutronBufferHasSpaceFor(buffer, sizeof(v))) {
    return false;
  }
  uint32_t *p = (uint32_t *)&v;
  *(uint32_t *)buffer->addr = *p;
  buffer->addr += sizeof(v);
  return true;
}


bool NeutronBufferWriteDoubleField(NeutronBuffer *buffer, double v) {
  return NeutronBufferWriteUint64Field(buffer, *(uint64_t *)&v);
}

bool NeutronBufferReadFloatField(NeutronBuffer *buffer, float *v) {
  return NeutronBufferReadUint32Field(buffer, (uint32_t *)v);;
}

bool NeutronBufferReadDoubleField(NeutronBuffer *buffer, double *v) {
  return NeutronBufferReadUint64Field(buffer, (uint64_t *)v);
}

// Put a fixed length string.
bool NeutronBufferWriteStringField(NeutronBuffer *buffer, const char *s,
                                   size_t len) {
  if (!NeutronBufferHasSpaceFor(buffer, len + 1)) {
    return false;
  }
  memcpy(buffer->addr, s, len + 1);
  buffer->addr += len + 1;
  return true;
}

bool NeutronBufferReadStringField(NeutronBuffer *buffer, char *s, size_t len) {
  if (!NeutronBufferHasSpaceFor(buffer, len + 1)) {
    return false;
  }
  memcpy(s, buffer->addr, len + 1);
  buffer->addr += len + 1;
  return true;
}

// Put array of primtives.
#define PUT_ARRAY(type, type_name)                                             \
  bool NeutronBufferWrite##type_name##Array(NeutronBuffer *buffer,             \
                                            const type *v, size_t len) {       \
    if (!NeutronBufferHasSpaceFor(buffer, len * sizeof(*v))) {                 \
      return false;                                                            \
    }                                                                          \
    memcpy(buffer->addr, v, len * sizeof(*v));                                 \
    buffer->addr += len * sizeof(*v);                                          \
    return true;                                                               \
  }
PUT_ARRAY(uint8_t, Uint8)
PUT_ARRAY(uint16_t, Uint16)
PUT_ARRAY(uint32_t, Uint32)
PUT_ARRAY(uint64_t, Uint64)
PUT_ARRAY(int8_t, Int8)
PUT_ARRAY(int16_t, Int16)
PUT_ARRAY(int32_t, Int32)
PUT_ARRAY(int64_t, Int64)
PUT_ARRAY(float, Float)
PUT_ARRAY(double, Double)
PUT_ARRAY(bool, Bool)
PUT_ARRAY(NeutronTime, Time)
PUT_ARRAY(NeutronDuration, Duration)
#undef PUT_ARRAY_INT

// Get array of Fields.
#define GET_ARRAY(type, type_name)                                             \
  bool NeutronBufferRead##type_name##Array(NeutronBuffer *buffer, type *v,     \
                                           size_t len) {                       \
    if (!NeutronBufferHasSpaceFor(buffer, len * sizeof(*v))) {                 \
      return false;                                                            \
    }                                                                          \
    memcpy(v, buffer->addr, len * sizeof(*v));                                 \
    buffer->addr += len * sizeof(*v);                                          \
    return true;                                                               \
  }
GET_ARRAY(uint8_t, Uint8)
GET_ARRAY(uint16_t, Uint16)
GET_ARRAY(uint32_t, Uint32)
GET_ARRAY(uint64_t, Uint64)
GET_ARRAY(int8_t, Int8)
GET_ARRAY(int16_t, Int16)
GET_ARRAY(int32_t, Int32)
GET_ARRAY(int64_t, Int64)
GET_ARRAY(float, Float)
GET_ARRAY(double, Double)
GET_ARRAY(bool, Bool)
GET_ARRAY(NeutronTime, Time)
GET_ARRAY(NeutronDuration, Duration)
#undef GET_ARRAY_INT

#if defined(__cplusplus)
}  // extern "C"
#endif