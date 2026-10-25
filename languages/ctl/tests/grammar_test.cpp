// The smoke test of the language: every document under example/ parses clean
// into a Proposition, and connectives.ctl's tree matches the precedence of
// CTL's four connectives.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "ctl/core/CoreServices.hpp"
#include "ctl/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "ctl: %s\n", what.c_str());
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

const char* opSym(ctl::ast::BinaryOp op) {
  switch (op) {
    case ctl::ast::BinaryOp::conjunction: return "∧";
    case ctl::ast::BinaryOp::disjunction: return "∨";
    case ctl::ast::BinaryOp::implication: return "⇒";
    case ctl::ast::BinaryOp::biconditional: return "⇔";
  }
  return "?";
}

// Renders a formula fully parenthesised, e.g. `((p∧q)∨r)`, so a
// probe's shape can be asserted as one string. Top/Bottom/Not use ASCII
// markers (T, F, ~) since ATOMIC names are lowercase-only and cannot clash.
std::string render(const cursiv::AstNode* n) {
  if (const auto* const b = as<ctl::ast::BinaryProp>(n); b != nullptr)
    return "(" + render(b->left) + opSym(b->op) + render(b->right) + ")";
  if (const auto* const a = as<ctl::ast::AtomicProp>(n); a != nullptr)
    return std::string(a->name);
  if (as<ctl::ast::Top>(n) != nullptr) return "T";
  if (as<ctl::ast::Bottom>(n) != nullptr) return "F";
  if (const auto* const not_ = as<ctl::ast::Not>(n); not_ != nullptr)
    return "~" + render(not_->operand);
  if (const auto* const g = as<ctl::ast::Grouped>(n); g != nullptr)
    return render(g->inner);
  if (const auto* const t = as<ctl::ast::Quantified>(n); t != nullptr)
    return std::string(t->quantifier == ctl::ast::PathQuantifier::inevitable ? "A" : "E") +
        render(t->path);
  if (const auto* const t = as<ctl::ast::UnaryTemporal>(n); t != nullptr) {
    const char* modalities[] = {"X", "F", "G"};
    return std::string(modalities[static_cast<unsigned>(t->modality)]) + render(t->operand);
  }
  if (const auto* const t = as<ctl::ast::BinaryTemporal>(n); t != nullptr)
    return "[" + render(t->left) +
        (t->relation == ctl::ast::TemporalRelation::until ? "U" : "R") + render(t->right) + "]";
  return "?";
}

// Every pair of adjacent levels in CTL's connective precedence (AND, OR,
// IMPLIES, IFF, tightest to loosest), plus the regression a
// flat single level would get backwards.
bool check_precedence() {
  const auto language = cursiv::create_language<
      ctl::CtlCoreServices, ctl::CtlSharedCoreServices,
      ctl::install_core_module>();

  struct Probe {
    const char* formula;
    const char* want;
  };
  static const Probe probes[] = {
      {"p∧q∨r", "((p∧q)∨r)"},
      {"p∨q⇒r", "((p∨q)⇒r)"},
      {"p⇒q⇔r", "((p⇒q)⇔r)"},
      {"p⇒q∧r", "(p⇒(q∧r))"},
  };
  bool ok = true;
  for (const Probe& p : probes) {
    const cursiv::ParseResult result =
        language.language.parser->parse(p.formula);
    const std::string got = render(result.value);
    if (got != p.want) {
      std::fprintf(stderr, "ctl: %s: got %s want %s\n", p.formula,
                   got.c_str(), p.want);
      ok = false;
    }
  }
  return ok;
}

