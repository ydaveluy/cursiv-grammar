#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "nasm/core/ast.hpp"

namespace nasm_lexical {
namespace ast = nasm::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "nasm lexical: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const auto& services, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    services.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& services) {
  const std::string source =
      "bits 64\n$eax equ 7\n$face equ 8\n$rax: dd $eax,$face,$0face\n"
      "mov eax,$eax\ndq $rax,$$,$\n";
  const auto parsed = services.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(services, parsed)) return fail("forced symbol source or ownership", source);
  const auto* root = cursiv::ast_cast<ast::Program>(parsed.value);
  if (!root || root->lines.size() != 6) return fail("six physical lines", source);
  const auto* eax = cursiv::ast_cast<ast::EquDecl>(root->lines[1]->stmt);
  const auto* face = cursiv::ast_cast<ast::EquDecl>(root->lines[2]->stmt);
  const auto* data = cursiv::ast_cast<ast::DataDecl>(root->lines[3]->stmt);
  const auto* instruction = cursiv::ast_cast<ast::Instruction>(root->lines[4]->stmt);
  const auto* last = cursiv::ast_cast<ast::DataDecl>(root->lines[5]->stmt);
  if (!eax || eax->label != "eax" || !face || face->label != "face" ||
      root->lines[3]->label != "rax" || !data || data->values.size() != 3 ||
      !instruction || instruction->operands.size() != 2 || !last || last->values.size() != 3)
    return fail("native symbol names and declaration/instruction shapes", source);
  const auto* a = cursiv::ast_cast<ast::NameRef>(data->values[0]->atom);
  const auto* b = cursiv::ast_cast<ast::NameRef>(data->values[1]->atom);
  const auto* hex = cursiv::ast_cast<ast::IntegerLit>(data->values[2]->atom);
  const auto* reg = cursiv::ast_cast<ast::RegisterRef>(instruction->operands[0]->value);
  const auto* symbol = cursiv::ast_cast<ast::NameRef>(instruction->operands[1]->value);
  const auto* rax = cursiv::ast_cast<ast::NameRef>(last->values[0]->atom);
  const auto* section = cursiv::ast_cast<ast::DollarExpr>(last->values[1]->atom);
  const auto* here = cursiv::ast_cast<ast::DollarExpr>(last->values[2]->atom);
  if (!a || a->name != "eax" || !b || b->name != "face" ||
      !hex || hex->text != "$0face" || !reg || reg->name != "eax" ||
      !symbol || symbol->name != "eax" || !rax || rax->name != "rax" ||
      !section || !section->section || !here || here->section)
    return fail("$name must bypass reserved words, while $digit/$$/$ retain their kinds", source);

  for (const char* literal : {"200t", "200T", "0t200", "0T200", "0c8x", "0C8X", "$0c8"}) {
    const std::string text = std::string("dd ") + literal + "\n";
    const auto result = services.parser->parse(text);
    unsigned exact = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<ast::IntegerLit>(node)) {
        if (value->text != literal) return fail("integer spelling changed", text);
        ++exact;
      }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        exact != 1 || !parents(services, result)) return fail("native integer alias", text);
  }
  for (const char* literal : {"1e2", "1E-2", "1_000e+2", "$0f.fp2",
                              "1e+", "1e-", "1e", "1e_", "1e+_", "1e-_",
                              "0x1p+", "0x1p-", "0x1p_"}) {
    const std::string text = std::string("dq ") + literal + "\n";
    const auto result = services.parser->parse(text);
    unsigned exact = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<ast::FloatLit>(node)) {
        if (value->text != literal) return fail("real spelling changed", text);
        ++exact;
      }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        exact != 1 || !parents(services, result)) return fail("native exponent/dollar real", text);
  }
  for (const char* text : {"dd $1g\n", "dd 0t2a\n", "dq 1e+z\n",
                           "dq 1e-z\n", "dq 0x1p+z\n", "dq 0x1p\n"}) {
    const auto result = services.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid native numeric token accepted", text);
  }
  return true;
}
}  // namespace nasm_lexical
