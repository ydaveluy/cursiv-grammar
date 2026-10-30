#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "squirrel/core/ast.hpp"
#include "squirrel/generated/Squirrel/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = squirrel::generated::Squirrel::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = squirrel::ast;
  auto r = parser->parse("local xs = [\n1,\n2\n]\nlocal y = "
                         "foo(\n1,\n2\n)\nlocal z = (\n1 +\n2\n)\n");
  check(r.accepted && !r.cst().recovered(),
        "newlines inside delimited expressions and after operators");
  auto rhs = [&](const cursiv::ParseResult& result) -> ast::Expr* {
    const auto* root = result.value_as<ast::Chunk>();
    const auto* decl = root && root->body.size() == 1
        ? cursiv::ast_cast<ast::LocalVarDecl>(root->body[0]) : nullptr;
    return decl && decl->vars.size() == 1 ? decl->vars[0]->init : nullptr;
  };
  for (const char* source : {"local x = a && b in c;", "local x = a == b in c;"}) {
    const auto result = parser->parse(source);
    const auto* outer = cursiv::ast_cast<ast::BinaryExpr>(rhs(result));
    const auto* inner = outer ? cursiv::ast_cast<ast::BinaryExpr>(outer->right) : nullptr;
    check(result.accepted && !result.cst().recovered() && outer && inner && inner->op == "in",
          "native in comparison precedence");
  }
  const auto logic = parser->parse("local x = a && b && c;");
  const auto* logical = cursiv::ast_cast<ast::BinaryExpr>(rhs(logic));
  check(logical && cursiv::ast_cast<ast::BinaryExpr>(logical->right),
        "native logical compiler recursion associates right");
  const auto ternary = parser->parse("local x = a ? b : c = d;");
  const auto* conditional = cursiv::ast_cast<ast::CondExpr>(rhs(ternary));
  check(conditional && cursiv::ast_cast<ast::AssignExpr>(conditional->elseValue),
        "assignment in conditional false arm stays inside that arm");
  const auto json = parser->parse("local x = {\"a\": 1, @\"b\": 2};");
  const auto* table = cursiv::ast_cast<ast::TableExpr>(rhs(json));
  check(json.accepted && !json.cst().recovered() && table && table->items.size() == 2,
        "native JSON table keys");
  if (table && table->items.size() == 2) {
    const auto* item = std::get<ast::KeyValue*>(table->items[1]);
    const auto* key = std::holds_alternative<ast::Expr*>(item->key)
        ? cursiv::ast_cast<ast::StringLit>(std::get<ast::Expr*>(item->key)) : nullptr;
    check(key && key->value == "b" && key->verbatim, "quoted key preserves verbatim mode");
  }
  for (const char* value : {"000", "0X2A", "'0'", "'\\n'", "'\\x41'"}) {
    const std::string source = std::string("local x = ") + value + ";";
    const auto result = parser->parse(source);
    const auto* integer = cursiv::ast_cast<ast::IntLit>(rhs(result));
    check(result.accepted && !result.cst().recovered() && integer && integer->text == value,
          "octal, uppercase hex and character spelling retained");
  }
  const auto array = parser->parse("local x = [1 2,3];");
  const auto* items = cursiv::ast_cast<ast::ArrayExpr>(rhs(array));
  check(array.accepted && !array.cst().recovered() && items && items->items.size() == 3,
        "native array comma is optional");
  const auto function = parser->parse("local f = function[env](x,) { return\n42; };\n");
  const auto* fn = cursiv::ast_cast<ast::FunctionExpr>(rhs(function));
  const auto* body = fn ? cursiv::ast_cast<ast::BlockStmt>(fn->body) : nullptr;
  const auto* ret = body && body->body.size() == 2
      ? cursiv::ast_cast<ast::ReturnStmt>(body->body[0]) : nullptr;
  check(function.accepted && !function.cst().recovered() && fn && fn->environment &&
            fn->params.size() == 1 && body && ret && !ret->value,
        "bound environment, trailing parameter comma and newline-terminated return");
  for (const char* source : {"local f = @[env](x,) x;", "function f[env](x,) { yield; }",
                             "class C { constructor[env](x,) {} }",
                             "try\n{ throw 1; }\ncatch (\ne\n)\n{}",
                             "function defaults(a,b=1) {}"}) {
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "native bound functions, empty yield, try line breaks or default parameters");
  }
  for (const char* source : {"local x = .5;", "local x = \"\\q\";", "local x = [,];"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "native leading-dot floats, unknown escapes and leading array commas rejected");
  }
  return ok;
}
} // namespace audit_regressions
