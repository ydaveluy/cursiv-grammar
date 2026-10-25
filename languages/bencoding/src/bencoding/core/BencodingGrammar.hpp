#pragma once
#include "bencoding/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include <string_view>
namespace bencoding {
struct Bencoding : cursiv::GrammarSpec {
  struct DIGITS : terminal<some(d)> {};
  struct INTEGER_TEXT : terminal<option(lit<"-">) + some(d)> {};
  struct ContentByte
      : terminal<lit<"i"> | lit<"l"> | lit<"d"> | lit<"e"> | some(d) | byte> {};
  struct StringContent : rule<many(ref<ContentByte>)> {};
  struct StringRule
      : rule<assign<&ast::ByteString::length>(ref<DIGITS>) + lit<":"> +
             assign<&ast::ByteString::value>(ref<StringContent>)> {};
  struct IntegerRule
      : rule<lit<"i"> + assign<&ast::Integer::text>(ref<INTEGER_TEXT>) +
             lit<"e">> {};
  struct ListRule;
  struct DictRule;
  static constexpr auto value =
      ref<IntegerRule> | ref<StringRule> | ref<ListRule> | ref<DictRule>;
  struct EntryRule : rule<assign<&ast::Entry::key>(ref<StringRule>) +
                          assign<&ast::Entry::value>(value)> {};
  struct ListRule
      : rule<lit<"l"> + many(append<&ast::List::items>(value)) + lit<"e">> {};
  struct DictRule
      : rule<lit<"d"> + many(append<&ast::Dict::entries>(ref<EntryRule>)) +
             lit<"e">> {};
  struct DocumentRule : rule<create<ast::Document> +
                             many(append<&ast::Document::values>(value))> {};
  using entry = DocumentRule;
};
} // namespace bencoding
