#pragma once

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include <memory>
#include <filesystem>

namespace daros {

class Message;

class PackageScanner;

class Package {
public:
  Package(std::shared_ptr<PackageScanner> scanner, std::string name) : scanner_(scanner), name_(std::move(name)) {}

  std::shared_ptr<Message> FindMessage(const std::string& name);

  void AddMessage(std::shared_ptr<Message> msg);
  void Dump(std::ostream& os);

  const std::string Name() const { return name_; }

  absl::Status ResolveMessages(std::shared_ptr<PackageScanner> scanner);

private:
  std::shared_ptr<PackageScanner> scanner_;
  std::string name_ = "";
  absl::flat_hash_map<std::string, std::shared_ptr<Message>> messages_;
};

class PackageScanner : public std::enable_shared_from_this<PackageScanner> {
public:
  PackageScanner(std::vector<std::filesystem::path> roots) : roots_(std::move(roots)) {}

  absl::Status ParseAllMessages();
  void Dump(std::ostream& os);

  std::shared_ptr<Message> FindMessage(const std::string package_name, const std::string& msg_name);

private:
  absl::Status ParseAllMessagesFrom(std::filesystem::path path);

  std::vector<std::filesystem::path> roots_;
  absl::flat_hash_map<std::string, std::shared_ptr<Package>> packages_;
};

}
