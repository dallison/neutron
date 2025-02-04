#include <gtest/gtest.h>
#include "neutron/serdes/other_msgs/Other.h"
#include "neutron/serdes/runtime.h"
#include "neutron/serdes/test_msgs/All.h"
#include "neutron/zeros/other_msgs/Other.h"
#include "neutron/zeros/runtime.h"
#include "neutron/zeros/test_msgs/All.h"
#include "toolbelt/hexdump.h"

using PayloadBuffer = toolbelt::PayloadBuffer;

TEST(Runtime, ZeroToSerdes) {
  char *buffer = (char *)malloc(4096);
          
  other_msgs::zeros::Other other = other_msgs::zeros::Other::CreateMutable(buffer, 4096);
  other.header->seq = 255;
  other.header->frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  other.en = other_msgs::zeros::Enum::FOO;

  toolbelt::Hexdump(other.Buffer(), other.Size());

  size_t length = other.SerializedSize();
  std::cout << length << std::endl;
  char *serdes_buffer = (char *)malloc(length);

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

TEST(Runtime, Pipeline) {
  // Build serializable message and serialize it.
  other_msgs::Other sother;

  sother.header.seq = 255;
  sother.header.frame_id = "frame1";
  sother.bar = "foobar";
  sother.value = 1234;
  size_t length1 = sother.SerializedSize();
  char *serdes_buffer1 = (char *)malloc(length1);

  auto status = sother.SerializeToArray(serdes_buffer1, length1);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());

  // serdes_buffer1 contains the serialized sother message.
  std::cout << "Serialized message\n";
  toolbelt::Hexdump(serdes_buffer1, length1);

  // Build a zero-copy message from the serialized message.
  char *zbuffer = (char *)malloc(4096);
  toolbelt::PayloadBuffer *pb = new (zbuffer) toolbelt::PayloadBuffer(4096);
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb,
                                     other_msgs::zeros::Other::BinarySize());

  std::cout << "binary size: " << other_msgs::zeros::Other::BinarySize()
            << std::endl;
  other_msgs::zeros::Other zother(std::make_shared<toolbelt::PayloadBuffer *>(pb),
                                  pb->message);

  status = zother.DeserializeFromArray(serdes_buffer1, length1);
  ASSERT_TRUE(status.ok());
  // zbuffer contains the zero copy message.
  std::cout << "Zero copy message\n";
  toolbelt::Hexdump(zbuffer, pb->Size());
  pb->Dump(std::cout);

  // Serialize zero-copy to serdes buffer.
  size_t length2 = zother.SerializedSize();
  char *serdes_buffer2 = (char *)malloc(length2);

  status = zother.SerializeToArray(serdes_buffer2, length2);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "Serialized Zero copy message\n";
  toolbelt::Hexdump(serdes_buffer2, length2);

  // serdes_buffer2 contains the serialized zother message.

  // Deserialize into another message.
  other_msgs::Other sother2;
  status = sother2.DeserializeFromArray(serdes_buffer2, length2);
  ASSERT_TRUE(status.ok());

  // Compare original message and the one that passed through the serialization
  // pipeline.
  ASSERT_EQ(sother, sother2);

  free(serdes_buffer1);
  free(serdes_buffer2);
  free(zbuffer);
}

