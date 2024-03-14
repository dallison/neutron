#pragma once

#include "absl/status/status.h"
#include "daros/package.h"
#include <filesystem>

namespace daros::serdes {

class Generator : public daros::Generator {
public:
  Generator(std::filesystem::path root, std::string runtime_path,
            std::string msg_path)
      : root_(std::move(root)), runtime_path_(std::move(runtime_path)),
        msg_path_(std::move(msg_path)) {}

  absl::Status Generate(const Message &msg) override;

private:
  absl::Status GenerateHeader(const Message &msg, std::ostream &os);
  absl::Status GenerateSource(const Message &msg, std::ostream &os);

  std::filesystem::path root_;
  std::string runtime_path_;
  std::string msg_path_;
};

} // namespace daros::serdes
