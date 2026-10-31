#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "focal/core/ast.hpp"

namespace focal_commands {
namespace ast = focal::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "focal command families: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& language) {
  const std::string source =
      "ERASE\nE ALL\nERASE 2.0\nE 2\nERASE 2.01\nMODIFY 5.41\nM 7.01\n"
      "01.01 SET X=2\nTYPE \"direct\"\nTYPE \"open\nTYPE \"next\"\n"
      "01.02 ASK \"PROMPT\",X\nWRITE ALL\n";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) return fail("direct/stored/native commands or ownership", source);
  const auto* program = cursiv::ast_cast<ast::Program>(parsed.value);
  if (!program || program->statements.size() != 13) return fail("thirteen physical lines", source);
  for (std::size_t i = 0; i < program->statements.size(); ++i)
    if ((program->statements[i]->number != nullptr) != (i == 7 || i == 11))
      return fail("direct vs stored-line number presence", source);
  const auto erase = [&](std::size_t i) {
    return cursiv::ast_cast<ast::EraseCommand>(program->statements[i]->command);
  };
  const auto* bare = erase(0);
  const auto* all = erase(1);
  const auto* groupLine = erase(2);
  const auto* group = erase(3);
  const auto* line = erase(4);
  const auto* modify = cursiv::ast_cast<ast::ModifyCommand>(program->statements[5]->command);
  const auto* shortModify = cursiv::ast_cast<ast::ModifyCommand>(program->statements[6]->command);
  if (!bare || bare->all || bare->group || bare->target || !all || !all->all ||
      !groupLine || !groupLine->target || groupLine->target->group != 2 ||
      groupLine->target->line != 0 || !group || group->group != 2 || group->target ||
      !line || !line->target || line->target->line != 1 ||
      !modify || !modify->target || modify->target->group != 5 || modify->target->line != 41 ||
      !shortModify || !shortModify->target ||
      shortModify->target->group != 7 || shortModify->target->line != 1)
    return fail("ERASE selection and MODIFY line values", source);
  for (std::size_t i = 8; i <= 10; ++i) {
    const auto* type = cursiv::ast_cast<ast::TypeCommand>(program->statements[i]->command);
    const auto* text = type && type->items.size() == 1
        ? std::get_if<std::string_view>(&type->items[0]) : nullptr;
    if (!text || *text != (i == 8 ? "direct" : i == 9 ? "open" : "next"))
      return fail("quote or physical-line text termination", source);
  }
  for (const std::string text : {std::string("TYPE \"open\rTYPE \"next\""),
                                std::string("TYPE \"open\r\nTYPE \"next\""),
                                std::string("TYPE \"open\nTYPE \"next\""),
                                std::string("TYPE \"open\nTYPE \"next\"\n")}) {
    const auto result = language.parser->parse(text);
    const auto* root = cursiv::ast_cast<ast::Program>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->statements.size() != 2 || !parents(language, result))
      return fail("CR/LF/CRLF and EOF quote boundaries", text);
  }
  for (const char* text : {"MODIFY", "M", "MODIFY ALL", "MODIFY 2",
                           "ERASE ALL 1.01", "ERASE 1.01,2.01", "HELP"}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-disallowed command shape accepted clean", text);
  }

  for (const char* arithmetic : {"TYPE 1+2\n", "TYPE 1 + 2\n", "TYPE 1-2\n"}) {
    const auto result = language.parser->parse(arithmetic);
    const auto* root = cursiv::ast_cast<ast::Program>(result.value);
    const auto* type = root && root->statements.size() == 1
        ? cursiv::ast_cast<ast::TypeCommand>(root->statements[0]->command) : nullptr;
    const auto* item = type && type->items.size() == 1
        ? std::get_if<ast::Expression*>(&type->items[0]) : nullptr;
    const auto* sum = item ? cursiv::ast_cast<ast::BinaryExpression>(*item) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !sum || sum->op != (std::string_view(arithmetic).find('-') != std::string_view::npos
                               ? ast::Operator::subtract : ast::Operator::add)) {
      std::fprintf(stderr, "TYPE arithmetic: accepted=%d recovered=%d diagnostics=%zu items=%zu sum=%d op=%d\n",
                   result.accepted, result.cst().recovered(), result.diagnostics.size(),
                   type ? type->items.size() : 0, sum != nullptr, sum ? int(sum->op) : -1);
      return fail("TYPE1+2 must retain one binary arithmetic expression", arithmetic);
    }
  }
  for (const char* text : {"TYPE 2++4", "SET A=2++4"}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("documented illegal double operator accepted clean", text);
  }
  const std::string signedValues = "TYPE -1,+2\n";
  const auto signedResult = language.parser->parse(signedValues);
  const auto* signedRoot = cursiv::ast_cast<ast::Program>(signedResult.value);
  const auto* signedType = signedRoot && signedRoot->statements.size() == 1
      ? cursiv::ast_cast<ast::TypeCommand>(signedRoot->statements[0]->command) : nullptr;
  if (!signedResult.accepted || signedResult.cst().recovered() ||
      !signedResult.diagnostics.empty() || !signedType || signedType->items.size() != 2 ||
      !parents(language, signedResult)) return fail("signed values separated by comma", signedValues);
  for (std::size_t i = 0; i < 2; ++i) {
    const auto* item = std::get_if<ast::Expression*>(&signedType->items[i]);
    const auto* number = item ? cursiv::ast_cast<ast::NumberLiteral>(*item) : nullptr;
    if (!number || number->value != (i == 0 ? -1 : 2))
      return fail("signed NumberLiteral values preserved", signedValues);
  }
  return true;
}
}  // namespace focal_commands
