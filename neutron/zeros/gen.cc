
#include "neutron/zeros/gen.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_format.h"
#include "neutron/common_gen.h"
#include "neutron/descriptor.h"
#include <fstream>
#include <memory>

namespace neutron::zeros {

// Magic helper templates for std::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

std::string Generator::Namespace(bool prefix_colon_colon) {
  std::string ns;
  if (namespace_.empty()) {
    return ns;
  }
  if (prefix_colon_colon) {
    ns = "::";
  }
  ns += namespace_;
  if (!prefix_colon_colon) {
    ns += "::";
  }
  return ns;
}

std::shared_ptr<Field> Generator::ResolveField(std::shared_ptr<Field> field) {
  if (field->IsArray()) {
    auto array = std::static_pointer_cast<ArrayField>(field);
    return array->Base();
  }
  return field;
}

static bool IsEnum(std::shared_ptr<Field> field) {
  auto msg_field = std::static_pointer_cast<MessageField>(field);
  return msg_field->Msg()->IsEnum();
}

absl::Status Generator::Generate(const Message &msg) {
  std::filesystem::path dir =
      root_ / std::filesystem::path(msg.Package()->Name());
  if (!std::filesystem::exists(dir) &&
      !std::filesystem::create_directories(dir)) {
    return absl::InternalError(
        absl::StrFormat("Unable to create directory %s", dir.string()));
  }

  std::filesystem::path header = dir / std::filesystem::path(msg.Name() + ".h");
  std::filesystem::path source =
      dir / std::filesystem::path(msg.Name() + ".cc");

  std::ofstream out(header.string());
  if (!out) {
    return absl::InternalError(
        absl::StrFormat("Unable to create %s", header.string()));
  }
  if (absl::Status status = GenerateHeader(msg, out); !status.ok()) {
    return status;
  }
  std::cout << "Generated header file " << header.string() << std::endl;

  {
    std::ofstream out(source.string());
    if (!out) {
      return absl::InternalError(
          absl::StrFormat("Unable to create %s", source.string()));
    }
    if (absl::Status status = GenerateSource(msg, out); !status.ok()) {
      return status;
    }
    std::cout << "Generated source file " << source.string() << std::endl;
  }

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
    return "Int64Field";
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
    return "neutron::Time";
  case FieldType::kDuration:
    return "neutron::Duration";
  case FieldType::kString:
    return "toolbelt::StringHeader";
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
    return "neutron::Time";
  case FieldType::kDuration:
    return "neutron::Duration";
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
    if (IsEnum(field)) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      return EnumCType(*msg_field->Msg());
    }
    return "int32_t";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type for " << field->Name() << " "
              << int(field->Type()) << std::endl;
    abort();
  }
}

