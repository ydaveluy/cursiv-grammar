#pragma once
#include "gedcom/core/ast.hpp"
#include <ostream>
namespace native_regressions {
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

  std::string level(std::string_view s) {
    auto i = s.find_first_not_of('0');
    return i == s.npos ? "0" : std::string(s.substr(i));
  }
  void tree(const gedcom::ast::Document *p) {
    out << '[';
    bool f = true;
    if (p)
      for (auto *x : p->lines) {
        if (!f)
          out << ',';
        f = false;
        out << '[';
        q(level(x->level));
        out << ',';
        q(x->name);
        out << ',';
        q(x->tag);
        out << ',';
        q(x->value);
        out << ',';
        out << (x->hasValue ? "true" : "false");
        out << ']';
      }
    out << ']';
  }
};
} // namespace native_regressions
