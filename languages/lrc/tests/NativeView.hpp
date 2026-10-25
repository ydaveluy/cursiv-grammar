#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "lrc/core/Time.hpp"
#include "lrc/core/ast.hpp"
#include <algorithm>
using J = cursiv::JsonValue;
inline J lyric_view(const lrc::ast::Document &doc) {
  J::Object meta{{"artist", ""}, {"album", ""},       {"title", ""},
                 {"author", ""}, {"lrc_creator", ""}, {"length", ""},
                 {"editor", ""}, {"version", ""}};
  std::int64_t offset = 0;
  for (auto *r : doc.lines)
    if (auto *m = cursiv::ast_cast<lrc::ast::Metadata>(r)) {
      std::string key;
      if (m->key == "ar")
        key = "artist";
      else if (m->key == "al")
        key = "album";
      else if (m->key == "ti")
        key = "title";
      else if (m->key == "au")
        key = "author";
      else if (m->key == "by")
        key = "lrc_creator";
      else if (m->key == "length")
        key = "length";
      else if (m->key == "re")
        key = "editor";
      else if (m->key == "ve")
        key = "version";
      auto value = lrc::trim_left(m->value);
      if (!key.empty())
        meta[key] = J(value);
      if (m->key == "offset")
        if (auto x = lrc::integer(m->value))
          offset = *x;
    }
  std::vector<std::pair<std::int64_t, std::string_view>> rows;
  for (auto *r : doc.lines)
    if (auto *l = cursiv::ast_cast<lrc::ast::Line>(r))
      for (auto *t : l->timestamps)
        if (auto ms = lrc::milliseconds(*t)) {
          if (offset > 0 &&
              *ms > std::numeric_limits<std::int64_t>::max() - offset)
            continue;
          if (offset < 0 &&
              *ms < std::numeric_limits<std::int64_t>::min() - offset)
            continue;
          rows.emplace_back(*ms + offset, l->text);
        }
  std::stable_sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
    return a.first < b.first;
  });
  J::Array lines;
  for (auto [ms, text] : rows)
    lines.emplace_back(J::Object{{"milliseconds", ms}, {"text", text}});
  return J::Object{{"metadata", std::move(meta)},
                   {"offset", 0},
                   {"lyrics", std::move(lines)}};
}

inline J source_view(const lrc::ast::Document &doc) {
  J::Array rows;
  for (auto *r : doc.lines) {
    if (auto *l = cursiv::ast_cast<lrc::ast::Line>(r)) {
      J::Array ts;
      for (auto *t : l->timestamps)
        ts.emplace_back(J::Object{
            {"minutes", t->minutes},
            {"seconds", t->seconds},
            {"fraction", t->fraction ? J(*t->fraction) : J(nullptr)}});
      rows.emplace_back(J::Object{
          {"kind", "line"}, {"text", l->text}, {"timestamps", std::move(ts)}});
    } else if (auto *m = cursiv::ast_cast<lrc::ast::Metadata>(r))
      rows.emplace_back(J::Object{{"kind", "metadata"},
                                  {"key", m->key},
                                  {"value", m->value},
                                  {"tail", m->tail}});
    else if (auto *l = cursiv::ast_cast<lrc::ast::RawLine>(r))
      rows.emplace_back(J::Object{{"kind", "raw"}, {"text", l->text}});
  }
  return rows;
}
