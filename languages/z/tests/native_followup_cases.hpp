#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "z/core/CoreServices.hpp"
#include "z/core/ast.hpp"

namespace native_followup {
inline bool run(const z::ZCoreServices& services) {
  namespace ast = z::ast;
  bool ok = true;
  const auto check = [&](bool valid, const char* message) {
    if (!valid) { std::fprintf(stderr, "z followup: %s\n", message); ok = false; }
  };
  const auto clean = [](const auto& r) { return r.accepted && !r.cst().recovered() && r.diagnostics.empty() && r.value; };
  const auto owns = [&](const auto& r) {
    bool valid = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      services.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        valid &= child.parent == &node; self(self, child);
      });
    };
    if (r.value) walk(walk, *r.value);
    return valid && seen.size() == r.nodes.size();
  };
  for (const std::string newline : {"\n", "\r\n", "\r", "\u2028"}) {
    const std::string text = "┌\tState" + newline + "x\t:\tℕ" + newline + "y : ℕ |" + newline +
                             "x = 1" + newline + "y = ٢" + newline + "└";
    const auto r = services.parser->parse(text);
    const auto* spec = cursiv::ast_cast<ast::Specification>(r.value);
    const auto* schema = spec && spec->items.size() == 1 ? cursiv::ast_cast<ast::SchemaDef>(spec->items[0]) : nullptr;
    check(clean(r) && owns(r) && schema && schema->name == "State" && schema->text &&
          schema->text->decls.size() == 2 && schema->text->where.size() == 2 && r.cst().text() == text,
          "TAB and each CZT newline keep separate schema declarations and predicates");
  }
  {
    const std::string text = "─ function ٣٠ rightassoc ( _ ⊞ _ ) └\n─ n == -1 └\n─ r == a ⊞ b + c └\n";
    const auto r = services.parser->parse(text);
    const auto* spec = cursiv::ast_cast<ast::Specification>(r.value);
    const auto* op = spec && spec->items.size() == 3 ? cursiv::ast_cast<ast::OperatorTemplate>(spec->items[0]) : nullptr;
    const auto* def = spec && spec->items.size() == 3 ? cursiv::ast_cast<ast::HorizontalDef>(spec->items[1]) : nullptr;
    const auto* negate = def ? cursiv::ast_cast<ast::Prefix>(def->value) : nullptr;
    const auto* one = negate ? cursiv::ast_cast<ast::Number>(negate->operand) : nullptr;
    const auto* custom = spec && spec->items.size() == 3 ? cursiv::ast_cast<ast::HorizontalDef>(spec->items[2]) : nullptr;
    const auto* sequence = custom ? cursiv::ast_cast<ast::OperatorSequence>(custom->value) : nullptr;
    check(clean(r) && owns(r) && op && op->hasPrec && op->precedence == "٣٠" && op->rightAssoc &&
          op->text.find("⊞") != std::string_view::npos && negate && negate->op == "-" && one && one->text == "1" &&
          sequence && sequence->steps.size() == 2 && sequence->steps[0]->op == "⊞" && sequence->steps[1]->op == "+",
          "Unicode precedence capture, native arithmetic negation and unresolved custom source order");
  }
  {
    const auto r = services.parser->parse("─ [X]\n a == 1\u2028 b == 2 └\n");
    const auto* spec = cursiv::ast_cast<ast::Specification>(r.value);
    const auto* first = spec && spec->items.size() == 3 ? cursiv::ast_cast<ast::HorizontalDef>(spec->items[1]) : nullptr;
    const auto* second = spec && spec->items.size() == 3 ? cursiv::ast_cast<ast::HorizontalDef>(spec->items[2]) : nullptr;
    check(clean(r) && owns(r) && spec && spec->items.size() == 3 &&
          cursiv::ast_cast<ast::GivenTypes>(spec->items[0]) && first && first->name == "a" && second && second->name == "b",
          "several native unboxed paragraph items retain their own AST nodes");
  }
  for (const char* text : {"─ [] └", "─ f [] == 1 └", "─ relation 30 leftassoc ( _ ⊞ _ ) └"})
    check(!clean(services.parser->parse(text)), "native nonempty names/formals and relation-template category constraint");
  return ok;
}
}  // namespace native_followup
