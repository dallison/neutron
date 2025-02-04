#pragma once

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <iostream>

namespace descriptor {
  struct Descriptor;
}

namespace neutron::serdes {

class Buffer;
class SerdesMessage;

struct MessageMetadata {
  absl::Span<const char> (*get_descriptor)();
  absl::Status (*serialize_to_array)(const SerdesMessage& msg, char* addr, size_t len, bool compact);
  absl::Status (*deserialize_from_array)(SerdesMessage& msg, const char* addr, size_t len, bool compact);
  absl::Status (*serialize_to_buffer)(const SerdesMessage& msg, Buffer& buffer, bool compact);
  absl::Status (*deserialize_from_buffer)(SerdesMessage& msg,Buffer& buffer, bool compact);
  size_t (*serialized_size)(const SerdesMessage& msg);
  size_t (*compact_serialized_size)(const SerdesMessage& msg);
  std::string (*debug_string)(const SerdesMessage& msg);
  void (*stream_to)(const SerdesMessage& msg, std::ostream& os);
  absl::Status (*compact)(const Buffer& src, Buffer& dest);
  absl::Status (*expand)(const Buffer& src, Buffer& dest);
};

class MessageMux {
  public:
  static MessageMux& Instance() {
    static MessageMux instance;
    return instance;
  }

  absl::StatusOr<absl::Span<const char>> GetDescriptor(const std::string& name);
  absl::Status SerializeToArray(const std::string& name, const SerdesMessage& msg, char* addr, size_t len, bool compact) ;
  absl::Status DeserializeFromArray(const std::string& name,  SerdesMessage& msg, const char* addr, size_t len, bool compact);
  absl::Status SerializeToBuffer(const std::string& name, const SerdesMessage& msg, neutron::serdes::Buffer& buffer, bool compact) ;
  absl::Status DeserializeFromBuffer(const std::string& name, SerdesMessage& msg, neutron::serdes::Buffer& buffer, bool compact);
  absl::StatusOr<size_t> SerializedSize(const std::string& name, const SerdesMessage& msg);
  absl::StatusOr<size_t> CompactSerializedSize(const std::string& name, const SerdesMessage& msg);
  absl::StatusOr<std::string> DebugString(const std::string& name, const SerdesMessage& msg);
  absl::Status StreamTo(const std::string& name, const SerdesMessage& msg, std::ostream& os);
  absl::Status Compact(const std::string& name, const Buffer& src, Buffer& dest);
  absl::Status Expand(const std::string& name, const Buffer& src, Buffer& dest);
  
  void Register(const std::string& name, MessageMetadata metadata) ;

private:
  absl::flat_hash_map<std::string, MessageMetadata> mux_;
};

}

