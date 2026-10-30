#pragma once
#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "modelica/core/ast.hpp"

namespace modelica_native {
inline bool run(const cursiv::Parser& parser) {
  using namespace modelica::ast;
  const std::string source = R"MO(model M
import A . {B,C};
import D . *;
Real 'a b"c' = .5;
Real '';
Real 'if';
equation
x = a[1].f();
a[1].f();
y = pure(f());
algorithm
x := -a*b + c;
end M;)MO";
  const auto result = parser.parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
    std::fputs("modelica: native lexical/indexed-call regression rejected\n", stderr);
    return false;
  }
  bool quoted = false, empty = false, keyword = false, decimal = false;
  bool indexedExpr = false, indexedEq = false, pure = false, sign = false;
  bool selective = false, wildcard = false;
  for (const auto* node : result.nodes) {
    if (const auto* n = cursiv::ast_cast<ComponentDecl>(node)) {
      quoted = quoted || n->name == "'a b\"c'";
      empty = empty || n->name == "''";
      keyword = keyword || n->name == "'if'";
    }
    if (const auto* n = cursiv::ast_cast<NumberLit>(node)) decimal = decimal || n->value == .5;
    const auto indexed = [](const ComponentRef* ref) {
      return ref && ref->parts.size() == 2 && ref->parts[0]->name == "a" &&
             ref->parts[0]->subscripts.size() == 1 && ref->parts[1]->name == "f";
    };
    if (const auto* n = cursiv::ast_cast<CallExpr>(node)) {
      indexedExpr = indexedExpr || indexed(n->callee);
      pure = pure || (n->callee && n->callee->parts.size() == 1 &&
                       n->callee->parts[0]->name == "pure");
    }
    if (const auto* n = cursiv::ast_cast<CallEquation>(node)) indexedEq = indexedEq || indexed(n->callee);
    if (const auto* n = cursiv::ast_cast<AssignStmt>(node)) {
      const auto* sum = cursiv::ast_cast<BinaryExpr>(n->value);
      const auto* unary = sum ? cursiv::ast_cast<UnaryExpr>(sum->left) : nullptr;
      sign = sign || (sum && sum->op == "+" && unary && unary->op == "-" &&
                       cursiv::ast_cast<BinaryExpr>(unary->operand));
    }
    if (const auto* n = cursiv::ast_cast<ImportClause>(node)) {
      selective = selective || n->selective.size() == 2;
      wildcard = wildcard || n->wildcard;
    }
  }
  if (!quoted || !empty || !keyword || !decimal || !indexedExpr || !indexedEq ||
      !pure || !sign || !selective || !wildcard) {
    std::fputs("modelica: native AST fields lost\n", stderr);
    return false;
  }
  for (const auto* source : {
         "model if end if;",
         "model M algorithm x:=a+-b; end M;",
         "model M Real x[1]={a=1}; end M;",
         "model M equation x=f(1,2 for i in 1:3); end M;",
         "model M equation x=f(function A() for i in 1:3); end M;",
         "within A; within B; model M end M;"}) {
    const auto parsed = parser.parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) {
      std::fprintf(stderr, "modelica: native-invalid syntax accepted: %s\n", source);
      return false;
    }
  }
  return true;
}
}  // namespace modelica_native
