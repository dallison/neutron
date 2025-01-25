#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <functional>
#include <iostream>
#include <string>
#include "absl/container/flat_hash_map.h"

namespace gemini {

enum class Token {
  kInvalid,
  kNumber,
  kFnumber,
  kIdentifier,
  kEqual,
  kLsquare,
  kRsquare,
  kSlash,
  kLess,

  // Builtin types
  kBool,
  kInt8,
  kUint8,
  kInt16,
  kUint16,
  kInt32,
  kUint32,
  kInt64,
  kUint64,
  kFloat32,
  kFloat64,
  kString,
  kTime,
  kDuration,
  kChar,
  kByte,
};

class LexicalAnalyzer {
 public:
  LexicalAnalyzer(std::string filename, std::istream &in,
                  std::function<void(const std::string &)> error_fn = {});

  void NextToken();
  void ReadLine();
  void SkipSpaces();

  Token CurrentToken() const { return current_token_; }
  bool Match(Token t) {
    if (CurrentToken() == t) {
      NextToken();
      return true;
    }
    return false;
  }
  std::string ReadToEndOfLine();

  const std::string &Spelling() const { return spelling_; }
  int64_t Number() const { return number_; }
  double Fnumber() const { return fnumber_; }

  void Error(const char *error, ...);
  void VError(const char *error, va_list ap);

  void Error(int lineno, const char *error, ...);
  void VError(int lineno, const char *error, va_list ap);

  int NumErrors() const { return num_errors_; }

  bool Eof() const { return in_.eof(); }
  int TokenLineNumber() const { return token_lineno_; }

 private:
  char NextChar() { return line_[ch_++]; }
  std::istream &in_;
  Token current_token_ = Token::kInvalid;
  std::string line_;
  size_t ch_ = 0;
  std::string spelling_;
  int64_t number_ = 0;
  double fnumber_ = 0.0;
  absl::flat_hash_map<std::string, Token> reserved_words_;
  std::string filename_;
  int lineno_ = 0;
  int token_lineno_ = 0;
  int num_errors_ = 0;
  std::function<void(const std::string &)> error_fn_ = {};
};

}  // namespace gemini
