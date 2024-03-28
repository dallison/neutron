#pragma once

#include "davros/package.h"
#include <filesystem>
#include "absl/status/status.h"

namespace davros::zeros {

class Generator : public davros::Generator {
public:
  Generator(std::filesystem::path root, std::string runtime_path,
            std::string msg_path)
      : root_(std::move(root)), runtime_path_(std::move(runtime_path)),
        msg_path_(std::move(msg_path)) {}

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

  std::filesystem::path root_;
  std::string runtime_path_;
  std::string msg_path_;
};


}

