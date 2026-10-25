#pragma once
#include "NativeModel.hpp"
#include "recfile/core/Value.hpp"
#include "recfile/core/ast.hpp"
inline J comment_view(const recfile::ast::Comment &c) {
  std::string text;
  for (std::size_t i = 0; i < c.lines.size(); ++i) {
    if (i)
      text += '#';
    text += c.lines[i];
  }
  return J::Object{{"kind", "comment"}, {"text", std::move(text)}};
}
inline J sets_view(const recfile::ast::Document &doc) {
  J::Array input;
  for (const auto &e : doc.entries) {
    if (auto *r = std::get_if<recfile::ast::Record *>(&e); r && *r) {
      J::Array entries;
      for (auto *l : (*r)->lines) {
        if (auto *f = cursiv::ast_cast<recfile::ast::Field>(l))
          entries.emplace_back(J::Object{{"kind", "field"},
                                         {"name", f->key},
                                         {"value", recfile::field_value(*f)}});
        else if (auto *c = cursiv::ast_cast<recfile::ast::Comment>(l))
          entries.push_back(comment_view(*c));
        else if (auto *x = cursiv::ast_cast<recfile::ast::RawLine>(l))
          entries.emplace_back(
              J::Object{{"kind", "invalid"}, {"text", x->text}});
      }
      input.emplace_back(
          J::Object{{"kind", "record"}, {"entries", std::move(entries)}});
    } else if (auto *c = std::get_if<recfile::ast::Comment *>(&e); c && *c)
      input.push_back(comment_view(**c));
  }
  return model_sets(input);
}