TEST(Runtime, AllZeroToSerdes) {
  char *buffer = (char *)malloc(4096);

  toolbelt::PayloadBuffer *pb = new (buffer) toolbelt::PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  toolbelt::PayloadBuffer::AllocateMainMessage(&pb, test_msgs::zeros::All::BinarySize());

  test_msgs::zeros::All all(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);

  // Set the fields.
  all.i8 = 1;
  all.ui8 = 2;
  all.i16 = 3;
  all.ui16 = 4;
  all.i32 = 5;
  all.ui32 = 6;
  all.i64 = 7;
  all.ui64 = 8;
  all.f32 = 9;
  all.f64 = 10;
  all.s = "dave";
  all.t = {45, 67};
  all.d = {78, 89};

  all.n->foo = 1234;
  all.n->bar = "bar";

  all.e8 = test_msgs::zeros::Enum8::X1;
  all.e16 = test_msgs::zeros::Enum16::X2;
  all.e32 = test_msgs::zeros::Enum32::X3;
  all.e64 = test_msgs::zeros::Enum64::X1;

  // Fixed size arrays.
  all.ai8[0] = 20;
  all.aui8[1] = 21;
  all.ai16[1] = 22;
  all.aui16[4] = 23;
  all.ai32[5] = 24;
  all.aui32[6] = 25;
  all.ai64[7] = 26;
  all.aui64[8] = 27;
  all.af32[2] = 28;
  all.af64[7] = 28;

  all.as[0] = "foo";
  all.as[1] = "bar";

  all.at[3] = {12, 13};
  all.ad[4] = {14, 15};

  all.an[1].foo = 16;
  all.an[1].bar = "an[1]";

  all.ae8[1] = test_msgs::zeros::Enum8::X1;
  all.ae16[2] = test_msgs::zeros::Enum16::X2;
  all.ae32[3] = test_msgs::zeros::Enum32::X3;
  all.ae64[4] = test_msgs::zeros::Enum64::X1;

  // Vectors
  all.vi8.push_back(20);
  all.vui8.push_back(21);
  all.vi16.push_back(22);
  all.vui16.push_back(23);
  all.vi32.push_back(24);
  all.vui32.push_back(25);
  all.vi64.push_back(26);
  all.vui64.push_back(27);

  all.vf32.push_back(28);
  all.vf64.push_back(28);

  all.vs.push_back("foo");
  all.vs.push_back("bar");

  all.vt.push_back({12, 13});
  all.vd.push_back({14, 15});

  test_msgs::zeros::Nested nested(all.buffer);
  nested.foo = 16;
  nested.bar = "vn";
  all.vn.push_back(std::move(nested));

  all.ve8.push_back(test_msgs::zeros::Enum8::X1);
  all.ve16.push_back(test_msgs::zeros::Enum16::X2);
  all.ve32.push_back(test_msgs::zeros::Enum32::X3);
  all.ve64.push_back(test_msgs::zeros::Enum64::X1);

  all.auto_ = 5678;
  all.virtual_ = 8765;

  toolbelt::Hexdump(buffer, pb->Size());

  size_t length = all.SerializedSize();
  std::cout << length << std::endl;
  char *serdes_buffer = (char *)malloc(length);

  auto status = all.SerializeToArray(serdes_buffer, length);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "serialized:\n";
  toolbelt::Hexdump(serdes_buffer, length);

  // Deserialize from buffer and print it:
  test_msgs::serdes::All sall;
  status = sall.DeserializeFromArray(serdes_buffer, length);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << sall.DebugString();

  ASSERT_EQ(sall.i8, 1);
  ASSERT_EQ(sall.ui8, 2);
  ASSERT_EQ(sall.i16, 3);
  ASSERT_EQ(sall.ui16, 4);
  ASSERT_EQ(sall.i32, 5);
  ASSERT_EQ(sall.ui32, 6);
  ASSERT_EQ(sall.i64, 7);
  ASSERT_EQ(sall.ui64, 8);
  ASSERT_EQ(sall.f32, 9);
  ASSERT_EQ(sall.f64, 10);
  ASSERT_EQ(sall.s, "dave");

  ASSERT_EQ(sall.e8, test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(sall.e16, test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(sall.e32, test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(sall.e64, test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(sall.n.foo, 1234);
  ASSERT_EQ(sall.n.bar, "bar");

  ASSERT_EQ(sall.ai8[0], 20);
  ASSERT_EQ(sall.aui8[1], 21);
  ASSERT_EQ(sall.ai16[1], 22);
  ASSERT_EQ(sall.aui16[4], 23);
  ASSERT_EQ(sall.ai32[5], 24);
  ASSERT_EQ(sall.aui32[6], 25);
  ASSERT_EQ(sall.ai64[7], 26);
  ASSERT_EQ(sall.aui64[8], 27);
  ASSERT_EQ(sall.af32[2], 28);
  ASSERT_EQ(sall.af64[7], 28);

  ASSERT_EQ(sall.as[0], "foo");
  ASSERT_EQ(sall.as[1], "bar");

  ASSERT_EQ(sall.at[3].secs, 12);
  ASSERT_EQ(sall.at[3].nsecs, 13);
  ASSERT_EQ(sall.ad[4].secs, 14);
  ASSERT_EQ(sall.ad[4].nsecs, 15);

  ASSERT_EQ(sall.an[1].foo, 16);
  ASSERT_EQ(sall.an[1].bar, "an[1]");

  ASSERT_EQ(sall.ae8[1], test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(sall.ae16[2], test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(sall.ae32[3], test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(sall.ae64[4], test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(sall.vi8[0], 20);
  ASSERT_EQ(sall.vui8[0], 21);
  ASSERT_EQ(sall.vi16[0], 22);
  ASSERT_EQ(sall.vui16[0], 23);
  ASSERT_EQ(sall.vi32[0], 24);
  ASSERT_EQ(sall.vui32[0], 25);
  ASSERT_EQ(sall.vi64[0], 26);
  ASSERT_EQ(sall.vui64[0], 27);
  ASSERT_EQ(sall.vf32[0], 28);
  ASSERT_EQ(sall.vf64[0], 28);

  ASSERT_EQ(sall.vs[0], "foo");
  ASSERT_EQ(sall.vs[1], "bar");

  ASSERT_EQ(sall.vt[0].secs, 12);
  ASSERT_EQ(sall.vt[0].nsecs, 13);
  ASSERT_EQ(sall.vd[0].secs, 14);
  ASSERT_EQ(sall.vd[0].nsecs, 15);

  ASSERT_EQ(sall.vn[0].foo, 16);
  ASSERT_EQ(sall.vn[0].bar, "vn");

  ASSERT_EQ(sall.ve8[0], test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(sall.ve16[0], test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(sall.ve32[0], test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(sall.ve64[0], test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(sall.auto_, 5678);
  ASSERT_EQ(sall.virtual_, 8765);

  free(serdes_buffer);
  free(buffer);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
