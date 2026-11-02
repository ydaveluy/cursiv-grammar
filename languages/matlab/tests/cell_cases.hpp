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

namespace matlab_cells_cases {
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

template <class Rows>
inline std::string rows(const Rows& sourceRows) {
  std::string text;
  for (const auto* row : sourceRows) {
    if (!text.empty()) text += ';';
    text += '[';
    bool first = true;
    if (row) for (const auto* element : row->elements) {
      if (!first) text += ',';
      first = false;
      text += expression(element);
    }
    text += ']';
  }
  return text;
}

inline std::string expression(const a::Expr* node) {
  if (!node) return "<null>";
  if (const auto* n = cursiv::ast_cast<a::Ident>(node)) return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<a::NumberLit>(node)) {
    for (unsigned i = 0; i <= 4; ++i)
      if (n->value == i) return std::to_string(i);
    return std::to_string(n->value);
  }
  if (const auto* n = cursiv::ast_cast<a::StringLit>(node))
    return "str(" + std::string(n->value) + ")";
  if (const auto* n = cursiv::ast_cast<a::FunctionHandle>(node))
    return "handle(" + std::string(n->name) + ")";
  if (const auto* n = cursiv::ast_cast<a::CellLit>(node))
    return "cell(" + rows(n->rows) + ")";
  if (const auto* n = cursiv::ast_cast<a::ArrayLit>(node))
    return "array(" + rows(n->rows) + ")";
  if (const auto* n = cursiv::ast_cast<a::IndexExpr>(node))
    return "named(" + std::string(n->name) + ";" + arguments(n->args) + ")";
  if (const auto* n = cursiv::ast_cast<a::SubscriptExpr>(node))
    return std::string(n->braces ? "brace(" : "round(") +
           expression(n->object) + ";" + arguments(n->args) + ")";
  if (const auto* n = cursiv::ast_cast<a::MemberExpr>(node))
    return "member(" + expression(n->object) + ";" + std::string(n->name) + ")";
  if (const auto* n = cursiv::ast_cast<a::BinaryExpr>(node)) {
    const char* op = "?";
    switch (n->op) {
      case a::Operator::range: op = ":"; break;
      case a::Operator::add: op = "+"; break;
      case a::Operator::subtract: op = "-"; break;
      case a::Operator::multiply: op = "*"; break;
      default: break;
    }
    return "(" + std::string(op) + " " + expression(n->left) + " " +
           expression(n->right) + ")";
  }
  if (const auto* n = cursiv::ast_cast<a::ParenExpr>(node))
    return "group(" + expression(n->inner) + ")";
  return "<unexpected-expression>";
}

