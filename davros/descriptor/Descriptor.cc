#include "davros/descriptor/Descriptor.h"
namespace descriptor {
absl::Status Descriptor::SerializeToArray(char* addr, size_t len) const {
  davros::serdes::Buffer buffer(addr, len);
  return SerializeToBuffer(buffer);
}

absl::Status Descriptor::DeserializeFromArray(const char* addr, size_t len) {
  davros::serdes::Buffer buffer(const_cast<char*>(addr), len);
  return DeserializeFromBuffer(buffer);
}

absl::Status Descriptor::SerializeToBuffer(
    davros::serdes::Buffer& buffer) const {
  if (absl::Status status = buffer.Write(this->package); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->name); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->imports); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(uint32_t(this->fields.size()));
      !status.ok())
    return status;
  for (auto& m : this->fields) {
    if (absl::Status status = m.SerializeToBuffer(buffer); !status.ok())
      return status;
  }
  return absl::OkStatus();
}

absl::Status Descriptor::DeserializeFromBuffer(davros::serdes::Buffer& buffer) {
  if (absl::Status status = buffer.Read(this->package); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->name); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->imports); !status.ok())
    return status;
  int32_t size;
  if (absl::Status status = buffer.Read(size); !status.ok()) return status;
  for (int32_t i = 0; i < size; i++) {
    if (absl::Status status = this->fields[i].DeserializeFromBuffer(buffer);
        !status.ok())
      return status;
  }
  return absl::OkStatus();
}

size_t Descriptor::SerializedLength() const {
  size_t length = 0;
  length += 4 + this->package.size();
  length += 4 + this->name.size();
  length += 4 + this->imports.size() * sizeof(std::string);
  length += 4 + this->fields.size() * sizeof(descriptor::Field);
  return length;
}

bool Descriptor::operator==(const Descriptor& m) const {
  if (this->package != m.package) return false;
  if (this->name != m.name) return false;
  if (this->imports != m.imports) return false;
  if (this->fields != m.fields) return false;
  return true;
}
}  // namespace descriptor
