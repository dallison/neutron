#include "neutron/package.h"
#include <filesystem>
#include <fstream>
#include "absl/strings/str_format.h"
#include "neutron/md5.h"

namespace neutron {

absl::Status PackageScanner::ParseAllMessages() {
  for (auto &root : roots_) {
    if (absl::Status status = ParseAllMessagesFrom(root); !status.ok()) {
      return status;
    }
    for (auto & [ name, package ] : packages_) {
      if (absl::Status status = package->ResolveMessages(shared_from_this());
          !status.ok()) {
        return status;
      }
    }
  }
  return absl::OkStatus();
}

absl::Status PackageScanner::ScanForMessages() {
  for (auto &root : roots_) {
    if (absl::Status status = ScanForMessagesFrom(root); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

absl::Status PackageScanner::ParseAllMessagesFrom(std::filesystem::path path) {
  // The root points to the directory of the package.  Inside there will
  // be a directory called 'msg' (ROS convention for some reason)
  // and in there are the .msg files.
  for (auto &dir : std::filesystem::directory_iterator(path)) {
    if (std::filesystem::is_directory(dir)) {
      // For a directory called "msg", parse all the files with the suffix
      // ".msg".
      if (dir.path().filename().string() == "msg") {
        auto package = std::make_shared<Package>(weak_from_this(),
                                                 path.filename().string());
        AddPackage(package);

        for (auto &file : std::filesystem::directory_iterator(dir)) {
          if (file.path().extension() == ".msg") {
            absl::StatusOr<std::shared_ptr<Message>> msg =
                package->ParseMessage(file.path());
            if (!msg.ok()) {
              return msg.status();
            }
          }
        }
      } else {
        if (absl::Status status = ParseAllMessagesFrom(dir.path());
            !status.ok()) {
          return status;
        }
      }
    }
  }
  return absl::OkStatus();
}

absl::Status PackageScanner::ScanForMessagesFrom(std::filesystem::path path) {
  for (auto &dir : std::filesystem::directory_iterator(path)) {
    if (std::filesystem::is_directory(dir)) {
      // For a directory called "msg", parse all the files with the suffix
      // ".msg".
      if (dir.path().filename().string() == "msg") {
        std::string package_name = path.filename().string();
        auto package =
            std::make_shared<Package>(weak_from_this(), package_name);
        AddPackage(package);
        for (auto &file : std::filesystem::directory_iterator(dir)) {
          if (file.path().extension() == ".msg") {
            std::string full_message_name =
                package_name + "/" + file.path().stem().string();
            discovered_message_files_[full_message_name] = file.path();
          }
        }
      } else {
        if (absl::Status status = ScanForMessagesFrom(dir.path());
            !status.ok()) {
          return status;
        }
      }
    }
  }
  return absl::OkStatus();
}

std::shared_ptr<Message> PackageScanner::FindMessage(
    const std::string package_name, const std::string &msg_name) {
  auto it = packages_.find(package_name);
  if (it == packages_.end()) {
    return nullptr;
  }
  return it->second->FindMessage(msg_name);
}

absl::StatusOr<std::shared_ptr<Message>> PackageScanner::ResolveImport(
    const std::string &package_name, const std::string &msg_name) {
  auto package = FindPackage(package_name);
  if (package == nullptr) {
    return absl::InternalError(
        absl::StrFormat("Cannot find package %s", package_name));
  }

  auto it = discovered_message_files_.find(package_name + "/" + msg_name);
  if (it == discovered_message_files_.end()) {
    return absl::InternalError(
        absl::StrFormat("Cannot find message %s/%s", package_name, msg_name));
  }
  absl::StatusOr<std::shared_ptr<Message>> msg =
      package->ParseMessage(it->second);
  if (!msg.ok()) {
    return msg.status();
  }
  return msg;
}

absl::StatusOr<std::shared_ptr<Message>> Package::ParseMessage(
    std::filesystem::path file) {

  absl::StatusOr<std::string> md5 = CalculateMD5Checksum(file.string());
  if (!md5.ok()) {
    return absl::InternalError(
        absl::StrFormat("Unable to calculate MD5 checksum for %s: %s", file.string(), md5.status().ToString()));
  }

  std::ifstream in(file);
  if (!in) {
    return absl::InternalError(
        absl::StrFormat("Unable to open message file %s", file.string()));
  }
  LexicalAnalyzer lex(file.string(), in);
  std::string msg_name = file.stem().string();
  auto msg = std::make_shared<Message>(weak_from_this(), msg_name, std::move(*md5));

  if (absl::Status status = msg->Parse(lex); !status.ok()) {
    return status;
  }
  AddMessage(msg);
  return msg;
}

void Package::AddMessage(std::shared_ptr<Message> msg) {
  messages_[msg->Name()] = msg;
}

std::shared_ptr<Message> Package::FindMessage(const std::string &name) {
  auto it = messages_.find(name);
  if (it == messages_.end()) {
    return nullptr;
  }
  return it->second;
}

void Package::Dump(std::ostream &os) {
  for (auto & [ name, msg ] : messages_) {
    os << "**** Message " << msg->Package()->Name() << "/" << msg->Name()
       << std::endl;
    msg->Dump(os);
    os << "****\n";
  }
}

absl::Status Package::ResolveMessages(std::shared_ptr<PackageScanner> scanner) {
  // Copy the messages we know about as resolving a message will modify
  // the messages_ map, invalidating the iterator.
  std::vector<std::shared_ptr<Message>> messages;
  for (auto & [ name, msg ] : messages_) {
    messages.push_back(msg);
  }
  for (auto &msg : messages) {
    if (absl::Status status = msg->Resolve(scanner); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

void PackageScanner::Dump(std::ostream &os) {
  for (auto & [ name, p ] : packages_) {
    p->Dump(os);
  }
}

}  // namespace neutron