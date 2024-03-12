#include "daros/package.h"
#include "absl/strings/str_format.h"
#include "daros/lex.h"
#include "daros/syntax.h"
#include <filesystem>
#include <fstream>

namespace daros {

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

absl::Status PackageScanner::ParseAllMessagesFrom(std::filesystem::path path) {
  // The root points to the directory of the package.  Inside there will
  // be a directory called 'msg' (ROS convention for some reason)
  // and in there are the .msg files.
  for (auto &dir : std::filesystem::directory_iterator(path)) {
    if (std::filesystem::is_directory(dir)) {
      // For a directory called "msg", parse all the files with the suffix
      // ".msg".
      if (dir.path().filename().string() == "msg") {
        auto package = std::make_shared<Package>(shared_from_this(),
                                                 path.filename().string());
        packages_[package->Name()] = package;

        for (auto &file : std::filesystem::directory_iterator(dir)) {
          if (file.path().extension() == ".msg") {
            std::ifstream in(file.path());
            if (!in) {
              return absl::InternalError(
                  absl::StrFormat("Unable to open message file %s", path));
            }
            LexicalAnalyzer lex(file.path().string(), in);
            std::string msg_name = file.path().stem().string();
            auto msg = std::make_shared<Message>(package, msg_name);

            if (absl::Status status = msg->Parse(lex); !status.ok()) {
              return status;
            }
            package->AddMessage(msg);
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

std::shared_ptr<Message>
PackageScanner::FindMessage(const std::string package_name,
                            const std::string &msg_name) {
  auto it = packages_.find(package_name);
  if (it == packages_.end()) {
    return nullptr;
  }
  return it->second->FindMessage(msg_name);
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
  for (auto & [ name, msg ] : messages_) {
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

} // namespace daros