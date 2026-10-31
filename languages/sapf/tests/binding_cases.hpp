#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "sapf/core/ast.hpp"

namespace sapf_bindings {
namespace ast = sapf::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "sapf bindings: %s\n%.*s\n", message,
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
  const std::string source =
      "1 = x 1 2 3 = (a b c) [1 2 3] = [left right] "
      "= (name? a=b) '123abc 'name? foo=bar =name 'it's \\a? b! [a? b!]";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) return fail("native bindings/symbols or ownership", source);
  const auto* root = cursiv::ast_cast<ast::File>(parsed.value);
  if (!root) return fail("typed File", source);
  unsigned single = 0, stack = 0, list = 0, quoted = 0, word = 0, functions = 0;
  for (const auto* item : root->items) {
    if (const auto* binding = cursiv::ast_cast<ast::Binding>(item)) {
      if (binding->kind == ast::BindingKind::single) {
        if (binding->names.size() != 1 || binding->names[0] != "x")
          return fail("single binding target", source);
        ++single;
      } else if (binding->kind == ast::BindingKind::list) {
        if (binding->names.size() != 2 || binding->names[0] != "left" || binding->names[1] != "right")
          return fail("list binding order", source);
        ++list;
      } else {
        if (!((binding->names.size() == 3 && binding->names[0] == "a" &&
               binding->names[1] == "b" && binding->names[2] == "c") ||
              (binding->names.size() == 2 && binding->names[0] == "name?" && binding->names[1] == "a=b")))
          return fail("stack binding order/punctuation", source);
        ++stack;
      }
    }
    if (const auto* symbol = cursiv::ast_cast<ast::Word>(item)) {
      if (symbol->prefix == ast::WordPrefix::quote) {
        if (symbol->name != "123abc" && symbol->name != "name?" && symbol->name != "it's")
          return fail("quoted native symbol", source);
        ++quoted;
      } else {
        if (symbol->name != "foo=bar" && symbol->name != "=name")
          return fail("equals inside a word must not become a binder", source);
        ++word;
      }
    }
    if (const auto* function = cursiv::ast_cast<ast::FunctionLiteral>(item)) {
      if (function->params.size() != 2 || function->params[0] != "a?" ||
          function->params[1] != "b!" || !function->body || function->body->items.size() != 2)
        return fail("symbol-shaped function parameters", source);
      ++functions;
    }
  }
  if (single != 1 || stack != 2 || list != 1 || quoted != 3 || word != 2 || functions != 1)
    return fail("binding kinds, symbol-prefix shapes or item counts", source);

  for (const char* text : {"1 = x", "1 2 = (a b)", "[1 2]=[a b]",
                           "= (a ; comment\n b)", "= [123 + name?]",
                           "{ :name? 1 :123 2 }", ",123 .name? `a=b"}) {
    const auto result = language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !parents(language, result)) return fail("native bind/name context", text);
  }
  for (const char* text : {"=", "= ()", "= []", "= ([a b])", "= [[a b]]",
                           "= (a,b)", "= \"name\"", "= (a", "= [a"}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-disallowed empty/nested/quoted binding target", text);
  }
  return true;
}
}  // namespace sapf_bindings
