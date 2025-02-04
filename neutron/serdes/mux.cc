#include "neutron/serdes/mux.h"

#include "neutron/serdes/runtime.h"

namespace neutron::serdes {

absl::StatusOr<absl::Span<const char>>
MessageMux::GetDescriptor(const std::string &name) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.get_descriptor();
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::SerializeToArray(const std::string &name,
                                          const SerdesMessage &msg, char *addr,
                                          size_t len, bool compact) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.serialize_to_array(msg, addr, len, compact);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::DeserializeFromArray(const std::string &name,
                                              SerdesMessage &msg, const char *addr,
                                              size_t len, bool compact) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.deserialize_from_array(msg, addr, len, compact);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::SerializeToBuffer(const std::string &name,
                                           const SerdesMessage &msg,
                                           neutron::serdes::Buffer &buffer,
                                           bool compact) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.serialize_to_buffer(msg, buffer, compact);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::DeserializeFromBuffer(const std::string &name,
                                               SerdesMessage &msg,
                                               neutron::serdes::Buffer &buffer,
                                               bool compact) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.deserialize_from_buffer(msg, buffer, compact);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::StatusOr<size_t> MessageMux::SerializedSize(const std::string &name,
                                                  const SerdesMessage &msg) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.serialized_size(msg);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::StatusOr<size_t>
MessageMux::CompactSerializedSize(const std::string &name, const SerdesMessage &msg) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.compact_serialized_size(msg);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::StatusOr<std::string> MessageMux::DebugString(const std::string &name,
                                                    const SerdesMessage &msg) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.debug_string(msg);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::StreamTo(const std::string &name, const SerdesMessage &msg,
                                  std::ostream &os) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    it->second.stream_to(msg, os);
    return absl::OkStatus();
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::Compact(const std::string &name, const Buffer &src,
                                 Buffer &dest) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.compact(src, dest);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

absl::Status MessageMux::Expand(const std::string &name, const Buffer &src,
                                Buffer &dest) {
  if (auto it = mux_.find(name); it != mux_.end()) {
    return it->second.expand(src, dest);
  }
  return absl::InternalError(absl::StrFormat("SerdesMessage %s not found", name));
}

void MessageMux::Register(const std::string &name,
                          MessageMetadata metadata) {
  mux_[name] = metadata;
}
} // namespace neutron::serdes
