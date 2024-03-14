#include "davros/serdes/gen.h"
#include <gtest/gtest.h>
#include <sstream>


TEST(GenTest, Scanner) {
  std::shared_ptr<davros::PackageScanner> scanner(new davros::PackageScanner({"./davros/testdata/std_msgs"}));
  auto status = scanner->ParseAllMessages();
  ASSERT_TRUE(status.ok());

  davros::serdes::Generator gen(".");
  for (auto& [name, package] : scanner->Packages()) {
    for (auto& [mname, msg] : package->Messages()) {
      absl::Status s = msg->Generate(gen);
      ASSERT_TRUE(s.ok());
    }
  }
}


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}


