#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ply/core/ast.hpp"
namespace ply {
using Number = std::variant<std::int64_t, std::uint64_t, double>;
struct Scalar {
  std::string_view raw;
  std::optional<Number> value;
};
struct Field {
  std::string_view count;
  std::vector<Scalar> values;
};
struct Record {
  std::vector<Field> fields;
};
struct ElementData {
  const ast::Element *declaration = nullptr;
  std::optional<std::uint64_t> count;
  std::vector<const ast::Property *> properties;
  std::vector<Record> records;
  std::uint64_t emptyRecords = 0;
  std::vector<std::string_view> comments;
};
struct DataIssue {
  const cursiv::AstNode *node;
  std::string_view raw;
  const char *code;
  const char *message;
};
struct Data {
  std::string_view encoding;
  std::vector<ElementData> elements;
  std::vector<std::string_view> comments;
  std::vector<std::string_view> objectInfo;
  std::string_view trailing;
  std::vector<DataIssue> issues;
  explicit operator bool() const { return issues.empty(); }
};
std::string_view canonical_type(std::string_view name);
Data read_data(const ast::Document &document);
} // namespace ply
