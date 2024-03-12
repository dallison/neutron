#include "daros/lex.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(LexTest, OneLine) {
  std::stringstream input;
  input << "int32 foo\n";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());
}

TEST(LexTest, NoNewline) {
  std::stringstream input;
  input << "int32 foo";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());
}

TEST(LexTest, MultiLine) {
  std::stringstream input;
  input << "\n# comment\n  int32 foo\n\tfloat32 bar\n\n";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kInt32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kFloat32, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("bar", lex.Spelling());
}

TEST(LexTest, Constants) {
  std::stringstream input;
  input << "123 -456 0.456 -1.314e+4 -1.314e+4\n";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kNumber, lex.CurrentToken());
  ASSERT_EQ(123, lex.Number());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kNumber, lex.CurrentToken());
  ASSERT_EQ(-456, lex.Number());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(0.456, lex.Fnumber(), 0.0001);

  lex.NextToken();
  ASSERT_EQ(daros::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(-1.314e+4, lex.Fnumber(), 0.0001);

  lex.NextToken();
  ASSERT_EQ(daros::Token::kFnumber, lex.CurrentToken());
  ASSERT_NEAR(-1.314e+4, lex.Fnumber(), 0.0001);
}

TEST(LexTest, Operators) {
  std::stringstream input;
  input << "=[]\n/ <\n";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kEqual, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kLsquare, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(daros::Token::kRsquare, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(daros::Token::kSlash, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(daros::Token::kLess, lex.CurrentToken());
}

TEST(LexTest, StringConstant) {
  std::stringstream input;
  input << "string foo =   this is the rest of the line   # comment\n\nint32 bar\n";

  daros::LexicalAnalyzer lex("stdin", input);
  ASSERT_EQ(daros::Token::kString, lex.CurrentToken());

  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("foo", lex.Spelling());

  lex.NextToken();
  if (lex.CurrentToken() == daros::Token::kEqual) {
    std::string s = lex.ReadToEndOfLine();
    ASSERT_EQ(s, "this is the rest of the line");
  }
  ASSERT_EQ(daros::Token::kInt32, lex.CurrentToken());
  lex.NextToken();
  ASSERT_EQ(daros::Token::kIdentifier, lex.CurrentToken());
  ASSERT_EQ("bar", lex.Spelling());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
