#pragma once
#include "aterm/core/Value.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include <bit>
#include <charconv>
#include <cstdio>
using J = cursiv::JsonValue;
inline std::string hex(std::string_view s) {
  const char *d = "0123456789abcdef";
  std::string out;
  for (unsigned char c : s) {
    out += d[c >> 4];
    out += d[c & 15];
  }
  return out;
}
inline std::string bits(double n) {
  char s[17];
  std::snprintf(
      s, sizeof(s), "%016llx",
      static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(n)));
  return s;
}
inline J plain(std::string_view name) {
  return J::Object{{"kind", "app"},
                   {"name_hex", hex(name)},
                   {"quoted", false},
                   {"args", J::Array{}},
                   {"annotations", J::Array{}}};
}
inline J view(const aterm::ast::Term *t, const cursiv::Cst &cst) {
  if (!t)
    return nullptr;
  J::Object out;
  if (auto *f = cursiv::ast_cast<aterm::ast::Function>(t)) {
    J::Array args;
    for (auto *a : f->args)
      args.push_back(view(a, cst));
    out = {{"kind", "app"},
           {"name_hex", hex(aterm::function_name(*f))},
           {"quoted", aterm::quoted(*f)},
           {"args", args}};
  } else if (auto *n = cursiv::ast_cast<aterm::ast::Number>(t)) {
    const auto s = n->raw;
    if (aterm::real(*n)) {
      double v;
      auto r = std::from_chars(s.data(), s.data() + s.size(), v);
      out = {{"kind", "real"},
             {"bits", r.ec == std::errc{} && r.ptr == s.data() + s.size()
                          ? J(bits(v))
                          : J(nullptr)}};
    } else {
      std::int64_t v;
      auto r = std::from_chars(s.data(), s.data() + s.size(), v);
      out = {{"kind", "integer"},
             {"value", r.ec == std::errc{} && r.ptr == s.data() + s.size()
                           ? J(std::to_string(v))
                           : J(s)}};
    }
  } else if (auto *l = cursiv::ast_cast<aterm::ast::List>(t)) {
    J::Array items;
    for (auto *v : l->items)
      items.push_back(view(v, cst));
    out = {{"kind", "list"}, {"items", items}};
  } else if (auto *p = cursiv::ast_cast<aterm::ast::Placeholder>(t))
    out = {{"kind", "placeholder"}, {"inner", view(p->inner, cst)}};
  J::Array ann;
  for (auto *a : t->annotations)
    ann.push_back(view(a, cst));
  auto put = [&](std::string_view key, J value) {
    J label = plain(key);
    J pair = J::Object{{"kind", "list"},
                       {"items", J::Array{label, value}},
                       {"annotations", J::Array{}}};
    for (auto &v : ann)
      if (v.isObject() && v.object().at("kind").string() == "list") {
        auto &xs = v.object().at("items").array();
        if (!xs.empty() && xs[0].toJsonString({.pretty = false}) ==
                               label.toJsonString({.pretty = false})) {
          v = pair;
          return;
        }
      }
    ann.insert(ann.begin(), pair);
  };
  if (t->typeAnnotation)
    put("type", view(t->typeAnnotation, cst));
  if (t->resultAnnotation)
    put("result", plain("true"));
  out.emplace("annotations", ann);
  return out;
}
