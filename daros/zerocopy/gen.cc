
#include "daros/serdes/gen.h"
#include <fstream>

namespace daros::zerocopy {

absl::Status Generator::Generate(const Message &msg) {
  if (absl::Status status = GenerateHeader(msg, std::cout); !status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

static std::string FieldClass(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "daros::Int8Field";
  case FieldType::kUint8:
    return "daros::Uint8Field";
  case FieldType::kInt16:
    return "daros::Int16Field";
  case FieldType::kUint16:
    return "daros::Uint16Field";
  case FieldType::kInt32:
    return "daros::Int32Field";
  case FieldType::kUint32:
    return "daros::Uint32Field";
  case FieldType::kInt64:
    return "daros::Int4Field";
  case FieldType::kUint64:
    return "daros::Uint64Field";
  case FieldType::kFloat32:
    return "daros::Float32Field";
  case FieldType::kFloat64:
    return "daros::Float64Field";
  case FieldType::kTime:
    return "daros::TimeField";
  case FieldType::kDuration:
    return "daros::DurationField";
  case FieldType::kString:
    return "daros::StringField";
  case FieldType::kMessage:
    return "daros:MessageField";
  case FieldType::kBool:
    return "daros:BoolField";
  case FieldType::kUnknown:
    abort();
  }
}

static std::string SanitizeFieldName(const std::string &name) { return name; }

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "#include \"daros/serdes/runtime.h\"\n";
  os << "\n";
  os << "namespace " << msg.Package()->Name() << " {\n";
  os << "class " << msg.Name() << " {\n";
  os << "public:\n";
  os << "  explicit " << msg.Name() << "(void* data) : data_(data) {}\n";
  for (auto &field : msg.Fields()) {
    if (field->IsArray()) {

    } else {
      os << "  " << FieldClass(field->Type()) << " "
         << SanitizeFieldName(field->Name()) << ";\n";
    }
  }
  os << "private:\n";
  os << "  void* data_;\n";
  os << "};\n";
  os << "}    // namespace " << msg.Package()->Name() << "\n";

  return absl::OkStatus();
}

absl::Status Generator::Serialize(const Message &msg, std::ostream &os) {
  return absl::OkStatus();
}
} // namespace daros::zerocopy
