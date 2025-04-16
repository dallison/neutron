
#include "neutron/c_serdes/gen.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_format.h"
#include "neutron/common_gen.h"
//#include "neutron/descriptor.h"
#include "neutron/md5.h"
#include <fstream>

namespace neutron::c_serdes {

std::string Generator::Namespace(bool prefix_underscore) {
  std::string ns;
  if (namespace_.empty()) {
    return ns;
  }
  if (prefix_underscore) {
    ns = "_";
  }
  ns += namespace_;
  if (!prefix_underscore) {
    ns += "_";
  }
  return ns;
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
  std::filesystem::path source = dir / std::filesystem::path(msg.Name() + ".c");

  std::ofstream out(header.string());
  if (!out) {
    return absl::InternalError(
        absl::StrFormat("Unable to create %s", header.string()));
  }
  if (absl::Status status = GenerateHeader(msg, out); !status.ok()) {
    return status;
  }
  std::cout << "Generated C header file " << header.string() << std::endl;

  {
    std::ofstream out(source.string());
    if (!out) {
      return absl::InternalError(
          absl::StrFormat("Unable to create %s", source.string()));
    }
    if (absl::Status status = GenerateSource(msg, out); !status.ok()) {
      return status;
    }
    std::cout << "Generated C source file " << source.string() << std::endl;
  }

  return absl::OkStatus();
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

static std::string EnumCTypeName(const Message &msg) {
  int size = EnumCSize(msg);

  switch (size) {
  case 0:
  default:
    return "Uint8";
  case 1:
    return "Uint8";
  case 2:
    return "Uint16";
  case 4:
    return "Uint32";
  case 8:
    return "Uint64";
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
    return "NeutronTime";
  case FieldType::kDuration:
    return "NeutronDuration";
  case FieldType::kString:
    return "char";
  case FieldType::kBool:
    return "bool";
  case FieldType::kMessage:
    std::cerr << "Can't use message field type in FieldCType\n";
    return "<message>";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(type) << std::endl;
    abort();
  }
}

static std::string FieldCTypeName(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "Int8";
  case FieldType::kUint8:
    return "Uint8";
  case FieldType::kInt16:
    return "Int16";
  case FieldType::kUint16:
    return "Uint16";
  case FieldType::kInt32:
    return "Int32";
  case FieldType::kUint32:
    return "Uint32";
  case FieldType::kInt64:
    return "Int64";
  case FieldType::kUint64:
    return "Uint64";
  case FieldType::kFloat32:
    return "Float";
  case FieldType::kFloat64:
    return "Double";
  case FieldType::kTime:
    return "Time";
  case FieldType::kDuration:
    return "Duration";
  case FieldType::kString:
    return "Char";
  case FieldType::kBool:
    return "Bool";
  case FieldType::kMessage:
    return ""; // Won't be used.
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(type) << std::endl;
    abort();
  }
}

static std::string SanitizeFieldName(const std::string &name) {
  return name + (IsCReservedWord(name) ? "_" : "");
}

std::string
Generator::MessageFieldTypeName(const Message &msg,
                                std::shared_ptr<MessageField> field) {
  std::string name;
  if (field->MsgPackage().empty()) {
    return msg.Package()->Name() + "_" + Namespace(false) + field->MsgName();
  }
  return field->MsgPackage() + "_" + Namespace(false) + field->MsgName();
}

static std::string
MessageFieldIncludeFile(const Message &msg,
                        std::shared_ptr<MessageField> field) {
  if (field->MsgPackage().empty()) {
    return "c_serdes/" + msg.Package()->Name() + "/" + field->MsgName() + ".h";
  }
  return "c_serdes/" + field->MsgPackage() + "/" + field->MsgName() + ".h";
}

std::shared_ptr<Field> Generator::ResolveField(std::shared_ptr<Field> field) {
  if (field->IsArray()) {
    auto array = std::static_pointer_cast<ArrayField>(field);
    return array->Base();
  }
  return field;
}

// Magic helper templates for std::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "// File was generated by Neutron "
        "(https://github.com/dallison/neutron)\n";
  os << "// It's probably best not to modify it, but I can't stop you\n";
  os << "#pragma once\n";
  os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
     << "neutron/c_serdes/runtime.h\"\n";
  // os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
  //    << "neutron/c_serdes/mux.h\"\n";
  os << "\n";
  // Include files for message fields
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

  os << "#if defined(__cplusplus)\n";
  os << "extern \"C\" {\n";
  os << "#endif\n";
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
  os << "#if defined(__cplusplus)\n";
  os << "}\n";
  os << "#endif\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateEnum(const Message &msg, std::ostream &os) {
  for (auto & [ name, c ] : msg.Constants()) {
    os << "const " << EnumCType(msg) << " " << FullMessageName(msg) << "_"
       << SanitizeFieldName(c->Name()) << " = " << std::get<0>(c->Value())
       << ";\n";
  }
  return absl::OkStatus();
}

