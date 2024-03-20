#include "davros/descriptor.h"
#include "davros/package.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(DescriptorTest, OneLine) {
    std::shared_ptr<davros::Package> pkg =
      std::make_shared<davros::Package>(nullptr, "other_msgs");
  auto msg = pkg->ParseMessage("./davros/testdata/other_msgs/msg/Other.msg");
  ASSERT_TRUE(msg.ok());

  (*msg)->Dump(std::cout);

  absl::StatusOr<descriptor::Descriptor> desc = davros::MakeDescriptor(**msg);
  ASSERT_TRUE(desc.ok());

  absl::Status s = davros::EncodeDescriptorAsHex(*desc, 80, true, std::cout);
  ASSERT_TRUE(s.ok());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
