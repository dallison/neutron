#include "neutron/serdes/descriptor/Descriptor.h"
#include "neutron/serdes/other_msgs/Other.h"
#include "neutron/serdes/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>

TEST(Runtime, FixedBuffer) {
  other_msgs::Other other;
  other.header.seq = 255;
  other.header.frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  size_t size = other.SerializedSize();
  std::cerr << "length " << size << std::endl;
  char buffer[256];
  memset(buffer, 0xda, sizeof(buffer));
  auto status = other.SerializeToArray(buffer, sizeof(buffer));
  std::cerr << "serialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer, size);

  other_msgs::Other read;
  status = read.DeserializeFromArray(buffer, size);
  std::cerr << "deserialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer, size);

  ASSERT_EQ(other, read);
}

TEST(Runtime, DynamicBuffer) {
  other_msgs::Other other;
  other.header.seq = 255;
  other.header.frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  size_t size = other.SerializedSize();
  std::cerr << "length " << size << std::endl;
  neutron::serdes::Buffer buffer;
  auto status = other.SerializeToBuffer(buffer);
  std::cerr << "serialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer.data(), size);

  neutron::serdes::Buffer buffer2(buffer.data(), buffer.size());
  other_msgs::Other read;
  status = read.DeserializeFromBuffer(buffer2);
  std::cerr << "deserialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer2.data(), size);

  ASSERT_EQ(other, read);
}

TEST(Runtime, Descriptor) {
  descriptor::Descriptor desc;
  std::cerr << sizeof(other_msgs::Other::_descriptor) << std::endl;
  auto status =
      desc.DeserializeFromArray((char *)other_msgs::Other::_descriptor,
                                sizeof(other_msgs::Other::_descriptor));
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cerr << desc.DebugString() << std::endl;
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
