#pragma once
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "redis/core/Argument.hpp"
using J = cursiv::JsonValue;
inline std::string hex(std::string_view s) {
  const char *ds = "0123456789abcdef";
  std::string out;
  for (unsigned char c : s) {
    out += ds[c >> 4];
    out += ds[c & 15];
  }
  return out;
}
inline J view(const redis::ast::Document &d, const cursiv::Cst &) {
  J::Array lines;
  for (auto *c : d.commands) {
    J::Array argv;
    if (c->name)
      argv.emplace_back(hex(redis::argument_value(*c->name).text));
    for (auto *v : c->arguments)
      argv.emplace_back(v ? J(hex(redis::argument_value(*v).text))
                          : J(nullptr));
    lines.emplace_back(argv);
  }
  return lines;
}
