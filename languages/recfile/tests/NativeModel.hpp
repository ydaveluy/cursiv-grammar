#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include <string>
using J = cursiv::JsonValue;
inline J model_sets(const J::Array &input) {
  J::Array sets, items;
  J descriptor = nullptr;
  std::size_t records = 0, pos = 0;
  auto flush = [&]() {
    if (!descriptor.isNull() || records) {
      J type = nullptr;
      if (!descriptor.isNull())
        for (const auto &f : descriptor.array())
          if (f.object().at("kind").string() == "field" &&
              f.object().at("name").string() == "%rec") {
            auto v = std::string_view(f.object().at("value").string());
            auto start = v.find_first_of(
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz%");
            if (start != v.npos) {
              auto end = v.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefg"
                                             "hijklmnopqrstuvwxyz0123456789_",
                                             start + 1);
              type = J(v.substr(start, end == v.npos ? v.size() - start
                                                     : end - start));
            }
            break;
          }
      sets.emplace_back(
          J::Object{{"type", std::move(type)},
                    {"descriptor_pos", static_cast<std::int64_t>(pos)},
                    {"descriptor", descriptor},
                    {"items", J(items)}});
    }
    items.clear();
    descriptor = nullptr;
    records = 0;
    pos = 0;
  };
  for (const auto &i : input) {
    if (i.object().at("kind").string() == "comment") {
      items.push_back(i);
      continue;
    }
    bool desc = false;
    for (const auto &f : i.object().at("entries").array())
      if (f.object().at("kind").string() == "field" &&
          f.object().at("name").string() == "%rec")
        desc = true;
    if (desc) {
      if (descriptor.isNull() && records == 0) {
        descriptor = i.object().at("entries");
        pos = items.size();
      } else {
        flush();
        descriptor = i.object().at("entries");
      }
    } else {
      items.push_back(i);
      ++records;
    }
  }
  flush();
  return sets;
}
