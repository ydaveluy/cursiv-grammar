#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "rfc1960/core/FilterValue.hpp"
#include "rfc1960/core/ast.hpp"
#include <optional>
#include <string>
using J = cursiv::JsonValue;
inline J byte_value(std::string_view raw) {
  const auto value = rfc1960::filter_value(raw);
  if (!value)
    return nullptr;
  J::Array bytes;
  for (const unsigned char c : *value)
    bytes.emplace_back(static_cast<int>(c));
  return bytes;
}
inline J filter_view(const rfc1960::ast::Filter *f) {
  using namespace rfc1960::ast;
  if (auto *n = cursiv::ast_cast<And>(f)) {
    J::Array a;
    for (auto *x : n->operands)
      a.push_back(filter_view(x));
    return J::Object{{"kind", "and"}, {"operands", std::move(a)}};
  }
  if (auto *n = cursiv::ast_cast<Or>(f)) {
    J::Array a;
    for (auto *x : n->operands)
      a.push_back(filter_view(x));
    return J::Object{{"kind", "or"}, {"operands", std::move(a)}};
  }
  if (auto *n = cursiv::ast_cast<Not>(f))
    return J::Object{{"kind", "not"},
                     {"operands", J::Array{filter_view(n->operand)}}};
  if (auto *n = cursiv::ast_cast<Simple>(f))
    return J::Object{{"kind", "simple"},
                     {"attr", n->attr},
                     {"op", n->op == CompareOp::equal              ? "="
                            : n->op == CompareOp::approx           ? "~="
                            : n->op == CompareOp::greater_or_equal ? ">="
                                                                   : "<="},
                     {"value", byte_value(n->value)}};
  if (auto *n = cursiv::ast_cast<Substring>(f)) {
    if (!n->initial && n->middle.empty() && !n->final_)
      return J::Object{{"kind", "present"}, {"attr", n->attr}};
    J::Array a;
    for (auto x : n->middle)
      a.push_back(byte_value(x));
    return J::Object{
        {"kind", "substring"},
        {"attr", n->attr},
        {"initial", n->initial ? byte_value(*n->initial) : J(nullptr)},
        {"middle", std::move(a)},
        {"final", n->final_ ? byte_value(*n->final_) : J(nullptr)}};
  }
  if (auto *n = cursiv::ast_cast<Extensible>(f)) {
    bool dn = !n->qualifiers.empty() && rfc1960::dn_qualifier(n->qualifiers[0]);
    return J::Object{{"kind", "extensible"},
                     {"attr", n->attr ? J(*n->attr) : J(nullptr)},
                     {"dn", dn},
                     {"rule", n->qualifiers.size() > (dn ? 1u : 0u)
                                  ? J(n->qualifiers.back())
                                  : J(nullptr)},
                     {"value", byte_value(n->value)}};
  }
  return nullptr;
}
