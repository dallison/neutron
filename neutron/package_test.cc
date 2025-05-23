#include "neutron/package.h"
#include <gtest/gtest.h>
#include <sstream>
#include "neutron/lex.h"

TEST(PackageTest, Scanner) {
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner({"./neutron/testdata"}));
  auto status = scanner->ParseAllMessages();
  std::cout << status << std::endl;
  ASSERT_TRUE(status.ok());
  scanner->Dump(std::cout);
}

TEST(PackageTest, OneMessageInScanner) {
  std::shared_ptr<neutron::PackageScanner> scanner(
      new neutron::PackageScanner({"./neutron/testdata/std_msgs"}));
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
  std::shared_ptr<neutron::Package> pkg =
      std::make_shared<neutron::Package>("other_msgs");
  auto msg = pkg->ParseMessage("./neutron/testdata/other_msgs/msg/Other.msg");
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
