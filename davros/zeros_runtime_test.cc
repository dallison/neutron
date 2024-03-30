#include "davros/zeros/other_msgs/Other.h"
#include "davros/serdes/other_msgs/Other.h"
#include "davros/zeros/runtime.h"
#include "davros/serdes/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>

using PayloadBuffer = davros::zeros::PayloadBuffer;

TEST(Runtime, FixedBuffer) {
  char *buffer = (char *)malloc(4096);

  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb,
                                     other_msgs::zeros::Other::BinarySize());

  other_msgs::zeros::Other other(std::make_shared<PayloadBuffer *>(pb),
                                 pb->message);

  other.header->seq = 255;
  other.header->frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  toolbelt::Hexdump(buffer, pb->Size());

  size_t length = other.SerializedSize();
  std::cout << length << std::endl;
  char *serdes_buffer = (char*)malloc(length);

  auto status = other.SerializeToArray(serdes_buffer, length);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(serdes_buffer, length);

  // Deserialize from buffer and print it:
  other_msgs::Other sother;
  status = sother.DeserializeFromArray(serdes_buffer, length);
  ASSERT_TRUE(status.ok());
  std::cout << sother.DebugString();
  free(serdes_buffer);
  free(buffer);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
