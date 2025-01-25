#pragma once

#include <filesystem>
#include "absl/status/status.h"
#include "neutron/package.h"

namespace neutron::serdes {

class Generator : public neutron::Generator {
 public:
  Generator(std::filesystem::path root, std::string runtime_path,
            std::string msg_path, std::string ns)
      : root_(std::move(root)),
        runtime_path_(std::move(runtime_path)),
        msg_path_(std::move(msg_path)),
        namespace_(std::move(ns)) {}

  absl::Status Generate(const Message &msg) override;

 private:
  absl::Status GenerateHeader(const Message &msg, std::ostream &os);
  absl::Status GenerateSource(const Message &msg, std::ostream &os);
  absl::Status GenerateEnum(const Message &msg, std::ostream &os);
  absl::Status GenerateStruct(const Message &msg, std::ostream &os);

  absl::Status GenerateSerializer(const Message &msg, std::ostream &os);
  absl::Status GenerateDeserializer(const Message &msg, std::ostream &os);
  absl::Status GenerateLength(const Message &msg, std::ostream &os);

  static std::shared_ptr<Field> ResolveField(std::shared_ptr<Field> field);
  std::string Namespace(bool prefix_colon_colon);

  std::string MessageFieldTypeName(const Message &msg,
                                   std::shared_ptr<MessageField> field);
  std::filesystem::path root_;
  std::string runtime_path_;
  std::string msg_path_;
  std::string namespace_;
};

}  // namespace neutron::serdes
