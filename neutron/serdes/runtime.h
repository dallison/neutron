#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "neutron/common_runtime.h"
#include "toolbelt/hexdump.h"
#include <array>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

namespace neutron::serdes {

// Base class for all messages.
class SerdesMessage {};

constexpr uint8_t kZeroMarker = 0xfa;
// The max number of zeroes in a run is one more than than the zero marker since
// the zero marker is followed by the number of zeroes - 2
constexpr size_t kMaxZeroes = kZeroMarker + 1;

template <typename T> inline size_t SignedLeb128Size(const T &v) {
  size_t size = 0;
  bool more = true;
  T value = v;
  while (more) {
    uint8_t byte = value & 0x7f;
    value >>= 7;
    bool sign_bit = (byte & 0x40) != 0;
    if ((value == 0 && !sign_bit) || (value == -1 && sign_bit)) {
      more = false;
    } else {
      byte |= 0x80;
    }
    size++;
    if (byte == kZeroMarker) {
      // Need to escape kZeroMarker because it is a zero-run marker.
      // kZeroMarker is written as kZeroMarker, kZeroMarker
      size++;
    }
  }
  return size;
}

template <typename T> inline size_t UnsignedLeb128Size(const T &x) {
  size_t size = 0;
  T v = x;
  do {
    size++;
    // If this will be encoded as kZeroMarker we need to escape it as it will be
    // written as kZeroMarker, kZeroMarker.
    if ((v & 0x7f) == (kZeroMarker & 0x7f) && (v >> 7) != 0) {
      size++;
    }
    v >>= 7;
  } while (v != 0);
  return size;
}

template <typename T> inline size_t Leb128Size(const T &v) {
  if constexpr (std::is_unsigned<T>::value) {
    return UnsignedLeb128Size(v);
  } else {
    return SignedLeb128Size(v);
  }
}

template <> inline size_t Leb128Size(const std::string &v) {
  return Leb128Size(v.size()) + v.size();
}

inline size_t Leb128Size(const float &v) { return sizeof(v); }

inline size_t Leb128Size(const double &v) { return sizeof(v); }

inline size_t Leb128Size(const Time &t) {
  return Leb128Size(t.secs) + Leb128Size(t.nsecs);
}

inline size_t Leb128Size(const Duration &d) {
  return Leb128Size(d.secs) + Leb128Size(d.nsecs);
}

template <typename T> inline size_t Leb128Size(const std::vector<T> &v) {
  size_t size = Leb128Size(v.size());
  for (auto &e : v) {
    size += Leb128Size(e);
  }
  return size;
}

template <> inline size_t Leb128Size(const std::vector<uint8_t> &v) {
  // Body is not leb128 encoded so that we can use memcpy.
  size_t size = Leb128Size(v.size());
  return size + v.size();
}

template <typename T, size_t N>
inline size_t Leb128Size(const std::array<T, N> &v) {
  size_t size = 0;
  for (auto &e : v) {
    size += Leb128Size(e);
  }
  return size;
}

template <size_t N> inline size_t Leb128Size(const std::array<uint8_t, N> &v) {
  return N;
}

class SizeAccumulator {
public:
  template <typename T> void Accumulate(const T &v) {
    if (v == 0) {
      if (num_zeroes_ == kMaxZeroes) {
        size_ += 2;
        num_zeroes_ = 1;
      } else {
        num_zeroes_++;
      }
    } else {
      if (num_zeroes_ > 0) {
        if (num_zeroes_ == 1) {
          size_++;
        } else {
          size_ += 2;
        }
        num_zeroes_ = 0;
      }
      size_ += Leb128Size(v);
    }
  }

  template <> void Accumulate(const float &v) {
    const uint32_t *p = reinterpret_cast<const uint32_t *>(&v);
    Accumulate(*p);
  }

  template <> void Accumulate(const double &v) {
    const uint64_t *p = reinterpret_cast<const uint64_t *>(&v);
    Accumulate(*p);
  }

  template <> void Accumulate(const std::string &s) {
    Accumulate(s.size());
    size_ += s.size();
  }

  template <> void Accumulate(const Time &t) {
    Accumulate(t.secs);
    Accumulate(t.nsecs);
  }

