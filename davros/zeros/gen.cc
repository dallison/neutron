
#include "davros/zeros/gen.h"
#include "absl/strings/str_format.h"
#include <fstream>
#include <memory>

namespace davros::zeros {

// Magic helper templates for std::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

absl::Status Generator::Generate(const Message &msg) {
  // std::filesystem::path dir =
  //     root_ / std::filesystem::path(msg.Package()->Name());
  // if (!std::filesystem::exists(dir) &&
  //     !std::filesystem::create_directories(dir)) {
  //   return absl::InternalError(
  //       absl::StrFormat("Unable to create directory %s", dir.string()));
  // }

  // std::filesystem::path header = dir / std::filesystem::path(msg.Name() +
  // ".h");

  // std::ofstream out(header.string());
  // if (!out) {
  //   return absl::InternalError(
  //       absl::StrFormat("Unable to create %s", header.string()));
  // }
  if (absl::Status status = GenerateHeader(msg, std::cout); !status.ok()) {
    return status;
  }
  // std::cout << "Generated header file " << header.string() << std::endl;

  return absl::OkStatus();
}

static std::string FieldClass(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "Int8Field";
  case FieldType::kUint8:
    return "Uint8Field";
  case FieldType::kInt16:
    return "Int16Field";
  case FieldType::kUint16:
    return "Uint16Field";
  case FieldType::kInt32:
    return "Int32Field";
  case FieldType::kUint32:
    return "Uint32Field";
  case FieldType::kInt64:
    return "Int4Field";
  case FieldType::kUint64:
    return "Uint64Field";
  case FieldType::kFloat32:
    return "Float32Field";
  case FieldType::kFloat64:
    return "Float64Field";
  case FieldType::kTime:
    return "TimeField";
  case FieldType::kDuration:
    return "DurationField";
  case FieldType::kString:
    return "StringField";
  case FieldType::kMessage:
    return "MessageField";
  case FieldType::kBool:
    return "dBoolField";
  case FieldType::kUnknown:
    abort();
  }
}

static std::string FieldCType(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "int8_t";
  case FieldType::kUint8:
    return "uint8_t";
  case FieldType::kInt16:
    return "int16_t";
  case FieldType::kUint16:
    return "uint16_t";
  case FieldType::kInt32:
    return "int32_t";
  case FieldType::kUint32:
    return "uint32_t";
  case FieldType::kInt64:
    return "int64_t";
  case FieldType::kUint64:
    return "uint64_t";
  case FieldType::kFloat32:
    return "float";
  case FieldType::kFloat64:
    return "double";
  case FieldType::kTime:
    return "davros::Time";
  case FieldType::kDuration:
    return "davros::Duration";
  case FieldType::kString:
    return "davros::zeros::StringHeader";
  case FieldType::kBool:
    return "uint8_t";
  case FieldType::kMessage:
    std::cerr << "Can't use message field type here\n";
    return "<message>";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(type) << std::endl;
    abort();
  }
}

static std::string FieldAlignmentType(std::shared_ptr<Field> field) {
  switch (field->Type()) {
  case FieldType::kInt8:
    return "int8_t";
  case FieldType::kUint8:
    return "uint8_t";
  case FieldType::kInt16:
    return "int16_t";
  case FieldType::kUint16:
    return "uint16_t";
  case FieldType::kInt32:
    return "int32_t";
  case FieldType::kUint32:
    return "uint32_t";
  case FieldType::kInt64:
    return "int64_t";
  case FieldType::kUint64:
    return "uint64_t";
  case FieldType::kFloat32:
    return "float";
  case FieldType::kFloat64:
    return "double";
  case FieldType::kTime:
    return "int32_t";
  case FieldType::kDuration:
    return "int32_t";
  case FieldType::kString:
    return "int32_t";
  case FieldType::kBool:
    return "uint8_t";
  case FieldType::kMessage:
    return "int32_t";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(field->Type()) << std::endl;
    abort();
  }
}

static std::string ConstantCType(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "int8_t";
  case FieldType::kUint8:
    return "uint8_t";
  case FieldType::kInt16:
    return "int16_t";
  case FieldType::kUint16:
    return "uint16_t";
  case FieldType::kInt32:
    return "int32_t";
  case FieldType::kUint32:
    return "uint32_t";
  case FieldType::kInt64:
    return "int64_t";
  case FieldType::kUint64:
    return "uint64_t";
  case FieldType::kFloat32:
    return "float";
  case FieldType::kFloat64:
    return "double";
  case FieldType::kTime:
    return "davros::Time";
  case FieldType::kDuration:
    return "davros::Duration";
  case FieldType::kString:
    return "std::string";
  case FieldType::kBool:
    return "uint8_t";
  case FieldType::kMessage:
    std::cerr << "Can't use message field type here\n";
    return "<message>";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(type) << std::endl;
    abort();
  }
}
static int EnumCSize(const Message &msg) {
  // Look for the biggest constant type.

  int size = 0;
  for (auto & [ name, c ] : msg.Constants()) {
    switch (c->Type()) {
    case FieldType::kInt8:
    case FieldType::kUint8:
      size = std::max(size, 1);
      break;
    case FieldType::kInt16:
    case FieldType::kUint16:
      size = std::max(size, 2);
      break;
    case FieldType::kInt32:
    case FieldType::kUint32:
      size = std::max(size, 4);
      break;
    case FieldType::kInt64:
    case FieldType::kUint64:
      size = std::max(size, 8);
      break;
    default:
      break;
    }
  }
  return size;
}

