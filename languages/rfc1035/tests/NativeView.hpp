#pragma once

#include "rfc1035/core/ast.hpp"
#include "rfc1035/core/Label.hpp"
#include "cursiv/core/services/JsonValue.hpp"

inline cursiv::JsonValue native_view(const rfc1035::ast::Domain& domain) {
  using J = cursiv::JsonValue;
  J::Array labels;
  const auto& raw = domain.labels;
  if (raw.size() == 1 && (raw[0].empty() || raw[0] == "@"))
    return J::Object{{"absolute", false}, {"labels", std::move(labels)}};
  const bool root = raw.size() == 2 && raw[0].empty() && raw[1].empty();
  for (std::size_t i = root ? 1 : 0; i < raw.size(); ++i) {
    const auto value = rfc1035::label_value(raw[i]);
    if (!value) return nullptr;
    J::Array bytes;
    for (const unsigned char c : *value) bytes.emplace_back(static_cast<int>(c));
    labels.emplace_back(std::move(bytes));
  }
  return J::Object{{"absolute", !raw.empty() && raw.back().empty()},
                   {"labels", std::move(labels)}};
}
