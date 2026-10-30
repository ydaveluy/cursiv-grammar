#pragma once

#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "muparser/core/CoreServices.hpp"

inline void muparser_regressions() {
  namespace ast = muparser::ast;
  const auto language =
      cursiv::create_language<muparser::MuparserCoreServices,
                              muparser::MuparserSharedCoreServices,
                              muparser::install_core_module>();
  const auto& services = language.language;
  {
    const std::string source = "+2^-3";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Program>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    const auto* plus = root && root->exprs.size() == 1
                           ? cursiv::ast_cast<ast::UnaryExpr>(root->exprs[0])
                           : nullptr;
    const auto* power =
        plus ? cursiv::ast_cast<ast::BinaryExpr>(plus->operand) : nullptr;
    const auto* minus =
        power ? cursiv::ast_cast<ast::UnaryExpr>(power->right) : nullptr;
    const auto* number =
        minus ? cursiv::ast_cast<ast::NumberLiteral>(minus->operand) : nullptr;
    CHECK(plus && plus->op == ast::Operator::add && power &&
          power->op == ast::Operator::pow && power->parent == plus && minus &&
          minus->op == ast::Operator::sub && minus->parent == power && number &&
          number->value == 3 && number->parent == minus);
    if (minus) {
      const auto span = result.cst()[minus->cstNodeId];
      CHECK(span.begin == 3 && span.end == 5 &&
            source.substr(span.begin, span.end - span.begin) == "-3");
    }
    CHECK(result.nodes.size() == 6);
  }
  {
    const auto result = services.parser->parse("1==2<3");
    const auto* root = cursiv::ast_cast<ast::Program>(result.value);
    const auto* outer = root && root->exprs.size() == 1
                            ? cursiv::ast_cast<ast::BinaryExpr>(root->exprs[0])
                            : nullptr;
    const auto* inner =
        outer ? cursiv::ast_cast<ast::BinaryExpr>(outer->left) : nullptr;
    CHECK(result.accepted && !result.cst().recovered() && outer && inner &&
          outer->op == ast::Operator::lt && inner->op == ast::Operator::eq);
  }
  {
    const auto result = services.parser->parse("1e3,atan2(1,2),custom0()");
    const auto* root = cursiv::ast_cast<ast::Program>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->exprs.size() == 3);
    if (root && root->exprs.size() == 3) {
      const auto* number = cursiv::ast_cast<ast::NumberLiteral>(root->exprs[0]);
      const auto* call = cursiv::ast_cast<ast::FunctionCall>(root->exprs[1]);
      const auto* empty = cursiv::ast_cast<ast::FunctionCall>(root->exprs[2]);
      CHECK(number && number->value == 1000 && number->parent == root);
      CHECK(call && call->name == "atan2" && call->args.size() == 2 &&
            call->parent == root);
      CHECK(empty && empty->name == "custom0" && empty->args.empty());
    }
  }
  {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:rnd", "muparser", 0, "rnd()"));
    document.language = &services;
    document.parseResult = services.parser->parse("rnd()");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() && diagnostics.empty());
  }
  for (const auto source : {"sin(1,2)", "atan2(1)", "min()", "rnd(1)", "1+x=3"}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:muparser", "muparser", 0, source));
    document.language = &services;
    document.parseResult = services.parser->parse(source);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
          *diagnostics[0].code == (source == std::string_view("1+x=3")
                                       ? "muparser.target"
                                       : "muparser.arity"));
    if (source == std::string_view("1+x=3") && diagnostics.size() == 1)
      CHECK(diagnostics[0].range.begin == 0 && diagnostics[0].range.end == 3);
  }
}
