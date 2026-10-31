#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "rexx/core/ast.hpp"

namespace rexx_parse_options {
namespace ast = rexx::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "rexx PARSE options: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool okay = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { okay = false; return; }
    language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          okay &= child.parent == &node && parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return okay && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& language) {
  struct Positive { const char* source; bool upper; bool lower; bool caseless; ast::ParseSource kind; };
  const Positive tests[] = {
      {"PARSE LOWER VAR Text a 'X' b\n", false, true, false, ast::ParseSource::Var},
      {"PARSE CASELESS VALUE 'AbC' WITH a 'B' b\n", false, false, true, ast::ParseSource::Value},
      {"PARSE UPPER CASELESS ARG a,b\n", true, false, true, ast::ParseSource::Arg},
      {"PARSE CASELESS UPPER PULL a\n", true, false, true, ast::ParseSource::Pull},
      {"PARSE LOWER CASELESS SOURCE os .\n", false, true, true, ast::ParseSource::Source},
      {"PARSE CASELESS LOWER VERSION v\n", false, true, true, ast::ParseSource::Version},
      {"PARSE LINEIN line\n", false, false, false, ast::ParseSource::Linein},
      {"PARSE EXTERNAL line\n", false, false, false, ast::ParseSource::External},
      {"parse lower caseless var lower caseless linein\n", false, true, true, ast::ParseSource::Var}};
  for (const auto& test : tests) {
    const auto parsed = language.parser->parse(test.source);
    const auto* root = cursiv::ast_cast<ast::Block>(parsed.value);
    const auto* parse = root && root->body.size() == 1
        ? cursiv::ast_cast<ast::ParseStmt>(root->body[0]) : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !parse || parse->upper != test.upper || parse->lower != test.lower ||
        parse->caseless != test.caseless || parse->source != test.kind ||
        parse->templates.empty() || !parents(language, parsed))
      return fail("native flag/source mode, typed PARSE or ownership", test.source);
    if (test.kind == ast::ParseSource::Var && parse->varName != "Text" && parse->varName != "lower")
      return fail("VAR source is contextual, not a flag", test.source);
    if (test.kind == ast::ParseSource::Value) {
      const auto* string = cursiv::ast_cast<ast::StringLit>(parse->valueExpr);
      if (!string || string->value != "AbC" || parse->templates.size() != 1 ||
          parse->templates[0]->items.size() != 3)
        return fail("VALUE expression and full template retained", test.source);
    }
    if (test.kind == ast::ParseSource::Arg && parse->templates.size() != 2)
      return fail("comma-separated ARG templates retained", test.source);
  }
  const std::string names = "lower=1;caseless=2;linein=3;lower:;say lower caseless linein\n";
  const auto named = language.parser->parse(names);
  const auto* namesRoot = cursiv::ast_cast<ast::Block>(named.value);
  if (!named.accepted || named.cst().recovered() || !named.diagnostics.empty() ||
      !namesRoot || namesRoot->body.size() != 5 ||
      !cursiv::ast_cast<ast::Assignment>(namesRoot->body[0]) ||
      !cursiv::ast_cast<ast::Label>(namesRoot->body[3]) ||
      !cursiv::ast_cast<ast::SayStmt>(namesRoot->body[4]) || !parents(language, named))
    return fail("PARSE options must remain ordinary names/labels elsewhere", names);
  for (const char* text : {"PARSE UPPER LOWER ARG x\n", "PARSE LOWER UPPER ARG x\n",
                           "PARSE CASELESS CASELESS ARG x\n", "PARSE UPPER UPPER ARG x\n",
                           "PARSE LOWER LOWER ARG x\n", "PARSE LOWER CASELESS UPPER ARG x\n",
                           "PARSE LOWER\n", "PARSE CASELESS\n"}) {
    const auto parsed = language.parser->parse(text);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("invalid flag combination or absent source accepted clean", text);
  }
  return true;
}
}  // namespace rexx_parse_options
