#pragma once
#include "guido/core/ast.hpp"
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
namespace native_regressions {
using namespace guido;
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

  bool first = true;
  void output(const char *s) { q(s); }
  void output(std::string_view s) { q(s); }
  void output(const std::string &s) { q(s); }
  void output(auto v) { out << v; }
  template <class... T> void event(const char *name, T... v) {
    if (!first)
      out << ',';
    first = false;
    out << '[';
    q(name);
    ((out << ',', output(v)), ...);
    out << ']';
  }
  std::int64_t number(const ast::Number *p) {
    if (!p)
      return 0;
    std::int64_t x = 0;
    std::from_chars(p->value.data(), p->value.data() + p->value.size(), x);
    return p->negative ? -x : x;
  }
  std::int64_t number(std::string_view s) {
    std::int64_t x = 0;
    if (s.starts_with("+"))
      s.remove_prefix(1);
    std::from_chars(s.data(), s.data() + s.size(), x);
    return x;
  }
  void parameter(const ast::Parameter *p) {
    if (!p || p->bare)
      return;
    std::string text(p->text);
    if (p->real) {
      float f{};
      auto t = std::string_view(text);
      if (t.starts_with("+"))
        t.remove_prefix(1);
      std::from_chars(t.data(), t.data() + t.size(), f);
      std::ostringstream v;
      v << std::setprecision(9) << f;
      text = v.str();
    } else if (!p->quoted && !p->variable)
      text = std::to_string(static_cast<std::int32_t>(number(p->text)));
    event("parameter",
          p->quoted     ? "string"
          : p->variable ? "variable"
          : p->real     ? "real"
                        : "integer",
          p->name, text, p->unit);
  }
  void item(const ast::SequenceItem *x) {
    if (auto *p = cursiv::ast_cast<ast::Note>(x)) {
      event("note", p->name);
      for (auto a : p->accidentals)
        event("acc", a == ast::Accidental::sharp ? 1 : -1);
      if (p->octave)
        event("oct", number(p->octave));
      if (p->duration) {
        auto *d = p->duration;
        if (d->milliseconds)
          event("ms", number(d->numerator));
        else {
          if (d->numerator)
            event("enum", number(d->numerator));
          if (d->denominator)
            event("denom", number(d->denominator));
        }
      }
      if (!p->dots.empty())
        event("dots", p->dots.size());
      event("append");
    } else if (auto *p = cursiv::ast_cast<ast::Chord>(x)) {
      event("chord");
      for (auto *m : p->members) {
        event("member");
        if (m)
          for (auto *y : m->items)
            item(y);
      }
      event("append-chord");
    } else if (auto *p = cursiv::ast_cast<ast::Tag>(x)) {
      event("tag", "\\" + std::string(p->name), number(p->id));
      for (const auto &v : p->parameters)
        parameter(v);
      event("add-tag");
      if (p->ranged)
        event("range");
      for (auto *y : p->notes)
        item(y);
      event("end-tag");
    } else if (auto *p = cursiv::ast_cast<ast::VariableRef>(x))
      event("variable-symbols", p->name);
  }
  void voice(const ast::Sequence *s) {
    event("voice");
    if (s)
      for (auto *x : s->items)
        item(x);
    event("end-voice");
  }
  void tree(const ast::Prog *p) {
    out << '[';
    if (p) {
      for (auto *d : p->variables) {
        auto *v = d->value;
        event("variable-declaration", d->name, v ? v->text : std::string_view(),
              v ? (v->quoted ? 0
                   : v->real ? 2
                             : 1)
                : 1);
      }
      for (auto *s : p->segments) {
        event("score");
        if (s)
          for (auto *v : s->sequences) {
            voice(v);
            event("append-voice");
          }
        event("end-score");
      }
      for (auto *v : p->sequences) {
        event("score");
        voice(v);
        event("end-score");
      }
    }
    out << ']';
  }
};
} // namespace native_regressions
