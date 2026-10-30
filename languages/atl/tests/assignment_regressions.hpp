#pragma once

#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>

#include "atl/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace atl_assignment_regressions {

inline bool run(const cursiv::Parser& parser) {
  namespace ast = atl::ast;
  // Classic ATL.tcs, Binding and BindingStat: isAssignment selects <:=;
  // <- is the ordinary binding. This is source syntax, not execution.
  const std::string source = R"ATL(module M;
create OUT:M from IN:M;
rule R {
  from s:IN!A
  to t:OUT!B (a <- s.a, b <:= s.b)
  do { t.a <- s.a; t.b <:= s.b; }
})ATL";
  const auto result = parser.parse(source);
  const auto fail = [](const char* reason) {
    std::fprintf(stderr, "atl assignment: %s\n", reason);
    return false;
  };
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
    return fail("mixed native binding operators did not parse cleanly");
  const auto* module = cursiv::ast_cast<ast::Module>(result.value);
  const auto* rule = module && module->elements.size() == 1
      ? cursiv::ast_cast<ast::MatchedRule>(module->elements[0]) : nullptr;
  const auto* output = rule && rule->outPattern && rule->outPattern->elements.size() == 1
      ? rule->outPattern->elements[0] : nullptr;
  if (!output || output->bindings.size() != 2 || !rule->action ||
      rule->action->statements.size() != 2)
    return fail("bindings or action statements were lost");
  const auto member = [](const ast::Expr* expression, std::string_view object,
                         std::string_view name, const cursiv::AstNode* parent) {
    const auto* access = cursiv::ast_cast<ast::MemberAccess>(expression);
    const auto* reference = access ? cursiv::ast_cast<ast::NameRef>(access->object) : nullptr;
    return access && reference && access->name == name && reference->name == object &&
           access->parent == parent && reference->parent == access;
  };
  for (unsigned i = 0; i < 2; ++i) {
    const auto* binding = output->bindings[i];
    const auto* statement = cursiv::ast_cast<ast::BindStmt>(rule->action->statements[i]);
    const std::string_view name = i == 0 ? "a" : "b";
    const bool assignment = i == 1;
    if (!binding || binding->name != name || binding->isAssignment != assignment ||
        binding->parent != output || !member(binding->value, "s", name, binding))
      return fail("out-pattern operator, value or ownership changed");
    if (!statement || statement->isAssignment != assignment ||
        statement->parent != rule->action ||
        !member(statement->target, "t", name, statement) ||
        !member(statement->value, "s", name, statement))
      return fail("action operator, operands or ownership changed");
  }

  const std::string prefix =
      "module M; create OUT:M from IN:M; rule R { from s:IN!A to t:OUT!B ";
  for (const char* op : {"<:", "<:==", "<=:"}) {
    const std::string binding = prefix + "(a " + op + " s.a) }";
    const std::string statement = prefix + "do { t.a " + op + " s.a; } }";
    for (const std::string* text : {&binding, &statement}) {
      const auto invalid = parser.parse(*text);
      if (invalid.accepted && !invalid.cst().recovered() && invalid.diagnostics.empty()) {
        std::fprintf(stderr, "atl malformed binding accepted: %s\n", text->c_str());
        return false;
      }
    }
  }
  return true;
}

}  // namespace atl_assignment_regressions
