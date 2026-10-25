#pragma once
#include "xsdregex/core/ast.hpp"
#include <string>
namespace xa = xsdregex::ast;
inline std::string pattern(const xa::RegExp &);
inline std::string prop(const xa::CharProp &p) {
  return std::string(p.negated ? "\\P{" : "\\p{") + std::string(p.name) + "}";
}
inline std::string character_class(const xa::CharClassExpr &c) {
  std::string out = c.negated ? "[^" : "[";
  for (auto &v : c.items)
    std::visit(
        [&](const auto &x) {
          using T = std::decay_t<decltype(x)>;
          if constexpr (std::is_same_v<T, std::string_view>)
            out += x;
          else if constexpr (std::is_same_v<T, xa::CharRange *>) {
            if (x) {
              out += x->from;
              out += '-';
              out += x->to;
            }
          } else if (x)
            out += prop(*x);
        },
        v);
  if (c.subtract) {
    out += '-';
    out += character_class(*c.subtract);
  }
  out += ']';
  return out;
}
inline std::string pattern(const xa::RegExp &r) {
  std::string out;
  bool first = true;
  for (auto *b : r.branches) {
    if (!first)
      out += '|';
    first = false;
    if (!b)
      continue;
    for (auto *p : b->pieces) {
      if (!p)
        continue;
      std::visit(
          [&](const auto &x) {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::string_view>)
              out += x;
            else if constexpr (std::is_same_v<T, xa::CharProp *>) {
              if (x)
                out += prop(*x);
            } else if constexpr (std::is_same_v<T, xa::CharClassExpr *>) {
              if (x)
                out += character_class(*x);
            } else if (x)
              out += '(' + pattern(*x) + ')';
          },
          p->atom);
      if (p->quant) {
        const auto &q = *p->quant;
        switch (q.kind) {
        case xa::QuantifierKind::question:
          out += '?';
          break;
        case xa::QuantifierKind::star:
          out += '*';
          break;
        case xa::QuantifierKind::plus:
          out += '+';
          break;
        case xa::QuantifierKind::counted:
          out += '{';
          out += q.min;
          if (q.comma) {
            out += ',';
            if (q.max)
              out += *q.max;
          }
          out += '}';
          break;
        }
      }
    }
  }
  return out;
}
