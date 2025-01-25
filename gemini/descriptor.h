#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "absl/status/statusor.h"
#include "gemini/descriptor/Descriptor.h"
#include "gemini/syntax.h"

namespace gemini {

absl::StatusOr<descriptor::Descriptor> MakeDescriptor(const Message &msg);
absl::Status EncodeDescriptorAsHex(const descriptor::Descriptor &desc,
                                   int max_width, bool with_0x_prefix,
                                   std::ostream &os);
absl::StatusOr<descriptor::Descriptor> DecodeDescriptorFromHex(
    std::istream &is);

}  // namespace gemini
