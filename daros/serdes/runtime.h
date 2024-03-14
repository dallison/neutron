#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <array>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace daros {

struct Time {
  uint32_t secs;
  uint32_t nsecs;
};

struct Duration {
  uint32_t secs;
  uint32_t nsecs;
};

class Buffer {
public:
  Buffer(char *addr, size_t len) : addr_(addr), end_(addr + len) {}

  // Alignment is not guaranteed for any copies so to comply with
  // norms we use memcpy.  Although all modern CPUs allow non-aligned
  // word reads and writes they can come with a performance degradation.
  // It won't make any difference anyway since the biggest performance
  // issue with serialization is large data sets, like camera images.
  template <typename T> absl::Status Write(T v) {
    if (absl::Status status = HasSpaceFor(sizeof(T)); !status.ok()) {
      return status;
    }
    memcpy(addr_, &v, sizeof(T));
    addr_ += sizeof(T);
    return absl::OkStatus();
  }

  template <typename T> absl::Status Read(T &v) {
    if (absl::Status status = HasSpaceFor(sizeof(T)); !status.ok()) {
      return status;
    }
    memcpy(&v, &addr_, sizeof(T));
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
    if (absl::Status status = HasSpaceFor(4); !status.ok()) {
      return status;
    }
    uint32_t size;
    memcpy(addr_, &size, sizeof(size));
    if (absl::Status status = HasSpaceFor(size_t(size)); !status.ok()) {
      return status;
    }
    v.resize(size);
    memcpy(v.data(), addr_ + 4, size);
    addr_ += 4 + v.size();
    return absl::OkStatus();
  }

  template <typename T> absl::Status Write(const std::vector<T> &vec) {
    if (absl::Status status = HasSpaceFor(4 + vec.size() * sizeof(T));
        !status.ok()) {
      return status;
    }
    uint32_t size = static_cast<uint32_t>(vec.size()) * sizeof(T);
    memcpy(addr_, &size, sizeof(size));
    addr_ += sizeof(size);
    for (auto &v : vec) {
      Write(v);
    }
    addr_ += 4 + size;
    return absl::OkStatus();
  }

  template <typename T> absl::Status Read(std::vector<T> &vec) {
    if (absl::Status status = HasSpaceFor(4); !status.ok()) {
      return status;
    }
    uint32_t size;
    memcpy(addr_, &size, sizeof(size));
    if (absl::Status status = HasSpaceFor(size_t(size) * sizeof(T));
        !status.ok()) {
      return status;
    }
    v.resize(size);
    memcpy(v.data(), addr_ + 4, size * sizeof(T));
    addr_ += 4 + v.size() * sizeof(T);
    return absl::OkStatus();
  }

  template <typename T, int N> absl::Status Write(const std::array<T, N> &vec) {
    if (absl::Status status = HasSpaceFor(N * sizeof(T)); !status.ok()) {
      return status;
    }

    for (auto &v : vec) {
      Write(v);
    }
    addr_ += N * sizeof(T);
    return absl::OkStatus();
  }

  template <typename T, int N> absl::Status Read(std::array<T, N> &vec) {
    if (absl::Status status = HasSpaceFor(N * sizeof(T)); !status.ok()) {
      return status;
    }
    v.resize(N);
    memcpy(v.data(), addr_ + 4, N * sizeof(T));
    addr_ += N * sizeof(T);
    return absl::OkStatus();
  }

  template <> absl::Status Write(const Time &t) {
    if (absl::Status status = HasSpaceFor(sizeof(t)); !status.ok()) {
      return status;
    }
    Write(t.secs);
    Write(t.nsecs);
    return absl::OkStatus();
  }

  template <> absl::Status Read(Time &t) {
    if (absl::Status status = HasSpaceFor(sizeof(t)); !status.ok()) {
      return status;
    }
    Read(t.secs);
    Read(r.nsecs);
    return absl::OkStatus();
  }

  template <> absl::Status Write(const Duration &d) {
    if (absl::Status status = HasSpaceFor(sizeof(d)); !status.ok()) {
      return status;
    }
    Write(d.secs);
    Write(d.nsecs);
    return absl::OkStatus();
  }

  template <> absl::Status Read(Duration &d) {
    if (absl::Status status = HasSpaceFor(sizeof(d)); !status.ok()) {
      return status;
    }
    Read(d.secs);
    Read(d.nsecs);
    return absl::OkStatus();
  }

private:
  absl::Status HasSpaceFor(size_t n) {
    char *next = addr_ + n;
    if (next < end_) {
      return absl::OkStatus();
    }
    return absl::InternalError("End of buffer");
  }

  char *addr_;
  char *end_;
};

} // namespace daros
