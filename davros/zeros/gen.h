#pragma once

#include "davros/package.h"
#include <filesystem>
#include "absl/status/status.h"

namespace davros::zeros {

class Generator : public davros::Generator {
public:
  Generator(std::filesystem::path root, std::string runtime_path,
            std::string msg_path, std::string ns)
      : root_(std::move(root)), runtime_path_(std::move(runtime_path)),
        msg_path_(std::move(msg_path)), namespace_(std::move(ns)) {}

  absl::Status Generate(const Message& msg) override;

private:
  absl::Status GenerateHeader(const Message& msg, std::ostream& os);
  absl::Status GenerateEnum(const Message &msg, std::ostream &os);
  absl::Status GenerateStruct(const Message &msg, std::ostream &os);
  absl::Status GenerateFieldInitializers(const Message& msg, std::ostream& os, const char* sep = " : ");
  absl::Status GenerateDefaultConstructor(const Message& msg, std::ostream& os);
  absl::Status GenerateEmbeddedConstructor(const Message& msg, std::ostream& os);
  absl::Status GenerateNonEmbeddedConstructor(const Message& msg, std::ostream& os);
  absl::Status GenerateBinarySize(const Message& msg, std::ostream& os);
  absl::Status GenerateStructStreamer(const Message& msg, std::ostream& os);
  absl::Status GenerateEnumStreamer(const Message& msg, std::ostream& os);

  absl::Status GenerateSource(const Message& msg, std::ostream& os);
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


}

