#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "unrealangelscript/core/ast.hpp"
#include "unrealangelscript/generated/UnrealAngelscript/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser =
      unrealangelscript::generated::UnrealAngelscript::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = unrealangelscript::ast;
  auto r =
      parser->parse("void Test() { TArray<int32> Values = TArray<int32>(); }");
  check(r.accepted && !r.cst().recovered(), "generic constructor syntax");
  auto *script = as<ast::Script>(r.value);
  auto *f = script && !script->items.empty()
                ? as<ast::FunctionDef>(script->items[0])
                : nullptr;
  auto *d = f && f->body && !f->body->items.empty()
                ? as<ast::VarDecl>(f->body->items[0])
                : nullptr;
  auto *init = d && !d->declarators.empty()
                   ? as<ast::ExprInit>(d->declarators[0]->init)
                   : nullptr;
  auto *c = init ? as<ast::ConstructExpr>(init->value) : nullptr;
  auto *type = c ? as<ast::GenericTypeSpec>(c->type) : nullptr;
  check(type && type->name == "TArray" && type->args.size() == 1,
        "generic constructor type and argument retained");
  return ok;
}
} // namespace audit_regressions
