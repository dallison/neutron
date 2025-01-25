#include "neutron/serdes/gen.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(GenTest, HeaderMessage) {
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner({"./neutron/testdata"}));
  auto status = scanner->ParseAllMessages();
  ASSERT_TRUE(status.ok());

  neutron::serdes::Generator gen("/tmp", "", "", "");
  std::shared_ptr<neutron::Package> package = scanner->FindPackage("std_msgs");
  ASSERT_NE(nullptr, package);
  for (auto & [ mname, msg ] : package->Messages()) {
    absl::Status s = msg->Generate(gen);
    std::cerr << s << std::endl;
    ASSERT_TRUE(s.ok());
  }
}

TEST(GenTest, OtherMessage) {
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner({"./neutron/testdata"}));
  auto status = scanner->ParseAllMessages();
  ASSERT_TRUE(status.ok());

  neutron::serdes::Generator gen("/tmp", "foo", "bar", "");
  std::shared_ptr<neutron::Package> package = scanner->FindPackage("other_msgs");
  ASSERT_NE(nullptr, package);
  for (auto & [ mname, msg ] : package->Messages()) {
    absl::Status s = msg->Generate(gen);
    ASSERT_TRUE(s.ok());
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
