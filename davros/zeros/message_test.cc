#include "davros/zeros/message.h"
#include <gtest/gtest.h>
#include <sstream>
#include "absl/strings/str_format.h"
#include "toolbelt/payload_buffer.h"
#include "davros/zeros/runtime.h"
#include "toolbelt/hexdump.h"

using PayloadBuffer = toolbelt::PayloadBuffer;
using BufferOffset = toolbelt::BufferOffset;
using Message = davros::zeros::Message;
using VectorHeader = toolbelt::VectorHeader;
using StringHeader = toolbelt::StringHeader;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

enum class EnumTest : uint16_t { FOO = 0xda, BAR = 0xad };

struct InnerMessage : public Message {
  InnerMessage()
      : str(offsetof(InnerMessage, str), 0),
        f(offsetof(InnerMessage, f),
          davros::zeros::AlignedOffset<uint64_t>(str.BinaryEndOffset())) {}
  explicit InnerMessage(std::shared_ptr<toolbelt::PayloadBuffer *> buffer)
      : str(offsetof(InnerMessage, str), 0),
        f(offsetof(InnerMessage, f),
          davros::zeros::AlignedOffset<uint64_t>(str.BinaryEndOffset())) {
    this->buffer = buffer;
    void *data = toolbelt::PayloadBuffer::Allocate(buffer.get(), BinarySize(), 8);
    this->absolute_binary_offset = (*buffer)->ToOffset(data);
    std::cout << "InnerMessage start: " << std::hex
              << this->absolute_binary_offset << std::dec << std::endl;
  }
  InnerMessage(std::shared_ptr<toolbelt::PayloadBuffer *> buffer, toolbelt::BufferOffset offset)
      : Message(buffer, offset),
        str(offsetof(InnerMessage, str), 0),
        f(offsetof(InnerMessage, f),
          davros::zeros::AlignedOffset<uint64_t>(str.BinaryEndOffset())) {}
  static constexpr size_t BinarySize() { return 16; }
  davros::zeros::StringField str;
  davros::zeros::Uint64Field f;
};