absl::Status Generator::GenerateStruct(const Message &msg, std::ostream &os) {

  // Constants.
  for (auto & [ name, c ] : msg.Constants()) {
    if (c->Type() == FieldType::kString) {
      os << "const char " << FullMessageName(msg) << "_"
         << SanitizeFieldName(c->Name()) << "[] = ";
    } else {
      os << "const " << FieldCType(c->Type()) << " " << FullMessageName(msg)
         << "_" << SanitizeFieldName(c->Name()) << " = ";
    }

    std::visit(overloaded{[&os](int64_t v) { os << v; },
                          [&os](double v) { os << v; },
                          [&os](std::string v) { os << '"' << v << '"'; }},
               c->Value());
    os << ";" << std::endl;
  }
  os << std::endl;

  os << "typedef struct __attribute__((packed)) {\n";

  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  " << EnumCType(*msg_field->Msg()) << " "
           << SanitizeFieldName(field->Name()) << ";\n";
      } else {
        os << "  " << MessageFieldTypeName(msg, msg_field) << " "
           << SanitizeFieldName(field->Name()) << ";\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        if (msg_field->Msg()->IsEnum()) {
          os << "  " << EnumCType(*msg_field->Msg());
        } else {
          os << "  " << MessageFieldTypeName(msg, msg_field);
        }
      } else {
        os << "  " << FieldCType(array->Base()->Type());
      }
      os << " " << SanitizeFieldName(field->Name());
      os << "[";
      if (array->IsFixedSize()) {
        os << array->Size();
      } else {
        return absl::InternalError("Vectors not supported yet");
      }
      os << "];\n";
    } else if (field->Type() == FieldType::kString) {
      return absl::InternalError("Strings not supported yet");
    } else {
      os << "  " << FieldCType(field->Type()) << " "
         << SanitizeFieldName(field->Name()) << ";\n";
    }
  }
  os << "} " << FullMessageName(msg) << ";\n";

  os << "\n";
  os << "const char* " << FullMessageName(msg) << "_"
     << "Name();\n";
  os << "const char* " << FullMessageName(msg) << "_"
     << "FullName();\n";
  os << "bool " << FullMessageName(msg) << "_"
     << "SerializeToArray(const " << FullMessageName(msg)
     << "* msg, char* addr, size_t len);\n";
  os << "bool " << FullMessageName(msg) << "_"
     << "SerializeToBuffer(const " << FullMessageName(msg)
     << "* msg, NeutronBuffer* buffer) "
        ";\n";
  os << "bool " << FullMessageName(msg) << "_"
     << "DeserializeFromArray(" << FullMessageName(msg)
     << "* msg, const char* addr, size_t "
        "len);\n";
  os << "bool " << FullMessageName(msg) << "_"
     << "DeserializeFromBuffer(" << FullMessageName(msg)
     << "* msg, NeutronBuffer* "
        "buffer);\n";

  os << "const char* " << FullMessageName(msg) << "_"
     << "MD5();\n";

  return absl::OkStatus();
} // namespace neutron::c_serdes

