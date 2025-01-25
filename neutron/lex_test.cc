#include "neutron/lex.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(LexTest, OneLine) {
  std::stringstream input;
  input << "int32 foo\n";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());
}

TEST(LexTest, NoNewline) {
  std::stringstream input;
  input << "int32 foo";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());
}

TEST(LexTest, MultiLine) {
  std::stringstream input;
  input << "\n# comment\n  int32 foo\n\tfloat32 bar\n\n";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kFloat32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("bar", lex.Spelling());
}

TEST(LexTest, Constants) {
  std::stringstream input;
  input << "123 -456 0.456 -1.314e+4 -1.314e+4\n";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kNumber, lex.CurrentToken());
  ASSERT_EQ(123, lex.Number());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kNumber, lex.CurrentToken());
  ASSERT_EQ(-456, lex.Number());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(0.456, lex.Fnumber(), 0.0001);

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(-1.314e+4, lex.Fnumber(), 0.0001);

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(-1.314e+4, lex.Fnumber(), 0.0001);
}

TEST(LexTest, Operators) {
  std::stringstream input;
  input << "=[]\n/ <\n";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kEqual, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kLsquare, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(neutron::Token::kRsquare, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(neutron::Token::kSlash, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(neutron::Token::kLess, lex.CurrentToken());
}

TEST(LexTest, StringConstant) {
  std::stringstream input;
  input << "string foo =   this is the rest of the line   # comment\n\nint32 "
           "bar\n";

  neutron::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(neutron::Token::kString, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());

  lex.NextToken();
  if (lex.CurrentToken() == neutron::Token::kEqual) {
    std::string s = lex.ReadToEndOfLine();
    ASSERT_EQ(s, "this is the rest of the line");
  }
  ASSERT_EQ(neutron::Token::kInt32, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(neutron::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("bar", lex.Spelling());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
