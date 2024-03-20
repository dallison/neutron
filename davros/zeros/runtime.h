#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace davros {

#define DEFINE_PRIMITIVE_FIELD(cname, type)                                    \
  class cname##Field {                                                         \
  public:                                                                      \
    explicit cname##Field(type *p) : ptr_(p) {}                                \
                                                                               \
    operator type() { return *ptr_; }                                          \
                                                                               \
    cname##Field &operator=(type i) {                                          \
      *ptr_ = i;                                                               \
      return *this;                                                            \
    }                                                                          \
                                                                               \
  private:                                                                     \
    type *ptr_;                                                                \
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
  explicit StringField(void *p) : ptr_(p) {}

  operator std::string() {
    uint32_t length = *reinterpret_cast<uint32_t *>(ptr_);
    const char *data = reinterpret_cast<const char *>(ptr_) + 4;
    return std::string(data, length);
  }

  StringField &operator=(const std::string &s) {
    *reinterpret_cast<uint32_t *>(ptr_) = static_cast<uint32_t>(s.size());
    char *data = reinterpret_cast<char *>(ptr_) + 4;
    memcpy(data, s.data(), s.size());
    return *this;
  }

private:
  void *ptr_;
};

template <typename T> class ArrayField {
public:
  ArrayField(void *p)
      : len_ptr_(reinterpret_cast<uint32_t *>(p)),
        data_ptr(reinterpret_cast<T *>(len_ptr_ + 1)) {}

private:
  uint32_t *len_ptr_;
  T *data_ptr_;
};
}
