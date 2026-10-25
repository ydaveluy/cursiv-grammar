#pragma once

#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "golang/core/ast.hpp"

namespace audit_regressions {

template <class T>
const T* as(const cursiv::AstNode* node) {
  return node && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

inline bool run(const cursiv::Parser& parser) {
  using namespace golang;
  bool ok = true;
  const auto check = [&](bool condition, const char* message) {
    if (!condition) {
      std::fprintf(stderr, "golang audit: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [&](const cursiv::ParseResult& result) {
    const bool good = result.accepted && !result.cst().recovered() &&
                      result.diagnostics.empty() && result.value;
    check(good, "expected an accepted AST without recovery or diagnostics");
    return good;
  };
  const auto body = [](const cursiv::ParseResult& result) {
    const auto* file = as<ast::SourceFile>(result.value);
    const auto* fn = file && file->decls.size() == 1
                         ? as<ast::FuncDecl>(file->decls[0]) : nullptr;
    return fn ? fn->body : nullptr;
  };
  const auto call_named = [](const ast::Expr* expr, const char* name) {
    const auto* call = as<ast::CallExpr>(expr);
    const auto* id = call ? as<ast::Ident>(call->callee) : nullptr;
    return call && call->args.empty() && id && id->name == name &&
           id->parent == call;
  };

  // scanner.Scanner inserts ';' after ')', including an LF in a comment.
  // A lone CR is whitespace, and a same-line suffix remains an actual call.
  struct Gap { const char* text; bool separates; };
  for (const Gap gap : {Gap{"\n", true}, Gap{"\r\n", true},
                        Gap{"/*a\nb*/", true}, Gap{"//a\n", true},
                        Gap{" ", false}, Gap{"\r", false},
                        Gap{"/*a*/", false}}) {
    const std::string text = std::string("package p\nfunc f(){ h()") +
                             gap.text + "(g()) }\n";
    const auto r = parser.parse(text);
    if (!clean(r)) continue;
    const auto* b = body(r);
    check(b && b->stmts.size() == (gap.separates ? 2u : 1u),
          "a newline separates calls instead of adding a call suffix");
    if (!b || b->stmts.size() != (gap.separates ? 2u : 1u)) continue;
    const auto* first = as<ast::ExprStmt>(b->stmts[0]);
    if (gap.separates) {
      const auto* second = as<ast::ExprStmt>(b->stmts[1]);
      const auto* paren = second ? as<ast::ParenExpr>(second->value) : nullptr;
      check(first && call_named(first->value, "h") && paren &&
                call_named(paren->inner, "g") &&
                first->parent == b && second->parent == b &&
                paren->parent == second && paren->inner->parent == paren,
            "both separate statements keep their complete call ASTs");
    } else {
      const auto* outer = first ? as<ast::CallExpr>(first->value) : nullptr;
      check(outer && call_named(outer->callee, "h") && outer->args.size() == 1 &&
                call_named(outer->args[0], "g") && outer->callee->parent == outer &&
                outer->args[0]->parent == outer,
            "same-line calls preserve callee, argument and ownership");
    }
  }

  for (const char* op : {"+", "-", "*", "&", "^", "<-"}) {
    const std::string text = std::string("package p\nfunc f(){ h()\n") +
                             op + "g() }\n";
    const auto r = parser.parse(text);
    if (!clean(r)) continue;
    const auto* b = body(r);
    const auto* first = b && b->stmts.size() == 2
                            ? as<ast::ExprStmt>(b->stmts[0]) : nullptr;
    const auto* second = b && b->stmts.size() == 2
                             ? as<ast::ExprStmt>(b->stmts[1]) : nullptr;
    const auto* unary = second ? as<ast::UnaryExpr>(second->value) : nullptr;
    check(first && call_named(first->value, "h") && unary && unary->op == op &&
              call_named(unary->operand, "g") && unary->parent == second &&
              unary->operand->parent == unary,
          "a leading unary/receive operator starts the next statement");
  }

  {
    const auto r = parser.parse(
        "package p\nfunc f(){ x := a +\nb*c-d; y := pkg.\nF(); ch <- x }\n");
    if (clean(r)) {
      const auto* b = body(r);
      const auto* x = b && b->stmts.size() == 3
                          ? as<ast::ShortVarDecl>(b->stmts[0]) : nullptr;
      const auto* minus = x && x->values.size() == 1
                              ? as<ast::BinaryExpr>(x->values[0]) : nullptr;
      const auto* plus = minus ? as<ast::BinaryExpr>(minus->left) : nullptr;
      const auto* times = plus ? as<ast::BinaryExpr>(plus->right) : nullptr;
      const auto* y = b && b->stmts.size() == 3
                          ? as<ast::ShortVarDecl>(b->stmts[1]) : nullptr;
      const auto* call = y && y->values.size() == 1
                             ? as<ast::CallExpr>(y->values[0]) : nullptr;
      const auto* selector = call ? as<ast::SelectorExpr>(call->callee) : nullptr;
      check(minus && minus->op == "-" && plus && plus->op == "+" &&
                times && times->op == "*" && times->parent == plus &&
                plus->parent == minus && selector && selector->field == "F" &&
                as<ast::SendStmt>(b->stmts[2]),
            "a newline after an operator/dot continues the expression and keeps precedence");
    }
  }

  {
    const auto r = parser.parse(
        "package p\nfunc f(){ if x := g()\nx > 0 {}\n"
        "for i := 0\ni < 10\ni++ {}\nswitch x := g()\nx { case 1: } }\n");
    if (clean(r)) {
      const auto* b = body(r);
      const auto* branch = b && b->stmts.size() == 3
                               ? as<ast::IfStmt>(b->stmts[0]) : nullptr;
      const auto* loop = b && b->stmts.size() == 3
                             ? as<ast::ForStmt>(b->stmts[1]) : nullptr;
      const auto* sw = b && b->stmts.size() == 3
                           ? as<ast::ExprSwitchStmt>(b->stmts[2]) : nullptr;
      const auto* cond = branch ? as<ast::BinaryExpr>(branch->cond) : nullptr;
      const auto* post = loop ? as<ast::IncDecStmt>(loop->post) : nullptr;
      const auto* tag = sw ? as<ast::Ident>(sw->tag) : nullptr;
      check(branch && as<ast::ShortVarDecl>(branch->init) && cond && cond->op == ">" &&
                branch->init->parent == branch && branch->cond->parent == branch &&
                loop && as<ast::ShortVarDecl>(loop->init) &&
                as<ast::BinaryExpr>(loop->cond) && post && post->op == "++" &&
                loop->init->parent == loop && loop->cond->parent == loop &&
                post->parent == loop && sw && as<ast::ShortVarDecl>(sw->init) &&
                tag && tag->name == "x" && sw->cases.size() == 1,
            "inserted header separators retain init, condition, post and switch tag");
    }
  }

  for (const char* spelling : {"089i", "0123i", "0_8_9i", "0x_FFi", "0b101i"}) {
    const std::string text = std::string("package p\nvar x = ") + spelling + "\n";
    const auto r = parser.parse(text);
    if (!clean(r)) continue;
    const auto* f = as<ast::SourceFile>(r.value);
    const auto* d = f && f->decls.size() == 1 ? as<ast::VarDecl>(f->decls[0]) : nullptr;
    const auto* spec = d && d->specs.size() == 1 ? d->specs[0] : nullptr;
    const auto* lit = spec && spec->values.size() == 1
                          ? as<ast::ImaginaryLit>(spec->values[0]) : nullptr;
    check(lit && lit->text == spelling && lit->parent == spec,
          "imaginary coefficients preserve the complete source spelling");
  }

  {
    const auto r = parser.parse(
        "package p\nfunc f(){ g[[]int, map[string]int,](nil); g[int,](nil); g([]int) }\n");
    if (clean(r)) {
      const auto* b = body(r);
      const auto* stmt = b && b->stmts.size() == 3 ? as<ast::ExprStmt>(b->stmts[0]) : nullptr;
      const auto* call = stmt ? as<ast::CallExpr>(stmt->value) : nullptr;
      const auto* index = call ? as<ast::IndexExpr>(call->callee) : nullptr;
      const auto* first = index && index->indices.size() == 2
                              ? as<ast::TypeExpr>(index->indices[0]) : nullptr;
      const auto* second = index && index->indices.size() == 2
                               ? as<ast::TypeExpr>(index->indices[1]) : nullptr;
      const auto* slice = first ? as<ast::SliceType>(first->type) : nullptr;
      const auto* map = second ? as<ast::MapType>(second->type) : nullptr;
      check(index && slice && map && first->parent == index && second->parent == index &&
                slice->parent == first && map->parent == second &&
                as<ast::NamedType>(slice->elem) && as<ast::NamedType>(map->key) &&
                as<ast::NamedType>(map->elem),
            "generic type arguments retain ordered type trees and ownership");
      const auto* plainStmt = b && b->stmts.size() == 3
                                  ? as<ast::ExprStmt>(b->stmts[1]) : nullptr;
      const auto* plainCall = plainStmt ? as<ast::CallExpr>(plainStmt->value) : nullptr;
      const auto* plainIndex = plainCall ? as<ast::IndexExpr>(plainCall->callee) : nullptr;
      check(plainIndex && plainIndex->indices.size() == 1 &&
                as<ast::Ident>(plainIndex->indices[0]),
            "named generic arguments keep the existing expression reading");
      const auto* typeStmt = b && b->stmts.size() == 3
                                 ? as<ast::ExprStmt>(b->stmts[2]) : nullptr;
      const auto* typeCall = typeStmt ? as<ast::CallExpr>(typeStmt->value) : nullptr;
      const auto* typeArg = typeCall && typeCall->args.size() == 1
                               ? as<ast::TypeExpr>(typeCall->args[0]) : nullptr;
      check(typeArg && as<ast::SliceType>(typeArg->type) && typeArg->parent == typeCall,
            "a call may carry a type without losing its AST");
    }
  }

  {
    const auto r = parser.parse("package p\nfunc f(){ ;;h();;label:;g() }\n");
    if (clean(r)) {
      const auto* b = body(r);
      const auto* label = b && b->stmts.size() == 6
                              ? as<ast::LabeledStmt>(b->stmts[4]) : nullptr;
      check(b && b->stmts.size() == 6 && as<ast::EmptyStmt>(b->stmts[0]) &&
                as<ast::EmptyStmt>(b->stmts[1]) && as<ast::ExprStmt>(b->stmts[2]) &&
                as<ast::EmptyStmt>(b->stmts[3]) && label && label->label == "label" &&
                as<ast::EmptyStmt>(label->stmt) && label->stmt->parent == label &&
                as<ast::ExprStmt>(b->stmts[5]),
            "explicit empty statements retain their count, order and labels");
    }
    const auto c = parser.parse("package p\nfunc f(){ switch { default: ;; } }\n");
    if (clean(c)) {
      const auto* b = body(c);
      const auto* sw = b && b->stmts.size() == 1
                           ? as<ast::ExprSwitchStmt>(b->stmts[0]) : nullptr;
      check(sw && sw->cases.size() == 1 && sw->cases[0]->body.size() == 2 &&
                as<ast::EmptyStmt>(sw->cases[0]->body[0]) &&
                as<ast::EmptyStmt>(sw->cases[0]->body[1]),
            "case bodies keep explicit empty statements too");
    }
  }

  {
    const auto r = parser.parse("package p\nfunc f(){ h() // comment\r(g())\n}\n");
    if (clean(r)) {
      const auto* b = body(r);
      const auto* stmt = b && b->stmts.size() == 1
                             ? as<ast::ExprStmt>(b->stmts[0]) : nullptr;
      check(stmt && call_named(stmt->value, "h"), "a CR does not end a line comment");
    }
    for (const char* spelling : {"\"a\rb\"", "'\r'"}) {
      const std::string text = std::string("package p\nvar x = ") + spelling + "\n";
      const auto value = parser.parse(text);
      if (!clean(value)) continue;
      const auto* f = as<ast::SourceFile>(value.value);
      const auto* d = f && f->decls.size() == 1 ? as<ast::VarDecl>(f->decls[0]) : nullptr;
      const auto* spec = d && d->specs.size() == 1 ? d->specs[0] : nullptr;
      const auto* expr = spec && spec->values.size() == 1 ? spec->values[0] : nullptr;
      const auto* str = as<ast::StringLit>(expr);
      const auto* rune = as<ast::RuneLit>(expr);
      check((str && str->text == spelling) || (rune && rune->text == spelling),
            "CR inside string and rune literals retains its exact spelling");
    }
  }
  return ok;
}

}  // namespace audit_regressions
