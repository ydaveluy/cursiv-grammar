#pragma once

#include <string>

#include "cursiv/test/Check.hpp"

inline void ltl_regressions(ltl::LtlCoreServices &services) {
  namespace ast = ltl::ast;
  const std::string source = "a1 || b_2 && c3 -> d_4 <-> e5";
  const auto result = services.parser->parse(source);
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *equivalence = cursiv::ast_cast<ast::BinaryExpr>(result.value);
  const auto *implication =
      equivalence ? cursiv::ast_cast<ast::BinaryExpr>(equivalence->left)
                  : nullptr;
  const auto *conjunction =
      implication ? cursiv::ast_cast<ast::BinaryExpr>(implication->left)
                  : nullptr;
  const auto *disjunction =
      conjunction ? cursiv::ast_cast<ast::BinaryExpr>(conjunction->left)
                  : nullptr;
  CHECK(equivalence && equivalence->op == ast::BinaryOp::equivalent &&
        !equivalence->parent);
  CHECK(implication && implication->op == ast::BinaryOp::implies &&
        implication->parent == equivalence);
  CHECK(conjunction && conjunction->op == ast::BinaryOp::land &&
        conjunction->parent == implication);
  CHECK(disjunction && disjunction->op == ast::BinaryOp::lor &&
        disjunction->parent == conjunction);
  if (disjunction) {
    const auto *a = cursiv::ast_cast<ast::AtomicProp>(disjunction->left);
    const auto *b = cursiv::ast_cast<ast::AtomicProp>(disjunction->right);
    CHECK(a && a->name == "a1" && a->parent == disjunction && b &&
          b->name == "b_2" && b->parent == disjunction);
  }
  if (equivalence && implication && conjunction) {
    const auto *c = cursiv::ast_cast<ast::AtomicProp>(conjunction->right);
    const auto *d = cursiv::ast_cast<ast::AtomicProp>(implication->right);
    const auto *e = cursiv::ast_cast<ast::AtomicProp>(equivalence->right);
    CHECK(c && c->name == "c3" && c->parent == conjunction && d &&
          d->name == "d_4" && d->parent == implication && e &&
          e->name == "e5" && e->parent == equivalence);
    const auto span = result.cst()[equivalence->cstNodeId];
    CHECK(span.begin == 0 && span.end == source.size());
  }
  CHECK(result.nodes.size() == 9);
  {
    const auto parsed = services.parser->parse("aUb");
    const auto *atom = cursiv::ast_cast<ast::AtomicProp>(parsed.value);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(atom && atom->name == "aUb" && parsed.nodes.size() == 1);
  }
  {
    const auto parsed = services.parser->parse("X(¬p U <>q)");
    const auto *next = cursiv::ast_cast<ast::UnaryExpr>(parsed.value);
    const auto *until =
        next ? cursiv::ast_cast<ast::BinaryExpr>(next->operand) : nullptr;
    const auto *notP =
        until ? cursiv::ast_cast<ast::UnaryExpr>(until->left) : nullptr;
    const auto *eventually =
        until ? cursiv::ast_cast<ast::UnaryExpr>(until->right) : nullptr;
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(next && next->op == ast::UnaryOp::next && until &&
          until->op == ast::BinaryOp::until && until->parent == next);
    CHECK(notP && notP->op == ast::UnaryOp::negate && notP->parent == until &&
          eventually && eventually->op == ast::UnaryOp::finally &&
          eventually->parent == until);
  }
  {
    const auto parsed = services.parser->parse("a && a || true");
    const auto *top = cursiv::ast_cast<ast::BinaryExpr>(parsed.value);
    const auto *both =
        top ? cursiv::ast_cast<ast::BinaryExpr>(top->left) : nullptr;
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(top && top->op == ast::BinaryOp::lor && both &&
          both->op == ast::BinaryOp::land && both->left != both->right &&
          parsed.nodes.size() == 5);
  }
}
