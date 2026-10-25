#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "z/core/ast.hpp"
#include "z/generated/Z/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = z::generated::Z::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = z::ast;
  auto r = parser->parse("─ function ( _ ⊞ _ ) └\n─ r == a ⊞ b + c └\n");
  check(r.accepted && !r.cst().recovered(), "custom operator template and use");
  auto *d = as<ast::Specification>(r.value);
  auto *op = d && d->items.size() == 2 ? as<ast::OperatorTemplate>(d->items[0])
                                       : nullptr;
  check(op && op->text.find("_ ⊞ _") != std::string_view::npos,
        "operator template body retained");
  auto *def =
      d && d->items.size() == 2 ? as<ast::HorizontalDef>(d->items[1]) : nullptr;
  auto *seq = def ? as<ast::OperatorSequence>(def->value) : nullptr;
  check(seq && seq->steps.size() == 2 && seq->steps[0]->op == "⊞" &&
            seq->steps[1]->op == "+",
        "custom and known operators retained in source order");
  return ok;
}
} // namespace audit_regressions
