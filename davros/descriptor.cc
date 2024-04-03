#include "davros/descriptor.h"
#include "absl/container/flat_hash_set.h"
#include "davros/package.h"
#include "toolbelt/hexdump.h"

namespace davros {

static int FromFieldType(FieldType t) {
  switch (t) {
    case FieldType::kInt8:
      return descriptor::Field::TYPE_INT8;
    case FieldType::kUint8:
      return descriptor::Field::TYPE_UINT8;
    case FieldType::kInt16:
      return descriptor::Field::TYPE_INT16;
    case FieldType::kUint16:
      return descriptor::Field::TYPE_UINT16;
    case FieldType::kInt32:
      return descriptor::Field::TYPE_INT32;
    case FieldType::kUint32:
      return descriptor::Field::TYPE_UINT32;
    case FieldType::kInt64:
      return descriptor::Field::TYPE_INT64;
    case FieldType::kUint64:
      return descriptor::Field::TYPE_UINT64;
    case FieldType::kFloat32:
      return descriptor::Field::TYPE_FLOAT32;
    case FieldType::kFloat64:
      return descriptor::Field::TYPE_FLOAT64;
    case FieldType::kTime:
      return descriptor::Field::TYPE_TIME;
    case FieldType::kDuration:
      return descriptor::Field::TYPE_DURATION;
    case FieldType::kString:
      return descriptor::Field::TYPE_STRING;
    case FieldType::kBool:
      return descriptor::Field::TYPE_BOOL;
    case FieldType::kMessage:
      return descriptor::Field::TYPE_MESSAGE;
    case FieldType::kUnknown:
      std::cerr << "Unknown field type " << int(t) << std::endl;
      abort();
  }
}

absl::StatusOr<descriptor::Descriptor> MakeDescriptor(const Message &msg) {
  descriptor::Descriptor desc;
  desc.package = msg.Package()->Name();
  desc.name = msg.Name();
  int index = 0;
  absl::flat_hash_set<std::string> imports;

  for (auto &field : msg.Fields()) {
    descriptor::Field f;
    f.index = index++;
    if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      f.type = FromFieldType(array->Base()->Type());
    } else {
      f.type = FromFieldType(field->Type());
    }
    f.name = field->Name();
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      f.msg_package = msg_field->MsgPackage().empty() ? msg.Package()->Name()
                                                      : msg_field->MsgPackage();
      f.msg_name = msg_field->Name();
      f.array_size = descriptor::Field::FIELD_PRIMITIVE;
      imports.insert(f.msg_package + "/" + f.msg_name);
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->IsFixedSize()) {
        f.array_size = array->Size();
      } else {
        f.array_size = descriptor::Field::FIELD_VECTOR;
      }
      if (array->Base()->Type() == FieldType::kMessage) {
        // Array of messages.
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        f.msg_package = msg_field->MsgPackage().empty()
                            ? msg.Package()->Name()
                            : msg_field->MsgPackage();
        f.msg_name = msg_field->Name();
        imports.insert(f.msg_package + "/" + f.msg_name);
      }
    } else {
      f.array_size = descriptor::Field::FIELD_PRIMITIVE;
    }
    desc.fields.push_back(f);
  }
  for (auto &import : imports) {
    desc.imports.push_back(import);
  }
  return desc;
}

absl::Status EncodeDescriptorAsHex(const descriptor::Descriptor &desc,
                                   int max_width, bool with_0x_prefix,
                                   std::ostream &os) {
  davros::serdes::Buffer buffer;
  if (absl::Status status = desc.SerializeToBuffer(buffer); !status.ok()) {
    return status;
  }

  const char *addr = buffer.data();
  ssize_t length = buffer.Size();

  while (length > 0) {
    int remaining = max_width;
    const char *sep = "";
    while (length > 0 && remaining > 0) {
      os << sep << (with_0x_prefix ? "0x" : "") << std::hex << std::setw(2)
         << std::setfill('0') << (*addr++ & 0xff);
      length--;
      remaining -= (with_0x_prefix ? 4 : 2);
      if (sep[0] == '\0') {
        sep = ",";
      } else {
        remaining--;
      }
      if (remaining <= 0) {
        os << ",\n";
      }
    }
  }
  os << std::endl;
  return absl::OkStatus();
}

absl::StatusOr<descriptor::Descriptor> DecodeDescriptor(std::istream &is) {
  descriptor::Descriptor desc;
  return desc;
}
}  // namespace davros
