
#include "davros/zeros/gen.h"
#include <fstream>

namespace davros::zerocopy {

absl::Status Generator::Generate(const Message &msg) {
  if (absl::Status status = GenerateHeader(msg, std::cout); !status.ok()) {
    return status;
  }
  return absl::OkStatus();
}

static std::string FieldClass(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "davros::Int8Field";
  case FieldType::kUint8:
    return "davros::Uint8Field";
  case FieldType::kInt16:
    return "davros::Int16Field";
  case FieldType::kUint16:
    return "davros::Uint16Field";
  case FieldType::kInt32:
    return "davros::Int32Field";
  case FieldType::kUint32:
    return "davros::Uint32Field";
  case FieldType::kInt64:
    return "davros::Int4Field";
  case FieldType::kUint64:
    return "davros::Uint64Field";
  case FieldType::kFloat32:
    return "davros::Float32Field";
  case FieldType::kFloat64:
    return "davros::Float64Field";
  case FieldType::kTime:
    return "davros::TimeField";
  case FieldType::kDuration:
    return "davros::DurationField";
  case FieldType::kString:
    return "davros::StringField";
  case FieldType::kMessage:
    return "davros:MessageField";
  case FieldType::kBool:
    return "davros:BoolField";
  case FieldType::kUnknown:
    abort();
  }
}

static std::string SanitizeFieldName(const std::string &name) { return name; }

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "#include \"davros/zeros/runtime.h\"\n";
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
} // namespace davros::zerocopy
