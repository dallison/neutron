#pragma once

#include <stdint.h>
#include <iostream>

namespace davros {

// Same as ros::Time.
struct Time {
  uint32_t secs;
  uint32_t nsecs;
  bool operator==(const Time &t) const {
    return secs == t.secs && nsecs == t.nsecs;
  }
  bool operator!=(const Time &t) const { return !this->operator==(t); }
};

inline std::ostream& operator<<(std::ostream& os, const Time& t) {
  os << "secs: " << t.secs << " nsecs: " << t.nsecs;
  return os;
}

// Same as ros::Duration.
struct Duration {
  uint32_t secs;
  uint32_t nsecs;

  bool operator==(const Duration &d) const {
    return secs == d.secs && nsecs == d.nsecs;
  }
  bool operator!=(const Duration &d) const { return !this->operator==(d); }
};

inline std::ostream& operator<<(std::ostream& os, const Duration& d) {
  os << "secs: " << d.secs << " nsecs: " << d.nsecs;
  return os;
}

inline uint32_t AlignSize(uint32_t s,
                          uint32_t alignment = uint32_t(sizeof(uint64_t))) {
  return (s + (alignment - 1)) & ~(alignment - 1);
}
}
