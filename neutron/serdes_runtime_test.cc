#include "neutron/descriptor.h"
#include "neutron/serdes/other_msgs/Other.h"
#include "neutron/serdes/runtime.h"
#include "neutron/serdes/test_msgs/All.h"
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

TEST(Runtime, Compact) {
  other_msgs::Other other;
  other.header.seq = 255;
  other.header.frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  size_t size = other.CompactSerializedSize();
  std::cerr << "length " << size << std::endl;
  char buffer[256];
  memset(buffer, 0xda, sizeof(buffer));
  auto status = other.SerializeToArray(buffer, sizeof(buffer), true);
  std::cerr << "serialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer, size);

  other_msgs::Other read;
  status = read.DeserializeFromArray(buffer, size, true);
  std::cerr << "deserialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer, size);

  ASSERT_EQ(other, read);
}

TEST(Runtime, Expand) {
  other_msgs::Other other;
  other.header.seq = 255;
  other.header.frame_id = "frame1";

  other.bar = "foobar";
  other.value = 0x1234;

  size_t size = other.CompactSerializedSize();
  std::cerr << "length " << size << std::endl;
  char buffer[256];
  memset(buffer, 0xda, sizeof(buffer));
  auto status = other.SerializeToArray(buffer, sizeof(buffer), true);
  std::cerr << "serialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(buffer, size);

  // Expand into a new buffer
  neutron::serdes::Buffer dest;
  status = other.Expand(neutron::serdes::Buffer(buffer, size), dest);
  std::cerr << "expand " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(dest.data(), dest.size());
  std::cerr << "expanded size " << dest.size() << std::endl;

  // Now deserialized from the expanded buffer.
  other_msgs::Other read;
  status = read.DeserializeFromArray(dest.data(), dest.size(), false);
  std::cerr << "deserialize " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(dest.data(), dest.size());

  ASSERT_EQ(other, read);

  // Now compact it back and compare the original buffer.
  neutron::serdes::Buffer dest2;
  dest.Rewind();
  status = other.Compact(dest, dest2);
  std::cerr << "compact " << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(dest2.data(), dest2.size());

  ASSERT_EQ(size, dest2.size());
  toolbelt::Hexdump(buffer, size);

  ASSERT_EQ(0, memcmp(buffer, dest2.data(), size));
}

TEST(Runtime, Descriptor) {
  auto descriptor = neutron::DecodeDescriptor(other_msgs::Other::GetDescriptor());
  ASSERT_TRUE(descriptor.ok());
  std::cerr << descriptor->DebugString() << std::endl;
}

static void FillAll(test_msgs::serdes::All& all) {
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

  all.n.foo = 1234;
  all.n.bar = "baz";

  all.e8 = test_msgs::serdes::Enum8::X1;
  all.e16 = test_msgs::serdes::Enum16::X2;
  all.e32 = test_msgs::serdes::Enum32::X3;
  all.e64 = test_msgs::serdes::Enum64::X1;

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

  all.at[3] = {0xa1, 0xa2};
  all.ad[4] = {0xa3, 0xa4};

  all.an[1].foo = 16;
  all.an[1].bar = "an[1]";

  all.ae8[1] = test_msgs::serdes::Enum8::X1;
  all.ae16[2] = test_msgs::serdes::Enum16::X2;
  all.ae32[3] = test_msgs::serdes::Enum32::X3;
  all.ae64[4] = test_msgs::serdes::Enum64::X1;

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

  all.vs.push_back("foo2");
  all.vs.push_back("bar2");

  all.vt.push_back({0xb1, 0xb2});
  all.vd.push_back({0xb3, 0xb4});

  test_msgs::serdes::Nested nested;
  nested.foo = 16;
  nested.bar = "vn";
  all.vn.push_back(std::move(nested));

  all.ve8.push_back(test_msgs::serdes::Enum8::X1);
  all.ve16.push_back(test_msgs::serdes::Enum16::X2);
  all.ve32.push_back(test_msgs::serdes::Enum32::X3);
  all.ve64.push_back(test_msgs::serdes::Enum64::X1);

  all.auto_ = 5678;
  all.virtual_ = 8765;

  // Get all the the field names from the descriptor
  auto descriptor = neutron::DecodeDescriptor(test_msgs::serdes::All::GetDescriptor());
  ASSERT_TRUE(descriptor.ok());
  std::vector<std::string> field = neutron::FieldNames(*descriptor);
  for (auto& f : field) {
    std::cout << f << std::endl;
  }
  std::cout << "descriptor size: " << test_msgs::serdes::All::GetDescriptor().size() << std::endl;

}

