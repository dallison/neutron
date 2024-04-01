#pragma once

// Array and vector iterators.

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


template <typename Field, typename T> struct FieldIterator {
  FieldIterator(Field *f, BufferOffset o, bool r = false)
      : field(f), offset(o), reverse(r) {}

  FieldIterator &operator++() {
    if (reverse) {
      offset -= sizeof(T);
    } else {
      offset += sizeof(T);
    }
    return *this;
  }
  FieldIterator &operator--() {
    if (reverse) {
      offset += sizeof(T);
    } else {
      offset -= sizeof(T);
    }
    return *this;
  }
  FieldIterator operator+(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() - i * sizeof(T));
    }
    return iterator(field, field->BaseOffset() + i * sizeof(T));
  }
  FieldIterator operator-(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() + i * sizeof(T));
    }
    return iterator(field, field->BaseOffset() - i * sizeof(T));
  }
  T &operator*() const {
    T *addr = field->GetBuffer()->template ToAddress<T>(offset);
    return *addr;
  }

  bool operator==(const FieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const FieldIterator &it) const { return !operator==(it); }

  Field *field;
  BufferOffset offset;
  bool reverse;
};

template <typename Field> struct StringFieldIterator {
  StringFieldIterator(Field *f, BufferOffset o, bool r = false)
      : field(f), offset(o), reverse(r) {}

  StringFieldIterator &operator++() {
    if (reverse) {
      offset -= sizeof(BufferOffset);
    } else {
      offset += sizeof(BufferOffset);
    }
    return *this;
  }
  StringFieldIterator &operator--() {
    if (reverse) {
      offset += sizeof(BufferOffset);
    } else {
      offset -= sizeof(BufferOffset);
    }
    return *this;
  }
  StringFieldIterator operator+(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() - i * sizeof(BufferOffset));
    }
    return iterator(field, field->BaseOffset() + i * sizeof(BufferOffset));
  }
  StringFieldIterator operator-(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() + i * sizeof(BufferOffset));
    }
    return iterator(field, field->BaseOffset() - i * sizeof(BufferOffset));
  }
  std::string_view operator*() const {
    return field->GetBuffer()->GetStringView(field->BaseOffset() + offset);
  }

  bool operator==(const StringFieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const StringFieldIterator &it) const {
    return !operator==(it);
  }

  Field *field;
  BufferOffset offset;
  bool reverse;
};

template <typename Field, typename T> struct EnumFieldIterator {
  EnumFieldIterator(Field *f, BufferOffset o, bool r = false)
      : field(f), offset(o), reverse(r) {}

  EnumFieldIterator &operator++() {
    if (reverse) {
      offset -= sizeof(T);
    } else {
      offset += sizeof(T);
    }
    return *this;
  }
  EnumFieldIterator &operator--() {
    if (reverse) {
      offset += sizeof(T);
    } else {
      offset -= sizeof(T);
    }
    return *this;
  }
  EnumFieldIterator operator+(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() -
                                 i * sizeof(std::underlying_type<T>::type));
    }
    return iterator(field, field->BaseOffset() +
                               i * sizeof(std::underlying_type<T>::type));
  }
  EnumFieldIterator operator-(size_t i) {
    if (reverse) {
      return iterator(field, field->BaseOffset() +
                                 i * sizeof(std::underlying_type<T>::type));
    }
    return iterator(field, field->BaseOffset() -
                               i * sizeof(std::underlying_type<T>::type));
  }

  T &operator*() const {
    using U = typename std::underlying_type<T>::type;
    U *addr = field->GetBuffer()->template ToAddress<U>(offset);
    return *reinterpret_cast<T *>(addr);
  }

  bool operator==(const EnumFieldIterator &it) const {
    return field == it.field && offset == it.offset;
  }
  bool operator!=(const EnumFieldIterator &it) const { return !operator==(it); }

  Field *field;
  BufferOffset offset;
  bool reverse;
};


}

