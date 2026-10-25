#pragma once

#include <string>
#include <vector>

#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"

inline void dice_regressions(const dice::DiceCoreServices &services) {
  namespace ast = dice::ast;
  const std::string source = "+0004294967296d00018446744073709551616/-0000";
  const auto result = services.parser->parse(source);
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *root = cursiv::ast_cast<ast::BinaryNotation>(result.value);
  CHECK(root && root->op == ast::Operator::divide && !root->parent);
  const auto *roll =
      root ? cursiv::ast_cast<ast::DiceRoll>(root->left) : nullptr;
  const auto *zero =
      root ? cursiv::ast_cast<ast::NumberLiteral>(root->right) : nullptr;
  CHECK(roll && roll->count && *roll->count == "0004294967296" &&
        roll->sides == "00018446744073709551616" && !roll->negative &&
        roll->parent == root);
  CHECK(zero && zero->negative && zero->value == "0000" &&
        zero->parent == root);
  CHECK(result.nodes.size() == 3);
  if (roll) {
    const auto span = result.cst()[roll->cstNodeId];
    CHECK(span.begin == 0 && span.end == source.find('/'));
  }
  {
    const auto parsed = services.parser->parse("((d6))");
    const auto *die = cursiv::ast_cast<ast::DiceRoll>(parsed.value);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(die && !die->count && !die->negative && die->sides == "6" &&
          !die->parent);
    CHECK(parsed.nodes.size() == 1);
  }
  {
    const auto parsed = services.parser->parse("1+-2*3-4");
    const auto *subtract = cursiv::ast_cast<ast::BinaryNotation>(parsed.value);
    const auto *add =
        subtract ? cursiv::ast_cast<ast::BinaryNotation>(subtract->left)
                 : nullptr;
    const auto *multiply =
        add ? cursiv::ast_cast<ast::BinaryNotation>(add->right) : nullptr;
    const auto *negative =
        multiply ? cursiv::ast_cast<ast::NumberLiteral>(multiply->left)
                 : nullptr;
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(subtract && subtract->op == ast::Operator::subtract && add &&
          add->op == ast::Operator::add && multiply &&
          multiply->op == ast::Operator::multiply);
    CHECK(negative && negative->negative && negative->value == "2");
  }
  cursiv::Document document(
      cursiv::TextDocument::create("memory:dice", "dice", 0, "1d6/0000"));
  document.language = &services;
  document.parseResult = services.parser->parse("1d6/0000");
  std::vector<cursiv::Diagnostic> diagnostics;
  services.documentValidator->validateDocument(document, diagnostics);
  CHECK(document.parseResult.accepted &&
        !document.parseResult.cst().recovered() &&
        document.parseResult.diagnostics.empty());
  CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
        *diagnostics[0].code == "dice.zero-divisor");
  if (diagnostics.size() == 1)
    CHECK(diagnostics[0].range.begin == 4 && diagnostics[0].range.end == 8);
}
