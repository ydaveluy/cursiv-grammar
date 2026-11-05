#pragma once

#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/parser/Parser.hpp"
#include "cursiv/syntax-tree/AstReflection.hpp"
#include "cursiv/syntax-tree/Cst.hpp"
#include "matlab/ast.hpp"

namespace matlab_handles_cases {
namespace a = matlab::ast;

inline std::string expression(const a::Expr* node) {
  if (!node) return "<null>";
  if (const auto* n = cursiv::ast_cast<a::Ident>(node))
    return std::string(n->name);
  if (const auto* n = cursiv::ast_cast<a::NumberLit>(node)) {
    if (n->value == 0) return "0";
    if (n->value == 1) return "1";
    if (n->value == 2) return "2";
    return std::to_string(n->value);
  }
  if (const auto* n = cursiv::ast_cast<a::FunctionHandle>(node))
    return "handle(" + std::string(n->name) + ")";
  if (const auto* n = cursiv::ast_cast<a::AnonymousFunction>(node)) {
    std::string text = "lambda(";
    for (const auto parameter : n->parameters) {
      if (text != "lambda(") text += ',';
      text += parameter;
    }
    return text + ":" + expression(n->body) + ")";
  }
  if (const auto* n = cursiv::ast_cast<a::ParenExpr>(node))
    return "group(" + expression(n->inner) + ")";
  if (const auto* n = cursiv::ast_cast<a::BinaryExpr>(node)) {
    const char* op = "?";
    switch (n->op) {
      case a::Operator::add: op = "+"; break;
      case a::Operator::subtract: op = "-"; break;
      case a::Operator::multiply: op = "*"; break;
      case a::Operator::ewise_power: op = ".^"; break;
      default: break;
    }
    return "(" + std::string(op) + " " + expression(n->left) + " " +
           expression(n->right) + ")";
  }
  if (const auto* n = cursiv::ast_cast<a::IndexExpr>(node)) {
    std::string text = "apply(" + std::string(n->name) + ":";
    bool first = true;
    for (const auto* argument : n->args) {
      if (!first) text += ',';
      first = false;
      text += argument ? (argument->all ? ":" : expression(argument->value)) : "<null>";
    }
    return text + ")";
  }
  if (const auto* n = cursiv::ast_cast<a::ArrayLit>(node)) {
    std::string text = "array(";
    for (const auto* row : n->rows) {
      if (text != "array(") text += ';';
      text += '[';
      bool first = true;
      if (row) for (const auto* element : row->elements) {
        if (!first) text += ',';
        first = false;
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
    bool accepted;
    const char* projection;
  };
  const Case cases[] = {
    {"named-builtin", "f = @sin;\n", true, "handle(sin)"},
    {"named-qualified", "f = @pkg.fun;\n", true, "handle(pkg.fun)"},
    {"anonymous-empty", "f = @() 1;\n", true, "lambda(:1)"},
    {"anonymous-identity", "f = @(x) x;\n", true, "lambda(x:x)"},
    {"anonymous-additive-body", "f = @(x) x + 1;\n", true, "lambda(x:(+ x 1))"},
    {"anonymous-two-parameters", "f = @(x,y) x - y;\n", true, "lambda(x,y:(- x y))"},
    {"anonymous-elementwise-power", "f = @(x) x.^2;\n", true, "lambda(x:(.^ x 2))"},
    {"anonymous-capture", "f = @(x) a*x.^2 + b*x + c;\n", true, "lambda(x:(+ (+ (* a (.^ x 2)) (* b x)) c))"},
    {"named-call-argument", "q = integral(@sin,0,1);\n", true, "apply(integral:handle(sin),0,1)"},
    {"anonymous-call-argument", "q = integral(@(x) x.^2,0,1);\n", true, "apply(integral:lambda(x:(.^ x 2)),0,1)"},
    {"nested-anonymous-argument", "g = @(c) (integral(@(x) (x.^2 + c*x + 1),0,1));\n", true, "lambda(c:group(apply(integral:lambda(x:group((+ (+ (.^ x 2) (* c x)) 1))),0,1)))"},
    {"anonymous-array-body", "f = @(x) [x,x+1];\n", true, "lambda(x:array([x,(+ x 1)]))"},
    {"named-parenthesized", "f = (@sin);\n", true, "group(handle(sin))"},
    {"anonymous-continuation", "f = @(x) ...\n x+1;\n", true, "lambda(x:(+ x 1))"},
    {"missing-handle-target", "f = @;\n", false, ""},
    {"empty-body-zero-parameters", "f = @();\n", false, ""},
    {"empty-body-one-parameter", "f = @(x);\n", false, ""},
    {"parameter-is-number", "f = @(1) 1;\n", false, ""},
    {"parameter-is-expression", "f = @(x+y) x;\n", false, ""},
    {"missing-middle-parameter", "f = @(x,,y) x;\n", false, ""},
    {"missing-parameter-close", "f = @(x x+1;\n", false, ""},
    {"incomplete-qualified-target", "f = @pkg.;\n", false, ""},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const auto* file = result.value_as<a::File>();
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && file;
    const auto check = [&](bool value, const char* message) {
      if (!value) {
        std::fprintf(stderr, "matlab handle %s: %s; source=%s\n",
                     test.name, message, test.source);
        ok = false;
      }
    };
    check(clean == test.accepted, "primary-derived syntax verdict");
    if (!clean) {
      if (test.accepted) {
        for (const auto& diagnostic : result.diagnostics)
          std::fprintf(stderr, "  kind=%u bytes=%u..%u message=%s\n",
                       unsigned(diagnostic.kind), diagnostic.begin,
                       diagnostic.end, diagnostic.message.c_str());
      }
      continue;
    }
    check(result.cst().text() == test.source, "whole source retained");
    const auto* assignment = !file->statements.empty()
        ? cursiv::ast_cast<a::AssignStmt>(file->statements[0]) : nullptr;
    check(assignment && assignment->value, "assignment owns handle expression");
    if (assignment && assignment->value) {
      const auto actual = expression(assignment->value);
      if (actual != test.projection)
        std::fprintf(stderr, "  expected=%s actual=%s\n", test.projection, actual.c_str());
      check(actual == test.projection, "typed name/parameter/body projection and precedence");
    }
    // A newline after the explicit semicolon remains the existing empty
    // statement. Do not silently change the port's statement model.
    for (std::size_t i = 1; i < file->statements.size(); ++i) {
      const auto* empty = cursiv::ast_cast<a::ExprStmt>(file->statements[i]);
      check(empty && !empty->value, "only existing trailing empty statements");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> pending = {result.value};
    while (!pending.empty()) {
      const auto* node = pending.back();
      pending.pop_back();
      if (!seen.insert(node).second) { check(false, "unique typed ownership"); continue; }
      check(node->cstNodeId < result.cst().size(), "every AST node has a CST span");
      if (node->cstNodeId < result.cst().size()) {
        if (const auto* handle = cursiv::ast_cast<a::FunctionHandle>(node))
          check(result.cst().text(node->cstNodeId) == "@" + std::string(handle->name),
                "named handle CST includes @ and the whole qualified target");
        if (const auto* function = cursiv::ast_cast<a::AnonymousFunction>(node))
          check(result.cst().text(node->cstNodeId).starts_with("@") &&
                function->body && function->body->cstNodeId < result.cst().size() &&
                result.cst()[node->cstNodeId].end ==
                    result.cst()[function->body->cstNodeId].end,
                "anonymous CST includes its opener and complete body");
      }
      reflection.visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < result.cst().size() &&
              child.cstNodeId < result.cst().size() &&
              result.cst().contains(node->cstNodeId, child.cstNodeId),
              "complete parent/CST containment");
        pending.push_back(&child);
      });
    }
    check(seen.size() == result.nodes().size() && !result.value->parent,
          "all arena allocations reached once; detached root");
  }
  std::printf("matlab handles: %zu primary-derived controls (no vendor execution)\n",
              std::size(cases));
  return ok;
}
}  // namespace matlab_handles_cases
