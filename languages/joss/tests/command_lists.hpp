#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "joss/core/ast.hpp"

namespace joss_command_lists {
template<class Language> bool run(const Language& language) {
  namespace a = joss::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "joss commands: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"joss(Do step 1.1, 3 times.
1.2 Do part 2, n+1 times if n>0.
1.3 Do step 1.1, 2 times.
Delete L, form 2, all parts.
Delete step 1.1, part 2, form 3, all values.
Delete A, B, step 1.1, C, all forms.
Delete all values, form 1.
Delete b(1,1).
Delete A, B.
Delete form 4.
Do part 1.
)joss";
  const auto result = language.language.parser->parse(source);
  const auto* root = cursiv::ast_cast<a::Program>(result.value);
  const bool clean = result.accepted && !result.cst().recovered() &&
                     result.diagnostics.empty() && root && root->statements.size() == 11;
  check(clean, "times and mixed Delete lists parse cleanly");
  if (!clean) {
    std::fprintf(stderr, "joss commands: accepted=%u recovered=%u statements=%zu\n",
        unsigned(result.accepted), unsigned(result.cst().recovered()),
        root ? root->statements.size() : 0);
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "joss commands: kind=%u bytes=%u..%u element=%u\n",
          unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    return false;
  }
  auto command = [&](std::size_t i) -> const a::Command* {
    if (const auto* s = cursiv::ast_cast<a::DirectStmt>(root->statements[i])) return s->command;
    if (const auto* s = cursiv::ast_cast<a::IndirectStmt>(root->statements[i])) return s->command;
    return nullptr;
  };
  const auto* first = cursiv::ast_cast<a::DoCmd>(command(0));
  const auto* firstCount = first ? cursiv::ast_cast<a::NumberLit>(first->times) : nullptr;
  const auto* firstTarget = first ? cursiv::ast_cast<a::RefSelector>(first->target) : nullptr;
  check(firstCount && firstCount->value == 3 && firstTarget &&
        firstTarget->kind == a::SelKind::step_ && firstTarget->number == "1.1" &&
        first->times->parent == first && first->target->parent == first,
        "Do step captures count and target with correct parents");
  const auto* second = cursiv::ast_cast<a::DoCmd>(command(1));
  const auto* sum = second ? cursiv::ast_cast<a::BinMath>(second->times) : nullptr;
  const auto* secondTarget = second ? cursiv::ast_cast<a::RefSelector>(second->target) : nullptr;
  check(sum && sum->op == a::MathOp::add && secondTarget &&
        secondTarget->kind == a::SelKind::part_ && secondTarget->number == "2" &&
        cursiv::ast_cast<a::IfCond>(second->cond),
        "repeat count is an expression and preserves the following if clause");
  const auto* third = cursiv::ast_cast<a::DoCmd>(command(2));
  check(third && third->times && !third->cond,
        "stored repeated Do keeps its count without inventing a condition");
  const auto* mixed = cursiv::ast_cast<a::DeleteListCmd>(command(3));
  check(mixed && mixed->targets.size() == 3, "mixed Delete has three ordered targets");
  if (mixed && mixed->targets.size() == 3) {
    const auto* variable = std::get_if<a::Variable*>(&mixed->targets[0]);
    const auto* form = std::get_if<a::Selector*>(&mixed->targets[1]);
    const auto* all = std::get_if<a::Selector*>(&mixed->targets[2]);
    const auto* f = form ? cursiv::ast_cast<a::RefSelector>(*form) : nullptr;
    const auto* p = all ? cursiv::ast_cast<a::AllSelector>(*all) : nullptr;
    check(variable && *variable && (*variable)->name == "L" && f &&
          f->kind == a::SelKind::form_ && f->number == "2" && p &&
          p->restriction == a::AllKind::parts_, "Delete preserves variable/form/all target order");
  }
  const auto* selectors = cursiv::ast_cast<a::DeleteListCmd>(command(4));
  const auto* longer = cursiv::ast_cast<a::DeleteListCmd>(command(5));
  const auto* allFirst = cursiv::ast_cast<a::DeleteListCmd>(command(6));
  check(selectors && selectors->targets.size() == 4 && longer &&
        longer->targets.size() == 5 && allFirst && allFirst->targets.size() == 2,
        "selector-first and multi-variable-first lists keep every target");
  const auto* legacySingle = cursiv::ast_cast<a::DeleteValCmd>(command(7));
  const auto* legacyList = cursiv::ast_cast<a::DeleteValCmd>(command(8));
  const auto* legacySelector = cursiv::ast_cast<a::DeleteCmdCmd>(command(9));
  const auto* legacyDo = cursiv::ast_cast<a::DoCmd>(command(10));
  check(legacySingle && legacySingle->vars.size() == 1 && legacySingle->vars[0]->secondIndex &&
        legacyList && legacyList->vars.size() == 2 && legacySelector && legacySelector->target &&
        legacyDo && !legacyDo->times, "existing simple command AST shapes remain intact");

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
  check(owned && seen.size() == result.nodes.size(), "all new captures have unique parents and CST ownership");
  for (const auto source : {"Do step 1.1 3 times.", "Do part 1, times.",
          "Do part 1, 3.", "Do part 1, 3 times times.", "Do form 1, 3 times.",
          "Delete form 1,.", "Delete , form 1.", "Delete form 1,, part 2.",
          "Delete form, part 2.", "Delete A, form 1, part."}) {
    const auto r = language.language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "joss commands: unexpectedly clean: %s\n", source);
    check(rejected, "malformed times or target list is rejected");
  }
  return ok;
}
} // namespace joss_command_lists
