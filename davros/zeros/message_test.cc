#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include "davros/zeros/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>
#include <sstream>

using PayloadBuffer = davros::zeros::PayloadBuffer;
using BufferOffset = davros::zeros::BufferOffset;
using Message = davros::zeros::Message;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

struct InnerMessage : public Message {
  InnerMessage(std::shared_ptr<PayloadBuffer *> buffer, BufferOffset offset)
      : Message(buffer, offset), str(offsetof(InnerMessage, str), 0) {}
 davros::zeros::StringField str;
};

struct TestMessage : public Message {
  TestMessage(std::shared_ptr<PayloadBuffer *> buffer, BufferOffset offset)
      : Message(buffer, offset), x(offsetof(TestMessage, x), 0),
        y(offsetof(TestMessage, y), 8), s(offsetof(TestMessage, s), 16),
        m(buffer, offsetof(TestMessage, m), 20) {}
  static uint32_t FixedSize() { return 24; }
  davros::zeros::Uint32Field x;
  davros::zeros::Uint64Field y;
  davros::zeros::StringField s;
  davros::zeros::MessageField<InnerMessage> m;
};
#pragma clang diagnostic pop


TEST(MessageTest, Basic) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
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

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;

  msg.s = "hello world";

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.s;
  ASSERT_EQ("hello world", s);
}

TEST(MessageTest, Message) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;

  msg.m->str = "hello world";

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.m->str;
  ASSERT_EQ("hello world", s);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
