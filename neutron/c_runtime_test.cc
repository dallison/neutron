#include "neutron/c_serdes/test_msgs/Fixed.h"
#include "neutron/serdes/test_msgs/Fixed.h"
#include "neutron/c_serdes/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>
#include <string.h>

/*
Fixed:
int32 foo
uint8[10] name
SubFixed sub
SubFixed[4] asub
int8 i8
Enum8 e8
Enum16 e16
Enum32 e32
Enum64 e64
float32[10] afloat
float64[20] adouble
Enum8[3] ae8
Enum16[3] ae16
Enum32[3] ae32
Enum64[3] ae64

SubFixed:
int32 seq
*/
TEST(Runtime, FromROS) {
  char buffer[4096];
  test_msgs::serdes::Fixed msg;
  msg.foo = 1234;
  strcpy((char*)msg.name.data(), "Dave");
  msg.i8 = 6;
  msg.e8 = test_msgs::serdes::Enum8::X1;
  msg.e16 = test_msgs::serdes::Enum16::X2;
  msg.e32 = test_msgs::serdes::Enum32::X3;
  msg.e64 = test_msgs::serdes::Enum64::X1;
  msg.sub.seq = 6543;
  for (int i = 0; i < 4; i++) {
    msg.asub[i].seq = 1000 + i;
  }
  for (int i = 0; i < 10; i++) {
    msg.afloat[i] = 1.0f + i;
  }
  for (int i = 0; i < 20; i++) {
    msg.adouble[i] = 2.0 + i;
  }
  auto status = msg.SerializeToArray(buffer, sizeof(buffer));
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  //toolbelt::Hexdump(buffer, sizeof(buffer));
  test_msgs_Fixed *c_msg = reinterpret_cast<test_msgs_Fixed *>(buffer);
  ASSERT_EQ(c_msg->foo, 1234);
  ASSERT_EQ(c_msg->i8, 6);
  ASSERT_EQ(c_msg->e8, test_msgs_Enum8_X1);
  ASSERT_EQ(c_msg->e16, test_msgs_Enum16_X2);
  ASSERT_EQ(c_msg->e32, test_msgs_Enum32_X3);
  ASSERT_EQ(c_msg->e64, test_msgs_Enum64_X1);
  ASSERT_EQ(c_msg->sub.seq, 6543);
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(c_msg->asub[i].seq, 1000 + i);
  }
  ASSERT_STREQ((char*)c_msg->name, "Dave");
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(c_msg->afloat[i], 1.0f + i);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(c_msg->adouble[i], 2.0 + i);
  }

}

