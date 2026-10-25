#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "propcalc/core/CoreServices.hpp"
#include "propcalc/core/ast.hpp"

namespace audit_regressions {
inline bool run() {
  namespace ast = propcalc::ast;
  const auto language = cursiv::create_language<propcalc::PropcalcCoreServices,
      propcalc::PropcalcSharedCoreServices, propcalc::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "propcalc regression: %s\n", message); ok = false; }
  };
  {
    const auto parsed = language.language.parser->parse("p v q ^ r -> s <-> t |- p");
    const auto* root = cursiv::ast_cast<ast::Proposition>(parsed.value);
    const auto* iff = root ? cursiv::ast_cast<ast::Binary>(root->premises) : nullptr;
    const auto* implication = iff ? cursiv::ast_cast<ast::Binary>(iff->left) : nullptr;
    const auto* disjunction = implication ? cursiv::ast_cast<ast::Binary>(implication->left) : nullptr;
    const auto* conjunction = disjunction ? cursiv::ast_cast<ast::Binary>(disjunction->right) : nullptr;
    check(parsed.accepted && !parsed.cst().recovered() && iff &&
              iff->op == ast::Connective::equivalence && implication &&
              implication->op == ast::Connective::implication && disjunction &&
              disjunction->op == ast::Connective::disjunction && conjunction &&
              conjunction->op == ast::Connective::conjunction,
          "conjunction, disjunction, implication and equivalence precedence");
  }
  {
    const auto parsed = language.language.parser->parse("p -> q -> r |- p");
    const auto* root = cursiv::ast_cast<ast::Proposition>(parsed.value);
    const auto* outer = root ? cursiv::ast_cast<ast::Binary>(root->premises) : nullptr;
    const auto* inner = outer ? cursiv::ast_cast<ast::Binary>(outer->left) : nullptr;
    check(outer && inner && outer->op == ast::Connective::implication &&
              inner->op == ast::Connective::implication,
          "implication chains retain the selected left-associative dialect");
  }
  for (const std::string text : {"|-", "! |- p", "p ^ |- q", "p -> |- q", "() |- p"}) {
    const auto result = language.language.parser->parse(text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "missing operands remain parseable");
    const auto validation = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == "propcalc.operand") found = true;
    check(found, "missing operand has a runtime diagnostic");
  }
  {
    const std::string text = "p q |- p";
    const auto parsed = language.language.parser->parse(text);
    const auto* root = cursiv::ast_cast<ast::Proposition>(parsed.value);
    const auto* variable = root ? cursiv::ast_cast<ast::VariableRef>(root->premises) : nullptr;
    check(parsed.accepted && !parsed.cst().recovered() && variable &&
              variable->name == "p" && variable->extraNames.size() == 1 &&
              variable->extraNames[0] == "q", "adjacent identifiers stay distinct");
    const auto validation = cursiv::test::validationHelper(language.language, text);
    check(validation.diagnostics.size() == 1 && validation.diagnostics[0].code &&
              validation.diagnostics[0].code->isString() &&
              validation.diagnostics[0].code->string() == "propcalc.operator" &&
              validation.diagnostics[0].range.begin == 2 &&
              validation.diagnostics[0].range.end == 3,
          "missing connective points to the next identifier");
  }
  return ok;
}
}  // namespace audit_regressions
