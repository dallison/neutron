#include "daros/lex.h"
#include "daros/package.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(PackageTest, Manual) {
  std::shared_ptr<daros::PackageScanner> scanner(new daros::PackageScanner({"./daros/testdata"}));
  auto status = scanner->ParseAllMessages();
  std::cout << status << std::endl;
  ASSERT_TRUE(status.ok());
  scanner->Dump(std::cout);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
