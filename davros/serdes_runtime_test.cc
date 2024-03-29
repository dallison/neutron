#include "davros/serdes/runtime.h"
#include "davros/serdes/other_msgs/Other.h"
#include <gtest/gtest.h>
#include "toolbelt/hexdump.h"

TEST(Runtime, FixedBuffer) {
  other_msgs::Other other;
  other.header.seq = 255;
  other.header.frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  size_t size = other.SerializedLength();
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

  size_t size = other.SerializedLength();
  std::cerr << "length " << size << std::endl;
  davros::serdes::Buffer buffer;
  auto status = other.SerializeToBuffer(buffer);
  std::cerr << "serialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer.data(), size);

  davros::serdes::Buffer buffer2(buffer.data(), buffer.size());
  other_msgs::Other read;
  status = read.DeserializeFromBuffer(buffer2);
  std::cerr << "deserialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer2.data(), size);

  ASSERT_EQ(other, read);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