struct TestMessage : public Message {
  TestMessage(std::shared_ptr<toolbelt::PayloadBuffer *> buffer, toolbelt::BufferOffset offset)
      : Message(buffer, offset),
        x(offsetof(TestMessage, x), 0),
        y(offsetof(TestMessage, y),
          davros::zeros::AlignedOffset<uint64_t>(x.BinaryEndOffset())),
        s(offsetof(TestMessage, s),
          davros::zeros::AlignedOffset<toolbelt::StringHeader>(y.BinaryEndOffset())),
        m(buffer, offsetof(TestMessage, m),
          davros::zeros::AlignedOffset<uint64_t>(s.BinaryEndOffset())),
        arr(offsetof(TestMessage, arr),
            davros::zeros::AlignedOffset<uint32_t>(m.BinaryEndOffset())),
        vec(offsetof(TestMessage, vec),
            davros::zeros::AlignedOffset<uint32_t>(arr.BinaryEndOffset())),
        marr(offsetof(TestMessage, marr),
             davros::zeros::AlignedOffset<uint32_t>(vec.BinaryEndOffset())),
        sarr(offsetof(TestMessage, sarr),
             davros::zeros::AlignedOffset<uint32_t>(marr.BinaryEndOffset())),
        mvec(offsetof(TestMessage, mvec),
             davros::zeros::AlignedOffset<uint32_t>(sarr.BinaryEndOffset())),
        svec(offsetof(TestMessage, svec),
             davros::zeros::AlignedOffset<uint32_t>(mvec.BinaryEndOffset())),
        e(offsetof(TestMessage, e),
          davros::zeros::AlignedOffset<uint32_t>(svec.BinaryEndOffset())),
        earr(offsetof(TestMessage, earr),
             davros::zeros::AlignedOffset<uint32_t>(e.BinaryEndOffset())),
        evec(offsetof(TestMessage, evec),
             davros::zeros::AlignedOffset<uint32_t>(earr.BinaryEndOffset())),
        carr(offsetof(TestMessage, carr),
             davros::zeros::AlignedOffset<int8_t>(evec.BinaryEndOffset())),
        cvec(offsetof(TestMessage, cvec),
             davros::zeros::AlignedOffset<uint32_t>(carr.BinaryEndOffset())) {}
  static constexpr size_t BinarySize() {
    size_t offset = 0;  // x
    offset =
        davros::zeros::AlignedOffset<uint64_t>(offset + sizeof(uint32_t));  // y
    offset =
        davros::zeros::AlignedOffset<uint32_t>(offset + sizeof(uint64_t));  // s
    offset = davros::zeros::AlignedOffset<uint64_t>(offset +
                                                    sizeof(toolbelt::StringHeader));  // m
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + InnerMessage::BinarySize());  // arr
    offset = davros::zeros::AlignedOffset<uint32_t>(offset + sizeof(uint32_t) *
                                                                 10);  // vec
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::VectorHeader));  // marr
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + InnerMessage::BinarySize() * 5);  // sarr
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::StringHeader) * 20);  // mvec
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::VectorHeader));  // svec
    offset = davros::zeros::AlignedOffset<uint32_t>(offset +
                                                    sizeof(toolbelt::VectorHeader));  // e
    offset = davros::zeros::AlignedOffset<uint32_t>(offset +
                                                    sizeof(uint32_t));  // earr
    offset = davros::zeros::AlignedOffset<toolbelt::VectorHeader>(
        offset + sizeof(uint16_t) * 10);  // evec

    offset = davros::zeros::AlignedOffset<int8_t>(
        offset + sizeof(toolbelt::VectorHeader));  // carr

    offset = davros::zeros::AlignedOffset<int8_t>(offset +
                                                  sizeof(int8_t) * 32);  // cvec

    offset = davros::zeros::AlignedOffset<uint16_t>(
        offset + sizeof(toolbelt::VectorHeader));  // END
    return offset;

    // return 4 + 4 + 8 + 4 + 4 + InnerMessage::BinarySize() +
    //        10 * sizeof(int32_t) + sizeof(toolbelt::VectorHeader) +
    //        InnerMessage::BinarySize() * 5 + sizeof(toolbelt::BufferOffset) * 20 +
    //        sizeof(toolbelt::VectorHeader) + sizeof(toolbelt::VectorHeader) +
    //        sizeof(std::underlying_type<EnumTest>::type);
  }

  // This is for debug for when the sizes don't match the actual field offset.
  size_t XBinarySize() {
    size_t offset = 0;  // x
    offset =
        davros::zeros::AlignedOffset<uint64_t>(offset + sizeof(uint32_t));  // y
    std::cout << offset << std::endl;
    std::cout << "y @" << y.BinaryOffset() << std::endl;
    offset =
        davros::zeros::AlignedOffset<uint32_t>(offset + sizeof(uint64_t));  // s
    std::cout << offset << std::endl;
    std::cout << "s @" << s.BinaryOffset() << " " << s.BinaryEndOffset()
              << std::endl;
    offset = davros::zeros::AlignedOffset<uint64_t>(offset +
                                                    sizeof(toolbelt::StringHeader));  // m
    std::cout << offset << std::endl;
    std::cout << "m @" << m.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + InnerMessage::BinarySize());  // arr
    std::cout << offset << std::endl;
    std::cout << "arr @" << arr.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(offset + sizeof(uint32_t) *
                                                                 10);  // vec
    std::cout << offset << std::endl;
    std::cout << "vec @" << vec.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::VectorHeader));  // marr
    std::cout << offset << std::endl;
    std::cout << "marr @" << marr.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + InnerMessage::BinarySize() * 5);  // sarr
    std::cout << offset << std::endl;
    std::cout << "sarr @" << sarr.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::StringHeader) * 20);  // mvec
    std::cout << offset << std::endl;
    std::cout << "mvec @" << mvec.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::VectorHeader));  // svec
    std::cout << offset << std::endl;
    std::cout << "svec @" << svec.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(offset +
                                                    sizeof(toolbelt::VectorHeader));  // e
    std::cout << offset << std::endl;
    std::cout << "earr @" << e.BinaryOffset() << std::endl;
    offset = davros::zeros::AlignedOffset<uint32_t>(offset +
                                                    sizeof(uint32_t));  // earr
    std::cout << offset << std::endl;
    std::cout << "e @" << earr.BinaryOffset() << std::endl;

    offset = davros::zeros::AlignedOffset<toolbelt::VectorHeader>(
        offset + sizeof(uint16_t) * 10);  // evec

    std::cout << offset << std::endl;
    std::cout << "evec @" << evec.BinaryOffset() << std::endl;

    offset = davros::zeros::AlignedOffset<int8_t>(
        offset + sizeof(toolbelt::VectorHeader));  // carr
    std::cout << offset << std::endl;
    std::cout << "carr @" << carr.BinaryOffset() << std::endl;

    offset = davros::zeros::AlignedOffset<int8_t>(offset +
                                                  sizeof(int8_t) * 32);  // cvec
    std::cout << offset << std::endl;
    std::cout << "cvec @" << cvec.BinaryOffset() << std::endl;

    offset = davros::zeros::AlignedOffset<uint32_t>(
        offset + sizeof(toolbelt::VectorHeader));  // END
    std::cout << offset << std::endl;
    return offset;
  }

  davros::zeros::Uint32Field x;
  davros::zeros::Uint64Field y;
  davros::zeros::StringField s;
  davros::zeros::MessageField<InnerMessage> m;
  davros::zeros::PrimitiveArrayField<int32_t, 10> arr;
  davros::zeros::PrimitiveVectorField<int32_t> vec;
  davros::zeros::MessageArrayField<InnerMessage, 5> marr;
  davros::zeros::StringArrayField<20> sarr;
  davros::zeros::MessageVectorField<InnerMessage> mvec;
  davros::zeros::StringVectorField svec;
  davros::zeros::EnumField<EnumTest> e;
  davros::zeros::EnumArrayField<EnumTest, 10> earr;
  davros::zeros::EnumVectorField<EnumTest> evec;
  davros::zeros::PrimitiveArrayField<int8_t, 32> carr;
  davros::zeros::PrimitiveVectorField<int8_t> cvec;
};

