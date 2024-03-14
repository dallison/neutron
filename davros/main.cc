// Copyright 2024 David Allison
// All Rights Reserved
// See LICENSE file for licensing information.

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "davros/serdes/gen.h"
#include "davros/package.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

ABSL_FLAG(bool, all, false, "Generate all messages in given directories");
ABSL_FLAG(std::string, runtime_path, "", "Path for davros runtime");
ABSL_FLAG(std::string, msg_path, "", "Path for generated messages");
ABSL_FLAG(std::string, out, "", "Where to put the output files");

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
    for (auto &[pname, package ]: scanner->Packages()) {
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
  for (auto &file : files) {
    std::filesystem::path path(file);
    std::filesystem::path dir = path.parent_path();
    if (dir.filename() == "msg") {
      dir = dir.parent_path();
    }
    std::string package_name = dir.filename();
    std::shared_ptr<davros::Package> pkg =
        std::make_shared<davros::Package>(nullptr, package_name);
    auto msg = pkg->ParseMessage(file);
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
}