bool check_native_forms() {
  const auto language = cursiv::create_language<ctl::CtlCoreServices,
      ctl::CtlSharedCoreServices, ctl::install_core_module>();
  struct Probe { const char* text; const char* tree; };
  for (const Probe p : {
      Probe{"p_1 and _busy", "(p_1∧_busy)"}, {"Close or Error", "(Close∨Error)"},
      {"true & false", "(T∧F)"}, {"true_1", "true_1"}, {"notp", "notp"},
      {"\"A\"", "A"}, {"not p", "~p"}, {"~p", "~p"}, {"\"a b\"", "a b"},
      {"\"a\\\"b\"", "a\\\"b"}, {"\"\"", ""},
      {"A X p", "AXp"}, {"E G p", "EGp"}, {"AF p", "AFp"},
      {"A[p U q]", "A[pUq]"}, {"E(p R q)", "E[pRq]"},
      {"A(p U q)", "A[pUq]"}, {"E[p R q]", "E[pRq]"},
      {"p --> q", "(p⇒q)"}, {"AXp", "AXp"}, {"A (X p)", "AXp"}, {"E ((G p))", "EGp"}}) {
    const auto parsed = language.language.parser->parse(p.text);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        render(parsed.value) != p.tree) {
      std::fprintf(stderr, "ctl: %s: got %s want %s\n", p.text,
                   render(parsed.value).c_str(), p.tree);
      return false;
    }
  }
  for (const auto& [text, code] : {
      std::pair{"A", "ctl.operand"}, {"E", "ctl.operand"}, {"not", "ctl.operand"}, {"X p", "ctl.quantifier"}, {"A p", "ctl.path"},
      {"A X X p", "ctl.state"}, {"p and X q", "ctl.state"},
      {"A(p U X q)", "ctl.state"}}) {
    const auto parsed = language.language.parser->parse(text);
    const auto diagnostics = cursiv::test::validationHelper(language.language, text).diagnostics;
    bool found = false;
    for (const auto& diagnostic : diagnostics)
      if (diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == code)
        found = true;
    if (!parsed.accepted || parsed.cst().recovered() || !found) {
      std::fprintf(stderr, "ctl: missing runtime diagnostic %s for %s\n", code, text);
      return false;
    }
  }
  return true;
}

// connectives.ctl mixes all three boundaries in one formula: the root must
// be the (loosest) implication, not the last-written (rightmost) ∧ a
// flat single level would produce.
bool check_connectives(const std::filesystem::path& test_dir) {
  const auto language = cursiv::create_language<
      ctl::CtlCoreServices, ctl::CtlSharedCoreServices,
      ctl::install_core_module>();

  const std::filesystem::path path = test_dir / "connectives.ctl";
  std::ifstream file(path);
  if (!file) return fail(path.string() + ": not readable"), false;
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto* const top = as<ctl::ast::BinaryProp>(result.value);
  bool ok = top != nullptr && top->op == ctl::ast::BinaryOp::implication;

  const auto* const topLeft =
      ok ? as<ctl::ast::BinaryProp>(top->left) : nullptr;
  ok = ok && topLeft != nullptr &&
       topLeft->op == ctl::ast::BinaryOp::disjunction;
  const auto* const orLeft =
      ok ? as<ctl::ast::BinaryProp>(topLeft->left) : nullptr;
  ok = ok && orLeft != nullptr &&
       orLeft->op == ctl::ast::BinaryOp::conjunction &&
       as<ctl::ast::Quantified>(orLeft->left) != nullptr &&    // AXp
       as<ctl::ast::Quantified>(orLeft->right) != nullptr &&   // EGq
       as<ctl::ast::Quantified>(topLeft->right) != nullptr;    // AFr

  const auto* const topRight =
      ok ? as<ctl::ast::BinaryProp>(top->right) : nullptr;
  ok = ok && topRight != nullptr &&
       topRight->op == ctl::ast::BinaryOp::conjunction &&
       as<ctl::ast::Quantified>(topRight->right) != nullptr;  // E[q U ~r]
  const auto* const rightLeft =
      ok ? as<ctl::ast::BinaryProp>(topRight->left) : nullptr;
  ok = ok && rightLeft != nullptr &&
       rightLeft->op == ctl::ast::BinaryOp::conjunction &&
       as<ctl::ast::Quantified>(rightLeft->right) != nullptr &&   // A[p U q]
       as<ctl::ast::Grouped>(rightLeft->left) != nullptr;    // (T<=>~F)

  if (!ok) fail(path.string() + ": the tree does not match precedence");
  return ok;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      ctl::CtlCoreServices,
      ctl::CtlSharedCoreServices,
      ctl::install_core_module>();

  int seen = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    ++seen;

    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        !root->type->derivesFrom(&cursiv::ast_type_tag<ctl::ast::Proposition>))
      return fail(entry.path().string() + ": the parse built no proposition");
  }
  if (seen == 0) return fail("no examples found under " CURSIV_EXAMPLE_DIR);

  bool ok = check_native_forms() && check_precedence();
  ok = check_connectives(CURSIV_TEST_DIR) && ok;
  if (!ok) return 1;

  std::printf("ctl: %d example(s) parse clean\n", seen);
  return 0;
}