inline bool run(const cursiv::Parser& parser,
                const cursiv::AstReflection& reflection) {
  struct Case {
    const char* name;
    const char* source;
    bool accepted;
    const char* target;
    const char* value;
  };
  const Case cases[] = {
    {"empty-literal", "C = {};\n", true, "C", "cell()"},
    {"one-cell-literal", "C = {1};\n", true, "C", "cell([1])"},
    {"heterogeneous-row", "C = {1,'text',@sin};\n", true, "C", "cell([1,str(text),handle(sin)])"},
    {"two-rows", "C = {1,2;3,4};\n", true, "C", "cell([1,2];[3,4])"},
    {"nested-cell-and-array", "C = {{1;2},[3,4]};\n", true, "C", "cell([cell([1];[2]),array([3,4])])"},
    {"read-one-content", "x = C{1};\n", true, "x", "brace(C;1)"},
    {"read-multidim-content", "x = C{2,3};\n", true, "x", "brace(C;2,3)"},
    {"write-content", "C{1,3} = 'text';\n", true, "brace(C;1,3)", "str(text)"},
    {"replace-cell-subset", "C(1,1:2) = {'a','b'};\n", true, "named(C;1,(: 1 2))", "cell([str(a),str(b)])"},
    {"concatenate-content-range", "x = [C{2,1:2}];\n", true, "x", "array([brace(C;2,(: 1 2))])"},
    {"nested-curly-access", "x = C{1}{2};\n", true, "x", "brace(brace(C;1);2)"},
    {"array-inside-cell", "x = C{2,3}(2,:);\n", true, "x", "round(brace(C;2,3);2,:)"},
    {"member-inside-cell", "x = C{2}.f2;\n", true, "x", "member(brace(C;2);f2)"},
    {"function-inside-cell", "y = C{2}(pi);\n", true, "y", "round(brace(C;2);pi)"},
    {"literal-missing-close", "C = {1,2;\n", false, "", ""},
    {"literal-wrong-close", "C = {1,2];\n", false, "", ""},
    {"nested-literal-missing-close", "C = {{1};\n", false, "", ""},
    {"contents-missing-close", "x = C{1;\n", false, "", ""},
    {"contents-wrong-close", "x = C{1);\n", false, "", ""},
    {"nested-contents-wrong-close", "x = C{1}{2);\n", false, "", ""},
    {"round-suffix-wrong-close", "x = C{1}(2};\n", false, "", ""},
    {"member-name-missing", "x = C{1}.;\n", false, "", ""},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const auto* file = result.value_as<a::File>();
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && file;
    const auto check = [&](bool value, const char* message) {
      if (!value) {
        std::fprintf(stderr, "matlab cell %s: %s; source=%s\n",
                     test.name, message, test.source);
        ok = false;
      }
    };
    check(clean == test.accepted, "primary-derived syntax verdict");
    if (!clean) {
      if (test.accepted) for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "  kind=%u bytes=%u..%u message=%s\n",
                     unsigned(diagnostic.kind), diagnostic.begin,
                     diagnostic.end, diagnostic.message.c_str());
      continue;
    }
    check(result.cst().text() == test.source, "whole source retained");
    const auto* assignment = !file->statements.empty()
        ? cursiv::ast_cast<a::AssignStmt>(file->statements[0]) : nullptr;
    check(assignment && assignment->target && assignment->value,
          "assignment owns both source expressions");
    if (assignment && assignment->target && assignment->value) {
      const auto target = expression(assignment->target);
      const auto value = expression(assignment->value);
      if (target != test.target || value != test.value)
        std::fprintf(stderr, "  target expected=%s actual=%s; value expected=%s actual=%s\n",
                     test.target, target.c_str(), test.value, value.c_str());
      check(target == test.target && value == test.value,
            "ordered rows/arguments/operations and typed target/value");
    }
    for (std::size_t i = 1; i < file->statements.size(); ++i) {
      const auto* empty = cursiv::ast_cast<a::ExprStmt>(file->statements[i]);
      check(empty && !empty->value, "existing trailing empty statements only");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> pending = {result.value};
    while (!pending.empty()) {
      const auto* node = pending.back();
      pending.pop_back();
      if (!seen.insert(node).second) { check(false, "unique typed ownership"); continue; }
      check(node->cstNodeId < result.cst().size(), "every AST node has a CST span");
      if (node->cstNodeId < result.cst().size()) {
        const auto text = result.cst().text(node->cstNodeId);
        if (cursiv::ast_cast<a::CellLit>(node))
          check(text.starts_with("{") && text.ends_with("}"), "cell CST includes its delimiters");
        if (const auto* subscript = cursiv::ast_cast<a::SubscriptExpr>(node))
          check(subscript->object && subscript->object->cstNodeId < result.cst().size() &&
                result.cst()[node->cstNodeId].begin ==
                    result.cst()[subscript->object->cstNodeId].begin &&
                text.ends_with(subscript->braces ? "}" : ")"),
                "subscript CST includes its whole target and matching closer");
        if (const auto* member = cursiv::ast_cast<a::MemberExpr>(node))
          check(member->object && member->object->cstNodeId < result.cst().size() &&
                result.cst()[node->cstNodeId].begin ==
                    result.cst()[member->object->cstNodeId].begin &&
                text.ends_with("." + std::string(member->name)),
                "member CST includes its whole target and field name");
      }
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
  std::printf("matlab cells: %zu primary-derived controls (no vendor execution)\n",
              std::size(cases));
  return ok;
}
}  // namespace matlab_cells_cases
