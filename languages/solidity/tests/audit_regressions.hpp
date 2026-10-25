#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "solidity/core/ast.hpp"
#include "solidity/generated/Solidity/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = solidity::generated::Solidity::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = solidity::ast;
  auto r = parser->parse(
      "contract C { function f() public { (uint a,,uint b) = g(); } }");
  check(r.accepted && !r.cst().recovered(), "typed destructuring with elision");
  auto *u = as<ast::SourceUnit>(r.value);
  auto *c = u && !u->items.empty() ? as<ast::ContractDefinition>(u->items[0])
                                   : nullptr;
  auto *f =
      c && !c->body.empty() ? as<ast::FunctionDefinition>(c->body[0]) : nullptr;
  auto *d = f && f->body && !as<ast::Block>(f->body)->statements.empty()
                ? as<ast::VariableDeclarationStatement>(
                      as<ast::Block>(f->body)->statements[0])
                : nullptr;
  check(d && d->tuple.size() == 3 && d->tuple[0]->name == "a" &&
            d->tuple[1]->type == nullptr && d->tuple[2]->name == "b",
        "tuple positions remain stable");
  return ok;
}
} // namespace audit_regressions