static std::string EnumCType(const Message &msg) {
  int size = EnumCSize(msg);

  switch (size) {
  case 0:
  default:
    return "uint8_t";
  case 1:
    return "uint8_t";
  case 2:
    return "uint16_t";
  case 4:
    return "uint32_t";
  case 8:
    return "uint64_t";
  }
}
static std::string MessageFieldTypeName(const Message &msg,
                                        std::shared_ptr<MessageField> field) {
  std::string name;
  if (field->MsgPackage().empty()) {
    return msg.Package()->Name() + "::" + field->MsgName();
  }
  return field->MsgPackage() + "::" + field->MsgName();
}

static std::string
MessageFieldIncludeFile(const Message &msg,
                        std::shared_ptr<MessageField> field) {
  if (field->MsgPackage().empty()) {
    return msg.Package()->Name() + "/" + field->MsgName() + ".h";
  }
  return field->MsgPackage() + "/" + field->MsgName() + ".h";
}

static std::string SanitizeFieldName(const std::string &name) { return name; }

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "// File was generated by Davros "
        "(https://github.com/dallison/davros)\n";
  os << "// It's probably best not to modify it, but I can't stop you\n";
  os << "#pragma once\n";
  os << "#include \"davros/zeros/runtime.h\"\n";

  // Include files for message fields
  os << "// Message field definitions.\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
         << MessageFieldIncludeFile(msg, msg_field) << "\"\n";
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
           << MessageFieldIncludeFile(msg, msg_field) << "\"\n";
      }
    }
  }
  os << "\n";
  os << "namespace " << msg.Package()->Name() << " {\n";

  if (msg.IsEnum()) {
    // Enumeration.
    if (absl::Status status = GenerateEnum(msg, os); !status.ok()) {
      return status;
    }
  } else {
    if (absl::Status status = GenerateStruct(msg, os); !status.ok()) {
      return status;
    }
  }
  os << "}    // namespace " << msg.Package()->Name() << "\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateEnum(const Message &msg, std::ostream &os) {
  os << "enum class " << msg.Name() << " : int {\n";
  for (auto & [ name, c ] : msg.Constants()) {
    os << c->Name() << " = " << std::get<0>(c->Value()) << ",\n";
  }
  os << "};\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateStruct(const Message &msg, std::ostream &os) {
  os << "#pragma clang diagnostic push\n";
  os << "#pragma clang diagnostic ignored \"-Winvalid-offsetof\"\n";
  os << "struct " << msg.Name() << " : public davros::zeros::Message {\n";

  if (absl::Status status = GenerateDefaultConstructor(msg, os); !status.ok()) {
    return status;
  }
  if (absl::Status status = GenerateEmbeddedConstructor(msg, os);
      !status.ok()) {
    return status;
  }
  if (absl::Status status = GenerateNonEmbeddedConstructor(msg, os);
      !status.ok()) {
    return status;
  }
  if (absl::Status status = GenerateBinarySize(msg, os); !status.ok()) {
    return status;
  }
  // Constants.
  for (auto & [ name, c ] : msg.Constants()) {
    if (c->Type() == FieldType::kString) {
      os << "  static inline constexpr const char " << c->Name() << "[] = ";
    } else {
      os << "  static constexpr " << ConstantCType(c->Type()) << " "
         << c->Name() << " = ";
    }

    std::visit(overloaded{[&os](int64_t v) { os << v; },
                          [&os](double v) { os << v; },
                          [&os](std::string v) { os << '"' << v << '"'; }},
               c->Value());
    os << ";" << std::endl;
  }

  for (auto &field : msg.Fields()) {
    os << "  davros::zeros::";
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "EnumField<" << MessageFieldTypeName(msg, msg_field);
      } else {
        os << "MessageField<" << MessageFieldTypeName(msg, msg_field);
      }
      os << ">";
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->IsFixedSize()) {
        if (array->Base()->Type() == FieldType::kString) {
          os << "StringArrayField<";
        } else if (array->Base()->Type() == FieldType::kMessage) {
          auto msg_field =
              std::static_pointer_cast<MessageField>(array->Base());
      if (msg_field->Msg()->IsEnum()) {
            os << "EnumArrayField<" << MessageFieldTypeName(msg, msg_field)
               << ", ";
          } else {
            os << "MessageArrayField<" << MessageFieldTypeName(msg, msg_field)
               << ", ";
          }
        } else {
          os << "PrimitiveArrayField<" << FieldCType(array->Base()->Type());
        }
        os << array->Size() << ">";
      } else {
        if (array->Base()->Type() == FieldType::kString) {
          os << "StringVectorField";
        } else if (array->Base()->Type() == FieldType::kMessage) {
          auto msg_field =
              std::static_pointer_cast<MessageField>(array->Base());
      if (msg_field->Msg()->IsEnum()) {
            os << "EnumVectorField<" << MessageFieldTypeName(msg, msg_field)
               << ">";
          } else {
            os << "MessageVectorField<" << MessageFieldTypeName(msg, msg_field)
               << ">";
          }
        } else {
          os << "PrimitiveVectorField<" << FieldCType(array->Base()->Type());
        }
      }
    } else {
      os << FieldClass(field->Type());
    }
    os << " " << SanitizeFieldName(field->Name()) << " = {};\n";
  }
  os << "};\n";
  os << "#pragma clang diagnostic pop\n";

  return absl::OkStatus();
}

