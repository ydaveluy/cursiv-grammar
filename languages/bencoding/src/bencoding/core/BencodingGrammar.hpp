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
  struct String
      : rule<assign<&ast::ByteString::length>(ref<DIGITS>) + lit<":"> +
             assign<&ast::ByteString::value>(ref<StringContent>)> {};
  struct Integer
      : rule<lit<"i"> + assign<&ast::Integer::text>(ref<INTEGER_TEXT>) +
             lit<"e">> {};
  struct List;
  struct Dict;
  static constexpr auto value =
      ref<Integer> | ref<String> | ref<List> | ref<Dict>;
  struct Entry : rule<assign<&ast::Entry::key>(ref<String>) +
                          assign<&ast::Entry::value>(value)> {};
  struct List
      : rule<lit<"l"> + many(append<&ast::List::items>(value)) + lit<"e">> {};
  struct Dict
      : rule<lit<"d"> + many(append<&ast::Dict::entries>(ref<Entry>)) +
             lit<"e">> {};
  struct Document : rule<create<ast::Document> +
                             many(append<&ast::Document::values>(value))> {};
  using entry = Document;
};
} // namespace bencoding
