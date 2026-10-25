#pragma once
#include "muddb/core/ast.hpp"
#include <charconv>
#include <ostream>
#include <string>
namespace native_regressions {
using namespace muddb;
struct Projection {
  std::ostream &out;
  explicit Projection(std::ostream &stream) : out(stream) {}
  void q(std::string_view s) {
    out << '"';
    for (unsigned char c : s) {
      if (c == '"' || c == '\\')
        out << '\\' << char(c);
      else if (c < 32) {
        const char *h = "0123456789abcdef";
        out << "\\u00" << h[c >> 4] << h[c & 15];
      } else
        out << char(c);
    }
    out << '"';
  }
  std::int64_t number(std::int64_t n) { return n; }
  std::int64_t number(std::string_view s) {
    std::int64_t n = 0;
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t'))
      s.remove_prefix(1);
    if (s.starts_with("+"))
      s.remove_prefix(1);
    std::from_chars(s.data(), s.data() + s.size(), n);
    return n;
  }
  void text(const auto &v) {
    if (v)
      q(*v);
    else
      out << "null";
  }
  bool isTrue(const ast::KeyExpr *p) {
    return !p || cursiv::ast_cast<ast::LegacyKey>(p);
  }
  void key(const ast::KeyExpr *p, bool invert = false) {
    if (invert && !isTrue(p)) {
      out << "[\"not\",";
      key(p);
      out << ']';
      return;
    }
    if (isTrue(p))
      out << "[\"true\"]";
    else if (auto *r = cursiv::ast_cast<ast::KeyReference>(p))
      out << "[\"ref\"," << static_cast<std::int32_t>(number(r->value)) << ']';
    else if (auto *r = cursiv::ast_cast<ast::KeyNot>(p)) {
      out << "[\"not\",";
      key(r->operand);
      out << ']';
    } else if (auto *r = cursiv::ast_cast<ast::KeyBinary>(p)) {
      out << (r->op == "&" ? "[\"and\"," : "[\"or\",");
      key(r->left);
      out << ',';
      key(r->right);
      out << ']';
    } else
      out << "null";
  }
  void tree(const ast::Db *p) {
    out << '[';
    bool f = true;
    if (p)
      for (auto *x : p->rooms) {
        if (!f)
          out << ',';
        f = false;
        out << '[';
        auto id = x->roomno;
        if (id.starts_with("#"))
          id.remove_prefix(1);
        q("#" + std::to_string(number(id)));
        out << ',';
        text(x->name);
        out << ',';
        text(x->description);
        for (const auto *v :
             {&x->location, &x->contents, &x->exits, &x->next_room})
          out << ',' << (v->has_value() ? number(**v) : 0);
        const auto flags = x->flags ? number(*x->flags) : 0;
        out << ',';
        key(x->key, flags & 8);
        out << ',';
        text(x->fail_message);
        out << ',';
        text(x->succ_message);
        out << ',';
        text(x->ofail);
        out << ',';
        text(x->osuccess);
        for (const auto *v : {&x->owner, &x->pennies})
          out << ',' << (v->has_value() ? number(**v) : 0);
        out << ',' << (flags & ~8) << ',';
        text(x->password);
        out << ']';
      }
    out << ']';
  }
};
} // namespace native_regressions
