#include "davros/syntax.h"
#include "davros/package.h"
#include "absl/strings/str_format.h"

namespace davros {

absl::flat_hash_map<Token, FieldType> Message::field_types_;

static bool IsIntField(FieldType t) {
  switch (t) {
  case FieldType::kBool:
    [[fallthrough]];
  case FieldType::kInt8:
    [[fallthrough]];
  case FieldType::kUint8:
    [[fallthrough]];
  case FieldType::kInt16:
    [[fallthrough]];
  case FieldType::kUint16:
    [[fallthrough]];
  case FieldType::kInt32:
    [[fallthrough]];
  case FieldType::kUint32:
    [[fallthrough]];
  case FieldType::kInt64:
    [[fallthrough]];
  case FieldType::kUint64:
    return true;
  default:
    return false;
  }
}

absl::Status Message::Parse(LexicalAnalyzer &lex) {
  if (field_types_.empty()) {
    field_types_.insert({Token::kBool, FieldType::kBool});
    field_types_.insert({Token::kInt8, FieldType::kInt8});
    field_types_.insert({Token::kUint8, FieldType::kUint8});
    field_types_.insert({Token::kInt16, FieldType::kInt16});
    field_types_.insert({Token::kUint16, FieldType::kUint16});
    field_types_.insert({Token::kInt32, FieldType::kInt32});
    field_types_.insert({Token::kUint32, FieldType::kUint32});
    field_types_.insert({Token::kInt64, FieldType::kInt64});
    field_types_.insert({Token::kUint64, FieldType::kUint64});
    field_types_.insert({Token::kFloat32, FieldType::kFloat32});
    field_types_.insert({Token::kFloat64, FieldType::kFloat64});
    field_types_.insert({Token::kString, FieldType::kString});
    field_types_.insert({Token::kTime, FieldType::kTime});
    field_types_.insert({Token::kDuration, FieldType::kDuration});
    field_types_.insert({Token::kChar, FieldType::kUint8});
    field_types_.insert({Token::kByte, FieldType::kInt8});
  }
  while (!lex.Eof()) {
    FieldType field_type = FieldType::kUnknown;
    std::string msg_package;
    std::string msg_name;

    switch (lex.CurrentToken()) {
    case Token::kIdentifier: {
      msg_package.clear();
      msg_name = lex.Spelling();
      lex.NextToken();
      // ROS only allows a single package followed by a /
      if (lex.Match(Token::kSlash)) {
        msg_package = msg_name;
        if (lex.CurrentToken() == Token::kIdentifier) {
          msg_name = lex.Spelling();
          lex.NextToken();
        } else {
          lex.Error("Invalid message name");
          lex.ReadLine();
          lex.NextToken();
          continue;
        }
      }

      field_type = FieldType::kMessage;

      // The unadorned type Header is special for the first field in the message.
      if (fields_.empty() && msg_package.empty() && msg_name == "Header") {
        msg_package = "std_msgs";
      }
      break;
    }

    case Token::kBool:
      [[fallthrough]];
    case Token::kInt8:
      [[fallthrough]];
    case Token::kUint8:
      [[fallthrough]];
    case Token::kInt16:
      [[fallthrough]];
    case Token::kUint16:
      [[fallthrough]];
    case Token::kInt32:
      [[fallthrough]];
    case Token::kUint32:
      [[fallthrough]];
    case Token::kInt64:
      [[fallthrough]];
    case Token::kUint64:
      [[fallthrough]];
    case Token::kFloat32:
      [[fallthrough]];
    case Token::kFloat64:
      [[fallthrough]];
    case Token::kString:
      [[fallthrough]];
    case Token::kTime:
      [[fallthrough]];
    case Token::kDuration:
      [[fallthrough]];
    case Token::kChar:
      [[fallthrough]];
    case Token::kByte:
      field_type = field_types_[lex.CurrentToken()];
      lex.NextToken();
      break;
    default:
      lex.Error("Invalid message field type");
      lex.ReadLine();
      lex.NextToken();
      break;
    }
    if (field_type == FieldType::kUnknown) {
      continue;
    }

    // Check for array type.
    int array_size = -1;
    if (lex.Match(Token::kLsquare)) {
      if (lex.CurrentToken() == Token::kRsquare) {
        array_size = 0;
      } else if (lex.CurrentToken() == Token::kNumber) {
        array_size = lex.Number();
        if (array_size <= 0) {
          lex.Error("Invalid array size %d", array_size);
        }
        lex.NextToken();
      }
      if (!lex.Match(Token::kRsquare)) {
        lex.Error("Missing ] for array");
      }
    }
    if (lex.CurrentToken() != Token::kIdentifier) {
      lex.Error("Missing field name");
      continue;
    }

    std::string field_name = lex.Spelling();
    // Record the line number of the name before reading the next token.
    int name_lineno = lex.TokenLineNumber();

    lex.NextToken();
    if (lex.CurrentToken() == Token::kEqual) {
      if (array_size != -1) {
        lex.Error("Cannot have an array constant");
        lex.ReadLine();
        lex.NextToken();
        continue;
      }
      if (constants_.find(field_name) != constants_.end()) {
        lex.Error("Duplicate constant %s", field_name.c_str());
        lex.ReadLine();
        lex.NextToken();
        continue;
      }
      // Constant definition.
      std::variant<int64_t, double, std::string> value;
      if (field_type == FieldType::kString) {
        // String constant values stretch to eol.
        value = lex.ReadToEndOfLine();
      } else if (field_type == FieldType::kFloat32 ||
                 field_type == FieldType::kFloat64) {
        lex.NextToken();
        if (lex.CurrentToken() == Token::kNumber) {
          // Float init by int.
          value = lex.Number();
          lex.NextToken();
        } else if (lex.CurrentToken() == Token::kFnumber) {
          value = lex.Fnumber();
          lex.NextToken();
        } else {
          lex.Error("Invalid floating point constant value");
          lex.ReadLine();
          lex.NextToken();
        }
      } else {
        lex.NextToken();
        if (IsIntField(field_type)) {
          if (lex.CurrentToken() != Token::kNumber) {
            lex.Error("Invalid value for integer constant");
            lex.ReadLine();
            lex.NextToken();
            continue;
          }
          value = lex.Number();
          lex.NextToken();
        }
      }
      constants_[field_name] =
          std::make_shared<Constant>(field_type, field_name, value);
      continue;
    }

    // TODO: ROS2 default value

    // Not a constant, must be a field.
    if (field_map_.contains(field_name)) {
      lex.Error(name_lineno, "Duplicate field %s", field_name.c_str());
      continue;
    }

    std::shared_ptr<Field> field;
    if (field_type == FieldType::kMessage) {
      field = std::make_shared<MessageField>(field_name, msg_package, msg_name);
    } else {
      field = std::make_shared<Field>(field_type, field_name);
    }

    if (array_size != -1) {
      // We have an array type.
      field = std::make_shared<ArrayField>(field, array_size);
    }
    fields_.push_back(field);
    field_map_[field_name] = field;
  }
  return lex.NumErrors() == 0
             ? absl::OkStatus()
             : absl::InternalError("Parsing errors encountered");
} // namespace davros

static std::string FieldTypeName(FieldType t) {
  switch (t) {
  case FieldType::kBool:
    return "bool";
  case FieldType::kInt8:
    return "int8";
  case FieldType::kUint8:
    return "uint8";
  case FieldType::kInt16:
    return "int16";
  case FieldType::kUint16:
    return "uint16";
  case FieldType::kInt32:
    return "int32";
  case FieldType::kUint32:
    return "uint32";
  case FieldType::kInt64:
    return "int64";
  case FieldType::kUint64:
    return "uint64";
  case FieldType::kFloat32:
    return "float32";
  case FieldType::kFloat64:
    return "float64";
  case FieldType::kString:
    return "string";
  case FieldType::kTime:
    return "time";
  case FieldType::kDuration:
    return "duration";
  default:
    return "unknown";
  }
}
std::string Field::TypeName() const { return FieldTypeName(type_); }

std::string Constant::TypeName() const { return FieldTypeName(type_); }

std::string MessageField::TypeName() const {
  if (msg_package_.empty()) {
    return msg_name_;
  }
  return absl::StrFormat("%s/%s", msg_package_, msg_name_);
}

std::string ArrayField::TypeName() const {
  std::string t = base_->TypeName();
  if (size_ == 0) {
    return t + "[]";
  }
  return absl::StrFormat("%s[%d]", t, size_);
}

// Magic helper templates for std::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

void Constant::Dump(std::ostream &os) const {
  os << TypeName() << " " << name_ << " = ";
  std::visit(overloaded{[&os](int64_t v) { os << v; },
                        [&os](double v) { os << v; },
                        [&os](std::string v) { os << v; }},
             value_);
  os << std::endl;
}

void Field::Dump(std::ostream &os) const {
  os << TypeName() << " " << name_ << std::endl;
}

void ArrayField::Dump(std::ostream &os) const {
  os << TypeName() << " " << base_->Name() << std::endl;
}

void Message::Dump(std::ostream &os) const {
  for (auto & [ name, c ] : constants_) {
    c->Dump(os);
  }
  for (auto &f : fields_) {
    f->Dump(os);
  }
}

absl::Status Message::Resolve(std::shared_ptr<PackageScanner> scanner) {
  for (auto &field : fields_) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      std::shared_ptr<Message> msg;
      if (msg_field->MsgPackage().empty()) {
        // No package, look in same package.
        if (package_ == nullptr) {
          return absl::InternalError(absl::StrFormat("No package set for %s", Name()));
        }
        msg = package_->FindMessage(msg_field->MsgName());
      } else {
        msg = scanner->FindMessage(msg_field->MsgPackage(), msg_field->MsgName());
      }
      if (msg == nullptr) {
        return absl::InternalError(absl::StrFormat("Unable to resolve message %s/%s", msg_field->MsgPackage(), msg_field->MsgName()));
      } else {
        msg_field->Resolved(msg);
      }
    }
  }
  return absl::OkStatus();
}


} // namespace davros
