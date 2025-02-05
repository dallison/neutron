#pragma once

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <string>

namespace neutron {
absl::StatusOr<std::string> CalculateMD5Checksum(const std::string &filePath);
}
