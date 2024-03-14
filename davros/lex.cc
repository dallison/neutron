#include "davros/lex.h"
#include "absl/strings/str_format.h"
#include <ctype.h>

namespace davros {

LexicalAnalyzer::LexicalAnalyzer(
    std::string filename, std::istream &in,
    std::function<void(const std::string &)> error_fn)
    : in_(in), filename_(std::move(filename)), error_fn_(std::move(error_fn)) {
  reserved_words_["bool"] = Token::kBool;
  reserved_words_["int8"] = Token::kInt8;
  reserved_words_["uint8"] = Token::kUint8;
  reserved_words_["int16"] = Token::kInt16;
  reserved_words_["uint16"] = Token::kUint16;
  reserved_words_["int32"] = Token::kInt32;
  reserved_words_["uint32"] = Token::kUint32;
  reserved_words_["int64"] = Token::kInt64;
  reserved_words_["uint64"] = Token::kUint64;
  reserved_words_["float32"] = Token::kFloat32;
  reserved_words_["float64"] = Token::kFloat64;
  reserved_words_["string"] = Token::kString;
  reserved_words_["time"] = Token::kTime;
  reserved_words_["duration"] = Token::kDuration;
  reserved_words_["char"] = Token::kChar;
  reserved_words_["byte"] = Token::kByte;
  ReadLine();
  NextToken();
}

void LexicalAnalyzer::NextToken() {
  while (!Eof()) {
    SkipSpaces();
    char ch = NextChar();
    if (ch_ >= line_.size()) {
      // End of line, read another
      ReadLine();
      continue;
    }
    if (ch == '#') {
      // Comment ends line.
      ReadLine();
      continue;
    }

    token_lineno_ = lineno_;
    // Reserved word or identifier.
    if (isalpha(ch)) {
      spelling_.clear();
      while (ch_ < line_.size() && (isalnum(ch) || ch == '_')) {
        spelling_ += ch;
        ch = NextChar();
      }
      ch_--; // One too far.
      auto it = reserved_words_.find(spelling_);
      if (it != reserved_words_.end()) {
        current_token_ = it->second;
      } else {
        current_token_ = Token::kIdentifier;
      }
      return;
    }

    // Integer or floating point constant.
    if (isdigit(ch) || ch == '-') {
      bool negative = ch == '-';
      spelling_.clear();
      if (negative) {
        spelling_ += '-';
        ch = NextChar();
      }
      bool dot_seen = false;
      bool exp_seen = false;
      bool sign_seen = false;
      while (ch_ < line_.size()) {
        if (isdigit(ch)) {
          spelling_ += ch;
          ch = NextChar();
          continue;
        }
        if (ch == '.' && !dot_seen) {
          dot_seen = true;
          spelling_ += ch;
          ch = NextChar();
          continue;
        }
        if (dot_seen && !exp_seen && (ch == 'e' || ch == 'E')) {
          exp_seen = true;
          spelling_ += ch;
          ch = NextChar();
          continue;
        }
        if (exp_seen && !sign_seen && (ch == '+' || ch == '-')) {
          sign_seen = true;
          spelling_ += ch;
          ch = NextChar();
          continue;
        }
        break;
      }
      if (dot_seen) {
        // Floating point constant.
        fnumber_ = strtod(spelling_.c_str(), nullptr);
        current_token_ = Token::kFnumber;
      } else {
        number_ = strtoll(spelling_.c_str(), nullptr, 10);
        current_token_ = Token::kNumber;
      }
      ch_--;
      return;
    }

    // Operator tokens.
    switch (ch) {
    case '/':
      current_token_ = Token::kSlash;
      return;
    case '=':
      current_token_ = Token::kEqual;
      return;
    case '[':
      current_token_ = Token::kLsquare;
      return;
    case ']':
      current_token_ = Token::kRsquare;
      return;
    case '<':
      current_token_ = Token::kLess;
      return;
    default:
      break;
    }
    // If we get here we have an invalid token.
    current_token_ = Token::kInvalid;
    return;
  }
}

void LexicalAnalyzer::ReadLine() {
  ch_ = 0;
  line_.clear();
  while (!in_.eof()) {
    int ch = in_.get();
    if (in_.eof()) {
      break;
    }
    line_ += ch;
    if (ch == '\n') {
      break;
    }
  }
  // Make sure line ends in newline.
  if (line_.back() != '\n') {
    line_ += '\n';
  }
  lineno_++;
}

void LexicalAnalyzer::SkipSpaces() {
  while (ch_ < line_.size() && isspace(line_[ch_])) {
    ch_++;
  }
}

std::string LexicalAnalyzer::ReadToEndOfLine() {
  SkipSpaces();
  std::string s;
  while (ch_ < line_.size() && line_[ch_] != '\n' && line_[ch_] != '#') {
    s += line_[ch_++];
  }
  // Remove trailing whitespace.
  while (!s.empty()) {
    if (isspace(s.back())) {
      s = s.substr(0, s.size() - 1);
    } else {
      break;
    }
  }
  ReadLine();
  NextToken();
  return s;
}

void LexicalAnalyzer::Error(const char *error, ...) {
  va_list ap;
  va_start(ap, error);
  VError(error, ap);
  va_end(ap);
}

void LexicalAnalyzer::VError(const char *error, va_list ap) {
  VError(token_lineno_, error, ap);
}

void LexicalAnalyzer::Error(int lineno, const char *error, ...) {
  va_list ap;
  va_start(ap, error);
  VError(lineno, error, ap);
  va_end(ap);
}

void LexicalAnalyzer::VError(int lineno, const char *error, va_list ap) {
  char buf[256];
  vsnprintf(buf, sizeof(buf), error, ap);
  std::string s =
      absl::StrFormat("%s: line %d: %s", filename_.c_str(), lineno, buf);
  if (error_fn_ != nullptr) {
    error_fn_(s);
  } else {
    std::cerr << s << std::endl;
  }
  num_errors_++;
}
} // namespace davros
