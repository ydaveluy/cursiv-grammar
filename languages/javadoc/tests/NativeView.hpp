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
    else if (const auto *value = std::get_if<javadoc::ast::InlineTag *>(&part)) {
      if (const auto* tag = *value)
        out.emplace_back(J::Object{{"name", tag->name},
                                   {"content", javadoc::raw_inline_content(*tag)},
                                   {"value", javadoc::inline_content(*tag)}});
    } else {
      std::string raw;
      javadoc::append_item_source(raw, part);
      out.emplace_back(J::Object{{"text", raw}});
    }
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
