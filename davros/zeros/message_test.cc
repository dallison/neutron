#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include "davros/zeros/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>
#include <sstream>

using PayloadBuffer = davros::zeros::PayloadBuffer;
using BufferOffset = davros::zeros::BufferOffset;
using Message = davros::zeros::Message;

struct TestMessage : public Message {
  TestMessage(PayloadBuffer *buffer)
      : Message(buffer), x(sizeof(Message), buffer->message + 0),
        y(sizeof(Message) + sizeof(davros::zeros::Uint32Field),
          buffer->message + 8),
        s(sizeof(Message) + sizeof(davros::zeros::Uint32Field) +
              sizeof(davros::zeros::Uint64Field),
          buffer->message + 16) {}
  static uint32_t FixedSize() { return 20; }
  davros::zeros::Uint32Field x;
  davros::zeros::Uint64Field y;
  davros::zeros::StringField s;
};

TEST(MessageTest, Basic) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(pb);
  msg.x = 1234;
  msg.y = 0xffff;

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  uint32_t x = msg.x;
  ASSERT_EQ(1234, x);

  uint64_t y = msg.y;
  ASSERT_EQ(0xffff, y);
}

TEST(MessageTest, String) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(pb);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;

  msg.s = "hello world";

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.s;
  ASSERT_EQ("hello world", s);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
