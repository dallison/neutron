#include "davros/lex.h"
#include "davros/package.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(PackageTest, Scanner) {
  std::shared_ptr<davros::PackageScanner> scanner(
      new davros::PackageScanner({"./davros/testdata"}));
  auto status = scanner->ParseAllMessages();
  std::cout << status << std::endl;
  ASSERT_TRUE(status.ok());
  scanner->Dump(std::cout);
}

TEST(PackageTest, OneMessageInScanner) {
  std::shared_ptr<davros::PackageScanner> scanner(
      new davros::PackageScanner({"./davros/testdata/std_msgs"}));
  auto status = scanner->ParseAllMessages();
  const char *expected = R"(**** Message std_msgs/Header
uint32 seq
time stamp
string frame_id
****
)";

  std::cout << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  scanner->Dump(out);
  ASSERT_EQ(expected, out.str());
}

TEST(PackageTest, SingleMessage) {
  std::shared_ptr<davros::Package> pkg =
      std::make_shared<davros::Package>(nullptr, "other_msgs");
  auto msg = pkg->ParseMessage("./davros/testdata/other_msgs/msg/Other.msg");
  ASSERT_TRUE(msg.ok());

  const char *expected = R"(int32 CONST = 1234
string SCONST = string constant
std_msgs/Header header
string bar
int32 value
int64[] arr
int8[10] farr
Enum en
)";
  std::stringstream out;
  (*msg)->Dump(out);
  ASSERT_EQ(expected, out.str());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
