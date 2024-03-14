#include "daros/lex.h"
#include "daros/package.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(PackageTest, Scanner) {
  std::shared_ptr<daros::PackageScanner> scanner(
      new daros::PackageScanner({"./daros/testdata"}));
  auto status = scanner->ParseAllMessages();
  std::cout << status << std::endl;
  ASSERT_TRUE(status.ok());
  scanner->Dump(std::cout);
}

TEST(PackageTest, OneMessageInScanner) {
  std::shared_ptr<daros::PackageScanner> scanner(
      new daros::PackageScanner({"./daros/testdata/std_msgs"}));
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
  std::shared_ptr<daros::Package> pkg =
      std::make_shared<daros::Package>(nullptr, "other_msgs");
  auto msg = pkg->ParseMessage("./daros/testdata/other_msgs/msg/Other.msg");
  ASSERT_TRUE(msg.ok());

  const char *expected = R"(std_msgs/Header header
string bar
int32 value
int64[] arr
int8[10] farr
)";
  std::stringstream out;
  (*msg)->Dump(out);
  ASSERT_EQ(expected, out.str());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
