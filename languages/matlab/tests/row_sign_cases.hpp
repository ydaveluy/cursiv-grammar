#pragma once

#include <cstdio>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "matlab/core/ast.hpp"

namespace matlab_row_sign_cases {
namespace a = matlab::ast;
inline std::string expression(const a::Expr* node);

template <class Args>
inline std::string arguments(const Args& args) {
  std::string text;
  for (const auto* arg : args) {
    if (!text.empty()) text += ',';
    text += arg ? (arg->all ? ":" : expression(arg->value)) : "<null>";
  }
  return text;
}

inline std::string expression(const a::Expr* node) {
  if (!node) return "<null>";
  if (const auto* n = cursiv::ast_cast<a::Ident>(node)) return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<a::NumberLit>(node)) {
    if (!n->imaginary) for (unsigned i = 0; i <= 4; ++i)
      if (n->value == i) return std::to_string(i);
    return "<unexpected-number>";
  }
  if (const auto* n = cursiv::ast_cast<a::UnaryExpr>(node))
    return "(" + std::string(n->op) + " " + expression(n->operand) + ")";
  if (const auto* n = cursiv::ast_cast<a::BinaryExpr>(node)) {
    const char* op = "?";
    switch (n->op) {
      case a::Operator::add: op = "+"; break;
      case a::Operator::subtract: op = "-"; break;
      case a::Operator::multiply: op = "*"; break;
      case a::Operator::power: op = "^"; break;
      default: break;
    }
    return "(" + std::string(op) + " " + expression(n->left) + " " +
           expression(n->right) + ")";
  }
  if (const auto* n = cursiv::ast_cast<a::ParenExpr>(node))
    return "group(" + expression(n->inner) + ")";
  if (const auto* n = cursiv::ast_cast<a::IndexExpr>(node))
    return "named(" + std::string(n->name) + ";" + arguments(n->args) + ")";
  if (const auto* n = cursiv::ast_cast<a::SubscriptExpr>(node))
    return std::string(n->braces ? "brace(" : "round(") +
           expression(n->object) + ";" + arguments(n->args) + ")";
  const auto* array = cursiv::ast_cast<a::ArrayLit>(node);
  const auto* cells = cursiv::ast_cast<a::CellLit>(node);
  if (array || cells) {
    std::string text = array ? "array(" : "cell(";
    const auto& rows = array ? array->rows : cells->rows;
    bool firstRow = true;
    for (const auto* row : rows) {
      if (!firstRow) text += ';';
      firstRow = false;
      text += '[';
      if (!row) return "<null-row>";
      bool firstElement = true;
      for (const auto* element : row->elements) {
        if (!firstElement) text += ',';
        firstElement = false;
        text += expression(element);
      }
      text += ']';
    }
    return text + ")";
  }
  return "<unexpected-expression>";
}

inline bool run(const cursiv::Parser& parser,
                const cursiv::AstReflection& reflection) {
  struct Case {
    const char* name;
    const char* source;
    const char* projection;
  };
  const Case cases[] = {
    {"minus-tight", "x = [1-2];\n", "array([(- 1 2)])"},
    {"minus-left-tight", "x = [1- 2];\n", "array([(- 1 2)])"},
    {"minus-both-spaced", "x = [1 - 2];\n", "array([(- 1 2)])"},
    {"minus-new-column", "x = [1 -2];\n", "array([1,(- 2)])"},
    {"plus-new-column", "x = [1 +2];\n", "array([1,(+ 2)])"},
    {"plus-both-spaced", "x = [1 + 2];\n", "array([(+ 1 2)])"},
    {"plus-left-tight", "x = [1+ 2];\n", "array([(+ 1 2)])"},
    {"parenthesis-reset", "x = [(1 -2)];\n", "array([group((- 1 2))])"},
    {"round-arguments-reset", "x = [f(1 -2)];\n", "array([named(f;(- 1 2))])"},
    {"curly-arguments-reset", "x = [C{1 -2}];\n", "array([brace(C;(- 1 2))])"},
    {"suffix-round-arguments-reset", "x = [C{1}(1 -2)];\n", "array([round(brace(C;1);(- 1 2))])"},
    {"cell-minus-column", "x = {1 -2};\n", "cell([1,(- 2)])"},
    {"cell-minus-binary", "x = {1 - 2};\n", "cell([(- 1 2)])"},
    {"nested-array-cell-context", "x = {[1 -2],(1 -2)};\n", "cell([array([1,(- 2)]),group((- 1 2))])"},
    {"signed-power-column", "x = [1 -2^2];\n", "array([1,(- (^ 2 2))])"},
    {"multiply-prefix", "x = [1*-2];\n", "array([(* 1 (- 2))])"},
    {"ordinary-expression-unchanged", "x = 1 -2;\n", "(- 1 2)"},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const auto* file = result.value_as<a::File>();
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && file;
    const auto check = [&](bool value, const char* message) {
      if (!value) {
        std::fprintf(stderr, "matlab row sign %s: %s; source=%s",
                     test.name, message, test.source);
        ok = false;
      }
    };
    check(clean, "clean syntax for primary-derived boundary inference");
    if (!clean) continue;
    check(result.cst().text() == test.source, "whole source retained");
    const auto* assignment = !file->statements.empty()
        ? cursiv::ast_cast<a::AssignStmt>(file->statements[0]) : nullptr;
    const auto* target = assignment
        ? cursiv::ast_cast<a::Ident>(assignment->target) : nullptr;
    check(target && target->name == "x" && assignment->value,
          "typed assignment target and value");
    if (assignment && assignment->value) {
      const auto actual = expression(assignment->value);
      if (actual != test.projection)
        std::fprintf(stderr, "  expected=%s actual=%s\n", test.projection, actual.c_str());
      check(actual == test.projection,
            "ordered source rows, sign/operator types and ordinary-context resets");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> pending = {result.value};
    while (!pending.empty()) {
      const auto* node = pending.back();
      pending.pop_back();
      if (!seen.insert(node).second) { check(false, "unique typed ownership"); continue; }
      check(node->cstNodeId < result.cst().size(), "every AST node has a CST span");
      reflection.visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < result.cst().size() &&
              child.cstNodeId < result.cst().size() &&
              result.cst().contains(node->cstNodeId, child.cstNodeId),
              "complete parent/CST containment");
        pending.push_back(&child);
      });
    }
    check(seen.size() == result.nodes.size() && !result.value->parent,
          "all arena allocations reached once; detached root");
  }
  std::printf("matlab row signs: %zu boundary inferences (no vendor execution)\n",
              std::size(cases));
  return ok;
}
}  // namespace matlab_row_sign_cases
