#pragma once
#include "stringtemplate/core/ast.hpp"
#include <ostream>
#include <string>
namespace native_regressions {
namespace ast = stringtemplate::ast;
struct Projection {
  std::ostream &out;
  explicit Projection(std::ostream &s) : out(s) {}
  void q(std::string_view);
  void args(const ast::ArgList *);
  void expr(const ast::Expr *);
  void elements(const auto &);
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
inline void Projection::args(const ast::ArgList *p) {
  out << '[';
  if (p) {
    bool f = true;
    for (auto *a : p->items) {
      if (!f)
        out << ',';
      f = false;
      out << '[';
      q(a->ellipsis ? std::string_view("...") : a->name);
      out << ',';
      expr(a->value);
      out << ']';
    }
  }
  out << ']';
}
inline void Projection::expr(const ast::Expr *p) {
  if (!p) {
    out << "null";
    return;
  }
  if (auto *n = cursiv::ast_cast<ast::MapExpr>(p)) {
    if (n->stages.empty() && n->sources.size() == 1) {
      expr(n->sources[0]);
      return;
    }
    out << "[\"mapping\",[";
    bool f = true;
    for (auto *s : n->sources) {
      if (!f)
        out << ',';
      f = false;
      expr(s);
    }
    out << "],[";
    f = true;
    for (auto *st : n->stages) {
      if (!f)
        out << ',';
      f = false;
      out << '[';
      bool f2 = true;
      for (auto *t : st->targets) {
        if (!f2)
          out << ',';
        f2 = false;
        expr(t);
      }
      out << ']';
    }
    out << "]]";
  } else if (auto *n = cursiv::ast_cast<ast::NameExpr>(p)) {
    out << "[\"name\",";
    q(n->name);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::StringExpr>(p)) {
    out << "[\"string\",";
    q(n->value);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::BoolExpr>(p)) {
    out << "[\"bool\"," << (n->value ? "true" : "false") << ']';
  } else if (auto *n = cursiv::ast_cast<ast::CallExpr>(p)) {
    out << "[\"call\",";
    q(n->name);
    out << ',';
    args(n->args);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::SuperCallExpr>(p)) {
    out << "[\"super\",";
    q(n->name);
    out << ',';
    args(n->args);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::RegionRefExpr>(p)) {
    out << "[\"regionRef\"," << (n->isSuper ? "true" : "false") << ',';
    q(n->name);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::DynamicCallExpr>(p)) {
    out << "[\"dynamic\",";
    expr(n->target);
    out << ',';
    args(n->args);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::ParenExpr>(p)) {
    out << "[\"paren\",";
    expr(n->inner);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::FieldAccess>(p)) {
    out << "[\"field\",";
    expr(n->object);
    out << ',';
    q(n->field);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::IndexAccess>(p)) {
    out << "[\"index\",";
    expr(n->object);
    out << ',';
    expr(n->key);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::ListExpr>(p)) {
    out << "[\"list\",[";
    bool f = true;
    for (auto *x : n->items) {
      if (!f)
        out << ',';
      f = false;
      expr(x);
    }
    out << "]]";
  } else if (auto *n = cursiv::ast_cast<ast::SubTemplateExpr>(p)) {
    out << "[\"sub\",[";
    bool f = true;
    for (auto x : n->params) {
      if (!f)
        out << ',';
      f = false;
      q(x);
    }
    out << "],";
    elements(n->body);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::NotExpr>(p)) {
    out << "[\"!\",";
    expr(n->operand);
    out << ']';
  } else if (auto *n = cursiv::ast_cast<ast::BoolBinExpr>(p)) {
    out << '[';
    q(n->op == ast::BoolOp::logical_and ? "&&" : "||");
    out << ',';
    expr(n->left);
    out << ',';
    expr(n->right);
    out << ']';
  } else if (cursiv::ast_cast<ast::NullExpr>(p))
    out << "[\"null\"]";
  else
    out << "[\"unknown\"]";
}
inline void Projection::elements(const auto &es) {
  out << '[';
  bool f = true;
  for (std::size_t i = 0; i < es.size(); ++i) {
    auto *e = es[i];
    if (!f)
      out << ',';
    f = false;
    if (auto *t = cursiv::ast_cast<ast::Text>(e)) {
      std::string text(t->value);
      while (i + 1 < es.size()) {
        auto *next = cursiv::ast_cast<ast::Text>(es[i + 1]);
        if (!next)
          break;
        text += next->value;
        ++i;
      }
      out << "[\"text\",";
      q(text);
      out << ']';
    } else if (auto *t = cursiv::ast_cast<ast::ExprTag>(e)) {
      out << "[\"tag\",";
      expr(t->value);
      out << ",[";
      bool ff = true;
      for (auto *o : t->options) {
        if (!ff)
          out << ',';
        ff = false;
        out << '[';
        q(o->name);
        out << ',';
        expr(o->value);
        out << ']';
      }
      out << "]]";
    } else if (auto *t = cursiv::ast_cast<ast::IfStat>(e)) {
      out << "[\"if\",[";
      bool ff = true;
      for (auto *b : t->branches) {
        if (!ff)
          out << ',';
        ff = false;
        out << '[';
        expr(b->cond);
        out << ',';
        elements(b->body);
        out << ']';
      }
      out << "]," << (t->hasElse ? "true" : "false") << ',';
      elements(t->elseBody);
      out << ']';
    } else if (auto *t = cursiv::ast_cast<ast::Region>(e)) {
      out << "[\"region\",";
      q(t->name);
      out << ',';
      elements(t->elements);
      out << ']';
    } else
      out << "null";
  }
  out << ']';
}

} // namespace native_regressions
