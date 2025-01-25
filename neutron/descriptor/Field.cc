#include "neutron/descriptor/Field.h"
namespace descriptor {
absl::Status Field::SerializeToArray(char* addr, size_t len) const {
  neutron::serdes::Buffer buffer(addr, len);
  return SerializeToBuffer(buffer);
}

absl::Status Field::DeserializeFromArray(const char* addr, size_t len) {
  neutron::serdes::Buffer buffer(const_cast<char*>(addr), len);
  return DeserializeFromBuffer(buffer);
}

absl::Status Field::SerializeToBuffer(neutron::serdes::Buffer& buffer) const {
  if (absl::Status status = buffer.Write(this->index); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->name); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->type); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->array_size); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->msg_package); !status.ok())
    return status;
  if (absl::Status status = buffer.Write(this->msg_name); !status.ok())
    return status;
  return absl::OkStatus();
}

absl::Status Field::DeserializeFromBuffer(neutron::serdes::Buffer& buffer) {
  if (absl::Status status = buffer.Read(this->index); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->name); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->type); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->array_size); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->msg_package); !status.ok())
    return status;
  if (absl::Status status = buffer.Read(this->msg_name); !status.ok())
    return status;
  return absl::OkStatus();
}

size_t Field::SerializedLength() const {
  size_t length = 0;
  length += sizeof(this->index);
  length += 4 + this->name.size();
  length += sizeof(this->type);
  length += sizeof(this->array_size);
  length += 4 + this->msg_package.size();
  length += 4 + this->msg_name.size();
  return length;
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
}  // namespace descriptor