absl::Status Generator::GenerateSource(const Message &msg, std::ostream &os) {
  os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
     << "c_serdes/" << msg.Package()->Name() << "/" << msg.Name() << ".h\"\n";

  if (msg.IsEnum()) {
    return absl::OkStatus();
  }
  os << "#if defined(__cplusplus)\n";
  os << "extern \"C\" {\n";
  os << "#endif\n";

  os << "const char* " << FullMessageName(msg) << "_"
     << "Name() { return \"" << msg.Name() << "\"; }\n";
  os << "const char* " << FullMessageName(msg) << "_"
     << "FullName() { return \"" << msg.Package()->Name() << "/" << msg.Name()
     << "\"; }\n";

  os << "const char* " << FullMessageName(msg) << "_"
     << "MD5() {\n";
  os << "  return \"" << msg.Md5() << "\";\n";
  os << "}\n\n";

  os << "bool " << FullMessageName(msg) << "_SerializeToArray(const "
     << FullMessageName(msg) << "*msg, char* addr, size_t len) {\n";
  os << "  NeutronBuffer buffer;\n";
  os << "  NeutronBufferInit(&buffer, addr, len);\n";
  os << "  return " << FullMessageName(msg)
     << "_SerializeToBuffer(msg, &buffer);\n";
  os << "}\n\n";
  os << "bool " << FullMessageName(msg) << "_DeserializeFromArray("
     << FullMessageName(msg)
     << "* msg, const char* addr, size_t len) "
        "{\n";
  os << "  NeutronBuffer buffer;\n";
  os << "  NeutronBufferInit(&buffer, (char*)addr, len);\n";
  os << "  return " << FullMessageName(msg)
     << "_DeserializeFromBuffer(msg, &buffer);\n";
  os << "}\n\n";

  if (absl::Status status = GenerateSerializer(msg, os); !status.ok()) {
    return status;
  }

  if (absl::Status status = GenerateDeserializer(msg, os); !status.ok()) {
    return status;
  }

  os << "#if defined(__cplusplus)\n";
  os << "}\n";
  os << "#endif\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateSerializer(const Message &msg,
                                           std::ostream &os) {
  os << "bool " << FullMessageName(msg) << "_SerializeToBuffer(const "
     << FullMessageName(msg) << "* msg, NeutronBuffer* buffer) {\n";
  os << "  bool status;\n";
  for (auto &field : msg.Fields()) {
    std::string write_field;
    std::string write = "Write";
    if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      write_field = write + FieldCTypeName(array->Base()->Type()) + "Array";
    } else {
      write_field = write + FieldCTypeName(field->Type()) + "Field";
    }
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  status = NeutronBufferWrite"
           << EnumCTypeName(*msg_field->Msg()) << "Field(buffer, msg->"
           << SanitizeFieldName(field->Name()) << ")"
           << "; if (!status) return status;\n";
      } else {
        os << "  status = " << MessageFieldTypeName(msg, msg_field) << "_"
           << "SerializeToBuffer(&msg->" << SanitizeFieldName(field->Name())
           << ", buffer)"
           << "; if (!status) return status;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());

        if (msg_field->Msg()->IsEnum()) {
          os << "  status = NeutronBufferWrite"
             << EnumCTypeName(*msg_field->Msg()) << "Array(buffer, msg->"
             << SanitizeFieldName(field->Name()) << ", " << array->Size()
             << "); "
                "if (!status) return status;\n";
        } else {
          os << "  for (size_t i = 0; i < " << array->Size() << "; i++) {\n";
          os << "    const " << MessageFieldTypeName(msg, msg_field)
             << "* m = &msg->" << SanitizeFieldName(field->Name()) << "[i];\n";
          os << "    status = " << MessageFieldTypeName(msg, msg_field) << "_"
             <<  "SerializeToBuffer(m, buffer)"
             << "; if (!status) return status;\n";
          os << "  }\n";
        }
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  status = NeutronBuffer" << write_field << "(buffer, msg->"
           << SanitizeFieldName(field->Name()) << ", " << array->Size()
           << "); if (!status) return status;\n";
      }
    } else {
      os << "  status = NeutronBuffer" << write_field << "(buffer, msg->"
         << SanitizeFieldName(field->Name())
         << "); if (!status) return status;\n";
    }
  }
  os << "  return true;\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateDeserializer(const Message &msg,
                                             std::ostream &os) {
  os << "bool " << FullMessageName(msg) << "_DeserializeFromBuffer("
     << FullMessageName(msg) << "* msg, NeutronBuffer* buffer) {\n";
  os << " bool status;\n";

  os << "#pragma clang diagnostic push\n";
  os << "#pragma clang diagnostic ignored \"-Waddress-of-packed-member\"\n";

  for (auto &field : msg.Fields()) {
    std::string read_field;
    std::string read = "Read";
    if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      read_field = read + FieldCTypeName(array->Base()->Type()) + "Array";
    } else {
      read_field = read + FieldCTypeName(field->Type()) + "Field";
    }
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
           os << "  status = NeutronBufferRead"
           << EnumCTypeName(*msg_field->Msg())
           << "Field(buffer, " << "  &msg->" << SanitizeFieldName(field->Name()) << ");" <<
              "if (!status) return status;\n";
      } else {
        os << "  status = " << MessageFieldTypeName(msg, msg_field)
           << "_DeserializeFromBuffer(&msg->" << SanitizeFieldName(field->Name())
           << ", buffer)"
           << "; if (!status) return "
              "status;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        if (!array->IsFixedSize()) {
          return absl::InternalError("Vectors not supported yet");
        }
        os << "    for (size_t i = 0; i < " << array->Size() << "; i++) {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "        status = NeutronBufferRead"
             << EnumCTypeName(*msg_field->Msg())
             << "Field(buffer, &msg->" << SanitizeFieldName(field->Name()) << "[i]);"
                "if (!status) "
                "return status;\n";
        } else {
          os << "      status = " << MessageFieldTypeName(msg, msg_field) << "_"
             << "DeserializeFromBuffer(&msg->" << SanitizeFieldName(field->Name())
             << "[i], buffer)"
             << "; if (!status) return status;\n";
        }
        os << "  }\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  status = NeutronBuffer" << read_field << "(buffer, msg->"
           << SanitizeFieldName(field->Name()) << ", " << array->Size()
           << "); if (!status) return status;\n";
      }
    } else {
      os << "  status = NeutronBuffer" << read_field << "(buffer, &msg->"
         << SanitizeFieldName(field->Name())
         << "); if (!status) return status;\n";
    }
  }
  os << "  return true;\n";
  os << "}\n\n";
  os << "#pragma clang diagnostic pop\n\n";
  return absl::OkStatus();
}

} // namespace neutron::c_serdes
