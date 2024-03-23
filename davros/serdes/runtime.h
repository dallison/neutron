#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include <array>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include "davros/common_runtime.h"

namespace davros {

// Provides a statically sized or dynamic buffer used for serialization
// of messages.
class Buffer {
public:
  // Dynamic buffer with own memory allocation.
  Buffer(size_t initial_size=16) : owned_(true), size_(initial_size) {
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

  template <typename T> T *Data() {
    return reinterpret_cast<T *>(start_);
  }

  char *data() { return Data<char>(); }

  std::string AsString() const { return std::string(start_, addr_ - start_); }

  template <typename T> absl::Span<const T> AsSpan() const {
    return absl::Span<T>(reinterpret_cast<const T *>(start_), addr_ - start_);
  }

  void Clear() {
    addr_ = start_;
    end_ = start_;
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

  template <typename T> absl::Status Read(T &v) {
    if (absl::Status status = Check(sizeof(T)); !status.ok()) {
      return status;
    }
    memcpy(&v, addr_, sizeof(T));
    addr_ += sizeof(T);
    return absl::OkStatus();
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

  template <> absl::Status Read(std::string &v) {
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

  template <typename T> absl::Status Write(const std::vector<T> &vec) {
    if (absl::Status status = HasSpaceFor(4); !status.ok()) {
      return status;
    }
    uint32_t size = static_cast<uint32_t>(vec.size()) * sizeof(T);
    memcpy(addr_, &size, sizeof(size));
    addr_ += 4;
    for (auto &v : vec) {
      if (absl::Status status = Write(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <typename T> absl::Status Read(std::vector<T> &vec) {
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

  template <typename T, int N> absl::Status Write(const std::array<T, N> &vec) {
    for (auto &v : vec) {
      if (absl::Status status = Write(v); !status.ok()) {
        return status;
      }
    }
    return absl::OkStatus();
  }

  template <typename T, int N> absl::Status Read(std::array<T, N> &vec) {
    for (int i = 0; i < N; i++) {
      if (absl::Status status = Read(vec[i]); !status.ok()) {
        return status;
      }
    }
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

  template <> absl::Status Read(Time &t) {
    if (absl::Status status = Read(t.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Read(t.nsecs); !status.ok()) {
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

  template <> absl::Status Read(Duration &d) {
    if (absl::Status status = Read(d.secs); !status.ok()) {
      return status;
    }
    if (absl::Status status = Read(d.nsecs); !status.ok()) {
      return status;
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
      return absl::InternalError("No space in buffer");
    }
    return absl::OkStatus();
  }

  absl::Status Check(size_t n) {
    char *next = addr_ + n;
    if (next <= end_) {
      return absl::OkStatus();
    }
    return absl::InternalError("End of buffer");
  }

  bool owned_ = false;    // Memory is owned by this buffer.
  char *start_ = nullptr; //
  size_t size_ = 0;
  char *addr_ = nullptr;
  char *end_ = nullptr;
};

} // namespace davros
