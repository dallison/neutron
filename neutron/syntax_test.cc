#include "neutron/syntax.h"
#include <gtest/gtest.h>
#include <sstream>
#include "neutron/lex.h"

TEST(SyntaxTest, SingleField) {
  std::stringstream input;
  input << "# a comment\nint32 foo\n";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  // Comment doesn't appear.
  ASSERT_EQ("int32 foo\n", out.str());
}

TEST(SyntaxTest, PrimitiveFields) {
  std::stringstream input;
  input << R"(int8 i8
uint8 ui8
int16 i16
uint16 ui16
int32 i32
uint32 ui32
int64 i64
uint64 ui64
string s
bool b
float32 f32
float64 f64
time tm
duration d
)";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  ASSERT_EQ(input.str(), out.str());
}

TEST(SyntaxTest, CharAndByte) {
  std::stringstream input;
  input << R"(char c
byte b
)";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  const char *expected = R"(uint8 c
int8 b
)";
  ASSERT_EQ(expected, out.str());
}

TEST(SyntaxTest, Messages) {
  std::stringstream input;
  input << R"(std_msgs/Header hdr
foo/Msg msg
)";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  ASSERT_EQ(input.str(), out.str());
}

TEST(SyntaxTest, Constants) {
  std::stringstream input;
  input << R"(int32 bar = 123
float64 foo = 1.5
float32 fop = 5
string s = this is a string
)";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  ASSERT_EQ(input.str(), out.str());
}

TEST(SyntaxTest, Arrays) {
  std::stringstream input;
  input << R"(int32[] i1
int8[4] i2
foo/Baz[] i3
foo/baz[6] i4
)";

  neutron::LexicalAnalyzer lex("stdin", input,
                              [](const std::string &error) { FAIL(); });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_TRUE(status.ok());
  std::stringstream out;
  msg.Dump(out);
  ASSERT_EQ(input.str(), out.str());
}

TEST(SyntaxTest, Errors) {
  std::stringstream input;
  input << R"(foo   # missing field name
  int32[-1] bar   # bad array size
  int32[ baz       # missing ]
  int8[] con = 123   # array constant
  /       # Invalid field type
  float32 f = bar   # bad floating value
  int32 i = +       # bad integer value
  int16 c1 = 123
  int8 c1 = 456     # dup constant
  string foobar
  string foobar     # dup field.
  *     # invalid token
)";

  int num_errors = 0;

  const char *errors[] = {
      "stdin: line 2: Missing field name",
      "stdin: line 2: Invalid array size -1",
      "stdin: line 3: Missing ] for array",
      "stdin: line 4: Cannot have an array constant",
      "stdin: line 5: Invalid message field type",
      "stdin: line 6: Invalid floating point constant value",
      "stdin: line 7: Invalid value for integer constant",
      "stdin: line 9: Duplicate constant c1",
      "stdin: line 11: Duplicate field foobar",
      "stdin: line 12: Invalid message field type",
      nullptr};
  neutron::LexicalAnalyzer lex("stdin", input,
                              [&num_errors, &errors](const std::string &error) {
                                std::cerr << error << std::endl;
                                ASSERT_NE(nullptr, errors[num_errors]);
                                ASSERT_EQ(errors[num_errors++], error);
                              });
  neutron::Message msg(nullptr, "Foo");

  absl::Status status = msg.Parse(lex);
  ASSERT_FALSE(status.ok());
  ASSERT_EQ(num_errors, lex.NumErrors());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
