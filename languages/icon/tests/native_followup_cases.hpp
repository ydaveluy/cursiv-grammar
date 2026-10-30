#pragma once
#include <cstdio>
#include <string>
#include "cursiv/test/ValidationHelper.hpp"
#include "icon/core/CoreServices.hpp"
#include "icon/core/ast.hpp"

namespace icon_native_followup {
inline bool run(const icon::IconCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "icon native: %s\n", message); ok = false; }
  };
  const std::string source = R"icon(
# Native 9.5.25a syntax, with explicit semicolons where useful.
invocable _worker, "+":2
record Empty()
procedure blank()
end
procedure _worker(rest[])
  local foo_bar;
  initial foo_bar := 0;
  foo_bar := [];
  foo_bar +:= 2;
  choice := 1 | 2 | 3;
  big := 123456789012345678901234567890;
  write(16rFF, 2e-3, 1., 'abc', "a\"b");
  write(, 1,,);
  values := [1,,3];
  alias := $<1,2$>;
  emptyBlock := $($);
  result := if 1 then 2;
  while 0;
  until 1;
  every 1 to 2;
  suspend;
  return;
end
)icon";
  const auto result = services.parser->parse(source);
  const auto* program = result.value_as<icon::ast::Program>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && program &&
        program->declarations.size() == 4, "native declaration and expression document");
  bool invocable = false, record = false, blank = false, radix = false, large = false;
  bool real = false, cset = false, escaped = false, augmented = false, alternative = false;
  bool nullArguments = false, nullList = false, emptyList = false, emptyBlock = false;
  bool ifNoElse = false, whileNoDo = false, untilNoDo = false, everyNoDo = false;
  bool bareSuspend = false, bareReturn = false;
  for (const auto* node : result.nodes) {
    if (const auto* declaration = cursiv::ast_cast<icon::ast::InvocableDeclaration>(node))
      invocable |= declaration->items.size() == 2 && declaration->items[0]->name == "_worker" &&
                   declaration->items[1]->name == "\"+\"" && declaration->items[1]->arity == "2";
    if (const auto* declaration = cursiv::ast_cast<icon::ast::RecordDeclaration>(node))
      record |= declaration->name == "Empty" && declaration->fields.empty();
    if (const auto* declaration = cursiv::ast_cast<icon::ast::Procedure>(node))
      blank |= declaration->name == "blank" && declaration->body.empty();
    if (const auto* literal = cursiv::ast_cast<icon::ast::IntLit>(node)) {
      radix |= literal->text == "16rFF";
      large |= literal->text == "123456789012345678901234567890";
    }
    if (const auto* literal = cursiv::ast_cast<icon::ast::RealLit>(node)) real |= literal->text == "2e-3";
    if (const auto* literal = cursiv::ast_cast<icon::ast::CsetLit>(node)) cset |= literal->text == "'abc'";
    if (const auto* literal = cursiv::ast_cast<icon::ast::StringLit>(node)) escaped |= literal->text == "\"a\\\"b\"";
    if (const auto* binary = cursiv::ast_cast<icon::ast::BinaryExpr>(node)) {
      augmented |= binary->op == "+:=";
      const auto* right = cursiv::ast_cast<icon::ast::BinaryExpr>(binary->right);
      alternative |= binary->op == "|" && right && right->op == "|";
    }
    if (const auto* call = cursiv::ast_cast<icon::ast::CallExpr>(node))
      nullArguments |= call->args.size() == 4 && cursiv::ast_cast<icon::ast::NullExpr>(call->args[0]) &&
                       cursiv::ast_cast<icon::ast::NullExpr>(call->args[2]) &&
                       cursiv::ast_cast<icon::ast::NullExpr>(call->args[3]);
    if (const auto* list = cursiv::ast_cast<icon::ast::ListLit>(node)) {
      emptyList |= list->items.empty();
      nullList |= list->items.size() == 3 && cursiv::ast_cast<icon::ast::NullExpr>(list->items[1]);
    }
    if (const auto* block = cursiv::ast_cast<icon::ast::Block>(node)) emptyBlock |= block->body.empty();
    if (const auto* control = cursiv::ast_cast<icon::ast::IfExpr>(node)) ifNoElse |= !control->elseValue;
    if (const auto* control = cursiv::ast_cast<icon::ast::WhileExpr>(node)) whileNoDo |= !control->doClause;
    if (const auto* control = cursiv::ast_cast<icon::ast::UntilExpr>(node)) untilNoDo |= !control->doClause;
    if (const auto* control = cursiv::ast_cast<icon::ast::EveryExpr>(node)) everyNoDo |= !control->doClause;
    if (const auto* control = cursiv::ast_cast<icon::ast::SuspendExpr>(node)) bareSuspend |= !control->value;
    if (const auto* control = cursiv::ast_cast<icon::ast::ReturnExpr>(node)) bareReturn |= !control->value;
  }
  check(invocable && record && blank && radix && large && real && cset && escaped && augmented &&
        alternative && nullArguments && nullList && emptyList && emptyBlock && ifNoElse &&
        whileNoDo && untilNoDo && everyNoDo && bareSuspend && bareReturn,
        "native literal/operator/null/control fields survive in AST");
  {
    const auto lines = services.parser->parse(
        "procedure lines()\nwrite(1)\nwrite(2)\n+3\nreturn\nwrite(4)\n(5)\nend\n");
    const auto* root = lines.value_as<icon::ast::Program>();
    const auto* procedure = root && root->declarations.size() == 1
        ? cursiv::ast_cast<icon::ast::Procedure>(root->declarations[0]) : nullptr;
    check(lines.accepted && !lines.cst().recovered() && lines.diagnostics.empty() && procedure &&
          procedure->body.size() == 6, "newline semicolon insertion between Beginner/Ender tokens");
    if (procedure && procedure->body.size() == 6) {
      const auto* returned = cursiv::ast_cast<icon::ast::ReturnExpr>(procedure->body[3]);
      check(cursiv::ast_cast<icon::ast::PrefixExpr>(procedure->body[2]) && returned && !returned->value &&
            cursiv::ast_cast<icon::ast::ExprList>(procedure->body[5]),
            "newline does not fuse prefix, return operand or postfix call");
    }
  }
  for (const char* source : {"procedure f()\nwrite(2r2)\nend\n",
                             "procedure f()\nwrite(37r1)\nend\n",
                             "procedure f()\nwrite(1.0e1e2)\nend\n"}) {
    const auto bad = services.parser->parse(source);
    check(!bad.accepted || bad.cst().recovered() || !bad.diagnostics.empty(), "invalid native numeric literal");
  }
  for (const char* source : {"procedure f(a[],b)\nend\n", "invocable foo:2\n",
                             "procedure f()\nsuspend do write(1)\nend\n"}) {
    const auto validated = cursiv::test::validationHelper(services, std::string(source));
    check(validated.diagnostics.size() == 1, "native vararg/invocable declaration constraint");
  }
  const auto empty = services.parser->parse("# empty native compilation unit\n");
  check(empty.accepted && !empty.cst().recovered() && empty.diagnostics.empty(), "empty native program");
  return ok;
}
} // namespace icon_native_followup
