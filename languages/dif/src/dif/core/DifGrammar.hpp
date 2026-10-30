#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cursiv/core/grammar/Conversion.hpp"
#include "dif/core/ast.hpp"

namespace dif {

inline constexpr auto unquote = [](
    std::string_view literal, std::pmr::memory_resource* memory) -> std::string_view {
  auto text = literal.substr(1, literal.size() - 2);
  if (text.find("\"\"") == std::string_view::npos) return text;
  auto room = cursiv::text_room(memory, text.size());
  std::size_t written = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    room[written++] = text[i];
    if (text[i] == '"' && i + 1 < text.size() && text[i + 1] == '"') ++i;
  }
  return {room.data(), written};
};

inline constexpr auto trim = [](std::string_view text) {
  while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
  while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
  return text;
};

struct Dif : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct TYPE : terminal<option(cr<"+-">) + some(d)> {};
  struct TOPIC : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9_">)> {};
  struct LINE : terminal<as<trim>(some(cr<R"(^\r\n)">))> {};
  struct STATUS : terminal<as<trim>(!lit<"\""> + some(cr<R"(^\r\n)">))> {};
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) +
                             lit<"\"">)> {};

  struct Pair : rule<assign<&ast::Pair::first>(ref<TYPE>) + lit<","> +
                         option(assign<&ast::Pair::second>(ref<LINE>))> {};

  struct Section : rule<assign<&ast::Section::topic>(ref<TOPIC>) + ref<NL> +
                            assign<&ast::Section::id>(ref<Pair>) + ref<NL> +
                            assign<&ast::Section::label>(ref<STRING>) + (ref<NL> | eof)> {};

  struct Header : rule<create<ast::Header> +
      many(append<&ast::Header::sections>(ref<Section>) + many(ref<NL>))> {};

  struct Datum : rule<assign<&ast::Datum::id>(ref<Pair>) + ref<NL> +
      (flag<&ast::Datum::quoted>(assign<&ast::Datum::value>(ref<STRING>)) |
       assign<&ast::Datum::value>(ref<STATUS>)) + (ref<NL> | eof)> {};

  struct Document : rule<create<ast::Document> + many(ref<NL>) +
      assign<&ast::Document::header>(ref<Header>) +
      many(append<&ast::Document::items>(ref<Datum>) + many(ref<NL>))> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace dif
