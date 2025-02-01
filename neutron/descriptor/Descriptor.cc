#include "neutron/descriptor/Descriptor.h"
namespace descriptor {
absl::Status Descriptor::SerializeToArray(char* addr, size_t len, bool compact) const {
  neutron::serdes::Buffer buffer(addr, len);
  return SerializeToBuffer(buffer, compact);
}

absl::Status Descriptor::DeserializeFromArray(const char* addr, size_t len, bool compact) {
  neutron::serdes::Buffer buffer(const_cast<char*>(addr), len);
  return DeserializeFromBuffer(buffer, compact);
}

absl::Status Descriptor::SerializeToBuffer(neutron::serdes::Buffer& buffer, bool compact) const {
  if (compact) {
    return WriteCompactToBuffer(buffer);
  }
  return WriteToBuffer(buffer);
}

absl::Status Descriptor::WriteToBuffer(neutron::serdes::Buffer& buffer) const {
  if (absl::Status status = buffer.Write(this->package); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->imports); !status.ok()) return status;
  if (absl::Status status = buffer.Write(uint32_t(this->fields.size())); !status.ok()) return status;
  for (auto& m : this->fields) {
    if (absl::Status status = m.WriteToBuffer(buffer); !status.ok()) return status;
  }
  return absl::OkStatus();
}

absl::Status Descriptor::WriteCompactToBuffer(neutron::serdes::Buffer& buffer) const {
  if (absl::Status status = buffer.WriteCompact(this->package); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->imports); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(uint32_t(this->fields.size())); !status.ok()) return status;
  for (auto& m : this->fields) {
    if (absl::Status status = m.WriteCompactToBuffer(buffer); !status.ok()) return status;
  }
  return absl::OkStatus();
}

absl::Status Descriptor::DeserializeFromBuffer(neutron::serdes::Buffer& buffer, bool compact) {
  if (compact) {
    return ReadCompactFromBuffer(buffer);
  }
  return ReadFromBuffer(buffer);
}

absl::Status Descriptor::ReadFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = buffer.Read(this->package); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->imports); !status.ok()) return status;
  {
    int32_t size;
    if (absl::Status status = buffer.Read(size); !status.ok()) return status;
    for (int32_t i = 0; i < size; i++) {
      descriptor::Field tmp;
      if (absl::Status status = tmp.ReadFromBuffer(buffer); !status.ok()) return status;
      this->fields.push_back(std::move(tmp));
    }
  }
  return absl::OkStatus();
}

absl::Status Descriptor::ReadCompactFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = buffer.ReadCompact(this->package); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->imports); !status.ok()) return status;
  {
    int32_t size;
    if (absl::Status status = buffer.ReadCompact(size); !status.ok()) return status;
    for (int32_t i = 0; i < size; i++) {
      descriptor::Field tmp;
      if (absl::Status status = tmp.ReadCompactFromBuffer(buffer); !status.ok()) return status;
      this->fields.push_back(std::move(tmp));
    }
  }
  return absl::OkStatus();
}

size_t Descriptor::SerializedSize() const {
  size_t length = 0;
  length += 4 + this->package.size();
  length += 4 + this->name.size();
  length += 4;
  for (auto& s : this->imports) {
    length += 4 + s.size();
  }
  length += 4;
  for (auto& m : this->fields) {
    length += m.SerializedSize();
  }
  return length;
}

size_t Descriptor::CompactSerializedSize() const {
  size_t length = 0;
  length +=  neutron::serdes::Leb128Size(this->package);
  length +=  neutron::serdes::Leb128Size(this->name);
  length +=  neutron::serdes::Leb128Size(this->imports);
  length +=  neutron::serdes::Leb128Size(this->fields.size());
  for (auto& m : this->fields) {
    length += m.CompactSerializedSize();
  }
  return length;
}

absl::Status Descriptor::Expand(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest) {
  if (absl::Status status = src.Expand<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand(std::vector<std::string>(), dest); !status.ok()) return status;
  {
    uint32_t size;
    if (absl::Status status = src.ReadUnsignedLeb128(size); !status.ok()) return status;
    if (absl::Status status = dest.Write(size); !status.ok()) return status;
    for (size_t i = 0; i < size_t(size); i++) {
      if (absl::Status status = descriptor::Field::Expand(src, dest); !status.ok())
        return status;
    }
  }
  return absl::OkStatus();
}

absl::Status Descriptor::Compact(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest) {
  if (absl::Status status = src.Compact<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact(std::vector<std::string>(), dest); !status.ok()) return status;
  {
    uint32_t size;
    if (absl::Status status = src.Read(size); !status.ok()) return status;
    dest.WriteUnsignedLeb128(size);
    for (size_t i = 0; i < size_t(size); i++) {
      if (absl::Status status = descriptor::Field::Compact(src, dest); !status.ok())
        return status;
    }
  }
  return absl::OkStatus();
}

  bool Descriptor::operator==(const Descriptor& m) const {
  if (this->package != m.package) return false;
  if (this->name != m.name) return false;
  if (this->imports != m.imports) return false;
  if (this->fields != m.fields) return false;
  return true;
}

std::string Descriptor::DebugString() const {
  std::stringstream s;
  s << *this;
  return s.str();
}
}    // namespace descriptor
