#pragma once

#include <cstdio>
#include <initializer_list>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "elixir/core/ElixirGrammar.hpp"

namespace elixir_heredoc_lines {

inline bool run(const auto& language) {
  namespace a = elixir::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message, std::string_view source = {}) {
    if (!value) {
      std::fprintf(stderr, "elixir heredoc: %s; source=%.*s\n", message,
                   int(source.size()), source.data() ? source.data() : "");
      ok = false;
    }
  };
  auto owned = [&](const cursiv::ParseResult& result) {
    if (!result.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *result.value);
    return valid && seen.size() == result.nodes.size();
  };
  unsigned positives = 0;
  auto literal = [&]<class T>(std::string_view source,
                              std::initializer_list<std::string_view> texts,
                              unsigned holes, std::string_view name = {},
                              std::string_view modifiers = {},
                              std::size_t delimiterSize = 3) {
    const auto result = language.parser->parse(source);
    const auto* block = cursiv::ast_cast<a::Block>(result.value);
    const auto* value = block && block->body.size() == 1
        ? cursiv::ast_cast<T>(block->body[0]) : nullptr;
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && value &&
                   value->texts.size() == texts.size() &&
                   value->texts.size() == value->holes.size() + 1 &&
                   value->holes.size() == holes;
    if (value && value->texts.size() == texts.size()) {
      unsigned i = 0;
      for (const auto expected : texts) correct &= value->texts[i++] == expected;
    }
    if constexpr (std::is_same_v<T, a::SigilLit>)
      correct &= value && value->name == name && value->modifiers == modifiers &&
                 value->delimiter.size() == delimiterSize;
    if (value)
      for (const auto* hole : value->holes) {
        const auto* body = cursiv::ast_cast<a::Block>(hole);
        const auto* number = body && body->body.size() == 1
            ? cursiv::ast_cast<a::IntegerLit>(body->body[0]) : nullptr;
        correct &= number && number->text == "1" && body->parent == value &&
                   number->parent == body;
      }
    check(correct, "raw segments, interpolation values and modifiers", source);
    check(owned(result), "complete AST/CST ownership", source);
    ++positives;
  };
  literal.template operator()<a::MultiStringLit>("\"\"\"\n\"\"\"", {"\n"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\" \t\nbody\n\"\"\"",
                                                {" \t\nbody\n"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\r\nbody\r\n \t\"\"\"",
                                                {"\r\nbody\r\n \t"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\nleft\rright\n\"\"\"",
                                                {"\nleft\rright\n"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\ninline \"\"\" text\n\"\"\"",
                                                {"\ninline \"\"\" text\n"}, 0);
  literal.template operator()<a::MultiCharlistLit>("'''\ninline ''' text\n\t'''",
                                                  {"\ninline ''' text\n\t"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\n#{1}\n \t\"\"\"",
                                                {"\n", "\n \t"}, 1);
  literal.template operator()<a::MultiCharlistLit>("'''\n#{1} #{1}\n'''",
                                                  {"\n", " ", "\n"}, 2);
  literal.template operator()<a::MultiStringLit>("\"\"\"\n\\#{literal} #{1}\n\"\"\"",
                                                {"\n\\#{literal} ", "\n"}, 1);
  literal.template operator()<a::MultiStringLit>("\"\"\"\nlast\\\n\"\"\"",
                                                {"\nlast\\\n"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\r\nlast\\\r\n\t\"\"\"",
                                                {"\r\nlast\\\r\n\t"}, 0);
  literal.template operator()<a::MultiStringLit>("\"\"\"\n\\\"\"\" not a closer\n\"\"\"",
                                                {"\n\\\"\"\" not a closer\n"}, 0);
  literal.template operator()<a::SigilLit>("~s\"\"\"\ninline \"\"\" #{1}\n \t\"\"\"u",
                                          {"\ninline \"\"\" ", "\n \t"}, 1, "s", "u");
  literal.template operator()<a::SigilLit>("~S\"\"\"\r\ninline \"\"\" #{raw}\r\n\t\"\"\"abc2",
                                          {"\r\ninline \"\"\" #{raw}\r\n\t"}, 0, "S", "abc2");
  literal.template operator()<a::SigilLit>("~c'''\ninline ''' #{1}\n'''",
                                          {"\ninline ''' ", "\n"}, 1, "c");
  literal.template operator()<a::SigilLit>("~C'''\ninline ''' #{raw}\n '''x",
                                          {"\ninline ''' #{raw}\n "}, 0, "C", "x");
  literal.template operator()<a::SigilLit>("~s|left #{1} right|u",
                                          {"left ", " right"}, 1, "s", "u", 1);
  literal.template operator()<a::SigilLit>("~S|#{raw}|abc2",
                                          {"#{raw}"}, 0, "S", "abc2", 1);
  {
    constexpr std::string_view source = "[\"\"\"\na\n\"\"\", '''\nb\n''']";
    const auto result = language.parser->parse(source);
    const auto* block = cursiv::ast_cast<a::Block>(result.value);
    const auto* list = block && block->body.size() == 1
        ? cursiv::ast_cast<a::ListExpr>(block->body[0]) : nullptr;
    const auto* first = list && list->items.size() == 2
        ? cursiv::ast_cast<a::MultiStringLit>(list->items[0]) : nullptr;
    const auto* second = list && list->items.size() == 2
        ? cursiv::ast_cast<a::MultiCharlistLit>(list->items[1]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          first && first->texts.size() == 1 && first->texts[0] == "\na\n" &&
          second && second->texts.size() == 1 && second->texts[0] == "\nb\n" &&
          owned(result), "adjacent punctuation and following heredoc stay separate", source);
    ++positives;
  }
  unsigned negatives = 0;
  for (const std::string_view source : {
      "\"\"\"\ninline\"\"\"", "'''\ninline'''", "~s\"\"\"\ninline\"\"\"",
      "~S'''\ninline'''", "\"\"\"\nbody\r\"\"\"", "\"\"\"\nbody\nx\"\"\"",
      "\"\"\"\nbody", "~S\"\"\"\nbody",
      "\"\"\"inline\"\"\"", "'''inline'''", "~S\"\"\"inline\"\"\"",
      "~ s|x|", "~s |x|"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "missing line-start closing delimiter cannot parse clean", source);
    ++negatives;
  }
  for (const std::string_view text : {"", "\"", "\"\"", "\"\"\"", "text\"\"\"", "\nx\"\"\""}) {
    const auto result = elixir::heredoc_tail<"\"\"\"">(text);
    check(!result && result.error.kind == cursiv::DiagnosticKind::conversion_error &&
          !result.error.message.empty(), "incomplete closing data-rule returns a conversion error", text);
  }
  for (const auto text : {std::string_view{"\n\"\"\""}, std::string_view{"\r\n \t\"\"\""}}) {
    const auto result = elixir::heredoc_tail<"\"\"\"">(text);
    check(result && *result.value == text.substr(0, text.size() - 3),
          "converter preserves closing newline and indentation", text);
  }
  std::printf("elixir heredoc: %u positive and %u negative documents, raw values and complete ownership\n",
              positives, negatives);
  return ok;
}

} // namespace elixir_heredoc_lines
