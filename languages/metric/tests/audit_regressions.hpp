#pragma once

#include <string>
#include <vector>

#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "metric/core/ast.hpp"

inline void metric_regressions(const metric::MetricCoreServices &services) {
  namespace ast = metric::ast;
  {
    const auto parse = services.parser->parse("MΩ·s");
    const auto* product = parse.value_as<ast::BinaryUom>();
    const auto* ohm = product ? cursiv::ast_cast<ast::Measure>(product->left) : nullptr;
    CHECK(parse.accepted && !parse.cst().recovered() && parse.diagnostics.empty());
    CHECK(product && product->op == ast::Operator::multiply);
    CHECK(ohm && ohm->prefix == ast::Prefix::M && ohm->unit == ast::Unit::ohm);
  }
  const std::string source = "(μm/um)^-9223372036854775809/1";
  const auto result = services.parser->parse(source);
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *root = cursiv::ast_cast<ast::BinaryUom>(result.value);
  CHECK(root && root->op == ast::Operator::divide && !root->parent);
  const auto *power =
      root ? cursiv::ast_cast<ast::PowerUom>(root->left) : nullptr;
  const auto *one =
      root ? cursiv::ast_cast<ast::Measure>(root->right) : nullptr;
  CHECK(power && power->parent == root &&
        power->exponent == "-9223372036854775809");
  CHECK(one && one->unit == ast::Unit::dimensionless && !one->prefix &&
        one->parent == root);
  const auto *quotient =
      power ? cursiv::ast_cast<ast::BinaryUom>(power->base) : nullptr;
  CHECK(quotient && quotient->parent == power &&
        quotient->op == ast::Operator::divide);
  if (quotient) {
    const auto *left = cursiv::ast_cast<ast::Measure>(quotient->left);
    const auto *right = cursiv::ast_cast<ast::Measure>(quotient->right);
    CHECK(left && right && left->prefix == ast::Prefix::micro &&
          right->prefix == ast::Prefix::micro);
    CHECK(left && right && left->unit == ast::Unit::m &&
          right->unit == ast::Unit::m && left->parent == quotient &&
          right->parent == quotient);
  }
  CHECK(result.nodes.size() == 6);
  if (power) {
    const auto span = result.cst()[power->cstNodeId];
    CHECK(span.begin == 0 && span.end == source.find("/1"));
  }
  {
    const auto parse = services.parser->parse("kg*m/s^+0002");
    const auto *divide = cursiv::ast_cast<ast::BinaryUom>(parse.value);
    const auto *multiply =
        divide ? cursiv::ast_cast<ast::BinaryUom>(divide->left) : nullptr;
    const auto *exponent =
        divide ? cursiv::ast_cast<ast::PowerUom>(divide->right) : nullptr;
    CHECK(parse.accepted && !parse.cst().recovered() &&
          parse.diagnostics.empty());
    CHECK(divide && divide->op == ast::Operator::divide && multiply &&
          multiply->op == ast::Operator::multiply);
    CHECK(exponent && exponent->exponent == "+0002");
  }
  cursiv::Document document(
      cursiv::TextDocument::create("memory:metric", "metric", 0, "k1"));
  document.language = &services;
  document.parseResult = services.parser->parse("k1");
  std::vector<cursiv::Diagnostic> diagnostics;
  services.documentValidator->validateDocument(document, diagnostics);
  CHECK(document.parseResult.accepted &&
        !document.parseResult.cst().recovered() &&
        document.parseResult.diagnostics.empty());
  CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
        *diagnostics[0].code == "metric.prefix");
  if (diagnostics.size() == 1)
    CHECK(diagnostics[0].range.begin == 0 && diagnostics[0].range.end == 1);
}
