#pragma once

#include "daros/package.h"
#include <filesystem>
#include "absl/status/status.h"

namespace daros::zerocopy {

class Generator : public daros::Generator {
public:
  Generator(std::filesystem::path root) : root_(std::move(root)) {}

  absl::Status Generate(const Message& msg) override;

private:
  absl::Status GenerateHeader(const Message& msg, std::ostream& os);
  absl::Status Serialize(const Message& msg, std::ostream& os);

  std::filesystem::path root_;
  std::filesystem::path header_;
  std::filesystem::path src_;
};


}