  template <> void Accumulate(const Duration &d) {
    Accumulate(d.secs);
    Accumulate(d.nsecs);
  }

  template <typename T> void Accumulate(const std::vector<T> &v) {
    Accumulate(v.size());
    for (auto &e : v) {
      Accumulate(e);
    }
  }

  template <> void Accumulate(const std::vector<uint8_t> &v) {
    if (v.empty()) {
      // Empty vector has a zero length and no body.  No need to flush
      // the zeroes.
      Accumulate(uint8_t(0));
      return;
    }
    Close();
    Accumulate(v.size());
    size_ += v.size();
  }

  template <typename T, size_t N> void Accumulate(const std::array<T, N> &v) {
    for (auto &e : v) {
      Accumulate(e);
    }
  }

  template <size_t N> void Accumulate(const std::array<uint8_t, N> &v) {
    Close(); // Flush any pending zeroes.
    size_ += N;
  }

  void Close() {
    if (num_zeroes_ > 0) {
      if (num_zeroes_ == 1) {
        size_++;
      } else {
        size_ += 2;
      }
      num_zeroes_ = 0;
    }
  }

  size_t Size() const { return size_; }

private:
  size_t size_ = 0;
  int num_zeroes_ = 0;
};

// Provides a statically sized or dynamic buffer used for serialization
// of messages.
class Buffer {
public:
  // Dynamic buffer with own memory allocation.
  Buffer(size_t initial_size = 16) : owned_(true), size_(initial_size) {
    if (initial_size < 16) {
      // Need a reasonable size to start with.
      abort();
    }
    start_ = reinterpret_cast<char *>(malloc(size_));
    if (start_ == nullptr) {
      abort();
    }
    addr_ = start_;
    end_ = start_ + size_;
  }

  // Fixed buffer in non-owned memory.
  Buffer(char *addr, size_t size)
      : owned_(false), start_(addr), size_(size), addr_(addr),
        end_(addr + size) {}

  ~Buffer() {
    if (owned_) {
      free(start_);
    }
  }

  size_t Size() const { return addr_ - start_; }

  size_t size() const { return Size(); }

  template <typename T> T *Data() { return reinterpret_cast<T *>(start_); }

  char *data() { return Data<char>(); }

  std::string AsString() const { return std::string(start_, addr_ - start_); }

  template <typename T> absl::Span<const T> AsSpan() const {
    return absl::Span<T>(reinterpret_cast<const T *>(start_), addr_ - start_);
  }

  void Clear() {
    addr_ = start_;
    end_ = start_;
  }

  void Rewind() { addr_ = start_; }

  absl::Status CheckAtEnd() const {
    if (addr_ != end_) {
      return absl::InternalError(
          absl::StrFormat("Extra data in buffer: start: %p, addr: %p, end_ %p",
                          start_, addr_, end_));
    }
    return absl::OkStatus();
  }

  // Alignment is not guaranteed for any copies so to comply with
  // norms we use memcpy.  Although all modern CPUs allow non-aligned
  // word reads and writes they can come with a performance degradation.
  // It won't make any difference anyway since the biggest performance
  // issue with serialization is large data sets, like camera images.
  template <typename T> absl::Status Write(const T &v) {
    if (absl::Status status = HasSpaceFor(sizeof(T)); !status.ok()) {
      return status;
    }
    memcpy(addr_, &v, sizeof(T));
    addr_ += sizeof(T);
    return absl::OkStatus();
  }

  template <typename T> absl::Status Read(T &v) const {
    if (absl::Status status = Check(sizeof(T)); !status.ok()) {
      return status;
    }
    memcpy(&v, addr_, sizeof(T));
    addr_ += sizeof(T);
    return absl::OkStatus();
  }

