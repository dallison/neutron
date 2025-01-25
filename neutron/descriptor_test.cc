#include "neutron/descriptor.h"
#include <gtest/gtest.h>
#include <sstream>
#include "neutron/package.h"

TEST(DescriptorTest, OneLine) {
  std::shared_ptr<neutron::Package> pkg =
      std::make_shared<neutron::Package>(nullptr, "other_msgs");
  auto msg = pkg->ParseMessage("./neutron/testdata/other_msgs/msg/Other.msg");
  ASSERT_TRUE(msg.ok());

  (*msg)->Dump(std::cout);

  absl::StatusOr<descriptor::Descriptor> desc = neutron::MakeDescriptor(**msg);
  ASSERT_TRUE(desc.ok());

  absl::Status s = neutron::EncodeDescriptorAsHex(*desc, 80, true, std::cout);
  ASSERT_TRUE(s.ok());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
