#pragma once
#include <cstdio>
#include <string>
#include <variant>
#include "cursiv/core/parser/Parser.hpp"
#include "moo/core/ast.hpp"

namespace moo_native {
inline bool run(const cursiv::Parser& parser) {
  using namespace moo::ast;
  const std::string source = R"MOO(@program #123:run
_var = #-1;
x = a*b;
y = a!=b;
z = "a" in {"A"};
n = #123.foo;
notify(this, "hello");
IF (0)
ELSEIF (1)
ELSEIF (2)
ELSE
ENDIF
for i in [1..3]
  continue;
  break;
endfor
while again (1)
  break again;
endwhile
try
  raise(E_TYPE);
except err (E_TYPE,E_ARGS)
  return err;
except (ANY)
  return;
endtry
try
  return;
finally
  notify(this, "cleanup");
endtry
#123:bar();
do = 1;
.)MOO";
  const auto result = parser.parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
    std::fputs("moo: native syntax and command wrapper regression rejected\n", stderr);
    return false;
  }
  bool object = false, multiply = false, unequal = false, membership = false;
  bool literalBase = false, functionStatement = false, branches = false;
  bool range = false, named = false, handlers = false, cleanup = false, softDo = false;
  for (const auto* node : result.nodes) {
    if (const auto* n = cursiv::ast_cast<ObjRef>(node)) object = object || n->text == "#-1";
    if (const auto* n = cursiv::ast_cast<BinaryExpr>(node)) {
      multiply = multiply || n->op == Operator::multiply;
      unequal = unequal || n->op == Operator::not_equal;
      membership = membership || n->op == Operator::in_;
    }
    if (const auto* n = cursiv::ast_cast<Property>(node)) {
      literalBase = literalBase || (n->base == "#123" && n->objectBase && n->path.size() == 1);
      softDo = softDo || n->base == "do";
    }
    if (const auto* n = cursiv::ast_cast<CommandStmt>(node))
      functionStatement = functionStatement || std::holds_alternative<FuncCall*>(n->command);
    if (const auto* n = cursiv::ast_cast<IfStmt>(node))
      branches = branches || (n->elseIfs.size() == 2 && n->hasElse && n->elseBody.empty() &&
                               n->elseIfs[0]->parent == n && n->elseIfs[1]->parent == n);
    if (const auto* n = cursiv::ast_cast<ForStmt>(node))
      range = range || (n->var == "i" && !n->iterable && n->from && n->to);
    if (const auto* n = cursiv::ast_cast<WhileStmt>(node)) named = named || n->name == "again";
    if (const auto* n = cursiv::ast_cast<TryStmt>(node)) {
      handlers = handlers || (n->clauses.size() == 2 && n->clauses[0]->variable == "err" &&
                               n->clauses[0]->codes.size() == 2 && n->clauses[1]->any &&
                               cursiv::ast_cast<ErrorLit>(n->clauses[0]->codes[0]));
      cleanup = cleanup || (n->hasFinally && n->cleanup.size() == 1);
    }
  }
  if (!object || !multiply || !unequal || !membership || !literalBase || !functionStatement ||
      !branches || !range || !named || !handlers || !cleanup || !softDo) {
    std::fputs("moo: native AST distinctions lost\n", stderr);
    return false;
  }
  const auto client = parser.parse("@verb foo:wild* any any any\n@display foo.prop*\n");
  if (!client.accepted || client.cst().recovered() || !client.diagnostics.empty()) {
    std::fputs("moo: imported client wildcard spellings were narrowed\n", stderr);
    return false;
  }
  for (const auto* text : {"@program foo:run\nif = 1;\n.",
                           "@program foo:run\nE_TYPE = 1;\n."}) {
    const auto parsed = parser.parse(text);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) return false;
  }
  return true;
}
}  // namespace moo_native
