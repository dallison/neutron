#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t secs;
  uint32_t nsecs;
} NeutronTime;

// Same as ros::Duration.
typedef struct {
  uint32_t secs;
  uint32_t nsecs;
} NeutronDuration;

typedef struct {
  char *start;
  char *addr;
  size_t size;
  size_t num_zeroes;
} NeutronBuffer;

#if defined(__cplusplus)
extern "C" {
#endif
void NeutronBufferInit(NeutronBuffer *buffer, char *addr, size_t size);
size_t NeutronBufferSize(NeutronBuffer *buffer);

size_t NeutronBufferRemaining(NeutronBuffer *buffer);

bool NeutronBufferCheckAtEnd(NeutronBuffer *buffer);

bool NeutronBufferHasSpaceFor(NeutronBuffer *buffer, size_t n);

bool NeutronBufferFlushZeroes(NeutronBuffer *buffer);

bool NeutronBufferWrite(NeutronBuffer *buffer, uint8_t ch);

bool NeutronBufferRead(NeutronBuffer *buffer, uint8_t *v);

#define PUT_INT(type, type_name)                                               \
  bool NeutronBufferWrite##type_name##Field(NeutronBuffer *buffer, type v);

PUT_INT(uint8_t, Uint8)
PUT_INT(uint16_t, Uint16)
PUT_INT(uint32_t, Uint32)
PUT_INT(uint64_t, Uint64)
PUT_INT(int8_t, Int8)
PUT_INT(int16_t, Int16)
PUT_INT(int32_t, Int32)
PUT_INT(int64_t, Int64)
PUT_INT(bool, Bool)
PUT_INT(NeutronTime, Time)
PUT_INT(NeutronDuration, Duration)
#undef PUT_INT

// Get an int.
#define GET_INT(type, type_name)                                               \
  bool NeutronBufferRead##type_name##Field(NeutronBuffer *buffer, type *v);
GET_INT(uint8_t, Uint8)
GET_INT(uint16_t, Uint16)
GET_INT(uint32_t, Uint32)
GET_INT(uint64_t, Uint64)
GET_INT(int8_t, Int8)
GET_INT(int16_t, Int16)
GET_INT(int32_t, Int32)
GET_INT(int64_t, Int64)
GET_INT(bool, Bool)
GET_INT(NeutronTime, Time)
GET_INT(NeutronDuration, Duration)
#undef GET_INT

// Possible alignment issues with float and double so we alias them to ints.
bool NeutronBufferWriteFloatField(NeutronBuffer *buffer, float v);

bool NeutronBufferWriteDoubleField(NeutronBuffer *buffer, double v);

bool NeutronBufferReadFloatField(NeutronBuffer *buffer, float *v);
bool NeutronBufferReadDoubleField(NeutronBuffer *buffer, double *v);

// Put a fixed length string.
bool NeutronBufferWriteStringField(NeutronBuffer *buffer, const char *s,
                                   size_t len);

bool NeutronBufferReadStringField(NeutronBuffer *buffer, char *s, size_t len);

// Put array of primtives.
#define PUT_ARRAY(type, type_name)                                             \
  bool NeutronBufferWrite##type_name##Array(NeutronBuffer *buffer,             \
                                            const type *v, size_t len);
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
#undef PUT_ARRAY

// Get array of Fields.
#define GET_ARRAY(type, type_name)                                             \
  bool NeutronBufferRead##type_name##Array(NeutronBuffer *buffer, type *v,     \
                                           size_t len);
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
#undef GET_ARRAY

#if defined(__cplusplus)
}  // extern "C"
#endif
