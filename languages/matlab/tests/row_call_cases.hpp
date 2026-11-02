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
#include "row_sign_cases.hpp"

namespace matlab_row_call_cases {
namespace a = matlab::ast;

inline std::string expression(const a::Expr* node) {
  // The ordinary documentation pair has division at the root. All nested
  // expressions use the existing sign suite's typed projection unchanged.
  if (const auto* binary = cursiv::ast_cast<a::BinaryExpr>(node))
    if (binary->op == a::Operator::divide)
      return "(/ " + matlab_row_sign_cases::expression(binary->left) + " " +
             matlab_row_sign_cases::expression(binary->right) + ")";
  return matlab_row_sign_cases::expression(node);
}

inline bool run(const cursiv::Parser& parser,
                const cursiv::AstReflection& reflection) {
  // The documented spaced sin example is syntactically valid; its native
  // missing-input error happens at evaluation, which this parser does not run.
  struct Case {
    const char* name;
    const char* source;
    const char* target;
    const char* projection;
  };
  const Case cases[] = {
    {"official-spaced-row-call", "x = [1 sin (pi) 3];\n", "x", "array([1,sin,group(pi),3])"},
    {"official-adjacent-row-call", "x = [1 sin(pi) 3];\n", "x", "array([1,named(sin;pi),3])"},
    {"official-ordinary-spaced-call", "y = sin (3 * pi) / 2\n", "y", "(/ named(sin;(* 3 pi)) 2)"},
    {"official-ordinary-tight-call", "y=sin(3*pi)/2\n", "y", "(/ named(sin;(* 3 pi)) 2)"},
    {"group-restores-ordinary-call", "x = [1 (sin (pi)) 3];\n", "x", "array([1,group(named(sin;pi)),3])"},
    {"round-args-restore-ordinary-call", "x = [f(sin (pi))];\n", "x", "array([named(f;named(sin;pi))])"},
    {"curly-args-restore-ordinary-call", "x = [C{sin (pi)}];\n", "x", "array([brace(C;named(sin;pi))])"},
    {"round-suffix-args-restore-ordinary-call", "x = [C{1}(sin (pi))];\n", "x", "array([round(brace(C;1);named(sin;pi))])"},
    {"cell-spaced-named-call", "x = {sin (pi)};\n", "x", "cell([sin,group(pi)])"},
    {"cell-adjacent-named-call", "x = {sin(pi)};\n", "x", "cell([named(sin;pi)])"},
    {"spaced-named-index", "x = [a (1)];\n", "x", "array([a,group(1)])"},
    {"adjacent-named-index", "x = [a(1)];\n", "x", "array([named(a;1)])"},
    {"unary-spaced-named-call", "x = [-sin (pi)];\n", "x", "array([(- sin),group(pi)])"},
    {"power-base-spaced-named-call", "x = [sin (pi)^2];\n", "x", "array([sin,(^ group(pi) 2)])"},
    {"power-right-spaced-named-call", "x = [2^sin (pi)];\n", "x", "array([(^ 2 sin),group(pi)])"},
    {"signed-adjacent-call-power", "x = [1 -sin(pi)^2];\n", "x", "array([1,(- (^ named(sin;pi) 2))])"},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const auto* file = result.value_as<a::File>();
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && file;
    const auto check = [&](bool value, const char* message) {
      if (!value) {
        std::fprintf(stderr, "matlab row call %s: %s; source=%s",
                     test.name, message, test.source);
        ok = false;
      }
    };
    check(clean, "primary-derived positive syntax expectation");
    if (!clean) {
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "  kind=%u bytes=%u..%u message=%s\n",
                     unsigned(diagnostic.kind), diagnostic.begin,
                     diagnostic.end, diagnostic.message.c_str());
      continue;
    }
    check(result.cst().text() == test.source, "whole source retained");
    const auto* assignment = !file->statements.empty()
        ? cursiv::ast_cast<a::AssignStmt>(file->statements[0]) : nullptr;
    const auto* target = assignment
        ? cursiv::ast_cast<a::Ident>(assignment->target) : nullptr;
    check(target && target->name == test.target && assignment->value,
          "typed assignment target and value");
    if (assignment && assignment->value) {
      const auto actual = expression(assignment->value);
      if (actual != test.projection)
        std::fprintf(stderr, "  expected=%s actual=%s\n", test.projection, actual.c_str());
      check(actual == test.projection,
            "ordered source entries, named calls, group/args resets and unary/power paths");
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
  std::printf("matlab row calls: %zu primary-derived controls (no vendor execution)\n",
              std::size(cases));
  return ok;
}
}  // namespace matlab_row_call_cases
