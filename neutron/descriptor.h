#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "absl/status/statusor.h"
#include "neutron/descriptor/Descriptor.h"
#include "neutron/syntax.h"
#include "absl/types/span.h"

namespace neutron {

absl::StatusOr<descriptor::Descriptor> MakeDescriptor(const Message &msg);
absl::Status EncodeDescriptorAsHex(const descriptor::Descriptor &desc,
                                   int max_width, bool with_0x_prefix,
                                   std::ostream &os);
absl::StatusOr<descriptor::Descriptor> DecodeDescriptor(
    const char* addr, size_t len);

absl::StatusOr<descriptor::Descriptor> DecodeDescriptor(
   absl::Span<const char> span);

std::vector<std::string> FieldNames(const descriptor::Descriptor &desc);


}  // namespace neutron
