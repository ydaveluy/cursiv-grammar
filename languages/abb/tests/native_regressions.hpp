#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <unordered_set>

namespace abb_tests {
template <class Language>
bool parents(const Language &language, const cursiv::ParseResult &result) {
  if (!result.value || result.value->parent)
    return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode *> seen;
  const auto visit = [&](auto &&self, const cursiv::AstNode &node) -> void {
    if (!seen.insert(&node).second) {
      valid = false;
      return;
    }
    language.astReflection->visitMembers(
        node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
          valid &= child.parent == &node &&
                   result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *result.value);
  return valid && seen.size() == result.nodes.size();
}

template <class Language> bool nativeRegressions(const Language &language) {
  namespace ast = abb::ast;
  const std::string source = R"RAPID(MODULE Native(SYSMODULE,NOSTEPIN)
RECORD Pair
  num a;
  string b;
ENDRECORD
VAR string msg:="hello world";
VAR num raw:=1e400;
FUNC num f(VAR num x,\bool y)
  RETURN x;
ENDFUNC
PROC p()
  VAR num temp:=1;
  IF temp THEN
    temp:=2;
  ELSEIF temp THEN
    WHILE temp DO Stop; ENDWHILE
  ELSE
    RETURN;
  ENDIF
  MoveL pose,v100,z10,tool0\WObj:=wobj0;
  TPWrite msg;
ENDPROC
ENDMODULE)RAPID";
  const auto result = language.parser->parse(source);
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !parents(language, result))
    return false;
  const auto *module = cursiv::ast_cast<ast::Module>(result.value);
  if (!module || module->options.size() != 2 ||
      module->options[0] != "SYSMODULE" || module->options[1] != "NOSTEPIN" ||
      module->records.size() != 1 || module->declarations.size() != 2 ||
      module->functions.size() != 1 || module->procedures.size() != 1)
    return false;
  const auto *record = module->records[0];
  if (record->name != "Pair" || record->fields.size() != 2 ||
      record->fields[0]->type != "num" || record->fields[0]->name != "a" ||
      record->fields[1]->type != "string" || record->fields[1]->name != "b")
    return false;
  const auto &message = module->declarations[0]->value;
  const auto *text = cursiv::ast_cast<ast::StringLiteral>(message);
  const auto &raw = module->declarations[1]->value;
  const auto *number = cursiv::ast_cast<ast::FloatLiteral>(raw);
  if (!text || text->value != "hello world" || !number || number->text != "1e400")
    return false;
  const auto *function = module->functions[0];
  if (function->name != "f" || function->returnType != "num" ||
      function->parameters.size() != 2 || function->statements.size() != 1 ||
      function->parameters[0]->mode != "VAR" ||
      function->parameters[0]->type != "num" ||
      function->parameters[0]->name != "x" ||
      function->parameters[0]->isOptional ||
      function->parameters[1]->type != "bool" ||
      function->parameters[1]->name != "y" ||
      !function->parameters[1]->isOptional)
    return false;
  const auto *ret = cursiv::ast_cast<ast::Return>(function->statements[0]);
  const auto *ref = ret ? cursiv::ast_cast<ast::Reference>(ret->value) : nullptr;
  if (!ref || ref->name != "x")
    return false;
  const auto *procedure = module->procedures[0];
  if (procedure->locals.size() != 1 || procedure->locals[0]->name != "temp" ||
      procedure->statements.size() != 3)
    return false;
  const auto *ifStmt = cursiv::ast_cast<ast::If>(procedure->statements[0]);
  if (!ifStmt || ifStmt->branches.size() != 2 || !ifStmt->hasElse ||
      ifStmt->elseStatements.size() != 1 ||
      ifStmt->branches[0]->statements.size() != 1 ||
      ifStmt->branches[1]->statements.size() != 1)
    return false;
  const auto *assignment =
      cursiv::ast_cast<ast::Assignment>(ifStmt->branches[0]->statements[0]);
  const auto *assigned = assignment ? cursiv::ast_cast<ast::Integer>(assignment->value)
                                     : nullptr;
  const auto *target = assignment ? cursiv::ast_cast<ast::Reference>(assignment->target)
                                   : nullptr;
  if (!target || target->name != "temp" || !assigned || assigned->text != "2")
    return false;
  const auto *loop =
      cursiv::ast_cast<ast::While>(ifStmt->branches[1]->statements[0]);
  const auto *stop = loop && loop->statements.size() == 1
                         ? cursiv::ast_cast<ast::Call>(loop->statements[0])
                         : nullptr;
  const auto *emptyReturn =
      cursiv::ast_cast<ast::Return>(ifStmt->elseStatements[0]);
  if (!stop || stop->name != "Stop" || !stop->arguments.empty() ||
      !emptyReturn || emptyReturn->value)
    return false;
  const auto *move = cursiv::ast_cast<ast::Call>(procedure->statements[1]);
  if (!move || move->name != "MoveL" || move->arguments.size() != 5)
    return false;
  const auto *binding = cursiv::ast_cast<ast::BindingArgument>(move->arguments[4]);
  const auto *workObject =
      binding ? cursiv::ast_cast<ast::Reference>(binding->value) : nullptr;
  if (!binding || binding->name != "WObj" || !binding->isOptional ||
      binding->isConditional || !workObject || workObject->name != "wobj0")
    return false;
  const auto *write = cursiv::ast_cast<ast::Call>(procedure->statements[2]);
  const auto *messageRef =
      write && write->arguments.size() == 1
          ? cursiv::ast_cast<ast::Reference>(write->arguments[0])
          : nullptr;
  if (!messageRef || messageRef->name != "msg")
    return false;
  if (!cursiv::test::validationHelper(language, source).diagnostics.empty())
    return false;
  const auto hasCode = [&](std::string_view input, std::string_view code) {
    const std::string owned(input);
    const auto parsed = language.parser->parse(owned);
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty())
      return false;
    for (const auto &diagnostic :
         cursiv::test::validationHelper(language, owned).diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == code)
        return true;
    return false;
  };
  return hasCode("MODULE M(UNKNOWN) ENDMODULE", "abb.module-options") &&
         hasCode("MODULE M() ENDMODULE", "abb.module-options") &&
         hasCode("MODULE M PROC p ENDPROC ENDMODULE",
                 "abb.procedure-parameters") &&
         hasCode("MODULE M PROC p(x) ENDPROC ENDMODULE", "abb.parameter-type");
}
} // namespace abb_tests
