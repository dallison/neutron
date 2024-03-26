#pragma once

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "davros/lex.h"
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace davros {

class Package;
class PackageScanner;
class Message;

enum class FieldType {
  kUnknown,
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
  kMessage,
};

class Constant {
public:
  Constant(FieldType type, std::string name,
           std::variant<int64_t, double, std::string> value)
      : type_(type), name_(std::move(name)), value_(std::move(value)) {}

  void Dump(std::ostream &os) const;
  std::string TypeName() const;
  const std::string &Name() const { return name_; }
  FieldType Type() const { return type_; }
std::variant<int64_t, double, std::string> Value() { return value_; }

private:
  FieldType type_;
  std::string name_;
  std::variant<int64_t, double, std::string> value_;
};

class Field {
public:
  Field() = default;
  virtual ~Field() = default;
  Field(FieldType type, std::string name)
      : type_(type), name_(std::move(name)){}

  virtual std::string TypeName() const;
  virtual void Dump(std::ostream &os) const;

  virtual const std::string &Name() const { return name_; }
  FieldType Type() const { return type_; }

  virtual bool IsArray() const { return false; }

private:
  FieldType type_;
  std::string name_;
  std::variant<int64_t, double, std::string> default_value_; // TODO: support this.
};

class MessageField : public Field {
public:
  MessageField(std::string name, std::string msg_package, std::string msg_name)
      : Field(FieldType::kMessage, std::move(name)),
        msg_package_(std::move(msg_package)), msg_name_(std::move(msg_name)) {}

  const std::string &MsgPackage() const { return msg_package_; }
  const std::string &MsgName() const { return msg_name_; }
  std::shared_ptr<Message> Msg() const { return msg_; }

  std::string TypeName() const override;

  void Resolved(std::shared_ptr<Message> msg) { msg_ = msg; }

private:
  std::string msg_package_;
  std::string msg_name_;
  std::shared_ptr<Message> msg_;
};

class ArrayField : public Field {
public:
  // If size is 0 then this is variable sized.
  ArrayField(std::shared_ptr<Field> base, int size)
      : base_(base), size_(size) {}

  std::shared_ptr<Field> Base() const { return base_; }
  int Size() const { return size_; }
  bool IsFixedSize() const { return size_ != 0; }

  std::string TypeName() const override;
  void Dump(std::ostream &os) const override;
  const std::string &Name() const override { return base_->Name(); }

  bool IsArray() const override { return true; }

private:
  std::shared_ptr<Field> base_;
  int size_;
};

// This is a generic generator that does nothing.  A language specific
// generator should derive from this class and provide the implemention
// of the Generate function.
class Generator {
public:
  Generator() = default;
  virtual ~Generator() = default;

  // Provide implementation in derived class.
  virtual absl::Status Generate(const Message& msg) {
    return absl::InternalError("No generator provided");
  }
};

class Message {
public:
  Message(std::shared_ptr<Package> package, std::string name)
      : package_(std::move(package)), name_(std::move(name)) {}

  virtual ~Message() = default;

  absl::Status Parse(LexicalAnalyzer &lex);

  void Dump(std::ostream &os) const;

  const std::shared_ptr<Package> Package() const { return package_; }
  const std::string Name() const { return name_; }
  const std::vector<std::shared_ptr<Field>>& Fields() const { return fields_; }
  const std::map<std::string, std::shared_ptr<Constant>>& Constants() const { return constants_; }
 
  absl::Status Resolve(std::shared_ptr<PackageScanner> scanner);

  absl::Status Generate(Generator &gen) const { return gen.Generate(*this); }

  bool IsEnum() const;

private:
  std::shared_ptr<class Package> package_;
  std::string name_;
  std::vector<std::shared_ptr<Field>> fields_;
  absl::flat_hash_map<std::string, std::shared_ptr<Field>> field_map_;

  // This is ordered alphabetically so we can compare against known values
  // in unit tests.
  std::map<std::string, std::shared_ptr<Constant>> constants_;

  static absl::flat_hash_map<Token, FieldType> field_types_;
};

} // namespace davros
