#pragma once

#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "tl/core/CoreServices.hpp"
#include "tl/core/ast.hpp"

namespace audit_regressions {
inline std::string shape(const tl::ast::Proposition* node) {
  namespace a = tl::ast;
  if (cursiv::ast_cast<a::Missing>(node)) return "?";
  if (cursiv::ast_cast<a::Bottom>(node)) return "⊥";
  if (const auto* n = cursiv::ast_cast<a::AtomicProp>(node)) return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<a::Group>(node)) return shape(n->inner);
  if (const auto* n = cursiv::ast_cast<a::Not>(node)) return "!(" + shape(n->operand) + ")";
  if (const auto* n = cursiv::ast_cast<a::TenseOp>(node))
    return std::string(n->tense == a::Tense::always ? "G(" : "H(") + shape(n->operand) + ")";
  if (const auto* n = cursiv::ast_cast<a::Or>(node)) return "(" + shape(n->left) + "∨" + shape(n->right) + ")";
  return "null";
}
inline bool run() {
  const auto language = cursiv::create_language<tl::TlCoreServices,
      tl::TlSharedCoreServices, tl::install_core_module>();
  struct Case { const char* text; const char* expected; std::size_t missing; };
  for (const Case c : {Case{"", "?", 1}, {"()", "?", 1}, {"⌐", "!(?)", 1},
      {"G", "G(?)", 1}, {"H", "H(?)", 1}, {"∨", "(?∨?)", 2},
      {"p∨", "(p∨?)", 1}, {"∨p", "(?∨p)", 1}, {"p∨q∨r", "((p∨q)∨r)", 0},
      {"Gp∨q", "G((p∨q))", 0}, {"p∨Gq", "(p∨G(q))", 0},
      {"Gp∨Hq∨r", "G((p∨H((q∨r))))", 0}, {"⌐Gp∨q", "!(G((p∨q)))", 0},
      {"G⌐p∨q", "G((!(p)∨q))", 0}, {"(Gp)∨q", "(G(p)∨q)", 0},
      {"⌐(p∨q)", "!((p∨q))", 0}}) {
    const auto parsed = language.language.parser->parse(c.text);
    const auto* root = cursiv::ast_cast<tl::ast::Formula>(parsed.value);
    if (!parsed.accepted || parsed.cst().recovered() || !root || shape(root->value) != c.expected) {
      std::fprintf(stderr, "tl regression: %s: got %s want %s\n", c.text,
                   root ? shape(root->value).c_str() : "no root", c.expected);
      return false;
    }
    const auto validation = cursiv::test::validationHelper(language.language, c.text);
    if (validation.diagnostics.size() != c.missing) return false;
    for (const auto& d : validation.diagnostics)
      if (!d.code || !d.code->isString() || d.code->string() != "tl.operand" ||
          d.range.begin != d.range.end) return false;
  }
  return true;
}
}  // namespace audit_regressions
