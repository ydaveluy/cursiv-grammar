#pragma once
#include "upnp/core/ast.hpp"
#ifndef AUDIT_BEFORE
#include "upnp/core/Value.hpp"
#endif
#include "cursiv/core/services/JsonValue.hpp"
using J = cursiv::JsonValue;
namespace audit {
inline std::string hex(std::string_view s) {
  constexpr char ds[] = "0123456789abcdef";
  std::string o;
  for (unsigned char c : s) {
    o += ds[c >> 4];
    o += ds[c & 15];
  }
  return o;
}
inline J expr(const upnp::ast::SearchExpr *p) {
  namespace a = upnp::ast;
  if (auto *n = cursiv::ast_cast<a::BinaryExpr>(p))
    return J::Object{{"op", n->op == a::LogOp::and_ ? "and" : "or"},
                     {"left", expr(n->left)},
                     {"right", expr(n->right)}};
  if (auto *n = cursiv::ast_cast<a::Comparison>(p))
    return J::Object{{"property", hex(n->property)},
                     {"op", static_cast<int>(n->op)},
                     {"value",
#ifdef AUDIT_BEFORE
                      hex(n->value)
#else
                      hex(upnp::comparison_value(*n))
#endif
                     }};
  if (auto *n = cursiv::ast_cast<a::Exists>(p))
    return J::Object{{"property", hex(n->property)},
                     {"op", 9},
                     {"value", hex(n->value ? "true" : "false")}};
  return nullptr;
}
inline J view(const upnp::ast::SearchCrit &d) {
  return d.wildcard ? J("wildcard") : expr(d.expr);
}
} // namespace audit
