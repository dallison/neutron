// Copyright 2024 David Allison
// All Rights Reserved
// See LICENSE file for licensing information.

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "neutron/package.h"
#include "neutron/serdes/gen.h"
#include "neutron/zeros/gen.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

ABSL_FLAG(bool, all, false, "Generate all messages in given directories");
ABSL_FLAG(std::string, runtime_path, "", "Path for neutron runtime");
ABSL_FLAG(std::string, msg_path, "", "Path for generated messages");
ABSL_FLAG(std::string, out, "", "Where to put the output files");
ABSL_FLAG(std::vector<std::string>, imports, {},
          "Comma separated list of paths for imported messages");
ABSL_FLAG(bool, ros, false, "Generate regular serializable ROS messages");
ABSL_FLAG(bool, zeros, false, "Generate zero-copy ROS messages");
ABSL_FLAG(std::string, add_namespace, "",
          "Add a namespace to the message classes");

void GenerateSerialization(const std::vector<std::filesystem::path> &files) {
  if (absl::GetFlag(FLAGS_all)) {
    std::shared_ptr<neutron::PackageScanner> scanner(
        new neutron::PackageScanner(files));
    auto status = scanner->ParseAllMessages();
    if (!status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
    neutron::serdes::Generator gen(
        absl::GetFlag(FLAGS_out), absl::GetFlag(FLAGS_runtime_path),
        absl::GetFlag(FLAGS_msg_path), absl::GetFlag(FLAGS_add_namespace));
    for (auto & [ pname, package ] : scanner->Packages()) {
      for (auto & [ mname, msg ] : package->Messages()) {
        absl::Status s = msg->Generate(gen);
        if (!status.ok()) {
          std::cerr << status << std::endl;
          exit(1);
        }
      }
    }
    exit(0);
  }

  std::vector<std::string> imports = absl::GetFlag(FLAGS_imports);
  std::vector<std::filesystem::path> roots;
  for (auto &dir : imports) {
    roots.push_back(dir);
  }
  for (auto &file : files) {
    roots.push_back(file.parent_path().parent_path());
  }

  // Generate for particular set of message files.
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner(roots));

  auto parse = [scanner](std::string file)
      -> absl::StatusOr<std::shared_ptr<neutron::Message>> {
    std::filesystem::path path(file);
    std::filesystem::path dir = path.parent_path();
    if (dir.filename() == "msg") {
      dir = dir.parent_path();
    }
    std::string package_name = dir.filename();
    std::shared_ptr<neutron::Package> pkg = scanner->FindPackage(package_name);
    if (pkg == nullptr) {
      pkg = std::make_shared<neutron::Package>(package_name);
      scanner->AddPackage(pkg);
    }
    auto msg = pkg->ParseMessage(file);
    if (!msg.ok()) {
      return msg.status();
    }
    return *msg;
  };

  if (absl::Status status = scanner->ScanForMessages(); !status.ok()) {
    std::cerr << status << std::endl;
    exit(1);
  }

  std::vector<std::shared_ptr<neutron::Message>> messages;
  for (auto &file : files) {
    absl::StatusOr<std::shared_ptr<neutron::Message>> msg = parse(file);
    if (!msg.ok()) {
      std::cerr << msg.status() << std::endl;
      exit(1);
    }
    messages.push_back(*msg);
  }

  // Resolve the messages.
  for (auto & [ name, package ] : scanner->Packages()) {
    if (absl::Status status = package->ResolveMessages(scanner); !status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
  }

  for (auto &msg : messages) {
    neutron::serdes::Generator gen(
        absl::GetFlag(FLAGS_out), absl::GetFlag(FLAGS_runtime_path),
        absl::GetFlag(FLAGS_msg_path), absl::GetFlag(FLAGS_add_namespace));
    absl::Status s = msg->Generate(gen);
    if (!s.ok()) {
      std::cerr << s << std::endl;
      exit(1);
    }
  }
  exit(0);
}

void GenerateZeroCopy(const std::vector<std::filesystem::path> &files) {
  if (absl::GetFlag(FLAGS_all)) {
    std::shared_ptr<neutron::PackageScanner> scanner(
        new neutron::PackageScanner(files));
    auto status = scanner->ParseAllMessages();
    if (!status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
    neutron::zeros::Generator gen(
        absl::GetFlag(FLAGS_out), absl::GetFlag(FLAGS_runtime_path),
        absl::GetFlag(FLAGS_msg_path), absl::GetFlag(FLAGS_add_namespace));
    for (auto & [ pname, package ] : scanner->Packages()) {
      for (auto & [ mname, msg ] : package->Messages()) {
        absl::Status s = msg->Generate(gen);
        if (!status.ok()) {
          std::cerr << status << std::endl;
          exit(1);
        }
      }
    }
    exit(0);
  }

  std::vector<std::string> imports = absl::GetFlag(FLAGS_imports);
  std::vector<std::filesystem::path> roots;
  for (auto &dir : imports) {
    roots.push_back(dir);
  }
  for (auto &file : files) {
    roots.push_back(file.parent_path().parent_path());
  }

  // Generate for particular set of message files.
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner(roots));

  auto parse = [scanner](std::string file)
      -> absl::StatusOr<std::shared_ptr<neutron::Message>> {
    std::filesystem::path path(file);
    std::filesystem::path dir = path.parent_path();
    if (dir.filename() == "msg") {
      dir = dir.parent_path();
    }
    std::string package_name = dir.filename();
    std::shared_ptr<neutron::Package> pkg = scanner->FindPackage(package_name);
    if (pkg == nullptr) {
      pkg = std::make_shared<neutron::Package>(package_name);
      scanner->AddPackage(pkg);
    }
    auto msg = pkg->ParseMessage(file);
    if (!msg.ok()) {
      return msg.status();
    }
    return *msg;
  };

  if (absl::Status status = scanner->ScanForMessages(); !status.ok()) {
    std::cerr << status << std::endl;
    exit(1);
  }

  std::vector<std::shared_ptr<neutron::Message>> messages;
  for (auto &file : files) {
    absl::StatusOr<std::shared_ptr<neutron::Message>> msg = parse(file);
    if (!msg.ok()) {
      std::cerr << msg.status() << std::endl;
      exit(1);
    }
    messages.push_back(*msg);
  }

  // Resolve the messages.
  for (auto & [ name, package ] : scanner->Packages()) {
    if (absl::Status status = package->ResolveMessages(scanner); !status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
  }

  for (auto &msg : messages) {
    neutron::zeros::Generator gen(
        absl::GetFlag(FLAGS_out), absl::GetFlag(FLAGS_runtime_path),
        absl::GetFlag(FLAGS_msg_path), absl::GetFlag(FLAGS_add_namespace));
    absl::Status s = msg->Generate(gen);
    if (!s.ok()) {
      std::cerr << s << std::endl;
      exit(1);
    }
  }
  exit(0);
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  std::vector<std::filesystem::path> files;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      continue;
    }
    files.push_back(argv[i]);
  }

  if (files.empty()) {
    std::cerr << "Supply either directories or message files\n";
    exit(1);
  }

  if (absl::GetFlag(FLAGS_ros)) {
    GenerateSerialization(files);
  }

  if (absl::GetFlag(FLAGS_zeros)) {
    GenerateZeroCopy(files);
  }

  std::cerr << "Tell me what to generate: --ros or --zeros\n";
  exit(1);
}