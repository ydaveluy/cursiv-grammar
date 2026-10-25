#pragma once
#include "wln/core/ast.hpp"
#include <ostream>
#include <string>
namespace native_regressions {
struct Projection {
  std::ostream &out;
  explicit Projection(std::ostream &stream) : out(stream) {}
  void q(std::string_view s) {
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
  void groups(const auto &gs) {
    out << '[';
    bool first = true;
    for (auto *g : gs) {
      if (!first)
        out << ',';
      out << "{\"separator\":";
      q(g->separator);
      first = false;
      out << ",\"symbols\":[";
      bool f = true;
      for (auto v : g->symbols) {
        if (!f)
          out << ',';
        f = false;
        q(v);
      }
      out << "]}";
    }
    out << ']';
  }
  void records(const wln::ast::Document *p) {
    out << '[';
    if (p) {
      bool first = true;
      for (auto *r : p->records) {
        if (!first)
          out << ',';
        first = false;
        out << "{\"prefix\":";
        q(r->prefix);
        out << ",\"groups\":";
        groups(r->groups);
        out << ",\"trailing\":";
        q(r->trailing);
        out << ",\"title\":";
        if (r->title)
          q(*r->title);
        else
          out << "null";
        out << '}';
      }
    }
    out << ']';
  }
};
} // namespace native_regressions
