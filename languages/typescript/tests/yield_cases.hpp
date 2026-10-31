#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"

namespace typescript_yield_cases {
namespace a = typescript::ast;
template <class T> const T* as(const cursiv::AstNode* node) { return cursiv::ast_cast<T>(node); }
inline bool namedYield(const a::Expr* expression) {
  const auto* identifier = as<a::Ident>(expression);
  return identifier && identifier->name == "yield";
}
inline std::string expression(const a::Expr* node) {
  if (!node) return "-";
  if (const auto* n = as<a::Ident>(node)) return std::string(n->name);
  if (const auto* n = as<a::NumberLit>(node)) return std::string(n->text);
  if (const auto* n = as<a::StringLit>(node)) return std::string(n->text);
  if (const auto* n = as<a::UnaryExpr>(node)) return "(" + std::string(n->op) + " " + expression(n->operand) + ")";
  if (const auto* n = as<a::BinaryExpr>(node)) return "(" + std::string(n->op) + " " + expression(n->left) + " " + expression(n->right) + ")";
  if (const auto* n = as<a::AssignExpr>(node)) return "(" + std::string(n->op) + " " + expression(n->target) + " " + expression(n->value) + ")";
  if (const auto* n = as<a::ConditionalExpr>(node)) return "(? " + expression(n->test) + " " + expression(n->consequent) + " " + expression(n->alternate) + ")";
  if (const auto* n = as<a::YieldExpr>(node)) return std::string(n->delegate ? "(yield* " : "(yield ") + expression(n->argument) + ")";
  if (const auto* n = as<a::AwaitExpr>(node)) return "(await " + expression(n->operand) + ")";
  return "?";
}

inline bool run(const typescript::TypeScriptCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* name, const char* message) {
    if (!value) { std::fprintf(stderr, "typescript yield %s: %s\n", name, message); ok = false; }
  };
  struct Argument { const char* value; bool delegate; };
  const Argument arguments[] = {
    {"1", false},
    {"items", true},
    {"1", false},
    {"(+ 1)", false},
    {"(? a b c)", false},
    {"(yield 1)", false},
    {"1", false},
    {"2", false},
    {"2", false},
    {"1", false},
    {"items", true},
    {"(await x)", false},
    {"-", false},
    {"-", false},
    {"1", false},
    {"(in a b)", false},
    {"1", false},
    {"1", false},
    {"items", true},
    {"1", false},
    {"1", false},
    {"-", false},
    {"1", false},
    {"1", false},
    {"2", false},
    {"1", false},
    {"1", false},
    {"2", false},
    {"2", false},
    {"1", false},
    {"2", false},
    {"3", false},
    {"2", false},
    {"(in b c)", false},
    {"\"a\"", false},
  };
  struct Case { const char* name; const char* source; bool parses; std::size_t yields, calls, binaries, tags, start, arguments; };
  // Fresh official5.9.3 parser/AST observations02+03, not checker acceptance.
  const Case cases[] = {
    {"generator-initializer", "function* g(){ const x=yield 1; }", true, 1, 0, 0, 0, 0, 1},
    {"generator-delegate", "function* g(){ const x=yield* items; }", true, 1, 0, 0, 0, 1, 1},
    {"generator-call-shaped-yield", "function* g(){ return yield(1); }", true, 1, 0, 0, 0, 2, 1},
    {"ordinary-yield-call", "function f(){ return yield(1); }", true, 0, 1, 0, 0, 3, 0},
    {"ordinary-yield-multiply", "function f(){ const x=yield*items; }", true, 0, 0, 1, 0, 3, 0},
    {"ordinary-yield-add", "function f(){ const x=yield+1; }", true, 0, 0, 1, 0, 3, 0},
    {"generator-unary-operand", "function* g(){ const x=yield+1; }", true, 1, 0, 0, 0, 3, 1},
    {"generator-conditional-operand", "function* g(){ const x=yield a?b:c; }", true, 1, 0, 0, 0, 4, 1},
    {"nested-yields", "function* g(){ const x=yield (yield 1); }", true, 2, 0, 0, 0, 5, 2},
    {"nested-ordinary-function", "function* g(){ function f(){return yield(1);} yield 2; }", true, 1, 1, 0, 0, 7, 1},
    {"nested-arrow-reset", "function* g(){ const f=()=>yield(1); yield 2; }", true, 1, 1, 0, 0, 8, 1},
    {"object-generator", "const o={*g(){ const x=yield(1); }};", true, 1, 0, 0, 0, 9, 1},
    {"class-generator", "class C{ *g(){ return yield* items; } }", true, 1, 0, 0, 0, 10, 1},
    {"async-generator", "async function* g(){ return yield await x; }", true, 1, 0, 0, 0, 11, 1},
    {"generator-bare", "function* g(){ let x=yield; }", true, 1, 0, 0, 0, 12, 1},
    {"generator-linebreak", "function* g(){ let x=yield\n1; }", true, 1, 0, 0, 0, 13, 1},
    {"ordinary-bare-name", "function f(){ let x=yield; }", true, 0, 0, 0, 0, 14, 0},
    {"ordinary-unambiguous-yield", "function f(){ let x=yield 1; }", true, 1, 0, 0, 0, 14, 1},
    {"ordinary-interpolated-template", "function f(){ const x=yield `a${b}`; }", true, 0, 0, 0, 1, 15, 0},
    {"ordinary-plain-template", "function f(){ const x=yield `a`; }", true, 0, 0, 0, 1, 15, 0},
    {"noin-parentheses-reset", "function* g(){ for(let x=yield(a in b);;){} }", true, 1, 0, 0, 0, 15, 1},
    {"yield-in-binary-operand-invalid", "function* g(){ const x=1 + yield 2; }", false, 0, 0, 0, 0, 16, 0},
    {"delegate-linebreak-invalid", "function* g(){ yield\n*items; }", false, 0, 0, 0, 0, 16, 0},
    {"noin-unparenthesized", "function* g(){ for(let x=yield a in b;;){} }", false, 0, 0, 0, 0, 16, 0},
    {"assignment-rhs", "function* g(){x=yield 1;}", true, 1, 0, 0, 0, 16, 1},
    {"conditional-arms", "function* g(){const x=a?yield 1:yield*items;}", true, 2, 0, 0, 0, 17, 2},
    {"conditional-alternate-assignment", "function* g(){x=a?b:c=yield 1;}", true, 1, 0, 0, 0, 19, 1},
    {"assignment-chain", "function* g(){x=y=yield 1;}", true, 1, 0, 0, 0, 20, 1},
    {"generator-comment-linebreak", "function* g(){let x=yield/*\n*/1;}", true, 1, 0, 0, 0, 21, 1},
    {"ordinary-comment-linebreak", "function f(){yield/*\n*/1;}", true, 0, 0, 0, 0, 22, 0},
    {"ordinary-comment-same-line", "function f(){yield/*x*/1;}", true, 1, 0, 0, 0, 22, 1},
    {"object-computed-generator-key", "function* g(){const o={*[yield 1](){yield 2;}};}", true, 2, 0, 0, 0, 23, 2},
    {"object-computed-ordinary-method", "function* g(){const o={[yield 1](){yield(2);}};}", true, 1, 1, 0, 0, 25, 1},
    {"class-computed-method", "function* g(){class C{[yield 1](){yield(2);}}}", true, 1, 1, 0, 0, 26, 1},
    {"class-field-reset", "function* g(){class C{x=yield(1);} yield 2;}", true, 1, 1, 0, 0, 27, 1},
    {"class-static-reset", "function* g(){class C{static{yield(1);}} yield 2;}", true, 1, 1, 0, 0, 28, 1},
    {"class-accessor-reset", "function* g(){class C{get x(){return yield(1);}set x(v){yield(2);}}}", true, 0, 2, 0, 0, 29, 0},
    {"generator-param-default", "function* g(x=yield 1){}", true, 1, 0, 0, 0, 29, 1},
    {"ordinary-param-default", "function* g(){function f(x=yield(1)){}yield 2;}", true, 1, 1, 0, 0, 30, 1},
    {"arrow-param-default-reset", "function* g(){const f=(x=yield(1))=>yield(2);yield 3;}", true, 1, 2, 0, 0, 31, 1},
    {"type-computed-reset", "function* g(){let x:{[yield(1)]:number};yield 2;}", true, 1, 1, 0, 0, 32, 1},
    {"noin-yield-conditional-consequent", "function* g(){for(using x=a?yield (b in c):d;;){} }", true, 1, 0, 0, 0, 33, 1},
    {"noin-yield-conditional-alternate", "function* g(){for(using x=a?b:yield c in d;;){} }", false, 0, 0, 0, 0, 34, 0},
    {"ordinary-keyword-lookahead-invalid", "function f(){const x=yield in obj;}", false, 0, 0, 0, 0, 34, 0},
    {"ordinary-string-lookahead", "function f(){const x=yield \"a\";}", true, 1, 0, 0, 0, 34, 1},
    {"ordinary-regexp-not-lookahead", "function f(){const x=yield / 2 / 1;}", true, 0, 0, 1, 0, 35, 0},
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.parses, test.name, "native syntax verdict");
    if (clean != test.parses) for (const auto& d : parsed.diagnostics)
      std::fprintf(stderr, "  kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), d.element, d.begin, d.end);
    if (!clean) continue;
    check(parsed.cst().text() == test.source, test.name, "complete source preserved");
    std::vector<const a::YieldExpr*> yields;
    std::size_t calls = 0, binaries = 0, tags = 0;
    for (const auto* node : parsed.nodes) {
      if (const auto* n = as<a::YieldExpr>(node)) yields.push_back(n);
      if (const auto* n = as<a::CallExpr>(node)) calls += namedYield(n->callee);
      if (const auto* n = as<a::BinaryExpr>(node)) binaries += namedYield(n->left);
      if (const auto* n = as<a::TemplateLit>(node)) tags += namedYield(n->tag);
    }
    check(yields.size() == test.yields && calls == test.calls && binaries == test.binaries && tags == test.tags,
          test.name, "Yield versus ordinary call/binary/template AST classification");
    // Allocation order need not be source order. The outer yield starts first.
    std::ranges::sort(yields, [&](const auto* left, const auto* right) {
      return parsed.cst()[left->cstNodeId].begin < parsed.cst()[right->cstNodeId].begin;
    });
    if (yields.size() == test.arguments) for (std::size_t i = 0; i < yields.size(); ++i) {
      const auto* node = yields[i];
      const auto& wanted = arguments[test.start + i];
      check(expression(node->argument) == wanted.value && node->delegate == wanted.delegate,
            test.name, "precise yield operand tree/value and delegation flag");
      if (node->argument) check(node->argument->parent == node, test.name, "yield operand has its direct owner");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique AST owner"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() && child.cstNodeId < parsed.cst().size() &&
              parsed.cst().contains(node->cstNodeId, child.cstNodeId), test.name, "every typed child owner and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "all allocated nodes reachable exactly once");
    const std::string_view name(test.name);
    const char* expected = name == "assignment-rhs" ? "(= x (yield 1))" :
        name == "assignment-chain" ? "(= x (= y (yield 1)))" :
        name == "conditional-alternate-assignment" ? "(= x (? a b (= c (yield 1))))" : nullptr;
    if (expected) {
      bool found = false;
      for (const auto* node : parsed.nodes) if (const auto* assignment = as<a::AssignExpr>(node))
        found |= expression(assignment) == expected;
      check(found, test.name, "right-associative assignment retains yield at the native operand level");
    }
    if (name == "conditional-arms") {
      bool found = false;
      for (const auto* node : parsed.nodes) if (const auto* conditional = as<a::ConditionalExpr>(node))
        found |= expression(conditional) == "(? a (yield 1) (yield* items))";
      check(found, test.name, "both conditional branches retain their distinct yield expressions");
    }
  }
  std::printf("typescript yield: %zu native syntax/AST cases\n", std::size(cases));
  return ok;
}
}  // namespace typescript_yield_cases
