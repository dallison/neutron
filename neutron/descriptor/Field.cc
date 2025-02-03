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
  if (absl::Status status = buffer.Write(this->index); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->type); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->array_size); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->msg_package); !status.ok()) return status;
  if (absl::Status status = buffer.Write(this->msg_name); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::WriteCompactToBuffer(neutron::serdes::Buffer& buffer, bool internal) const {
  if (absl::Status status = buffer.WriteCompact(this->index); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->type); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->array_size); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->msg_package); !status.ok()) return status;
  if (absl::Status status = buffer.WriteCompact(this->msg_name); !status.ok()) return status;
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
  if (absl::Status status = buffer.Read(this->index); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->type); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->array_size); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->msg_package); !status.ok()) return status;
  if (absl::Status status = buffer.Read(this->msg_name); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::ReadCompactFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = buffer.ReadCompact(this->index); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->name); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->type); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->array_size); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->msg_package); !status.ok()) return status;
  if (absl::Status status = buffer.ReadCompact(this->msg_name); !status.ok()) return status;
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
  acc.Accumulate(this->index);
  acc.Accumulate(this->name);
  acc.Accumulate(this->type);
  acc.Accumulate(this->array_size);
  acc.Accumulate(this->msg_package);
  acc.Accumulate(this->msg_name);
}

size_t Field::CompactSerializedSize() const {
  neutron::serdes::SizeAccumulator acc;
  CompactSerializedSize(acc);
  acc.Close();
  return acc.Size();
}

absl::Status Field::Expand(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest) {
  if (absl::Status status = src.Expand<int16_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<uint8_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<int16_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Expand<std::string>(dest); !status.ok()) return status;
  return absl::OkStatus();
}

absl::Status Field::Compact(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest, bool internal) {
  if (absl::Status status = src.Compact<int16_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<uint8_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<int16_t>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<std::string>(dest); !status.ok()) return status;
  if (absl::Status status = src.Compact<std::string>(dest); !status.ok()) return status;
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
