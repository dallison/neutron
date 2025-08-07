#include "neutron/descriptor/Field.h"
namespace descriptor {
absl::Status Field::SerializeToArray(char* addr, size_t len, bool compact) const {
  neutron::serdes::Buffer buffer(addr, len);
  return SerializeToBuffer(buffer, compact);
}

absl::Status Field::DeserializeFromArray(const char* addr, size_t len, bool compact) {
  neutron::serdes::Buffer buffer(const_cast<char*>(addr), len);
  return DeserializeFromBuffer(buffer, compact);
}

absl::Status Field::SerializeToBuffer(neutron::serdes::Buffer& buffer, bool compact) const {
  if (compact) {
    return WriteCompactToBuffer(buffer);
  }
  return WriteToBuffer(buffer);
}

absl::Status Field::WriteToBuffer(neutron::serdes::Buffer& buffer) const {
  if (absl::Status status = Write(buffer, this->index); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->type); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->array_size); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->msg_package); !status.ok()) return status;
  if (absl::Status status = Write(buffer, this->msg_name); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::WriteCompactToBuffer(neutron::serdes::Buffer& buffer, bool internal) const {
  if (absl::Status status = WriteCompact(buffer, this->index); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->type); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->array_size); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->msg_package); !status.ok()) return status;
  if (absl::Status status = WriteCompact(buffer, this->msg_name); !status.ok()) return status;
  if (!internal) {
     return buffer.FlushZeroes();
  }
  return absl::OkStatus();
}

absl::Status Field::DeserializeFromBuffer(neutron::serdes::Buffer& buffer, bool compact) {
  if (compact) {
    return ReadCompactFromBuffer(buffer);
  }
  return ReadFromBuffer(buffer);
}

absl::Status Field::ReadFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = Read(buffer, this->index); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->type); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->array_size); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->msg_package); !status.ok()) return status;
  if (absl::Status status = Read(buffer, this->msg_name); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::ReadCompactFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = ReadCompact(buffer, this->index); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->name); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->type); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->array_size); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->msg_package); !status.ok()) return status;
  if (absl::Status status = ReadCompact(buffer, this->msg_name); !status.ok()) return status;
  return absl::OkStatus();
}

size_t Field::SerializedSize() const {
  size_t length = 0;
  length += sizeof(this->index);
  length += 4 + this->name.size();
  length += sizeof(this->type);
  length += sizeof(this->array_size);
  length += 4 + this->msg_package.size();
  length += 4 + this->msg_name.size();
  return length;
}

void Field::CompactSerializedSize(neutron::serdes::SizeAccumulator& acc) const {
  Accumulate(acc, this->index);
  Accumulate(acc, this->name);
  Accumulate(acc, this->type);
  Accumulate(acc, this->array_size);
  Accumulate(acc, this->msg_package);
  Accumulate(acc, this->msg_name);
}

size_t Field::CompactSerializedSize() const {
  neutron::serdes::SizeAccumulator acc;
  CompactSerializedSize(acc);
  acc.Close();
  return acc.Size();
}

absl::Status Field::Expand(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest) {
  if (absl::Status status = ExpandField(src, dest, int16_t{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, uint8_t{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, int16_t{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = ExpandField(src, dest, std::string{}); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::Compact(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest, bool internal) {
  if (absl::Status status = CompactField(src, dest, int16_t{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, uint8_t{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, int16_t{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, std::string{}); !status.ok()) return status;
  if (absl::Status status = CompactField(src, dest, std::string{}); !status.ok()) return status;
  if (!internal) {
    return dest.FlushZeroes();
  }
  return absl::OkStatus();
}

  bool Field::operator==(const Field& m) const {
  if (this->index != m.index) return false;
  if (this->name != m.name) return false;
  if (this->type != m.type) return false;
  if (this->array_size != m.array_size) return false;
  if (this->msg_package != m.msg_package) return false;
  if (this->msg_name != m.msg_name) return false;
  return true;
}

std::string Field::DebugString() const {
  std::stringstream s;
  s << *this;
  return s.str();
}
}    // namespace descriptor
