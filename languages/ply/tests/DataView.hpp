#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "ply/core/Data.hpp"
#include <bit>
#include <cmath>
#include <cstdio>
using J = cursiv::JsonValue;
std::string hex(std::string_view text) {
  std::string out;
  for (unsigned char c : text) {
    out += "0123456789abcdef"[c >> 4];
    out += "0123456789abcdef"[c & 15];
  }
  return out;
}
J number(const ply::Number &number) {
  return std::visit(
      [](auto v) -> J {
        using T = decltype(v);
        if constexpr (std::is_same_v<T, double>) {
          if (std::isnan(v))
            return std::signbit(v) ? "f:-nan" : "f:nan";
          char t[19];
          std::snprintf(
              t, sizeof t, "f:%016llx",
              static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(v)));
          return t;
        } else
          return std::string(std::is_signed_v<T> ? "i:" : "u:") +
                 std::to_string(v);
      },
      number);
}
J project(const ply::Data &data) {
  J::Array elements;
  for (const auto &e : data.elements) {
    J::Array ps, rs, comments;
    for (auto *p : e.properties) {
      J::Object f{{"name", p->name},
                  {"type", ply::canonical_type(p->type)},
                  {"list", p->list}};
      if (p->list)
        f["count_type"] = ply::canonical_type(p->countType);
      ps.push_back(f);
    }
    for (auto c : e.comments)
      comments.push_back(c);
    for (const auto &r : e.records) {
      J::Array fs;
      for (const auto &f : r.fields) {
        J::Array values;
        for (auto &v : f.values)
          values.push_back(v.value ? number(*v.value) : J(nullptr));
        fs.push_back(values);
      }
      rs.push_back(fs);
    }
    for (std::uint64_t k = 0; k < e.emptyRecords && k < 100000; k++)
      rs.push_back(J::Array{});
    elements.push_back(J::Object{
        {"name", e.declaration->name},
        {"count", e.count ? std::to_string(*e.count) : std::string("invalid")},
        {"properties", ps},
        {"records", rs},
        {"comments", comments}});
  }
  J::Array comments, info;
  for (auto x : data.comments)
    comments.push_back(x);
  for (auto x : data.objectInfo)
    info.push_back(x);
  return J::Object{{"encoding", data.encoding},
                   {"elements", elements},
                   {"comments", comments},
                   {"obj_info", info}};
}

J common(J value) {
  if (value.isObject()) {
    value.object().erase("comments");
    value.object().erase("obj_info");
    for (auto &[key, child] : value.object())
      child = common(std::move(child));
  } else if (value.isArray())
    for (auto &child : value.array())
      child = common(std::move(child));
  return value;
}
J metadata(const ply::Data &data) {
  J::Array comments, info, elements;
  for (auto s : data.comments)
    comments.push_back(s);
  for (auto s : data.objectInfo)
    info.push_back(s);
  for (const auto &e : data.elements) {
    J::Array c;
    for (auto s : e.comments)
      c.push_back(s);
    elements.push_back(c);
  }
  return J::Object{
      {"comments", comments}, {"obj_info", info}, {"elements", elements}};
}
