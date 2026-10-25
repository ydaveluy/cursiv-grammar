#pragma once
#include "cursiv/core/services/JsonValue.hpp"
#include "javadoc/core/Content.hpp"
namespace audit {
using J = cursiv::JsonValue;
inline J items(const auto &source) {
  J::Array out;
  for (const auto &part : source) {
    if (const auto *text = std::get_if<std::string_view>(&part))
      out.emplace_back(J::Object{{"text", *text}});
    else if (const auto *tag = std::get<javadoc::ast::InlineTag *>(part))
      out.emplace_back(J::Object{{"name", tag->name},
                                 {"content", tag->content},
                                 {"value", javadoc::inline_content(*tag)}});
  }
  return out;
}
inline J view(const javadoc::ast::Documentation &doc) {
  J::Array tags;
  for (const auto *tag : doc.tags)
    tags.emplace_back(
        J::Object{{"name", tag->name}, {"content", items(tag->content)}});
  return J::Object{{"wrapped", doc.wrapped},
                   {"description", items(doc.description)},
                   {"tags", std::move(tags)}};
}
} // namespace audit