absl::Status Generator::GenerateFieldInitializers(const Message &msg,
                                                  std::ostream &os,
                                                  const char *sep) {
  auto &fields = msg.Fields();
  if (fields.empty()) {
    return absl::OkStatus();
  }
  // First field is at offset 0.
  auto &field = fields[0];
  os << sep << SanitizeFieldName(field->Name()) << "(offsetof(" << msg.Name()
     << ", " << SanitizeFieldName(field->Name()) << "), 0)\n";

  // The remaining fields are aligned by their type from the end of the
  // previous field.
  for (size_t i = 1; i < fields.size(); i++) {
    auto &prev = fields[i - 1];
    auto &field = fields[i];
    os << "  , " << SanitizeFieldName(field->Name()) << "(offsetof("
       << msg.Name() << ", " << SanitizeFieldName(field->Name())
       << "), davros::zeros::AlignedOffset<" << FieldAlignmentType(field)
       << ">(" << SanitizeFieldName(prev->Name()) << ".BinaryEndOffset()))\n";
  }

  return absl::OkStatus();
}

absl::Status Generator::GenerateDefaultConstructor(const Message &msg,
                                                   std::ostream &os) {
  os << "  " << msg.Name() << "()";
  if (absl::Status status = GenerateFieldInitializers(msg, os); !status.ok()) {
    return status;
  }
  os << " {}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateEmbeddedConstructor(const Message &msg,
                                                    std::ostream &os) {
  os << "  " << msg.Name()
     << "(std::shared_ptr<PayloadBuffer *> buffer, BufferOffset offset) : "
        "Message(buffer, offset)";
  if (absl::Status status = GenerateFieldInitializers(msg, os, ", ");
      !status.ok()) {
    return status;
  }
  os << " {}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateNonEmbeddedConstructor(const Message &msg,
                                                       std::ostream &os) {
  os << "  " << msg.Name() << "(std::shared_ptr<PayloadBuffer *> buffer)";
  if (absl::Status status = GenerateFieldInitializers(msg, os); !status.ok()) {
    return status;
  }
  os << " {\n";

  os << "    this->buffer = buffer;\n";
  os << "    void *data = PayloadBuffer::Allocate(buffer.get(), BinarySize(), "
        "8);\n";
  os << "    this->absolute_binary_offset = (*buffer)->ToOffset(data);\n";
  os << "  }\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateBinarySize(const Message &msg,
                                           std::ostream &os) {
  os << "  static constexpr size_t" << msg.Name() << "::BinarySize() {\n";
  auto &fields = msg.Fields();
  if (fields.empty()) {
    os << "    return 0;\n";
    return absl::OkStatus();
  }
  os << "    size_t offset = 0;\n";

  auto align = [&os, msg](std::shared_ptr<Field> prev,
                          std::shared_ptr<Field> field) {
    // The new offset is calculated by adding the current offset to the size of
    // the previous field and then aligning it to the alignment of the current
    // field.
    os << "    offset = davros::zeros::AlignedOffset<"
       << FieldAlignmentType(field) << ">(";
    os << "offset + ";
    if (prev->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(prev);
      os << MessageFieldTypeName(msg, msg_field) << "::BinarySize());\n";
    } else if (prev->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(prev);
      if (array->IsFixedSize()) {
        if (array->Base()->Type() == FieldType::kMessage) {
          auto msg_field =
              std::static_pointer_cast<MessageField>(array->Base());
          os << MessageFieldTypeName(msg, msg_field) << "::BinarySize() * "
             << array->Size() << ");\n";
        } else {
          os << "sizeof(" << FieldCType(prev->Type()) << ") * " << array->Size()
             << ");\n";
        }
      } else {
        os << "sizeof(davros::zeros::VectorHeader));\n";
      }
    } else {
      os << "sizeof(" << FieldCType(prev->Type()) << "));\n";
    }
  };

  // The remaining fields are aligned by their type from the end of the
  // previous field.
  for (size_t i = 1; i < fields.size(); i++) {
    auto &prev = fields[i - 1];
    auto &field = fields[i];
    align(prev, field);
  }
  // Last field.
  align(fields.back(), fields.back());

  os << "    return offset;\n";
  os << "  }\n\n";
  return absl::OkStatus();
}
} // namespace davros::zeros
