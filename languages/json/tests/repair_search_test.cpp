#include <string>

#include "cursiv/Check.hpp"
#include "json/ast.hpp"
#include "json/generated/Json/Module.hpp"

int main() {
  const auto parser = json::generated::Json::createParser();
  const auto check = [&](const std::string& text, cursiv::DiagnosticKind kind,
                         unsigned begin, unsigned end, unsigned items) {
    const auto result = parser->parse(text);
    const auto* doc = result.valueAs<json::ast::Doc>();
    const auto* value = doc ? std::get_if<json::ast::Value*>(&doc->value) : nullptr;
    const auto* array = value ? cursiv::ast_cast<json::ast::Array>(*value) : nullptr;
    CHECK(result.accepted && array && array->items.size() == items);
    CHECK(result.diagnostics.size() == 1);
    for (const auto& d : result.diagnostics)
      CHECK(d.kind == kind && d.begin == begin && d.end == end);
  };
  check(R"([{"a": 1, {"b": 2}, {"c": 3}])", cursiv::DiagnosticKind::Deleted, 10, 11, 2);
  std::string records = R"([{"c": [1, 2 })";
  for (unsigned i = 0; i < 60; ++i) records += R"(, {"a": 1})";
  records += "]";
  check(records, cursiv::DiagnosticKind::Inserted, 12, 12, 61);
  return cursiv::test::report("json_repair_search_test");
}
