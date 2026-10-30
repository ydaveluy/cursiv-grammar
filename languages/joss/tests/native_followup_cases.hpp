#pragma once
#include <cstdio>
#include <string_view>
#include <variant>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "joss/core/ast.hpp"
namespace joss_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = joss::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "joss native: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"joss(Type .035-1.2, 7·3, 2*3*2, |4-7|, "JOSS is easy-to-use".
2.01 Set b(1,1) = .5.
2.02 To part 3 if n ≠ 0.
2.03 Demand x as "value?".
Type 2+2, .035-1.2 in form 1.
Delete b(1,1).
Let P(n) = prod[k=1(1)n:k].
Type min[17.5, 1/2, -5, sqrt(2)], sum[1,2,3].
Form 1:
_    __.___
)joss";
  const auto result = language.language.parser->parse(source);
  const auto* root = result.template value_as<a::Program>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        root && root->statements.size() == 9, "native primer document parses clean");
  if (result.cst().recovered() || !result.diagnostics.empty())
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "joss kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  bool multiply = false, power = false, absolute = false, decimal = false, text = false;
  bool subscript = false, step = false, conditional = false, prompt = false, form = false;
  bool layout = false, removed = false, formula = false, range = false, bracket = false, sum = false;
  for (const auto* node : result.nodes) {
    if (const auto* p = cursiv::ast_cast<a::BinMath>(node)) {
      multiply |= p->op == a::MathOp::mul;
      const auto* left = cursiv::ast_cast<a::BinMath>(p->left);
      power |= p->op == a::MathOp::power && left && left->op == a::MathOp::power;
    }
    if (const auto* p = cursiv::ast_cast<a::AbsoluteMath>(node)) absolute |= p->inner && p->inner->parent == p;
    if (const auto* p = cursiv::ast_cast<a::NumberLit>(node)) decimal |= p->value == .035;
    if (const auto* p = cursiv::ast_cast<a::TypeDataContent>(node))
      for (const auto& item : p->items)
        if (const auto* s = std::get_if<std::string_view>(&item)) text |= *s == "JOSS is easy-to-use";
    if (const auto* p = cursiv::ast_cast<a::Variable>(node)) subscript |= p->name == "b" && p->index && p->secondIndex && p->secondIndex->parent == p;
    if (const auto* p = cursiv::ast_cast<a::IndirectStmt>(node)) step |= p->stepNumber == "2.01";
    if (const auto* p = cursiv::ast_cast<a::ToCmd>(node)) conditional |= cursiv::ast_cast<a::IfCond>(p->cond) && p->cond->parent == p;
    if (const auto* p = cursiv::ast_cast<a::DemandCmd>(node)) prompt |= p->prompt && *p->prompt == "value?";
    if (const auto* p = cursiv::ast_cast<a::TypeFormContent>(node)) form |= p->vars.size() == 2 && cursiv::ast_cast<a::BinMath>(p->vars[0]) && p->form && p->form->number == "1";
    if (const auto* p = cursiv::ast_cast<a::FormStmt>(node)) layout |= p->partNumber == "1" && p->layout && *p->layout == "_    __.___";
    if (const auto* p = cursiv::ast_cast<a::DeleteValCmd>(node)) removed |= p->vars.size() == 1 && p->vars[0]->secondIndex;
    if (const auto* p = cursiv::ast_cast<a::LetCmd>(node)) formula |= p->assignment && p->assignment->target->name == "P";
    if (const auto* p = cursiv::ast_cast<a::RangeCall>(node)) range |= p->fn == a::NaryFn::prod_ && p->range && p->range->var->name == "k" && p->value && p->value->parent == p;
    if (const auto* p = cursiv::ast_cast<a::NaryCall>(node)) {
      bracket |= p->fn == a::NaryFn::min_ && p->rest.size() == 3;
      sum |= p->fn == a::NaryFn::sum_ && p->rest.size() == 2;
    }
  }
  check(multiply && power && absolute && decimal && text, "native arithmetic symbols, left power, decimals and quoted text retained");
  check(subscript && step && conditional && prompt && form && layout && removed,
        "native indices, step number, conditional To, prompt, forms and Delete retained");
  check(formula && range && bracket && sum, "formula and list/range function captures retained");
  if (root && !result.cst().recovered()) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *root);
    check(owned && seen.size() == result.nodes.size(), "all AST captures are uniquely owned");
  }
  for (const auto example : {"1.1 To part 2 if n = 0.", "Type 2*3.", "Type 7•3.",
                              "Type min[17.5, 1/2, -5, sqrt(2)].",
                              "Type _, n, P(n).", "Delete all values."}) {
    const auto r = language.language.parser->parse(example);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), example);
  }
  return ok;
}
} // namespace joss_native_followup
