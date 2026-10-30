#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "sharc/core/ast.hpp"
#include "sharc/generated/Sharc/Module.hpp"
#include <cstdio>
#include <string>
#include <string_view>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sharc::generated::Sharc::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sharc::ast;
  auto r = parser->parse(".var n = (1 + 2) * 3;\nr0 = n + 1;\n");
  check(r.accepted && !r.cst().recovered(),
        "structured and opaque operands share expression syntax");
  // Analog Devices Programming Reference, tables 12-3 and 12-5:
  // averaging uses /2, and multiplier operands are data registers.
  for (const char* source : {"r0 = (r1 + r2) / 2;", "r3 = r3 * r4;", "R0 = R1 - R2 + CI - 1;"}) {
    const auto valid = parser->parse(source);
    check(valid.accepted && !valid.cst().recovered() && valid.diagnostics.empty(),
          "native averaging and register multiplication");
  }
  for (const char* source : {"r0 = (r1 + r2) / r3;", "r3 = mr0f * r4;", "bit set astat r1;"}) {
    const auto invalid = parser->parse(source);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "unsupported instruction forms must not pass the native corpus");
  }
  {
    // Programming reference rev2.4, Type18a example (pp9-66/9-67):
    // the final operand is an immediate mask, not a data register.
    const auto parsed = parser->parse("bit set mode2 0x00000070;");
    const auto* root = parsed.value_as<ast::Program>();
    const auto* statement = root && root->statements.size() == 1 ? root->statements[0] : nullptr;
    const auto* misc = statement ? cursiv::ast_cast<ast::Misc>(statement->atom) : nullptr;
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
              statement && statement->parent == root && misc && misc->parent == statement &&
              misc->raw == "bit set mode2 0x00000070",
          "native Type18 immediate mask and instruction AST ownership");
  }
  const auto expressions = parser->parse(
      ".SECTION program;\n.VAR p = 1 + 2 * 3;\n.VAR q = 1 ^ 2 | 3 & 4;\n"
      ".VAR n = ~1 + 2;\n.VAR bits = B#1010;\n.VAR f = 0.5r;\n"
      ".VAR ptr = ADDRESS(foo$bar);\n.ALIGN 1 << 3;\nIF EQ JUMP Target;\nR0 = ABS R1;\n");
  check(expressions.accepted && !expressions.cst().recovered() && expressions.diagnostics.empty(),
        "native case, default section, expression operators, binary/fract/address forms");
  const auto* program = expressions.value_as<ast::Program>();
  check(program && program->statements.size() == 10, "all native statements retained");
  if (program && program->statements.size() == 10) {
    const auto* section = cursiv::ast_cast<ast::SectionDecl>(program->statements[0]->atom);
    check(section && section->qualifier.empty() && section->name == "program", "unqualified section AST");
    const auto value = [&](std::size_t index) -> ast::ValueExpr* {
      const auto* declaration = cursiv::ast_cast<ast::Declaration>(program->statements[index]->atom);
      return declaration && declaration->values.size() == 1 ? declaration->values[0] : nullptr;
    };
    const auto* plus = cursiv::ast_cast<ast::BinaryExpr>(value(1));
    const auto* times = plus ? cursiv::ast_cast<ast::BinaryExpr>(plus->right) : nullptr;
    check(plus && plus->op == ast::Operator::add && times && times->op == ast::Operator::multiply,
          "multiplication binds above addition");
    const auto* xorExpr = cursiv::ast_cast<ast::BinaryExpr>(value(2));
    const auto* orExpr = xorExpr ? cursiv::ast_cast<ast::BinaryExpr>(xorExpr->right) : nullptr;
    const auto* andExpr = orExpr ? cursiv::ast_cast<ast::BinaryExpr>(orExpr->right) : nullptr;
    check(xorExpr && xorExpr->op == ast::Operator::bit_xor && orExpr && orExpr->op == ast::Operator::bit_or &&
              andExpr && andExpr->op == ast::Operator::bit_and, "documented bitwise group order");
    const auto* unaryPlus = cursiv::ast_cast<ast::BinaryExpr>(value(3));
    check(unaryPlus && cursiv::ast_cast<ast::BitNot>(unaryPlus->left), "ones complement AST");
    const auto* bits = cursiv::ast_cast<ast::NumberLit>(value(4));
    const auto* fraction = cursiv::ast_cast<ast::NumberLit>(value(5));
    const auto* address = cursiv::ast_cast<ast::AddrOf>(value(6));
    check(bits && bits->value == "B#1010" && fraction && fraction->value == "0.5r" &&
              address && address->name == "foo$bar", "native literal and symbol spellings retained");
    const auto* directive = cursiv::ast_cast<ast::Directive>(program->statements[7]->atom);
    const auto* shift = directive && directive->values.size() == 1
        ? cursiv::ast_cast<ast::BinaryExpr>(directive->values[0]) : nullptr;
    const auto* flow = cursiv::ast_cast<ast::FlowControl>(program->statements[8]->atom);
    const auto* compute = cursiv::ast_cast<ast::Compute>(program->statements[9]->atom);
    check(shift && shift->op == ast::Operator::shift_left && flow && flow->raw == "IF EQ JUMP Target" &&
              compute && compute->raw == "R0 = ABS R1", "directive tree and raw payload token boundaries");
  }
  // Table 1-11 reserves exact, case-insensitive words, not their prefixes.
  for (const std::string_view name : {
      "r1_name", "r1.local", "r1$foo", "R1_Name", "R1.Local", "R1$Foo", ".r1"}) {
    const std::string label = std::string(name) + "_label";
    const std::string source = ".VAR " + std::string(name) + " = 1;\n" +
        label + ": .VAR sink = " + std::string(name) + ";\n";
    const auto parsed = parser->parse(source);
    const auto* root = parsed.value_as<ast::Program>();
    const auto* first = root && root->statements.size() == 2 ? root->statements[0] : nullptr;
    const auto* second = root && root->statements.size() == 2 ? root->statements[1] : nullptr;
    const auto* declared = first ? cursiv::ast_cast<ast::Declaration>(first->atom) : nullptr;
    const auto* referred = second ? cursiv::ast_cast<ast::Declaration>(second->atom) : nullptr;
    const auto* value = declared && declared->values.size() == 1
        ? cursiv::ast_cast<ast::NumberLit>(declared->values[0]) : nullptr;
    const auto* reference = referred && referred->values.size() == 1
        ? cursiv::ast_cast<ast::NameRef>(referred->values[0]) : nullptr;
    const bool retained = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
        parsed.cst().text() == source && first && second &&
        first->parent == root && second->parent == root &&
        declared && declared->parent == first && declared->names.size() == 1 && declared->names[0] == name &&
        value && value->value == "1" && value->parent == declared &&
        second->labels.size() == 1 && second->labels[0] == label &&
        referred && referred->parent == second && referred->names.size() == 1 && referred->names[0] == "sink" &&
        reference && reference->name == name && reference->parent == referred;
    check(retained, "reserved-word prefixes retain symbol spelling and declaration/label/reference AST");
    if (!retained) std::fprintf(stderr, "sharc reserved boundary: source=[%s] accepted=%d recovered=%d diagnostics=%zu\n",
        source.c_str(), parsed.accepted, parsed.cst().recovered(), parsed.diagnostics.size());
  }
  for (const std::string_view word : {
      "r1", "R1", "mr0f", "MR1B", "MRF", "LCNTR", "MODE1", "FLAG0_IN",
      "abs", "ADDRESS", ".ALIGN", "__DATE__"}) {
    for (const std::string source : {
        ".VAR " + std::string(word) + " = 1;",
        std::string(word) + ": NOP;",
        ".VAR sink = " + std::string(word) + ";"}) {
      const auto invalid = parser->parse(source);
      const bool rejected = !invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty();
      check(rejected, "reserved word cannot declare a symbol, label or scalar symbol reference");
      if (!rejected) std::fprintf(stderr, "sharc accepted reserved symbol: [%s]\n", source.c_str());
    }
  }
  {
    const char* source = "r0 = -1 + r1;";
    const auto invalid = parser->parse(source);
    const auto* program = cursiv::ast_cast<ast::Program>(invalid.value);
    std::fprintf(stderr,
                 "sharc immediate-first negative: source=%s accepted=%d "
                 "recovered=%d diagnostics=%zu program=%d statements=%zu nodes=%zu\n",
                 source, invalid.accepted, invalid.cst().recovered(),
                 invalid.diagnostics.size(), program != nullptr,
                 program ? program->statements.size() : 0, invalid.nodes.size());
    for (const auto& diagnostic : invalid.diagnostics)
      std::fprintf(stderr,
                   "sharc immediate-first diagnostic: kind=%d begin=%zu end=%zu "
                   "element=%u message=%s\n",
                   static_cast<int>(diagnostic.kind),
                   static_cast<std::size_t>(diagnostic.begin),
                   static_cast<std::size_t>(diagnostic.end),
                   static_cast<unsigned>(diagnostic.element), diagnostic.message.c_str());
    if (program) for (const auto* statement : program->statements) {
      const auto* atom = statement ? statement->atom : nullptr;
      const char* kind = "other-or-missing";
      std::string_view raw;
      if (const auto* compute = cursiv::ast_cast<ast::Compute>(atom)) {
        kind = "Compute"; raw = compute->raw;
      } else if (const auto* move = cursiv::ast_cast<ast::ImmMov>(atom)) {
        kind = "ImmMov"; raw = move->raw;
      } else if (const auto* move = cursiv::ast_cast<ast::ComputeMovExpr>(atom)) {
        kind = "ComputeMovExpr"; raw = move->raw;
      } else if (const auto* conditional = cursiv::ast_cast<ast::IfComputeMov>(atom)) {
        kind = "IfComputeMov"; raw = conditional->raw;
      }
      std::fprintf(stderr, "sharc immediate-first AST: kind=%s raw=[%.*s]\n",
                   kind, static_cast<int>(raw.size()), raw.empty() ? "" : raw.data());
    }
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "the old immediate-first addition fixture is not a documented SHARC instruction");
  }
  return ok;
}
} // namespace audit_regressions