static void CheckAll(const test_msgs::serdes::All& all) {
  ASSERT_EQ(all.i8, 1);
  ASSERT_EQ(all.ui8, 2);
  ASSERT_EQ(all.i16, 3);
  ASSERT_EQ(all.ui16, 4);
  ASSERT_EQ(all.i32, 5);
  ASSERT_EQ(all.ui32, 6);
  ASSERT_EQ(all.i64, 7);
  ASSERT_EQ(all.ui64, 8);
  ASSERT_EQ(all.f32, 9);
  ASSERT_EQ(all.f64, 10);
  ASSERT_EQ(all.s, "dave");

  ASSERT_EQ(all.e8, test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(all.e16, test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(all.e32, test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(all.e64, test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(all.n.foo, 1234);
  ASSERT_EQ(all.n.bar, "baz");

  ASSERT_EQ(all.ai8[0], 20);
  ASSERT_EQ(all.aui8[1], 21);
  ASSERT_EQ(all.ai16[1], 22);
  ASSERT_EQ(all.aui16[4], 23);
  ASSERT_EQ(all.ai32[5], 24);
  ASSERT_EQ(all.aui32[6], 25);
  ASSERT_EQ(all.ai64[7], 26);
  ASSERT_EQ(all.aui64[8], 27);
  ASSERT_EQ(all.af32[2], 28);
  ASSERT_EQ(all.af64[7], 28);

  ASSERT_EQ(all.as[0], "foo");
  ASSERT_EQ(all.as[1], "bar");

  ASSERT_EQ(all.at[3].secs, 0xa1);
  ASSERT_EQ(all.at[3].nsecs, 0xa2);
  ASSERT_EQ(all.ad[4].secs, 0xa3);
  ASSERT_EQ(all.ad[4].nsecs, 0xa4);

  ASSERT_EQ(all.an[1].foo, 16);
  ASSERT_EQ(all.an[1].bar, "an[1]");

  ASSERT_EQ(all.ae8[1], test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(all.ae16[2], test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(all.ae32[3], test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(all.ae64[4], test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(all.vi8[0], 20);
  ASSERT_EQ(all.vui8[0], 21);
  ASSERT_EQ(all.vi16[0], 22);
  ASSERT_EQ(all.vui16[0], 23);
  ASSERT_EQ(all.vi32[0], 24);
  ASSERT_EQ(all.vui32[0], 25);
  ASSERT_EQ(all.vi64[0], 26);
  ASSERT_EQ(all.vui64[0], 27);
  ASSERT_EQ(all.vf32[0], 28);
  ASSERT_EQ(all.vf64[0], 28);

  ASSERT_EQ(all.vs[0], "foo2");
  ASSERT_EQ(all.vs[1], "bar2");

  ASSERT_EQ(all.vt[0].secs, 0xb1);
  ASSERT_EQ(all.vt[0].nsecs, 0xb2);
  ASSERT_EQ(all.vd[0].secs, 0xb3);
  ASSERT_EQ(all.vd[0].nsecs, 0xb4);

  ASSERT_EQ(all.vn[0].foo, 16);
  ASSERT_EQ(all.vn[0].bar, "vn");

  ASSERT_EQ(all.ve8[0], test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(all.ve16[0], test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(all.ve32[0], test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(all.ve64[0], test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(all.auto_, 5678);
  ASSERT_EQ(all.virtual_, 8765);
}

TEST(Runtime, AllSerializeStandard) {
  test_msgs::serdes::All all;

  // Set the fields.
  FillAll(all);

  neutron::serdes::Buffer dest;

  size_t length = all.SerializedSize();
  std::cout << length << std::endl;

  auto status = all.SerializeToBuffer(dest);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "serialized:\n";
  ASSERT_EQ(length, dest.size());
  toolbelt::Hexdump(dest.data(), length);

  // Deserialize from buffer and check it:
  test_msgs::serdes::All all2;
  status = all2.DeserializeFromArray(dest.data(), length);
  ASSERT_TRUE(status.ok());
  // std::cout << all.DebugString();
  CheckAll(all2);
}

TEST(Runtime, AllSerializeCompact) {
  test_msgs::serdes::All all;

  // Set the fields.
  FillAll(all);

  neutron::serdes::Buffer dest;

  size_t length = all.CompactSerializedSize();
  std::cout << length << std::endl;

  auto status = all.SerializeToBuffer(dest, true);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "serialized:\n";
  ASSERT_EQ(length, dest.size());
  toolbelt::Hexdump(dest.data(), length);

  // Deserialize from buffer and check it:
  test_msgs::serdes::All all2;
  status = all2.DeserializeFromArray(dest.data(), length, true);
  ASSERT_TRUE(status.ok());
  // std::cout << all.DebugString();
  CheckAll(all2);
}

TEST(Runtime, AllCompact) {
  test_msgs::serdes::All all;

  // Set the fields.
  FillAll(all);

  neutron::serdes::Buffer dest;

  // Serialize in standard format.
  size_t length = all.SerializedSize();
  std::cout << length << std::endl;

  auto status = all.SerializeToBuffer(dest);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "serialized:\n";
  ASSERT_EQ(length, dest.size());
  toolbelt::Hexdump(dest.data(), length);

  // Compact the serialized buffer.
  neutron::serdes::Buffer compacted;
  dest.Rewind();

  status = test_msgs::serdes::All::Compact(dest, compacted);
  std::cerr << "compact: " << status << std::endl;
  ASSERT_TRUE(status.ok());

  // Deserialize from buffer and check it:
  test_msgs::serdes::All all2;
  size_t compacted_size = compacted.size();
  std::cerr << "compacted size: " << compacted_size << std::endl;
  toolbelt::Hexdump(compacted.data(), compacted_size);
  compacted.Rewind();

  status = all2.DeserializeFromArray(compacted.data(), compacted_size, true);
  ASSERT_TRUE(status.ok());
  // std::cout << all.DebugString();
  CheckAll(all2);
}


TEST(Runtime, AllExpand) {
  test_msgs::serdes::All all;

  // Set the fields.
  FillAll(all);

  neutron::serdes::Buffer dest;

  // Serialize in standard format.
  size_t length = all.CompactSerializedSize();
  std::cout << length << std::endl;

  auto status = all.SerializeToBuffer(dest, true);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "serialized:\n";
  ASSERT_EQ(length, dest.size());
  toolbelt::Hexdump(dest.data(), length);

  // Expand the serialized buffer.
  neutron::serdes::Buffer expanded;
  dest.Rewind();

  status = test_msgs::serdes::All::Expand(dest, expanded);
  std::cerr << "expand: " << status << std::endl;
  ASSERT_TRUE(status.ok());

  // Deserialize from buffer and check it:
  test_msgs::serdes::All all2;
  size_t expanded_size = expanded.size();
  std::cerr << "expanded size: " << expanded_size << std::endl;
  toolbelt::Hexdump(expanded.data(), expanded_size);
  expanded.Rewind();

  status = all2.DeserializeFromArray(expanded.data(), expanded_size, false);
  ASSERT_TRUE(status.ok());
  // std::cout << all.DebugString();
  CheckAll(all2);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
