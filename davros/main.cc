// Copyright 2024 David Allison
// All Rights Reserved
// See LICENSE file for licensing information.

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "davros/package.h"
#include "davros/serdes/gen.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

ABSL_FLAG(bool, all, false, "Generate all messages in given directories");
ABSL_FLAG(std::string, runtime_path, "", "Path for davros runtime");
ABSL_FLAG(std::string, msg_path, "", "Path for generated messages");
ABSL_FLAG(std::string, out, "", "Where to put the output files");
ABSL_FLAG(std::vector<std::string>, imports, {},
          "Comma separated list of paths for imported messages");

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

  if (absl::GetFlag(FLAGS_all)) {
    std::shared_ptr<davros::PackageScanner> scanner(
        new davros::PackageScanner(files));
    auto status = scanner->ParseAllMessages();
    if (!status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
    davros::serdes::Generator gen(absl::GetFlag(FLAGS_out),
                                  absl::GetFlag(FLAGS_runtime_path),
                                  absl::GetFlag(FLAGS_msg_path));
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

  // Generate for particular set of message files.
  std::shared_ptr<davros::PackageScanner> scanner(
      new davros::PackageScanner({}));

  auto parse = [scanner](std::string file)
      -> absl::StatusOr<std::shared_ptr<davros::Message>> {
    std::filesystem::path path(file);
    std::filesystem::path dir = path.parent_path();
    if (dir.filename() == "msg") {
      dir = dir.parent_path();
    }
    std::string package_name = dir.filename();
    std::shared_ptr<davros::Package> pkg = scanner->FindPackage(package_name);
    if (pkg == nullptr) {
      pkg = std::make_shared<davros::Package>(nullptr, package_name);
      scanner->AddPackage(pkg);
    }
    auto msg = pkg->ParseMessage(file);
    if (!msg.ok()) {
      return msg.status();
    }
    return *msg;
  };

  std::vector<std::string> imports = absl::GetFlag(FLAGS_imports);
  // Parse all the imports.  These are passed in the -imports= flag and
  // specify all the .msg files that are needed to parse this message
  for (auto &file : imports) {
    if (absl::StatusOr<std::shared_ptr<davros::Message>> msg = parse(file);
        !msg.ok()) {
      std::cerr << msg.status() << std::endl;
      exit(1);
    }
  }

  for (auto &file : files) {
    absl::StatusOr<std::shared_ptr<davros::Message>> msg = parse(file);
    if (!msg.ok()) {
      std::cerr << msg.status() << std::endl;
      exit(1);
    }
    davros::serdes::Generator gen(absl::GetFlag(FLAGS_out),
                                  absl::GetFlag(FLAGS_runtime_path),
                                  absl::GetFlag(FLAGS_msg_path));
    absl::Status s = (*msg)->Generate(gen);
    if (!s.ok()) {
      std::cerr << s << std::endl;
      exit(1);
    }
  }
  // Resolve the messages.
  for (auto & [ name, package ] : scanner->Packages()) {
    if (absl::Status status = package->ResolveMessages(scanner); !status.ok()) {
      std::cerr << status << std::endl;
      exit(1);
    }
  }
}