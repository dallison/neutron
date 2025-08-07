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
  if (absl::Status status = Write(buffer, this->package); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->imports); !status.ok()) return status;
  if (absl::Status status = Write(buffer, uint32_t(this->fields.size())); !status.ok()) return status;
  for (auto& m : this->fields) {
    if (absl::Status status = m.WriteToBuffer(buffer); !status.ok()) return status;
  }
  return absl::OkStatus();
}

absl::Status Descriptor::WriteCompactToBuffer(neutron::serdes::Buffer& buffer, bool internal) const {
  if (absl::Status status = WriteCompact(buffer, this->package); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->imports); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, uint32_t(this->fields.size())); !status.ok()) return status;
  for (auto& m : this->fields) {
    if (absl::Status status = m.WriteCompactToBuffer(buffer, true); !status.ok()) return status;
  }
  if (!internal) {
     return buffer.FlushZeroes();
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
  if (absl::Status status = Read(buffer, this->package); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->imports); !status.ok()) return status;
  {
    int32_t size;
    if (absl::Status status = Read(buffer, size); !status.ok()) return status;
    for (int32_t i = 0; i < size; i++) {
      descriptor::Field tmp;
      if (absl::Status status = tmp.ReadFromBuffer(buffer); !status.ok()) return status;
      this->fields.push_back(std::move(tmp));
    }
  }
  return absl::OkStatus();
}

absl::Status Descriptor::ReadCompactFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = ReadCompact(buffer, this->package); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->imports); !status.ok()) return status;
  {
    int32_t size;
    if (absl::Status status = ReadCompact(buffer, size); !status.ok()) return status;
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

void Descriptor::CompactSerializedSize(neutron::serdes::SizeAccumulator& acc) const {
  Accumulate(acc, this->package);
  Accumulate(acc, this->name);
  Accumulate(acc, this->imports);
  Accumulate(acc, fields.size());
  for (auto& m : this->fields) {
    m.CompactSerializedSize(acc);
  }
}

size_t Descriptor::CompactSerializedSize() const {
  neutron::serdes::SizeAccumulator acc;
  CompactSerializedSize(acc);
  acc.Close();
  return acc.Size();
}

absl::Status Descriptor::Expand(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest) {
  if (absl::Status status = ExpandField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, std::vector<std::string>(), dest); !status.ok()) return status;
  {
    uint32_t size;
    if (absl::Status status = src.ReadUnsignedLeb128(size); !status.ok()) return status;
    if (absl::Status status = Write(dest, size); !status.ok()) return status;
    for (size_t i = 0; i < size_t(size); i++) {
      if (absl::Status status = descriptor::Field::Expand(src, dest); !status.ok())
        return status;
    }
  }
  return absl::OkStatus();
}

absl::Status Descriptor::Compact(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest, bool internal) {
  if (absl::Status status = CompactField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, std::vector<std::string>(), dest); !status.ok()) return status;
  {
    uint32_t size;
    if (absl::Status status = Read(src, size); !status.ok()) return status;
    if (absl::Status status = dest.WriteUnsignedLeb128(size); !status.ok()) return status;
    for (size_t i = 0; i < size_t(size); i++) {
      if (absl::Status status = descriptor::Field::Compact(src, dest, true); !status.ok())
        return status;
    }
  }
  if (!internal) {
    return dest.FlushZeroes();
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