static std::string SerdesFieldCType(FieldType type) {
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
    return "neutron::Time";
  case FieldType::kDuration:
    return "neutron::Duration";
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

std::string
Generator::MessageFieldTypeName(const Message &msg,
                                std::shared_ptr<MessageField> field) {
  std::string name;
  if (field->MsgPackage().empty()) {
    return msg.Package()->Name() + "::" + Namespace(false) + field->MsgName();
  }
  return field->MsgPackage() + "::" + Namespace(false) + field->MsgName();
}

static std::string
MessageFieldIncludeFile(const Message &msg,
                        std::shared_ptr<MessageField> field) {
  if (field->MsgPackage().empty()) {
    return "zeros/" + msg.Package()->Name() + "/" + field->MsgName() + ".h";
  }
  return "zeros/" + field->MsgPackage() + "/" + field->MsgName() + ".h";
}

static std::string SanitizeFieldName(const std::string &name) {
  return name + (IsCppReservedWord(name) ? "_" : "");
}

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "// File was generated by Neutron "
        "(https://github.com/dallison/neutron)\n";
  os << "// It's probably best not to modify it, but I can't stop you\n";
  os << "#pragma once\n";
  os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
     << "neutron/zeros/runtime.h\"\n";
  os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
     << "neutron/zeros/buffer.h\"\n";
  // Include files for message fields
  os << "// Message field definitions.\n";
  absl::flat_hash_set<std::string> hdrs;
  for (auto field : msg.Fields()) {
    field = ResolveField(field);
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      std::string hdr = MessageFieldIncludeFile(msg, msg_field);
      if (hdrs.contains(hdr)) {
        continue;
      }
      hdrs.insert(hdr);
      os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/")) << hdr
         << "\"\n";
    }
  }
  os << "\n";
  os << "namespace " << msg.Package()->Name() << Namespace(true) << " {\n";

  if (msg.IsEnum()) {
    // Enumeration.
    if (absl::Status status = GenerateEnum(msg, os); !status.ok()) {
      return status;
    }
    if (absl::Status status = GenerateEnumStreamer(msg, os); !status.ok()) {
      return status;
    }
  } else {
    if (absl::Status status = GenerateStruct(msg, os); !status.ok()) {
      return status;
    }
    if (absl::Status status = GenerateStructStreamer(msg, os); !status.ok()) {
      return status;
    }
  }
  os << "}    // namespace " << msg.Package()->Name() << Namespace(true)
     << "\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateEnum(const Message &msg, std::ostream &os) {
  os << "enum class " << msg.Name() << " : " << EnumCType(msg) << " {\n";
  for (auto & [ name, c ] : msg.Constants()) {
    os << "  " << SanitizeFieldName(c->Name()) << " = "
       << std::get<0>(c->Value()) << ",\n";
  }
  os << "};\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateStruct(const Message &msg, std::ostream &os) {
  os << "#pragma clang diagnostic push\n";
  os << "#pragma clang diagnostic ignored \"-Winvalid-offsetof\"\n";
  os << "struct " << msg.Name() << " : public neutron::zeros::Message {\n";

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
  if (absl::Status status = GenerateCreators(msg, os); !status.ok()) {
    return status;
  }

  // Constants.
  for (auto & [ name, c ] : msg.Constants()) {
    if (c->Type() == FieldType::kString) {
      os << "  static inline constexpr const char "
         << SanitizeFieldName(c->Name()) << "[] = ";
    } else {
      os << "  static constexpr " << ConstantCType(c->Type()) << " "
         << SanitizeFieldName(c->Name()) << " = ";
    }

    std::visit(overloaded{[&os](int64_t v) { os << v; },
                          [&os](double v) { os << v; },
                          [&os](std::string v) { os << '"' << v << '"'; }},
               c->Value());
    os << ";" << std::endl;
  }

  for (auto &field : msg.Fields()) {
    os << "  neutron::zeros::";
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
          os << "PrimitiveArrayField<" << FieldCType(array->Base()->Type())
             << ", ";
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
          os << "PrimitiveVectorField<" << FieldCType(array->Base()->Type())
             << ">";
        }
      }
    } else {
      os << FieldClass(field->Type());
    }
    os << " " << SanitizeFieldName(field->Name()) << " = {};\n";
  }
  os << "  static const char* Name() { return \"" << msg.Name() << "\"; }\n";
  os << "  static const char* FullName() { return \"" << msg.Package()->Name()
     << "/" << msg.Name() << "\"; }\n";
  os << "  absl::Status SerializeToArray(char* addr, size_t len) const;\n";
  os << "  absl::Status SerializeToBuffer(neutron::zeros::Buffer& buffer) "
        "const;\n";
  os << "  absl::Status DeserializeFromArray(const char* addr, size_t "
        "len);\n";
  os << "  absl::Status DeserializeFromBuffer(neutron::zeros::Buffer& "
        "buffer);\n";
  os << "  size_t SerializedSize() const;\n";
  os << "  bool operator==(const " << msg.Name() << "& m) const;\n";
  os << "  bool operator!=(const " << msg.Name() << "& m) const {\n";
  os << "    return !this->operator==(m);\n";
  os << "  }\n";
  os << "  std::string DebugString() const;\n";
  os << "  static constexpr unsigned char _descriptor[] = {\n";
  absl::StatusOr<descriptor::Descriptor> desc = MakeDescriptor(msg);
  if (!desc.ok()) {
    return desc.status();
  }
  if (absl::Status status = EncodeDescriptorAsHex(*desc, 80, true, os);
      !status.ok()) {
    return status;
  }
  os << "  };\n";
  os << "};\n";
  os << "#pragma clang diagnostic pop\n\n";

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
  auto field = ResolveField(fields[0]);
  os << sep << SanitizeFieldName(field->Name()) << "(";
  if (field->Type() == FieldType::kMessage && !IsEnum(field)) {
    os << "buffer, ";
  }
  os << "offsetof(" << msg.Name() << ", " << SanitizeFieldName(field->Name())
     << "), 0)\n";

  // The remaining fields are aligned by their type from the end of the
  // previous field.
  for (size_t i = 1; i < fields.size(); i++) {
    auto prev = fields[i - 1];
    auto field = fields[i];

    auto resolved_field = ResolveField(field);
    os << "  , " << SanitizeFieldName(field->Name()) << "(";
    if (!field->IsArray() && resolved_field->Type() == FieldType::kMessage &&
        !IsEnum(resolved_field)) {
      os << "buffer, ";
    }
    os << "offsetof(" << msg.Name() << ", " << SanitizeFieldName(field->Name())
       << "), neutron::zeros::AlignedOffset<"
       << FieldAlignmentType(resolved_field) << ">("
       << SanitizeFieldName(prev->Name()) << ".BinaryEndOffset()))\n";
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
     << "(std::shared_ptr<toolbelt::PayloadBuffer *> buffer, "
        "toolbelt::BufferOffset offset) : "
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
  os << "  " << msg.Name()
     << "(std::shared_ptr<toolbelt::PayloadBuffer *> buffer)";
  if (absl::Status status = GenerateFieldInitializers(msg, os); !status.ok()) {
    return status;
  }
  os << " {\n";

  os << "    this->buffer = buffer;\n";
  os << "    void *data = toolbelt::PayloadBuffer::Allocate(buffer.get(), "
        "BinarySize(), "
        "8);\n";
  os << "    this->absolute_binary_offset = (*buffer)->ToOffset(data);\n";
  os << "  }\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateBinarySize(const Message &msg,
                                           std::ostream &os) {
  os << "  static constexpr size_t BinarySize() {\n";
  auto &fields = msg.Fields();
  if (fields.empty()) {
    os << "    return 0;\n";
    return absl::OkStatus();
  }
  os << "    size_t offset = 0;\n";

  auto align = [this, &os, msg](std::shared_ptr<Field> prev,
                                std::shared_ptr<Field> field) {
    auto resolved_prev = ResolveField(prev);
    auto resolved_field = ResolveField(field);
    // The new offset is calculated by adding the current offset to the size
    // of the previous field and then aligning it to the alignment of the
    // current field.
    os << "    /* ";
    if (prev == field) {
      // END field
      os << "END */ ";
    } else {
      os << field->Name() << " */ ";
    }
    os << "offset = neutron::zeros::AlignedOffset<"
       << FieldAlignmentType(resolved_field) << ">(";
    os << "offset + ";
    if (prev->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(prev);
      if (msg_field->Msg()->IsEnum()) {
        os << "sizeof(" << EnumCType(*msg_field->Msg()) << "));\n";
      } else {
        os << MessageFieldTypeName(msg, msg_field) << "::BinarySize());\n";
      }
    } else if (prev->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(prev);
      if (array->IsFixedSize()) {
        if (array->Base()->Type() == FieldType::kMessage) {
          auto msg_field =
              std::static_pointer_cast<MessageField>(array->Base());
          if (msg_field->Msg()->IsEnum()) {
            os << "sizeof (" << EnumCType(*msg_field->Msg()) << ") * "
               << array->Size() << ");\n";
          } else {
            os << MessageFieldTypeName(msg, msg_field) << "::BinarySize() * "
               << array->Size() << ");\n";
          }
        } else {
          os << "sizeof(" << FieldCType(resolved_prev->Type()) << ") * "
             << array->Size() << ");\n";
        }
      } else {
        os << "sizeof(toolbelt::VectorHeader));\n";
      }
    } else {
      os << "sizeof(" << FieldCType(resolved_prev->Type()) << "));\n";
    }
  };

  // The remaining fields are aligned by their type from the end of the
  // previous field.
  for (size_t i = 1; i < fields.size(); i++) {
    auto prev = fields[i - 1];
    auto field = fields[i];
    align(prev, field);
  }
  // Last field.
  align(fields.back(), fields.back());

  os << "    return offset;\n";
  os << "  }\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateStructStreamer(const Message &msg,
                                               std::ostream &os) {
  os << "inline std::ostream& operator<<(std::ostream& os, const " << msg.Name()
     << "&msg) {\n";
  for (auto &field : msg.Fields()) {
    os << "  os << msg." << SanitizeFieldName(field->Name())
       << " << std::endl;\n";
  }
  os << "  return os;\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateEnumStreamer(const Message &msg,
                                             std::ostream &os) {
  os << "inline std::ostream& operator<<(std::ostream& os, const " << msg.Name()
     << "&e) {\n";
  os << "  switch (e) {\n";
  for (auto &c : msg.Constants()) {
    os << "  case " << msg.Name() << "::" << SanitizeFieldName(c.first)
       << ":\n";
    os << "    os << \"" << c.first << "\" << std::endl;\n";
    os << "    break;\n";
  }
  os << "  }\n";
  os << "  return os;\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateSource(const Message &msg, std::ostream &os) {
  os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
     << "zeros/" << msg.Package()->Name() << "/" << msg.Name() << ".h\"\n";

  if (msg.IsEnum()) {
    return absl::OkStatus();
  }
  os << "namespace " << msg.Package()->Name() << Namespace(true) << " {\n";
  os << "absl::Status " << msg.Name()
     << "::SerializeToArray(char* addr, size_t len) const {\n";
  os << "  neutron::zeros::Buffer buffer(addr, len);\n";
  os << "  return SerializeToBuffer(buffer);\n";
  os << "}\n\n";
  os << "absl::Status " << msg.Name()
     << "::DeserializeFromArray(const char* addr, size_t len) {\n";
  os << "  neutron::zeros::Buffer buffer(const_cast<char*>(addr), len);\n";
  os << "  return DeserializeFromBuffer(buffer);\n";
  os << "}\n\n";

  if (absl::Status status = GenerateSerializer(msg, os); !status.ok()) {
    return status;
  }

  if (absl::Status status = GenerateDeserializer(msg, os); !status.ok()) {
    return status;
  }

  if (absl::Status status = GenerateLength(msg, os); !status.ok()) {
    return status;
  }

  os << "  bool " << msg.Name() << "::operator==(const " << msg.Name()
     << "& m) const {\n";
  for (auto &field : msg.Fields()) {
    os << "  if (this->" << SanitizeFieldName(field->Name()) << " != m."
       << SanitizeFieldName(field->Name()) << ") return false;\n";
  }
  os << "  return true;\n";
  os << "}\n\n";

  os << "std::string " << msg.Name() << "::DebugString() const {\n";
  os << "  std::stringstream s;\n";
  os << "  s << *this;\n";
  os << "  return s.str();\n";
  os << "}\n";
  os << "}    // namespace " << msg.Package()->Name() << Namespace(true)
     << "\n";

  return absl::OkStatus();
}

