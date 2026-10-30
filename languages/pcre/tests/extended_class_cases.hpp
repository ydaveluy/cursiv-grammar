#pragma once

#include <cstdio>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "pcre/core/ast.hpp"

namespace pcre_audit {
namespace extended {
namespace a = pcre::ast;

inline std::string shape(const a::Node *node) {
  if (auto n = cursiv::ast_cast<a::ClassSetBinary>(node))
    return "(" + shape(n->left) + std::string(n->op) + shape(n->right) + ")";
  if (auto n = cursiv::ast_cast<a::ClassSetUnary>(node))
    return std::string(n->op) + shape(n->operand);
  if (auto n = cursiv::ast_cast<a::ClassSetGroup>(node))
    return "group(" + shape(n->expression) + ")";
  if (auto n = cursiv::ast_cast<a::ExtendedCharacterClass>(node))
    return shape(n->expression);
  if (auto n = cursiv::ast_cast<a::CharacterClass>(node)) {
    std::string text = n->negate ? "[^" : "[";
    for (auto atom : n->atoms) text += shape(atom);
    return text + "]";
  }
  if (auto n = cursiv::ast_cast<a::CharacterClassRange>(node))
    return shape(n->from) + "-" + shape(n->to);
  if (auto n = cursiv::ast_cast<a::Literal>(node)) return std::string(n->text);
  if (auto n = cursiv::ast_cast<a::CharacterType>(node)) return std::string(n->text);
  if (auto n = cursiv::ast_cast<a::PosixCharacterClass>(node))
    return std::string(n->negate ? "[:^" : "[:") + std::string(n->name) + ":]";
  return "<missing>";
}

inline const a::Node *item(const cursiv::ParseResult &result, std::size_t index = 0) {
  auto root = cursiv::ast_cast<a::Pcre>(result.value);
  const auto *sequence = root && root->pattern && root->pattern->branches.size() == 1
      ? root->pattern->branches[0] : nullptr;
  return sequence && index < sequence->items.size() ? sequence->items[index] : nullptr;
}

inline bool owned(const cursiv::ParseResult &result) {
  std::unordered_set<const cursiv::AstNode *> nodes;
  bool ok = true;
  auto walk = [&](auto &&self, const cursiv::AstNode *node,
                  const cursiv::AstNode *parent) -> void {
    if (!node || !nodes.insert(node).second) { ok = false; return; }
    if (node->parent != parent) ok = false;
    auto child = [&](const cursiv::AstNode *value) { self(self, value, node); };
    if (auto n = cursiv::ast_cast<a::Pcre>(node)) child(n->pattern);
    if (auto n = cursiv::ast_cast<a::Alternation>(node))
      for (auto value : n->branches) child(value);
    if (auto n = cursiv::ast_cast<a::Sequence>(node))
      for (auto value : n->items) child(value);
    if (auto n = cursiv::ast_cast<a::ExtendedCharacterClass>(node)) child(n->expression);
    if (auto n = cursiv::ast_cast<a::ClassSetBinary>(node)) { child(n->left); child(n->right); }
    if (auto n = cursiv::ast_cast<a::ClassSetUnary>(node)) child(n->operand);
    if (auto n = cursiv::ast_cast<a::ClassSetGroup>(node)) child(n->expression);
    if (auto n = cursiv::ast_cast<a::CharacterClass>(node))
      for (auto value : n->atoms) child(value);
    if (auto n = cursiv::ast_cast<a::CharacterClassRange>(node)) { child(n->from); child(n->to); }
    if (auto n = cursiv::ast_cast<a::Quantified>(node)) {
      child(n->operand);
      for (auto value : n->annotations) child(value);
    }
  };
  walk(walk, result.value, nullptr);
  return ok && nodes.size() == result.nodes.size();
}
} // namespace extended

template <class Language> bool extended_class_cases(const Language &language) {
  namespace a = pcre::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool value, const char *message, std::string_view source) {
    ++checks;
    if (!value) {
      std::fprintf(stderr, "pcre extended: %s [%.*s]\n", message,
                   static_cast<int>(source.size()), source.data());
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto result = parser.parse(source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    check(clean, "must parse without recovery", source);
    if (!clean)
      for (const auto &d : result.diagnostics)
        std::fprintf(stderr, "kind=%u element=%u bytes=%u..%u\n",
                     static_cast<unsigned>(d.kind), static_cast<unsigned>(d.element),
                     static_cast<unsigned>(d.begin), static_cast<unsigned>(d.end));
    check(extended::owned(result), "all AST nodes have exact field parents", source);
    return result;
  };
  struct Case { std::string_view source, expected; };
  const Case valid[] = {
      {R"PC((?[[a]|[b]]))PC", "([a]|[b])"},
      {R"PC((?[[a]+[b]]))PC", "([a]+[b])"},
      {R"PC((?[[a-f]&[d-z]]))PC", "([a-f]&[d-z])"},
      {R"PC((?[[a-f]-[bd]]))PC", "([a-f]-[bd])"},
      {R"PC((?[[ab]^[bc]]))PC", "([ab]^[bc])"},
      {R"PC((?[[a-z]&![aeiou]]))PC", "([a-z]&![aeiou])"},
      {R"PC((?[!!\d]))PC", "!!\\d"},
      {R"PC((?[[a]|[b]&[c]]))PC", "([a]|([b]&[c]))"},
      {R"PC((?[[ab]-[b]+[b]]))PC", "(([ab]-[b])+[b])"},
      {R"PC((?[( [ab] - ([b] + [a]) ) | \x63]))PC",
       "(group(([ab]-group(([b]+[a]))))|\\x63)"},
      {R"PC((?[\d | [:alpha:]]))PC", "(\\d|[:alpha:])"},
      {R"PC((?[\pL & [^Q]]))PC", "(\\pL&[^Q])"},
      {R"PC((?[\n | \x41 | \b]))PC", "((\\n|\\x41)|\\b)"},
      {"(?[ [ a \t - c ] ])", "[a-c]"},
      {R"PC((?[[\Qa\E - z]]))PC", "[a-z]"},
      {R"PC((?[[\Qab\E - z]]))PC", "[ab-z]"},
      {R"PC((?[\Q\E\E [a] \Q\E + \E[b]]))PC", "([a]+[b])"},
      {R"PC((?[\  | \t]))PC", "(\\ |\\t)"},
      {R"PC((?[\g | \k]))PC", "(\\g|\\k)"},
      {"(?[[\\Q \t()&\\E]])", "[ \t()&]"},
      {R"PC((?[[:^digit:] & [a-z]]))PC", "([:^digit:]&[a-z])"},
      {"(?[[\n]])", "[\n]"},
      {R"PC((?[[\Q\E ^ a]]))PC", "[^a]"},
  };
  for (const auto &entry : valid) {
    auto result = parse(entry.source);
    const auto *set = cursiv::ast_cast<a::ExtendedCharacterClass>(extended::item(result));
    check(set && extended::shape(set) == entry.expected,
          "set operator/leaf/group values and precedence", entry.source);
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///extended-valid.pcre", "pcre", 0, std::string(entry.source)));
    document.language = &language.language;
    document.parseResult = std::move(result);
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document, diagnostics);
    check(diagnostics.empty(), "valid set has no validation error", entry.source);
  }
  {
    const auto result = parse(" (?[[a]]) +");
    const auto *first = cursiv::ast_cast<a::Literal>(extended::item(result, 0));
    const auto *set = cursiv::ast_cast<a::ExtendedCharacterClass>(extended::item(result, 1));
    const auto *repeat = cursiv::ast_cast<a::Quantified>(extended::item(result, 2));
    const auto *space = repeat ? cursiv::ast_cast<a::Literal>(repeat->operand) : nullptr;
    check(first && first->text == " " && set && space && space->text == " " &&
              repeat->kind == a::QuantifierKind::plus,
          "intrinsic xx does not consume caller whitespace", " (?[[a]]) +");
  }
  {
    const auto result = parse(R"PC((?[[a]|[b]])+)PC");
    const auto *repeat = cursiv::ast_cast<a::Quantified>(extended::item(result));
    const auto *set = repeat ? cursiv::ast_cast<a::ExtendedCharacterClass>(repeat->operand) : nullptr;
    check(repeat && repeat->kind == a::QuantifierKind::plus && set &&
              extended::shape(set) == "([a]|[b])",
          "set class remains one quantifiable character atom", "quantified set");
  }
  for (std::string_view source : {
           R"PC((?[a]))PC", R"PC((?[[a][b]]))PC", R"PC((?[\Qx\E]))PC",
           "(?[\n\\d])", R"PC((?[[a]&]))PC", R"PC((?[]))PC",
           R"PC((?[()]))PC", R"PC((?[^[a]]))PC", R"PC((?[[a]] ))PC",
           R"PC((?[[a] #comment]))PC"}) {
    auto result = parser.parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "invalid set syntax is rejected", source);
  }
  for (const auto &[source, code] : {
           std::pair{"(?[\\R])", "pcre.class-escape"},
           std::pair{"(?[\\C])", "pcre.class-escape"},
           std::pair{"(?[\\y])", "pcre.escape"},
           std::pair{"(?[[:bogus:]])", "pcre.posix-name"},
           std::pair{"(?[[z-a]])", "pcre.class-range"}}) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///extended.pcre", "pcre", 0, std::string(source)));
    document.language = &language.language;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(),
          "semantic class error reaches validator", source);
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document, diagnostics);
    unsigned found = 0;
    for (const auto &d : diagnostics)
      if (d.code && d.code->isString() && d.code->string() == code) ++found;
    check(found == 1, "exact class validation code/count", source);
  }
  std::printf("pcre extended: %zu set forms, %u checks\n", std::size(valid), checks);
  return ok;
}
} // namespace pcre_audit
