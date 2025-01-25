#include "gemini/descriptor.h"
#include <gtest/gtest.h>
#include <sstream>
#include "gemini/package.h"

TEST(DescriptorTest, OneLine) {
  std::shared_ptr<gemini::Package> pkg =
      std::make_shared<gemini::Package>(nullptr, "other_msgs");
  auto msg = pkg->ParseMessage("./gemini/testdata/other_msgs/msg/Other.msg");
  ASSERT_TRUE(msg.ok());

  (*msg)->Dump(std::cout);

  absl::StatusOr<descriptor::Descriptor> desc = gemini::MakeDescriptor(**msg);
  ASSERT_TRUE(desc.ok());

  absl::Status s = gemini::EncodeDescriptorAsHex(*desc, 80, true, std::cout);
  ASSERT_TRUE(s.ok());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