TEST(Runtime, ToROS) {
  char buffer[4096];
  test_msgs_Fixed *c_msg = reinterpret_cast<test_msgs_Fixed *>(buffer);
  c_msg->foo = 1234;
  c_msg->i8 = 6;
  c_msg->e8 = test_msgs_Enum8_X1;
  c_msg->e16 = test_msgs_Enum16_X2;
  c_msg->e32 = test_msgs_Enum32_X3;
  c_msg->e64 = test_msgs_Enum64_X1;
  c_msg->sub.seq = 6543;
  for (int i = 0; i < 4; i++) {
    c_msg->asub[i].seq = 1000 + i;
  }
  strcpy((char*)c_msg->name, "Dave");
  for (int i = 0; i < 10; i++) {
    c_msg->afloat[i] = 1.0f + i;
  }
  for (int i = 0; i < 20; i++) {
    c_msg->adouble[i] = 2.0 + i;
  }
  
  test_msgs::serdes::Fixed msg;
  
  auto status = msg.DeserializeFromArray(buffer, sizeof(test_msgs_Fixed));

  std::cerr << status << std::endl;
  
  ASSERT_TRUE(status.ok());
  
  // toolbelt::Hexdump(buffer, sizeof(buffer));
  
  ASSERT_EQ(msg.foo, c_msg->foo);
  ASSERT_EQ(msg.e8,test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(msg.e16, test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(msg.e32, test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(msg.e64, test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(msg.sub.seq, c_msg->sub.seq);
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(msg.asub[i].seq, c_msg->asub[i].seq);
  }
  ASSERT_STREQ((char*)msg.name.data(), (char*)c_msg->name);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(msg.afloat[i], c_msg->afloat[i]);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(msg.adouble[i], c_msg->adouble[i]);
  }

}

TEST(Runtime, FromROSSerdes) {
  char buffer[4096];
  test_msgs::serdes::Fixed msg;
  msg.foo = 1234;
  strcpy((char*)msg.name.data(), "Dave");
  msg.i8 = 6;
  msg.e8 = test_msgs::serdes::Enum8::X1;
  msg.e16 = test_msgs::serdes::Enum16::X2;
  msg.e32 = test_msgs::serdes::Enum32::X3;
  msg.e64 = test_msgs::serdes::Enum64::X1;
  msg.sub.seq = 6543;
  for (int i = 0; i < 4; i++) {
    msg.asub[i].seq = 1000 + i;
  }
  for (int i = 0; i < 10; i++) {
    msg.afloat[i] = 1.0f + i;
  }
  for (int i = 0; i < 20; i++) {
    msg.adouble[i] = 2.0 + i;
  }
  auto status = msg.SerializeToArray(buffer, sizeof(buffer));
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  //toolbelt::Hexdump(buffer, sizeof(buffer));

  test_msgs_Fixed c_msg;
  bool ok = test_msgs_Fixed_DeserializeFromArray(&c_msg, buffer, msg.SerializedSize());
  ASSERT_TRUE(ok);

  ASSERT_EQ(c_msg.foo, 1234);
  ASSERT_EQ(c_msg.i8, 6);
  ASSERT_EQ(c_msg.e8, test_msgs_Enum8_X1);
  ASSERT_EQ(c_msg.e16, test_msgs_Enum16_X2);
  ASSERT_EQ(c_msg.e32, test_msgs_Enum32_X3);
  ASSERT_EQ(c_msg.e64, test_msgs_Enum64_X1);
  ASSERT_EQ(c_msg.sub.seq, 6543);
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(c_msg.asub[i].seq, 1000 + i);
  }
  ASSERT_STREQ((char*)c_msg.name, "Dave");
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(c_msg.afloat[i], 1.0f + i);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(c_msg.adouble[i], 2.0 + i);
  }

}

TEST(Runtime, ToROSSerdes) {
  char buffer[4096];
  test_msgs_Fixed c_msg;
  c_msg.foo = 1234;
  c_msg.i8 = 6;
  c_msg.e8 = test_msgs_Enum8_X1;
  c_msg.e16 = test_msgs_Enum16_X2;
  c_msg.e32 = test_msgs_Enum32_X3;
  c_msg.e64 = test_msgs_Enum64_X1;
  c_msg.sub.seq = 6543;
  for (int i = 0; i < 4; i++) {
    c_msg.asub[i].seq = 1000 + i;
  }
  strcpy((char*)c_msg.name, "Dave");
  for (int i = 0; i < 10; i++) {
    c_msg.afloat[i] = 1.0f + i;
  }
  for (int i = 0; i < 20; i++) {
    c_msg.adouble[i] = 2.0 + i;
  }
  bool ok = test_msgs_Fixed_SerializeToArray(&c_msg, buffer, sizeof(buffer));
  std::cerr << "serialize " << ok << std::endl;
  ASSERT_TRUE(ok);

  test_msgs::serdes::Fixed msg;
  
  auto status = msg.DeserializeFromArray(buffer, sizeof(test_msgs_Fixed));

  std::cerr << status << std::endl;
  
  ASSERT_TRUE(status.ok());
  
  // toolbelt::Hexdump(buffer, sizeof(buffer));
  
  ASSERT_EQ(msg.foo, c_msg.foo);
  ASSERT_EQ(msg.e8,test_msgs::serdes::Enum8::X1);
  ASSERT_EQ(msg.e16, test_msgs::serdes::Enum16::X2);
  ASSERT_EQ(msg.e32, test_msgs::serdes::Enum32::X3);
  ASSERT_EQ(msg.e64, test_msgs::serdes::Enum64::X1);

  ASSERT_EQ(msg.sub.seq, c_msg.sub.seq);
  for (int i = 0; i < 4; i++) {
    ASSERT_EQ(msg.asub[i].seq, c_msg.asub[i].seq);
  }
  ASSERT_STREQ((char*)msg.name.data(), (char*)c_msg.name);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(msg.afloat[i], c_msg.afloat[i]);
  }
  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(msg.adouble[i], c_msg.adouble[i]);
  }

}
