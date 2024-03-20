#pragma once

#include "absl/status/statusor.h"
#include "davros/descriptor/Descriptor.h"
#include "davros/syntax.h"
#include <iostream>
#include <string>
#include <vector>

namespace davros {

absl::StatusOr<descriptor::Descriptor> MakeDescriptor(const Message &msg);
absl::Status EncodeDescriptorAsHex(const descriptor::Descriptor &desc,
                                   int max_width, bool with_0x_prefix,
                                   std::ostream &os);
absl::StatusOr<descriptor::Descriptor> DecodeDescriptorFromHex(std::istream &is);

} // namespace davros
