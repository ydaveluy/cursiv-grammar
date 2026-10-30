#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "snobol/core/ast.hpp"
#include "snobol/generated/Snobol/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = snobol::generated::Snobol::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = snobol::ast;
  auto r = parser->parse(" X = replace('abc','a','b','extra')\nEND\n");
  check(r.accepted && !r.cst().recovered(),
        "builtin arguments are a runtime concern");
  auto *d = as<ast::Program>(r.value);
  auto *st = d && !d->lines.empty() ? as<ast::Statement>(d->lines[0]) : nullptr;
  auto *c = st && !st->replacement.empty()
                ? as<ast::FunctionCall>(st->replacement[0])
                : nullptr;
  check(c && c->fn == ast::Builtin::replace && c->args.size() == 4,
        "all call arguments retained");
  for (const char* source : {" X = 2^3^4\nEND\n", " X = 2**3**4\nEND\n",
                             " X = 2!3!4\nEND\n", " X = 8/2*2\nEND\n"}) {
    const auto power_result = parser->parse(source);
    const auto* root = power_result.value_as<ast::Program>();
    const auto* statement = root && !root->lines.empty()
        ? cursiv::ast_cast<ast::Statement>(root->lines[0]) : nullptr;
    const auto* outer = statement && statement->replacement.size() == 1
        ? cursiv::ast_cast<ast::BinaryExpr>(statement->replacement[0]) : nullptr;
    const auto* right = outer ? cursiv::ast_cast<ast::BinaryExpr>(outer->right) : nullptr;
    check(power_result.accepted && !power_result.cst().recovered() &&
              power_result.diagnostics.empty() && outer && right,
          "native exponent aliases associate right; multiplication binds above division");
    if (outer && right)
      check((outer->op == ast::Operator::pow && right->op == ast::Operator::pow) ||
                (outer->op == ast::Operator::divide && right->op == ast::Operator::multiply),
            "native SNOBOL arithmetic tree");
  }
  const auto branches = parser->parse(
      " X_1.A = REPLACE('abc','a','b') :S(YES)F(NO)\n"
      " &TRIM =\n :f(NO)s(YES)\nend\n");
  check(branches.accepted && !branches.cst().recovered() && branches.diagnostics.empty(),
        "native names, builtin case folding, null replacement and both conditional gotos");
  const auto* root = branches.value_as<ast::Program>();
  check(root && root->lines.size() == 4, "four native statement lines");
  if (root && root->lines.size() == 4) {
    const auto* first = cursiv::ast_cast<ast::Statement>(root->lines[0]);
    const auto* second = cursiv::ast_cast<ast::Statement>(root->lines[1]);
    const auto* jump = cursiv::ast_cast<ast::GotoLine>(root->lines[2]);
    const auto* call = first && first->replacement.size() == 1
        ? cursiv::ast_cast<ast::FunctionCall>(first->replacement[0]) : nullptr;
    check(first && first->subject && first->subject->name == "X_1.A" &&
              first->hasReplacement && call && call->fn == ast::Builtin::replace,
          "names and case-insensitive builtin map to the existing enum");
    const auto* transfer = first ? first->transfer : nullptr;
    check(transfer && transfer->mode == "S" && transfer->label == "YES" &&
              transfer->next && transfer->next->mode == "F" && transfer->next->label == "NO",
          "both branch targets retained in source order");
    check(second && second->subject && second->subject->keyword && second->hasReplacement &&
              second->replacement.empty(), "keyword access and explicit null replacement stay distinct");
    check(jump && jump->transfer && jump->transfer->mode == "f" && jump->transfer->next &&
              jump->transfer->next->mode == "s", "reverse conditional branch order and spelling");
  }
  return ok;
}
} // namespace audit_regressions