#pragma clang diagnostic pop

TEST(MessageTest, Basic) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
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
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;

  msg.s = "hello world";

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.s;
  ASSERT_EQ("hello world", s);
}

TEST(MessageTest, Enum) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  msg.e = EnumTest::BAR;

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  EnumTest e = msg.e;
  ASSERT_EQ(EnumTest::BAR, e);
}

TEST(MessageTest, Message) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;
  msg.s = "now is the time for all good men to come to the aid of the party";

  // NOTE use of -> here because you can't overload the . operator.
  msg.m->str = "hello world";

  // Uses the 'operator M&' overload in the MessageField to get access
  // to the actual message type.
  InnerMessage &m = msg.m;
  m.f = 0xaaaaaaaaaaaaaaaa;

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.m->str;
  ASSERT_EQ("hello world", s);

  ASSERT_EQ(0xaaaaaaaaaaaaaaaa, uint64_t(msg.m->f));
}

TEST(MessageTest, Array) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 10; i++) {
    msg.arr[i] = i + 1;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int x = 1;
  for (auto &i : msg.arr) {
    ASSERT_EQ(x, i);
    x++;
  }
}

TEST(MessageTest, CharArray) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 32; i++) {
    msg.carr[i] = 'A' + i;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int8_t x = 'A';
  for (auto &i : msg.carr) {
    ASSERT_EQ(x, i);
    x++;
  }
}

TEST(MessageTest, MessageArray) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);
  std::cout << "message size " << std::dec << TestMessage::BinarySize()
            << std::dec << std::endl;

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 5; i++) {
    pb->Dump(std::cout);
    toolbelt::Hexdump(pb, pb->hwm);
    auto &im = msg.marr[i];
    im.str = absl::StrFormat("dave-%d", i);
    im.f = i * 2;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int x = 0;
  for (auto &m : msg.marr) {
    std::string s = absl::StrFormat("dave-%d", x);
    std::string_view ms = m.str;
    ASSERT_EQ(s, ms);
    ASSERT_EQ(x * 2, uint64_t(m.f));
    x++;
  }
}

TEST(MessageTest, StringArray) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  for (int i = 0; i < 20; i++) {
    std::string ss = absl::StrFormat("dave-%d", i);
    msg.sarr[i] = ss;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int x = 0;
  for (auto &ms : msg.sarr) {
    std::string s = absl::StrFormat("dave-%d", x);
    std::string_view sv = ms;
    ASSERT_EQ(s, sv);
    x++;
  }
}

TEST(MessageTest, EnumArray) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 10; i++) {
    msg.earr[i] = EnumTest::BAR;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  for (auto &i : msg.earr) {
    ASSERT_EQ(EnumTest::BAR, i);
  }
}

