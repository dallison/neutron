#include "davros/zeros/runtime.h"
#include "davros/zeros/other_msgs/Other.h"
#include <gtest/gtest.h>
#include "toolbelt/hexdump.h"

using PayloadBuffer = davros::zeros::PayloadBuffer;

TEST(Runtime, FixedBuffer) {
  char *buffer = (char *)malloc(4096);

  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, other_msgs::zeros::Other::BinarySize());

  other_msgs::zeros::Other other(std::make_shared<PayloadBuffer *>(pb), pb->message);

  other.header->seq = 255;
  other.header->frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  toolbelt::Hexdump(buffer, pb->Size());
  free(buffer);
}


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
