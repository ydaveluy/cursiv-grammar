#pragma once

#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "properties/core/PropertiesString.hpp"
#include "properties/core/ast.hpp"

namespace properties {

struct Properties : cursiv::GrammarSpec {
  static constexpr auto horizontal = cr<R"( \t\f)">;
  static constexpr auto continued = lit<"\\"> + eol + many(horizontal);
  struct COMMENT : terminal<many(continued) + cr<"!#"> + many(cr<R"(^\r\n)">)> {
  };
  struct NEWLINE : terminal<eol> {};
  struct CONTINUED : terminal<some(continued)> {};
  inline static constexpr auto decode = [](std::string_view text,
                                           std::pmr::memory_resource *mem) {
    while (!text.empty() &&
           (text[0] == ' ' || text[0] == '\t' || text[0] == '\f'))
      text.remove_prefix(1);
    if (text.find('\\') == std::string_view::npos)
      return text;
    const auto value = decode_text(text);
    return cursiv::copy_into(mem, value.text);
  };
  struct HWS : terminal<some(cr<R"( \t\f)">)> {};
  static constexpr auto esc =
      lit<"\\"> + ((eol + many(horizontal)) | any | eof);

  struct KEY : terminal<as<decode>(
                   !(many(continued) + cr<"!#">)+!(some(continued) + eol) +
                   !(some(continued) + some(horizontal) + eof) +
                   some(esc | cr<R"(^ :=\t\f\r\n\\)">))> {};
  struct VALUE : terminal<as<decode>(some(esc | cr<R"(^\r\n\\)">))> {};
  struct Delimiter : terminal<cr<":="> + option(ref<HWS>)> {};
  struct Separator
      : terminal<(ref<HWS> + option(ref<Delimiter>)) | ref<Delimiter>> {};

  static constexpr auto row_end = ref<NEWLINE> | eof;

  struct Comment
      : rule<option(ref<HWS>) + assign<&ast::Comment::text>(ref<COMMENT>) +
             row_end> {};

  struct EmptyKey : rule<&ref<Delimiter>> {};
  struct Value : rule<option(ref<VALUE>)> {};

  struct Entry
      : rule<option(ref<HWS>) +
             ((assign<&ast::Entry::key>(ref<KEY>) +
               option(ref<Separator> +
                      assign<&ast::Entry::value>(ref<Value>))) |
              (assign<&ast::Entry::key>(ref<EmptyKey>) + ref<Delimiter> +
               assign<&ast::Entry::value>(ref<Value>))) +
             row_end> {};
  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::rows>(ref<Entry> |
                                               ref<Comment>) |
                  (option(ref<HWS>) + !ref<KEY> + option(ref<CONTINUED>) +
                   ref<NEWLINE>) |
                  (option(ref<HWS>) + !ref<KEY> + ref<CONTINUED> + eof)) +
             option(ref<HWS>)> {};

  using entry = Document;
};

} // namespace properties