TEST(MessageTest, BasicVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  msg.vec.push_back(0xffee);

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(1, msg.vec.size());
  ASSERT_EQ(0xffee, msg.vec[0]);
}

TEST(MessageTest, VectorExpansion) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 10; i++) {
    msg.arr[i] = 0xda;
  }
  for (int i = 0; i < 10; i++) {
    msg.vec.push_back(i + 1);
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(10, msg.vec.size());
  int x = 1;
  for (auto &i : msg.vec) {
    ASSERT_EQ(x, i);
    x++;
  }
  ASSERT_EQ(16, msg.vec.capacity());
}

TEST(MessageTest, VectorResize) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 10; i++) {
    msg.arr[i] = 0xda;
  }
  msg.vec.resize(10);
  ASSERT_EQ(10, msg.vec.size());
  for (int i = 0; i < 10; i++) {
    msg.vec[i] = i + 1;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int x = 1;
  for (auto &i : msg.vec) {
    ASSERT_EQ(x, i);
    x++;
  }
  ASSERT_EQ(10, msg.vec.capacity());

  // Push one more.
  msg.vec.push_back(100);
  ASSERT_EQ(11, msg.vec.size());
  ASSERT_EQ(100, uint32_t(msg.vec[10]));
  ASSERT_EQ(20, msg.vec.capacity());
}

TEST(MessageTest, BasicMessageVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  InnerMessage m(msg.buffer);
  m.f = 0xffee;

  msg.mvec.push_back(std::move(m));

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(1, msg.mvec.size());
  InnerMessage &mv = msg.mvec[0];
  ASSERT_EQ(0xffee, uint64_t(mv.f));
}

TEST(MessageTest, ExtendedMessageVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  for (int i = 0; i < 10; i++) {
    InnerMessage m(msg.buffer);
    m.f = i + 1;
    msg.mvec.push_back(std::move(m));
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(10, msg.mvec.size());
  int i = 1;
  for (auto &m : msg.mvec) {
    uint64_t f = m->f;
    ASSERT_EQ(i, f);
    i++;
  }
}

TEST(MessageTest, MessageVectorReserve) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  std::cout << "message size: " << TestMessage::BinarySize() << std::endl;
  std::cout << "message end "
            << (void *)(pb->ToAddress<char>(pb->message) +
                        TestMessage::BinarySize())
            << std::endl;
  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  msg.XBinarySize();
  std::cout << "sarr end: "
            << (void *)(pb->ToAddress<char>(msg.sarr.BinaryEndOffset()) +
                        pb->message)
            << std::endl;

  pb->Dump(std::cout);

  toolbelt::Hexdump(pb, 400);
  msg.mvec.reserve(20);
  ASSERT_EQ(20, msg.mvec.capacity());

  toolbelt::Hexdump(pb, pb->hwm);

  for (int i = 0; i < 10; i++) {
    InnerMessage m(msg.buffer);
    m.f = i + 1;
    msg.mvec.push_back(std::move(m));
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(10, msg.mvec.size());
  int i = 1;
  for (auto &m : msg.mvec) {
    uint64_t f = m->f;
    ASSERT_EQ(i, f);
    i++;
  }
}

TEST(MessageTest, BasicStringVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  msg.svec.push_back("foobar");

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(1, msg.svec.size());
  std::string_view sv = msg.svec[0];
  ASSERT_EQ("foobar", sv);
}

TEST(MessageTest, StringVectorExpansion) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  for (int i = 0; i < 20; i++) {
    msg.svec.push_back(absl::StrFormat("foobar-%d", i));
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(20, msg.svec.size());
  int index = 0;
  for (std::string_view sv : msg.svec) {
    auto s = absl::StrFormat("foobar-%d", index);
    ASSERT_EQ(s, sv);
    index++;
  }
}

TEST(MessageTest, EnumVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 30; i++) {
    msg.evec.push_back((i & 1) ? EnumTest::FOO : EnumTest::BAR);
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(30, msg.evec.size());
  int index = 0;
  for (auto e : msg.evec) {
    ASSERT_EQ((index & 1) ? EnumTest::FOO : EnumTest::BAR, e);
    index++;
  }
}

TEST(MessageTest, CharVector) {
  char *buffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, TestMessage::BinarySize());

  TestMessage msg(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 100; i++) {
    msg.cvec.push_back('A' + i);
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int8_t x = 'A';
  for (auto &i : msg.cvec) {
    ASSERT_EQ(x, i);
    x++;
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
