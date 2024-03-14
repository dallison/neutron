#pragma once

#include "davros/package.h"
#include <filesystem>
#include "absl/status/status.h"

namespace davros::zerocopy {

class Generator : public davros::Generator {
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

