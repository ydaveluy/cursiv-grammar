#pragma once
#include "b/core/ast.hpp"
#include <ostream>
#include <string>
namespace native_regressions {
namespace ast = b::ast;
struct Projection {
  std::ostream &out;
  explicit Projection(std::ostream &s) : out(s) {}
  void q(std::string_view);
  std::uint64_t number(std::string_view);
  void expr(const ast::Expr *);
  void callArgs(const ast::Expr *, bool &);
};
inline void Projection::q(std::string_view s) {
  out << '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\')
      out << '\\' << char(c);
    else if (c == '\t')
      out << "\\t";
    else if (c == '\n')
      out << "\\n";
    else if (c == '\r')
      out << "\\r";
    else if (c == '\b')
      out << "\\b";
    else if (c == '\f')
      out << "\\f";
    else if (c < 32) {
      const char *h = "0123456789abcdef";
      out << "\\u00" << h[c >> 4] << h[c & 15];
    } else
      out << char(c);
  }
  out << '"';
}
inline std::uint64_t Projection::number(std::string_view s) {
  std::uint64_t n = 0;
  unsigned base = s.starts_with('0') ? 8 : 10;
  for (char c : s)
    n = n * base + c - '0';
  return n;
}

inline void Projection::callArgs(const ast::Expr *p, bool &first) {
  if (auto *n = cursiv::ast_cast<ast::BinaryExpr>(p); n && n->op == ",") {
    callArgs(n->left, first);
    callArgs(n->right, first);
    return;
  }
  if (!first)
    out << ',';
  first = false;
  expr(p);
}
inline void Projection::expr(const ast::Expr *p) {
  if (!p) {
    out << "null";
    return;
  }
  if (auto *n = cursiv::ast_cast<ast::Ident>(p)) {
    out << "[\"name\",";
    q(n->name.substr(0, 8));
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::IntLit>(p)) {
    out << "[\"int\",";
    q(std::to_string(number(n->value)));
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::CharLit>(p)) {
    std::uint64_t v = 0;
    for (std::size_t i = 0; i < n->text.size() && i < 8; ++i)
      v |= std::uint64_t(static_cast<unsigned char>(n->text[i])) << (8 * i);
    out << "[\"int\",";
    q(std::to_string(v));
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::StringLit>(p)) {
    out << "[\"string\",";
    q(n->text);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::PrefixExpr>(p)) {
    if (n->op == "*")
      if (auto *a = cursiv::ast_cast<ast::AddrExpr>(n->operand)) {
        expr(a->target);
        return;
      }
    out << '[';
    q(std::string(n->op) + "u");
    out << ',';
    expr(n->operand);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::PostfixExpr>(p)) {
    out << '[';
    q("u" + std::string(n->op));
    out << ',';
    expr(n->operand);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::AddrExpr>(p)) {
    if (auto *a = cursiv::ast_cast<ast::PrefixExpr>(n->target);
        a && a->op == "*") {
      expr(a->operand);
      return;
    }
    if (auto *a = cursiv::ast_cast<ast::IndexExpr>(n->target)) {
      out << "[\"+\",";
      expr(a->array);
      out << ',';
      expr(a->index);
      out << ']';
      return;
    }
    out << "[\"&u\",";
    expr(n->target);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::IndexExpr>(p)) {
    out << "[\"*u\",[\"+\",";
    expr(n->array);
    out << ',';
    expr(n->index);
    out << "]]";
  } else if (auto *n = cursiv::ast_cast<ast::BinaryExpr>(p)) {
    out << '[';
    q(n->op);
    out << ',';
    expr(n->left);
    out << ',';
    expr(n->right);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::AssignExpr>(p)) {
    out << '[';
    q("=" + std::string(n->op));
    out << ',';
    expr(n->target);
    out << ',';
    expr(n->value);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::CondExpr>(p)) {
    out << "[\"?\",";
    expr(n->cond);
    out << ",[\":\",";
    expr(n->thenValue);
    out << ',';
    expr(n->elseValue);
    out << "]]";
  } else if (auto *n = cursiv::ast_cast<ast::CallExpr>(p)) {
    out << "[\"call\",";
    expr(n->callee);
    out << ",[";
    bool f = true;
    for (auto *a : n->args)
      callArgs(a, f);
    out << "]]";
  } else
    out << "[\"unknown\"]";
}
} // namespace native_regressions
