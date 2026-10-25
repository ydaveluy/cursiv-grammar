#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "protobuf/core/Numbers.hpp"
#include <bit>
#include <cmath>
#include <cstdio>
using J = cursiv::JsonValue;
void append_numbers(const cursiv::AstNode &n, J::Array &nums) {
  auto add = [&](const char *role, std::string_view raw, bool real = false) {
    auto v = protobuf::number_value(raw, real);
    J::Object x{{"role", role},
                {"raw", raw},
                {"kind", "invalid"},
                {"value", "invalid"}};
    if (v)
      std::visit(
          [&](auto value) {
            using T = decltype(value);
            if constexpr (std::is_same_v<T, double>) {
              char b[17];
              std::snprintf(b, sizeof b, "%016llx",
                            static_cast<unsigned long long>(
                                std::bit_cast<std::uint64_t>(value)));
              x["kind"] = "float";
              x["value"] = std::isnan(value) ? "nan" : std::string(b);
            } else {
              x["kind"] = std::is_signed_v<T> ? "signed" : "unsigned";
              x["value"] = std::to_string(value);
            }
          },
          *v);
    if (std::string_view(role) != "option" &&
        std::string_view(role) != "default") {
      x["kind"] = "signed";
      auto value = protobuf::integer_value(raw);
      x["value"] = value ? std::to_string(*value) : "invalid";
    }
    nums.push_back(x);
  };
  if (auto *p = cursiv::ast_cast<protobuf::ast::Field>(&n))
    add("field", p->number);
  if (auto *p = cursiv::ast_cast<protobuf::ast::OneofField>(&n))
    add("field", p->number);
  if (auto *p = cursiv::ast_cast<protobuf::ast::MapField>(&n))
    add("field", p->number);
  if (auto *p = cursiv::ast_cast<protobuf::ast::EnumField>(&n))
    add("enum", p->value);
  if (auto *p = cursiv::ast_cast<protobuf::ast::ReservedRange>(&n)) {
    add("range-from", p->from);
    if (p->to)
      add("range-to", *p->to);
    else if (p->toMax)
      add("range-to", p->parent && cursiv::ast_cast<protobuf::ast::EnumDef>(
                                       p->parent->parent)
                          ? "2147483647"
                          : "536870911");
  }
  if (auto *p = cursiv::ast_cast<protobuf::ast::NumberLiteral>(&n)) {
    auto *o = cursiv::ast_cast<protobuf::ast::FieldOption>(p->parent);
    auto *f = o ? cursiv::ast_cast<protobuf::ast::Field>(o->parent) : nullptr;
    const bool def = o && o->name && o->name->name == "default" && f;
    add(def ? "default" : "option", p->text, p->floating);
    if (def) {
      auto v = protobuf::number_value(p->text, p->floating);
      auto &x = nums.back().object();
      if (f->type == "float" || f->type == "double") {
        if (v) {
          double d = std::visit([](auto n) { return double(n); }, *v);
          char b[17];
          std::snprintf(
              b, sizeof b, "%016llx",
              static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(d)));
          x["kind"] = "float";
          x["value"] = std::string(b);
        }
      } else if (f->type == "int32" || f->type == "int64") {
        x["kind"] = "signed";
        auto i = protobuf::integer_value(p->text);
        x["value"] = i ? std::to_string(*i) : "invalid";
      }
    }
  }
}