absl::Status Generator::GenerateSerializer(const Message &msg,
                                           std::ostream &os) {
  os << "absl::Status " << msg.Name()
     << "::SerializeToBuffer(neutron::zeros::Buffer& buffer) const {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg() == nullptr) {
        abort();
      }
      if (msg_field->Msg()->IsEnum()) {
        os << "  if (absl::Status status = buffer.Write(this->"
           << SanitizeFieldName(field->Name()) << ".GetUnderlying())"
           << "; !status.ok()) return status;\n";
      } else {
        os << "  if (absl::Status status = this->"
           << SanitizeFieldName(field->Name()) << ".SerializeToBuffer(buffer)"
           << "; !status.ok()) return status;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        if (!array->IsFixedSize()) {
          os << "  if (absl::Status status = buffer.Write(uint32_t(this->"
             << SanitizeFieldName(field->Name())
             << ".size())); !status.ok()) return status;\n";
        }
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "  for (auto& m : this->" << SanitizeFieldName(field->Name())
           << ") {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "    if (absl::Status status = buffer.Write("
             << EnumCType(*msg_field->Msg())
             << "(m)); "
                "!status.ok()) return status;\n";
        } else {
          os << "    if (absl::Status status = m.SerializeToBuffer(buffer)"
             << "; !status.ok()) return status;\n";
        }
        os << "  }\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  if (absl::Status status = buffer.Write(this->"
           << SanitizeFieldName(field->Name())
           << "); !status.ok()) return status;\n";
      }
    } else {
      os << "  if (absl::Status status = buffer.Write(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateDeserializer(const Message &msg,
                                             std::ostream &os) {
  os << "absl::Status " << msg.Name()
     << "::DeserializeFromBuffer(neutron::zeros::Buffer& buffer) {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  if (absl::Status status = buffer.Read(this->"
           << SanitizeFieldName(field->Name())
           << "); !status.ok()) return "
              "status;\n";
      } else {
        os << "  if (absl::Status status = this->"
           << SanitizeFieldName(field->Name())
           << ".DeserializeFromBuffer(buffer); !status.ok()) return "
              "status;\n";
      }

    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "  {\n";
        if (array->IsFixedSize()) {
          os << "    int32_t size = " << array->Size() << ";\n";
        } else {
          os << "    int32_t size;\n";
          os << "    if (absl::Status status = buffer.Read(size); "
                "!status.ok()) "
                "return status;\n";
        }
        os << "    for (int32_t i = 0; i < size; i++) {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "        " << EnumCType(*msg_field->Msg()) << " tmp;\n";
          os << "        if (absl::Status status = buffer.Read(tmp); "
                "!status.ok()) "
                "return status;\n";
          os << "        " << SanitizeFieldName(field->Name())
             << "[i] = static_cast<" << MessageFieldTypeName(msg, msg_field)
             << ">(tmp);\n";
          os << "      }\n";
        } else {
          os << "        if (absl::Status status = this->"
             << SanitizeFieldName(field->Name())
             << "[i].DeserializeFromBuffer(buffer)"
             << "; !status.ok()) return status;\n";
          os << "     }\n";
        }
        os << "  }\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  if (absl::Status status = buffer.Read(this->"
           << SanitizeFieldName(field->Name())
           << "); !status.ok()) return status;\n";
      }

    } else {
      os << "  if (absl::Status status = buffer.Read(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateLength(const Message &msg, std::ostream &os) {
  // Non-compact (ROS) serialized size.
  os << "size_t " << msg.Name() << "::SerializedSize() const {\n";
  os << "  size_t length = 0;\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  length += " << EnumCSize(*msg_field->Msg()) << ";\n";
      } else {
        os << "  length += this->" << SanitizeFieldName(field->Name())
           << ".SerializedSize();\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        if (msg_field->Msg()->IsEnum()) {
          os << "  length += " << (array->IsFixedSize() ? 0 : 4) << " + this->"
             << SanitizeFieldName(field->Name()) << ".size() * "
             << EnumCSize(*msg_field->Msg()) << ";\n";
        } else {

          if (!array->IsFixedSize()) {
            os << "  length += 4;\n";
          }
          os << "  for (auto& m : this->" << SanitizeFieldName(field->Name())
             << ") {\n";
          os << "    length += m.SerializedSize();\n";
          os << "  }\n";
        }
      } else {
        if (array->Base()->Type() == FieldType::kString) {
          // Each element is a string with a 4-byte length prefix.
          if (!array->IsFixedSize()) {
            os << "  length += 4;\n";
          }
          os << "  for (auto& s : this->" << SanitizeFieldName(field->Name())
             << ") {\n";
          os << "    length += 4 + s.size();\n";
          os << "  }\n";
        } else {
          os << "  length += " << (array->IsFixedSize() ? 0 : 4) << " + this->"
             << SanitizeFieldName(field->Name()) << ".size() * sizeof("
             << SerdesFieldCType(array->Base()->Type()) << ");\n";
        }
      }
    } else {
      if (field->Type() == FieldType::kString) {
        os << "  length += 4 + this->" << SanitizeFieldName(field->Name())
           << ".size();\n";
      } else {
        os << "  length += sizeof(" << SerdesFieldCType(field->Type())
           << ");\n";
      }
    }
  }
  os << "  return length;\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateCreators(const Message &msg, std::ostream &os) {
  os << "// Create a mutable message in the given memory.\n";
  os << "[[maybe_unused]] static " << msg.Name()
     << " CreateMutable(void *addr, size_t size) {\n"
        "  ::toolbelt::PayloadBuffer *pb = new (addr) "
        "::toolbelt::PayloadBuffer(size);\n"
        "  ::toolbelt::PayloadBuffer::AllocateMainMessage(&pb, "
     << msg.Name() << "::BinarySize());\n"
     << "  return " << msg.Name()
     << "(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);\n"
        "}\n\n";

  os << "// Create a readonly message that already exists at the given "
        "address with a size.\n";
  os << "[[maybe_unused]] static " << msg.Name()
     << " CreateReadonly(const void *addr, size_t size) {\n"
        "  ::toolbelt::PayloadBuffer *pb ="
        "reinterpret_cast<::toolbelt::PayloadBuffer "
        "*>(const_cast<void*>(addr));\n"
     << "  return " << msg.Name()
     << "(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);\n"
        "}\n\n";

  os << "// Create a message in a dynamically resized buffer allocated from "
        "the heap.\n";
  os << "[[maybe_unused]] static " << msg.Name()
     << " CreateDynamicMutable(size_t initial_size, "
        "std::function<absl::StatusOr<void*>(size_t)> alloc, "
        "std::function<void(void*)> free,"
        "std::function<absl::StatusOr<void*>(void*, size_t, size_t)> realloc) "
        "{\n"
        "  absl::StatusOr<::toolbelt::PayloadBuffer *> pbs = "
        "::neutron::zeros::NewDynamicBuffer(initial_size, std::move(alloc), "
        "std::move(realloc));\n"
        "  if (!pbs.ok()) abort();\n"
        "  ::toolbelt::PayloadBuffer *pb = *pbs;\n"
        "  ::toolbelt::PayloadBuffer::AllocateMainMessage(&pb, "
     << msg.Name() << "::BinarySize());\n"
     << "  return " << msg.Name()
     << "(std::make_shared<toolbelt::PayloadBuffer *>(pb), pb->message);\n"
        "}\n\n";

  os << "[[maybe_unused]] static " << msg.Name()
     << " CreateDynamicMutable(size_t initial_size = 1024) {\n";
  os << "  return CreateDynamicMutable(initial_size, [](size_t size) -> "
        "absl::StatusOr<void*>{ return ::malloc(size);},"
        " ::free,"
        " [](void* p, size_t old_size, size_t new_size) -> "
        "absl::StatusOr<void*> { return ::realloc(p, new_size);});\n";
  os << "}\n\n";

  os << "  // The buffer being used\n";
  os << "  char* Buffer() const { return reinterpret_cast<char*>(*buffer); }\n";
  os << "\n";

  os << "  // Size of the full message in the payload buffer\n";
  os << "  size_t Size() const { return (*buffer)->Size(); }\n";
  os << "\n";

  return absl::OkStatus();
}
} // namespace neutron::zeros
