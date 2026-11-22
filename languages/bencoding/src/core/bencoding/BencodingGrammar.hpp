#pragma once
#include "bencoding/ast.hpp"
#include "cursiv/grammar/GrammarSpec.hpp"
#include <charconv>
#include <limits>
#include <string_view>

namespace bencoding {

inline std::size_t read_byte_string(std::string_view input) {
  if (input.size() < 2) return 0;
  if (input.front() == '0') return input[1] == ':' ? 2 : 0;
  const auto digits = input.substr(0, std::numeric_limits<std::size_t>::digits10 + 1);
  std::size_t count;
  const auto [colon, error] = std::from_chars(digits.data(), digits.data() + digits.size(), count);
  if (error != std::errc{} || colon == input.data() + input.size() || *colon != ':') return 0;
  const auto header = static_cast<std::size_t>(colon + 1 - input.data());
  return count <= input.size() - header ? header + count : 0;
}

struct Bencoding : cursiv::GrammarSpec {
  // Keep leading zeros for the validator. The scanner reads only the significant count.
  struct BYTE_STRING : terminal<(&d) + many(lit<"0">) + scan<read_byte_string>> {};
  struct INTEGER_TEXT : terminal<option(lit<"-">) + some(d)> {};
  struct String : rule<assign<&ast::ByteString::encoded>(ref<BYTE_STRING>)> {};
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
