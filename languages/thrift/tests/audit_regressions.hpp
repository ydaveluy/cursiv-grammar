#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "thrift/core/ast.hpp"
#include "thrift/generated/Thrift/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = thrift::generated::Thrift::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = thrift::ast;
  auto r = parser->parse(
      "const i64 a = 0x7fffffffffffffff\nconst i64 b = -0x8000000000000000\n");
  check(r.accepted && !r.cst().recovered(), "hexadecimal signed boundaries");
  auto *d = as<ast::Document>(r.value);
  auto *a = d && d->definitions.size() == 2
                ? as<ast::ConstDef>(d->definitions[0])
                : nullptr;
  auto *b = d && d->definitions.size() == 2
                ? as<ast::ConstDef>(d->definitions[1])
                : nullptr;
  check(a && b && std::get<std::int64_t>(a->value) == INT64_MAX &&
            std::get<std::int64_t>(b->value) == INT64_MIN,
        "no wrap at hexadecimal boundaries");
  auto bad = parser->parse("const i64 a = 0x10000000000000000\n");
  check(!bad.diagnostics.empty(), "overflow is diagnosed, never silently zero");
  const std::string source = R"thrift(
namespace cpp sample (a="b")
typedef list cpp_type "std::vector<int>" <i32> Values;
typedef list<i32> cpp_type "legacy<int>" Legacy,
const bool enabled=true
const bool disabled=false
struct T xsd_all {
  1:i8 n=1;
  2:uuid id;
  3:bool flag=true;
  4:optional string & name xsd_optional xsd_nillable xsd_attrs {1:i32 code}
}
)thrift";
  const auto parsed = parser->parse(source);
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
        "native Thrift forms parse clean");
  const auto* doc = as<ast::Document>(parsed.value);
  check(doc != nullptr && doc->definitions.size() == 5 && doc->headers.size() == 1,
        "Thrift declaration and namespace counts");
  if (doc != nullptr && doc->definitions.size() == 5) {
    const auto* modern = as<ast::TypedefDef>(doc->definitions[0]);
    const auto* legacy = as<ast::TypedefDef>(doc->definitions[1]);
    const auto* modernList = modern ? as<ast::ListType>(modern->type) : nullptr;
    const auto* legacyList = legacy ? as<ast::ListType>(legacy->type) : nullptr;
    check(modernList && modernList->cppTypes.size() == 1 &&
              modernList->cppTypes[0] == "std::vector<int>" &&
              legacyList && legacyList->cppTypes.size() == 1 &&
              legacyList->cppTypes[0] == "legacy<int>",
          "both list cpp_type placements are retained");
    const auto* enabled = as<ast::ConstDef>(doc->definitions[2]);
    const auto* disabled = as<ast::ConstDef>(doc->definitions[3]);
    const auto* enabledValue = enabled ? std::get_if<std::int64_t>(&enabled->value) : nullptr;
    const auto* disabledValue = disabled ? std::get_if<std::int64_t>(&disabled->value) : nullptr;
    check(enabledValue && *enabledValue == 1 && disabledValue && *disabledValue == 0,
          "boolean constants use native integer values");
    const auto* type = as<ast::StructDef>(doc->definitions[4]);
    check(type && type->xsdAll && type->fields.size() == 4,
          "XSD struct and field count");
    if (type && type->fields.size() == 4) {
      const auto* small = as<ast::BaseType>(type->fields[0]->type);
      const auto* uuid = as<ast::BaseType>(type->fields[1]->type);
      const auto* flag = std::get_if<std::int64_t>(&type->fields[2]->value);
      const auto* name = type->fields[3];
      check(small && small->keyword == "i8" && uuid && uuid->keyword == "uuid",
            "i8 and uuid are builtin types");
      check(flag && *flag == 1, "boolean field default is retained");
      check(name->isReference && name->xsdOptional && name->xsdNillable &&
                name->xsdAttrs.size() == 1 && name->xsdAttrs[0]->name == "code",
            "reference and XSD attributes are retained");
    }
  }
  return ok;
}
} // namespace audit_regressions
