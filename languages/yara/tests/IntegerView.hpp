#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "yara/core/Integer.hpp"
#include "yara/core/ast.hpp"
using J = cursiv::JsonValue;
void append_numbers(const cursiv::AstNode &n, J::Array &nums) {
  auto add = [&](const char *role, std::string_view raw) {
    auto v = yara::integerValue(raw);
    nums.push_back(J::Object{{"role", role},
                             {"raw", raw},
                             {"value", v ? std::to_string(*v) : "invalid"}});
  };
  if (auto *p = cursiv::ast_cast<yara::ast::IntegerLiteral>(&n))
    add("meta", p->text);
  if (auto *p = cursiv::ast_cast<yara::ast::StringModifier>(&n)) {
    if (p->lo)
      add("lo", *p->lo);
    if (p->hi)
      add("hi", *p->hi);
  }
  if (auto *p = cursiv::ast_cast<yara::ast::Jump>(&n)) {
    if (!p->lo.empty())
      add("jump-lo", p->lo);
    if (!p->hi.empty())
      add("jump-hi", p->hi);
  }
  if (auto *p = cursiv::ast_cast<yara::ast::Literal>(&n);
      p && !p->quoted && !p->text.empty() && p->text[0] >= '0' &&
      p->text[0] <= '9')
    add("expr", p->text);
}
