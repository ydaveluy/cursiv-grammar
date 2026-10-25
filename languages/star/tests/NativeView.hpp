#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "star/core/Value.hpp"
using J = cursiv::JsonValue;
namespace sa = star::ast;
inline star::StringSyntax syntax = star::StringSyntax::cif1;
inline J value_view(const sa::Value *v);
inline J loop_view(const sa::Loop &l) {
  J::Array columns, values;
  if (l.header)
    for (auto c : star::loop_columns(*l.header))
      columns.emplace_back(c);
  for (auto *v : l.values)
    if (!cursiv::ast_cast<sa::Stop>(v))
      values.emplace_back(value_view(v));
  return J::Object{{"kind", "loop"}, {"columns", columns}, {"values", values}};
}
inline J value_view(const sa::Value *v) {
  if (!v)
    return nullptr;
  if (auto *s = cursiv::ast_cast<sa::StringLit>(v))
    return star::string_value(*s, syntax);
  if (auto *s = cursiv::ast_cast<sa::Literal>(v))
    return s->text;
  if (auto *l = cursiv::ast_cast<sa::Loop>(v))
    return loop_view(*l);
  if (auto *l = cursiv::ast_cast<sa::List>(v)) {
    J::Array a;
    for (auto *x : l->values)
      a.push_back(value_view(x));
    return J::Object{{"kind", "list"}, {"values", a}};
  }
  if (auto *t = cursiv::ast_cast<sa::Table>(v)) {
    J::Array a;
    for (auto *e : t->entries)
      a.emplace_back(J::Object{
          {"key", e->key ? J(star::string_value(*e->key, syntax)) : J(nullptr)},
          {"value", value_view(e->value)}});
    return J::Object{{"kind", "table"}, {"entries", a}};
  }
  return nullptr;
}
inline J content_view(const sa::Content &c, std::string_view name) {
  J::Array items, frames;
  for (const auto &e : c.elements)
    std::visit(
        [&](auto *p) {
          if (!p)
            return;
          using T = std::decay_t<decltype(p)>;
          if constexpr (std::is_same_v<T, sa::KeyVal *>)
            items.emplace_back(J::Object{{"kind", "pair"},
                                         {"name", p->name},
                                         {"value", value_view(p->value)}});
          else if constexpr (std::is_same_v<T, sa::Loop *>)
            items.push_back(loop_view(*p));
          else
            frames.push_back(content_view(*p, p->name));
        },
        e);
  return J::Object{{"name", name}, {"items", items}, {"frames", frames}};
}
inline J doc_view(const sa::Doc &d) {
  J::Array blocks;
  for (const auto &v : d.blocks)
    std::visit(
        [&](auto *b) {
          if (!b)
            return;
          using T = std::decay_t<decltype(b)>;
          if constexpr (std::is_same_v<T, sa::DataBlock *>)
            blocks.push_back(content_view(*b, b->name));
          else
            blocks.push_back(content_view(*b, "GLOBAL"));
        },
        v);
  return J::Object{{"blocks", blocks}};
}
inline std::string prefix(std::string_view s) {
  return std::string(s.substr(0, s.find('.')));
}
inline std::string shortname(std::string_view s) {
  auto p = s.find('.');
  return std::string(p == s.npos ? s : s.substr(p + 1));
}
inline J nmr_view(const sa::Doc &d) {
  const auto saved = syntax;
  syntax = star::StringSyntax::nmr;
  J::Array frames;
  std::string id;
  for (const auto &b : d.blocks)
    if (auto *p = std::get_if<sa::DataBlock *>(&b); p && *p) {
      id = (*p)->name;
      for (const auto &e : (*p)->elements)
        if (auto *f = std::get_if<sa::SaveFrame *>(&e); f && *f) {
          J::Array tags, loops;
          J category = nullptr, tagPrefix = nullptr;
          for (const auto &item : (*f)->elements)
            std::visit(
                [&](auto *x) {
                  if (!x)
                    return;
                  using T = std::decay_t<decltype(x)>;
                  if constexpr (std::is_same_v<T, sa::KeyVal *>) {
                    if (tagPrefix.isNull())
                      tagPrefix = prefix(x->name);
                    auto name = shortname(x->name);
                    tags.emplace_back(J::Array{J(name), value_view(x->value)});
                    if (name == "Sf_category")
                      category = value_view(x->value);
                  } else if constexpr (std::is_same_v<T, sa::Loop *>) {
                    J::Array names, rows;
                    std::vector<J> values;
                    std::string cat;
                    if (x->header)
                      for (auto c : star::loop_columns(*x->header)) {
                        if (cat.empty())
                          cat = prefix(c);
                        names.emplace_back(shortname(c));
                      }
                    for (auto *v : x->values)
                      if (!cursiv::ast_cast<sa::Stop>(v))
                        values.push_back(value_view(v));
                    if (!names.empty())
                      for (std::size_t i = 0; i + names.size() <= values.size();
                           i += names.size()) {
                        J::Array row;
                        for (std::size_t k = 0; k < names.size(); ++k)
                          row.push_back(values[i + k]);
                        rows.emplace_back(row);
                      }
                    loops.emplace_back(J::Object{
                        {"category", cat.empty() ? J(nullptr) : J(cat)},
                        {"tags", names},
                        {"data", rows}});
                  }
                },
                item);
          frames.emplace_back(J::Object{{"name", (*f)->name},
                                        {"category", category},
                                        {"tag_prefix", tagPrefix},
                                        {"tags", tags},
                                        {"loops", loops}});
        }
    }
  syntax = saved;
  return J::Object{{"entry_id", id}, {"saveframes", frames}};
}
