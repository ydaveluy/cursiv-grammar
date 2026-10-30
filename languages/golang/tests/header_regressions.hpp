#pragma once

#include <algorithm>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "golang/core/ast.hpp"

namespace go_header_regressions {

inline void diagnostics(std::string_view name, std::string_view source,
                        const cursiv::ParseResult& result) {
  std::fprintf(stderr, "golang probe %.*s: accepted=%d recovered=%d diagnostics=%zu\n",
               static_cast<int>(name.size()), name.data(), result.accepted,
               result.cst().recovered(), result.diagnostics.size());
  for (const auto& diagnostic : result.diagnostics) {
    const auto begin = std::min(static_cast<std::size_t>(diagnostic.begin), source.size());
    const auto start = begin > 48 ? begin - 48 : 0;
    const auto context = source.substr(start, std::min<std::size_t>(96, source.size() - start));
    std::string escaped;
    for (const char c : context) {
      if (c == '\n') escaped += "\\n";
      else if (c == '\r') escaped += "\\r";
      else if (c == '\t') escaped += "\\t";
      else escaped += c;
    }
    std::fprintf(stderr,
                 "golang diagnostic: kind=%d begin=%zu end=%zu element=%zu "
                 "contextStart=%zu context=%s\n",
                 static_cast<int>(diagnostic.kind),
                 static_cast<std::size_t>(diagnostic.begin),
                 static_cast<std::size_t>(diagnostic.end),
                 static_cast<std::size_t>(diagnostic.element), start,
                 escaped.c_str());
  }
}

inline bool run(const cursiv::Parser& parser) {
  namespace ast = golang::ast;
  enum class Kind { name, parenthesized, comparison, range, switch_, select, label };
  struct Case { const char* name; const char* source; Kind kind; };
  bool passed = true;
  for (const Case test : {
           Case{"if-name", "package p\nfunc f(){if x {g()}}\n", Kind::name},
           Case{"if-name-explicit-semis", "package p;func f(){if x {g();};};", Kind::name},
           Case{"if-paren", "package p\nfunc f(){if (x) {g()}}\n", Kind::parenthesized},
           Case{"if-comparison", "package p\nfunc f(){if x != nil {g()}}\n", Kind::comparison},
           Case{"range-name", "package p\nfunc f(){for range x {g()}}\n", Kind::range},
           Case{"switch-name", "package p\nfunc f(){switch x {default:g()}}\n", Kind::switch_},
           Case{"select-receive", "package p\nfunc f(){select{case <-ch:}}\n", Kind::select},
           Case{"empty-label", "package p\nfunc f(){L:}\n", Kind::label}}) {
    const auto result = parser.parse(test.source);
    const auto* file = cursiv::ast_cast<ast::SourceFile>(result.value);
    const auto* function = file && file->decls.size() == 1
        ? cursiv::ast_cast<ast::FuncDecl>(file->decls[0]) : nullptr;
    const auto* statement = function && function->body && function->body->stmts.size() == 1
        ? function->body->stmts[0] : nullptr;
    const auto* branch = cursiv::ast_cast<ast::IfStmt>(statement);
    bool shape = false;
    switch (test.kind) {
      case Kind::name:
        shape = branch && cursiv::ast_cast<ast::Ident>(branch->cond); break;
      case Kind::parenthesized:
        shape = branch && cursiv::ast_cast<ast::ParenExpr>(branch->cond); break;
      case Kind::comparison: {
        const auto* binary = branch ? cursiv::ast_cast<ast::BinaryExpr>(branch->cond) : nullptr;
        const auto* left = binary ? cursiv::ast_cast<ast::Ident>(binary->left) : nullptr;
        const auto* right = binary ? cursiv::ast_cast<ast::Ident>(binary->right) : nullptr;
        shape = binary && binary->op == "!=" && left && left->name == "x" &&
                right && right->name == "nil"; break;
      }
      case Kind::range: {
        const auto* loop = cursiv::ast_cast<ast::RangeStmt>(statement);
        shape = loop && cursiv::ast_cast<ast::Ident>(loop->range); break;
      }
      case Kind::switch_: {
        const auto* sw = cursiv::ast_cast<ast::ExprSwitchStmt>(statement);
        shape = sw && cursiv::ast_cast<ast::Ident>(sw->tag); break;
      }
      case Kind::select: shape = cursiv::ast_cast<ast::SelectStmt>(statement); break;
      case Kind::label: {
        const auto* label = cursiv::ast_cast<ast::LabeledStmt>(statement);
        shape = label && label->label == "L" && !label->stmt; break;
      }
    }
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    if (!clean || !shape) {
      diagnostics(test.name, test.source, result);
      std::fprintf(stderr, "golang header AST: shape=%d if=%d compositeCondition=%d\n",
                   shape, branch != nullptr,
                   branch && cursiv::ast_cast<ast::CompositeLit>(branch->cond));
      passed = false;
    }
  }

  // Go1.24 parser.go permits named composites again inside delimiters and
  // function bodies. Restricting a header must not restrict those scopes.
  struct ContextCase {
    const char* source;
    unsigned named;
    unsigned anonymous;
  };
  for (const ContextCase test : {
           ContextCase{"package p\nfunc f(){if (T{}).ok {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){if g(T{}) {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){if a[T{}] {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){if map[T]int{T{}:1} != nil {}}\n", 1, 1},
           ContextCase{"package p\nfunc f(){if []T{{}} != nil {}}\n", 0, 1},
           ContextCase{"package p\nfunc f(){if g(func() bool {return T{}.ok}) {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){if x := (T{}); x.ok {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){for range (T{}) {}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){switch (T{}) {default:}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){switch g(T{}).(type) {default:}}\n", 1, 0},
           ContextCase{"package p\nfunc f(){for x:=g(T{}); x; x=g(T{}) {}}\n", 2, 0},
           ContextCase{"package p\nvar x = T{}\n", 1, 0},
           ContextCase{"package p\nfunc f(){if g(G[int]{}) {}}\n", 1, 0}}) {
    const auto result = parser.parse(test.source);
    unsigned named = 0, anonymous = 0;
    bool ownership = true;
    for (const auto* node : result.nodes) {
      const auto* composite = cursiv::ast_cast<ast::CompositeLit>(node);
      if (!composite) continue;
      ownership &= composite->parent && composite->type && composite->value &&
                   composite->type->parent == composite &&
                   composite->value->parent == composite;
      if (cursiv::ast_cast<ast::NamedType>(composite->type)) ++named;
      else ++anonymous;
    }
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    if (!clean || !ownership || named != test.named || anonymous != test.anonymous) {
      diagnostics("nested composite scope", test.source, result);
      std::fprintf(stderr,
                   "golang composite AST: named=%u expected=%u anonymous=%u "
                   "expected=%u ownership=%d source=%s\n",
                   named, test.named, anonymous, test.anonymous, ownership,
                   test.source);
      passed = false;
    }
  }
  // The corresponding unprotected named-composite forms are not valid
  // headers. These are syntax checks, independent of symbol/type legality.
  for (const char* source : {
           "package p\nfunc f(){if x == T{} {}}\n",
           "package p\nfunc f(){if x := T{}; x.ok {}}\n",
           "package p\nfunc f(){for range T{} {}}\n",
           "package p\nfunc f(){for x:=T{}; ; {}}\n",
           "package p\nfunc f(){for ; ; x=T{} {}}\n",
           "package p\nfunc f(){switch T{} {}}\n",
           "package p\nfunc f(){switch T{}.(type) {}}\n",
           "package p\nfunc f(){if G[int]{} {}}\n"}) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "golang unprotected header composite accepted: %s\n", source);
      passed = false;
    }
  }
  return passed;
}

}  // namespace go_header_regressions