  template <typename T> absl::Status WriteCompact(const T &v) {

    if (std::is_unsigned<T>::value) {
      if (absl::Status status = WriteUnsignedLeb128(v); !status.ok()) {
        return status;
      }
    } else {
      if (absl::Status status = WriteSignedLeb128(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <typename T> absl::Status ReadCompact(T &v) const {
    if constexpr (std::is_unsigned<T>::value) {
      return ReadUnsignedLeb128(v);
    } else {
      return ReadSignedLeb128(v);
    }
  }

  template <typename T> absl::Status Expand(Buffer &dest) const {
    if (absl::Status status = dest.HasSpaceFor(sizeof(T)); !status.ok()) {
      return status;
    }
    T *copyaddr = reinterpret_cast<T *>(dest.addr_);
    if constexpr (std::is_unsigned<T>::value) {
      if (absl::Status status = ReadUnsignedLeb128(*copyaddr); !status.ok()) {
        return status;
      }
    } else {
      if (absl::Status status = ReadSignedLeb128(*copyaddr); !status.ok()) {
        return status;
      }
    }

    dest.addr_ += sizeof(T);
    return absl::OkStatus();
  }

  template <typename T> absl::Status Compact(Buffer &dest) const {
    // Check that we have a value to compact in the source buffer.
    if (absl::Status status = Check(sizeof(T)); !status.ok()) {
      return status;
    }

    T *v = reinterpret_cast<T *>(addr_);
    if constexpr (std::is_unsigned<T>::value) {
      if (absl::Status status = dest.WriteUnsignedLeb128(*v); !status.ok()) {
        return status;
      }
    } else {
      if (absl::Status status = dest.WriteSignedLeb128(*v); !status.ok()) {
        return status;
      }
    }
    addr_ += sizeof(T);
    return absl::OkStatus();
  }

  absl::Status WriteCompact(const float &v) {
    const uint32_t *x = reinterpret_cast<const uint32_t *>(&v);
    return WriteCompact(*x);
  }

  absl::Status ReadCompact(float &v) const {
    uint32_t x = 0;
    if (absl::Status status = ReadCompact(x); !status.ok()) {
      return status;
    }
    v = *reinterpret_cast<float *>(&x);
    return absl::OkStatus();
  }

  absl::Status WriteCompact(const double &v) {
    const uint64_t *x = reinterpret_cast<const uint64_t *>(&v);
    return WriteCompact(*x);
  }

  absl::Status ReadCompact(double &v) const {
    uint64_t x = 0;
    if (absl::Status status = ReadCompact(x); !status.ok()) {
      return status;
    }
    v = *reinterpret_cast<double *>(&x);
    return absl::OkStatus();
  }

  template <> absl::Status Expand<float>(Buffer &dest) const {
    uint32_t v = 0;
    if (absl::Status status = ReadCompact(v); !status.ok()) {
      return status;
    }
    return dest.Write(v);
  }

  template <> absl::Status Compact<float>(Buffer &dest) const {
    uint32_t *v = reinterpret_cast<uint32_t *>(addr_);
    addr_ += sizeof(float);
    return dest.WriteCompact(*v);
  }

  template <> absl::Status Expand<double>(Buffer &dest) const {
    uint64_t v = 0;
    if (absl::Status status = ReadCompact(v); !status.ok()) {
      return status;
    }
    return dest.Write(v);
  }

  template <> absl::Status Compact<double>(Buffer &dest) const {
    uint64_t *v = reinterpret_cast<uint64_t *>(addr_);
    addr_ += sizeof(double);
    return dest.WriteCompact(*v);
  }

  template <> absl::Status Write(const std::string &v) {
    if (absl::Status status = HasSpaceFor(4 + v.size()); !status.ok()) {
      return status;
    }

    uint32_t size = static_cast<uint32_t>(v.size());
    memcpy(addr_, &size, sizeof(size));
    memcpy(addr_ + 4, v.data(), v.size());
    addr_ += 4 + v.size();
    return absl::OkStatus();
  }

  template <> absl::Status Read(std::string &v) const {
    if (absl::Status status = Check(4); !status.ok()) {
      return status;
    }
    uint32_t size = 0;
    memcpy(&size, addr_, sizeof(size));
    if (absl::Status status = Check(size_t(size)); !status.ok()) {
      return status;
    }
    v.resize(size);
    memcpy(v.data(), addr_ + 4, size);
    addr_ += 4 + v.size();
    return absl::OkStatus();
  }

  template <> absl::Status WriteCompact(const std::string &v) {
    if (absl::Status status = WriteUnsignedLeb128(v.size()); !status.ok()) {
      return status;
    }
    if (absl::Status status = HasSpaceFor(v.size()); !status.ok()) {
      return status;
    }
    memcpy(addr_, v.data(), v.size());
    addr_ += v.size();
    return absl::OkStatus();
  }

  template <> absl::Status ReadCompact(std::string &v) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    if (absl::Status status = Check(size_t(size)); !status.ok()) {
      return status;
    }
    v.resize(size);
    memcpy(v.data(), addr_, size);
    addr_ += size;
    return absl::OkStatus();
  }

  template <> absl::Status Expand<std::string>(Buffer &dest) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.HasSpaceFor(4 + size); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, &size, sizeof(size));
    memcpy(dest.addr_ + 4, addr_, size);
    dest.addr_ += 4 + size;
    addr_ += size;
    return absl::OkStatus();
  }

  template <> absl::Status Compact<std::string>(Buffer &dest) const {
    uint32_t size = *reinterpret_cast<uint32_t *>(addr_);
    if (absl::Status status = Check(4 + size); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.WriteUnsignedLeb128(size); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.HasSpaceFor(size); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, addr_ + 4, size);
    addr_ += 4 + size;
    dest.addr_ += size;
    return absl::OkStatus();
  }

  template <typename T> absl::Status Write(const std::vector<T> &vec) {
    if (absl::Status status = HasSpaceFor(4); !status.ok()) {
      return status;
    }
    uint32_t size = static_cast<uint32_t>(vec.size());
    memcpy(addr_, &size, sizeof(size));
    addr_ += 4;
    for (auto &v : vec) {
      if (absl::Status status = Write(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <typename T> absl::Status Read(std::vector<T> &vec) const {
    if (absl::Status status = Check(4); !status.ok()) {
      return status;
    }
    uint32_t size = 0;
    memcpy(&size, addr_, sizeof(size));
    addr_ += 4;
    vec.resize(size);
    for (uint32_t i = 0; i < size; i++) {
      if (absl::Status status = Read(vec[i]); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <typename T> absl::Status WriteCompact(const std::vector<T> &vec) {
    if (absl::Status status = WriteUnsignedLeb128(vec.size()); !status.ok()) {
      return status;
    }
    for (auto &v : vec) {
      if (absl::Status status = WriteCompact(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  // Specialization vector of uint8_t so we can use memcpy instead of processing
  // the body byte by byte.
  template <> absl::Status WriteCompact(const std::vector<uint8_t> &vec) {
    if (vec.empty()) {
      // Empty vector has a zero length and no body.  No need to flush
      // the zeroes.
      if (absl::Status status = WriteUnsignedLeb128(0); !status.ok()) {
        return status;
      }
      return absl::OkStatus();
    }
    if (absl::Status status = FlushZeroes(); !status.ok()) {
      return status;
    }
    if (absl::Status status = WriteUnsignedLeb128(vec.size()); !status.ok()) {
      return status;
    }
    if (absl::Status status = HasSpaceFor(vec.size()); !status.ok()) {
      return status;
    }
    memcpy(addr_, vec.data(), vec.size());
    addr_ += vec.size();
    return absl::OkStatus();
  }

  template <typename T> absl::Status ReadCompact(std::vector<T> &vec) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    if (absl::Status status = Check(size_t(size)); !status.ok()) {
      return status;
    }

    vec.resize(size);
    for (uint32_t i = 0; i < size; i++) {
      if (absl::Status status = ReadCompact(vec[i]); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  // uint8_t specialization for memcpy.
  template <> absl::Status ReadCompact(std::vector<uint8_t> &vec) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    if (absl::Status status = Check(size_t(size)); !status.ok()) {
      return status;
    }

    vec.resize(size);
    memcpy(vec.data(), addr_, size);
    addr_ += size;
    return absl::OkStatus();
  }

  template <typename T>
  absl::Status Expand(const std::vector<T> &, Buffer &dest) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    if (absl::Status status = dest.Check(size_t(size)); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, &size, sizeof(size));
    dest.addr_ += 4;
    for (uint32_t i = 0; i < size; i++) {
      if (absl::Status status = Expand<T>(dest); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <>
  absl::Status Expand(const std::vector<uint8_t> &, Buffer &dest) const {
    uint32_t size = 0;
    if (absl::Status status = ReadUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    if (absl::Status status = dest.Check(size_t(size)); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, &size, sizeof(size));
    dest.addr_ += 4;
    if (absl::Status status = dest.HasSpaceFor(size); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, addr_, size);
    addr_ += size;
    dest.addr_ += size;
    return absl::OkStatus();
  }

  template <typename T>
  absl::Status Compact(const std::vector<T> &, Buffer &dest) const {
    if (absl::Status status = Check(4); !status.ok()) {
      return status;
    }
    uint32_t size = *reinterpret_cast<uint32_t *>(addr_);
    if (absl::Status status = dest.WriteUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    addr_ += 4;
    for (uint32_t i = 0; i < size; i++) {
      if (absl::Status status = Compact<T>(dest); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <>
  absl::Status Compact(const std::vector<uint8_t> &, Buffer &dest) const {
    if (absl::Status status = Check(4); !status.ok()) {
      return status;
    }
    uint32_t size = *reinterpret_cast<uint32_t *>(addr_);
    if (size == 0) {
      // Empty vector has a zero length and no body.  No need to flush
      // the zeroes.
      if (absl::Status status = dest.WriteUnsignedLeb128(0); !status.ok()) {
        return status;
      }
      return absl::OkStatus();
    }
    if (absl::Status status = dest.FlushZeroes(); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.WriteUnsignedLeb128(size); !status.ok()) {
      return status;
    }

    addr_ += 4;
    if (absl::Status status = dest.HasSpaceFor(size); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, addr_, size);
    addr_ += size;
    dest.addr_ += size;
    return absl::OkStatus();
  }

  template <typename T, size_t N>
  absl::Status Write(const std::array<T, N> &vec) {
    for (auto &v : vec) {
      if (absl::Status status = Write(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N> absl::Status Write(const std::array<uint8_t, N> &vec) {
    if (absl::Status status = HasSpaceFor(N); !status.ok()) {
      return status;
    }
    memcpy(addr_, vec.data(), N);
    addr_ += N;
    return absl::OkStatus();
  }

  template <typename T, size_t N>
  absl::Status Read(std::array<T, N> &vec) const {
    for (int i = 0; i < N; i++) {
      if (absl::Status status = Read(vec[i]); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N> absl::Status Read(std::array<uint8_t, N> &vec) const {
    if (absl::Status status = Check(N); !status.ok()) {
      return status;
    }
    memcpy(vec.data(), addr_, N);
    addr_ += N;
    return absl::OkStatus();
  }

  template <typename T, size_t N>
  absl::Status WriteCompact(const std::array<T, N> &vec) {
    for (auto &v : vec) {
      if (absl::Status status = WriteCompact(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N>
  absl::Status WriteCompact(const std::array<uint8_t, N> &vec) {
    if (absl::Status status = FlushZeroes(); !status.ok()) {
      return status;
    }
    // Not compacted since want this to be a memcpy.
    return Write(vec);
  }

  template <typename T, size_t N>
  absl::Status ReadCompact(std::array<T, N> &vec) const {
    for (int i = 0; i < N; i++) {
      if (absl::Status status = ReadCompact(vec[i]); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N>
  absl::Status ReadCompact(std::array<uint8_t, N> &vec) const {
    return Read(vec);
  }

  template <typename T, size_t N>
  absl::Status Expand(const std::array<T, N> &, Buffer &dest) const {
    for (int i = 0; i < N; i++) {
      if (absl::Status status = Expand<T>(dest); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N>
  absl::Status Expand(const std::array<uint8_t, N> &, Buffer &dest) const {
    if (absl::Status status = Check(N); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.HasSpaceFor(N); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, addr_, N);
    addr_ += N;
    dest.addr_ += N;
    return absl::OkStatus();
  }

  template <typename T, size_t N>
  absl::Status Compact(const std::array<T, N> &, Buffer &dest) const {
    for (int i = 0; i < N; i++) {
      if (absl::Status status = Compact<T>(dest); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <size_t N>
  absl::Status Compact(const std::array<uint8_t, N> &, Buffer &dest) const {
    if (absl::Status status = dest.FlushZeroes(); !status.ok()) {
      return status;
    }
    if (absl::Status status = dest.HasSpaceFor(N); !status.ok()) {
      return status;
    }
    if (absl::Status status = Check(N); !status.ok()) {
      return status;
    }
    memcpy(dest.addr_, addr_, N);
    addr_ += N;
    dest.addr_ += N;
    return absl::OkStatus();
  }

  template <> absl::Status Write(const Time &t) {
    if (absl::Status status = Write(t.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Write(t.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Read(Time &t) const {
    if (absl::Status status = Read(t.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Read(t.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status WriteCompact(const Time &t) {
    if (absl::Status status = WriteCompact(t.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = WriteCompact(t.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status ReadCompact(Time &t) const {
    if (absl::Status status = ReadCompact(t.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = ReadCompact(t.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Expand<Time>(Buffer &dest) const {
    if (absl::Status status = Expand<uint32_t>(dest); !status.ok()) {
      return status;
    }
    if (absl::Status status = Expand<uint32_t>(dest); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Compact<Time>(Buffer &dest) const {
    if (absl::Status status = Compact<uint32_t>(dest); !status.ok()) {
      return status;
    }
    if (absl::Status status = Compact<uint32_t>(dest); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Write(const Duration &d) {
    if (absl::Status status = Write(d.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Write(d.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Read(Duration &d) const {
    if (absl::Status status = Read(d.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Read(d.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status WriteCompact(const Duration &d) {
    if (absl::Status status = WriteCompact(d.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = WriteCompact(d.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status ReadCompact(Duration &d) const {
    if (absl::Status status = ReadCompact(d.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = ReadCompact(d.nsecs); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Expand<Duration>(Buffer &dest) const {
    if (absl::Status status = Expand<uint32_t>(dest); !status.ok()) {
      return status;
    }
    if (absl::Status status = Expand<uint32_t>(dest); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  template <> absl::Status Compact<Duration>(Buffer &dest) const {
    if (absl::Status status = Compact<uint32_t>(dest); !status.ok()) {
      return status;
    }
    if (absl::Status status = Compact<uint32_t>(dest); !status.ok()) {
      return status;
    }
    return absl::OkStatus();
  }

  // These are public because we need it for vector expansion.
  template <typename T> absl::Status ReadUnsignedLeb128(T &v) const {
    int shift = 0;
    uint8_t byte;
    v = 0;
    do {
      if (absl::Status status = Get(byte); !status.ok()) {
        return status;
      }
      T b = T(byte & 0x7f);
      v |= b << shift;
      shift += 7;
    } while (byte & 0x80);

    return absl::OkStatus();
  }

  template <typename T> absl::Status WriteUnsignedLeb128(T v) {
    do {
      uint8_t byte = v & 0x7f;
      v >>= 7;
      if (v != 0) {
        byte |= 0x80;
      }
      if (absl::Status status = Put(byte); !status.ok()) {
        return status;
      }
    } while (v != 0);
    return absl::OkStatus();
  }

  absl::Status FlushZeroes() {
    if (num_zeroes_ > 0) {
      if (num_zeroes_ == 1) {
        if (absl::Status status = HasSpaceFor(1); !status.ok()) {
          return status;
        }
        *addr_++ = 0;
      } else {
        if (absl::Status status = HasSpaceFor(2); !status.ok()) {
          return status;
        }
        *addr_++ = kZeroMarker;     // Add zero marker.
        *addr_++ = num_zeroes_ - 2; // Marker is followed by count - 2.
      }
      num_zeroes_ = 0;
    }
    return absl::OkStatus();
  }

private:
  absl::Status HasSpaceFor(size_t n) {
    char *next = addr_ + n;
    // Off-by-one complexity here.  The end is one past the end of the buffer.
    if (next > end_) {
      if (owned_) {
        // Expand the buffer.
        size_t new_size = size_ * 2;

        char *new_start = reinterpret_cast<char *>(realloc(start_, new_size));
        if (new_start == nullptr) {
          abort();
        }
        size_t curr_length = addr_ - start_;
        start_ = new_start;
        addr_ = start_ + curr_length;
        end_ = start_ + new_size;
        size_ = new_size;
        return absl::OkStatus();
      }
      return absl::InternalError(absl::StrFormat(
          "No space in buffer: length: %d, need: %d", size_, next - start_));
    }
    return absl::OkStatus();
  }

  absl::Status Check(size_t n) const {
    char *next = addr_ + n;
    if (next <= end_) {
      return absl::OkStatus();
    }
    return absl::InternalError(
        absl::StrFormat("Buffer overun when checking for %d bytes; current "
                        "address is %p, end is %p",
                        n, addr_, end_));
  }

  // There caller must have checked that there is space in the buffer.
  template <typename T> absl::Status WriteSignedLeb128(T value) {
    bool more = true;
    while (more) {
      uint8_t byte = value & 0x7F;
      value >>= 7;

      // Sign bit of byte is second high order bit (0x40)
      if ((value == 0 && (byte & 0x40) == 0) ||
          (value == -1 && (byte & 0x40) != 0)) {
        more = false;
      } else {
        byte |= 0x80;
      }
      if (absl::Status status = Put(byte); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  // This checks that there is data in the buffer to cover the value.
  template <typename T> absl::Status ReadSignedLeb128(T &value) const {
    int shift = 0;
    uint8_t byte;
    value = 0;

    do {
      if (absl::Status status = Get(byte); !status.ok()) {
        return status;
      }
      T b = T(byte & 0x7f);
      value |= b << shift;
      shift += 7;

      if ((byte & 0x80) == 0 && (byte & 0x40) != 0) {
        value |= -(1 << shift);
      }
    } while (byte & 0x80);
    return absl::OkStatus();
  }

  absl::Status Put(uint8_t ch) {
    if (ch == 0) {
      // Max of kMaxZeroes zeroes in a run.
      if (num_zeroes_ == kMaxZeroes) {
        if (absl::Status status = FlushZeroes(); !status.ok()) {
          return status;
        }
      }
      num_zeroes_++;
      return absl::OkStatus();
    }
    if (absl::Status status = FlushZeroes(); !status.ok()) {
      return status;
    }
    if (ch == kZeroMarker) {
      // Need to escape kZeroMarker because it is a zero-run marker.
      // kZeroMarker is written as kZeroMarker, kZeroMarker
      if (absl::Status status = HasSpaceFor(2); !status.ok()) {
        return status;
      }
      *addr_++ = kZeroMarker;
      *addr_++ = kZeroMarker;
      return absl::OkStatus();
    }
    if (absl::Status status = HasSpaceFor(1); !status.ok()) {
      return status;
    }
    *addr_++ = char(ch);
    return absl::OkStatus();
  }

  absl::Status Get(uint8_t &v) const {
    if (num_zeroes_ > 0) {
      // We are running through a run of zeroes.
      num_zeroes_--;
      v = 0;
      return absl::OkStatus();
    }
    if (absl::Status status = Check(1); !status.ok()) {
      return status;
    }
    // Look at next char.
    uint8_t ch = uint8_t(*addr_++);
    // If we have a zero marker, this means that we have a run of zeroes.  The
    // next byte is the count of zeroes - 2.
    // Also, kZeroMarker followed by kZeroMarker is a literal kZeroMarker.
    if (ch == kZeroMarker) {
      if (absl::Status status = Check(1); !status.ok()) {
        return status;
      }
      ch = uint8_t(*addr_++);
      if (ch == kZeroMarker) {
        // kZeroMarker is written as kZeroMarker, kZeroMarker
        v = kZeroMarker;
        return absl::OkStatus();
      }
      num_zeroes_ = ch + 1; // +1 because we will have consumed one zero.
      v = 0;
      return absl::OkStatus();
    }
    v = ch;
    return absl::OkStatus();
  }

  bool owned_ = false;           // Memory is owned by this buffer.
  char *start_ = nullptr;        // Start of memory.
  size_t size_ = 0;              // Size of memory.
  mutable char *addr_ = nullptr; // Current read/write address.
  char *end_ = nullptr;          // End of buffer.
  mutable int num_zeroes_ = 0; // Number of zero bytes to write in compact mode.
};

} // namespace neutron::serdes
