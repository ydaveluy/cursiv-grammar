#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vmf/core/ast.hpp"

namespace vmf {

inline constexpr auto unquote = [](std::string_view text, std::pmr::memory_resource* memory) {
  text = text.substr(1, text.size() - 2);
  if (text.find_first_of("\\\r") == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t count = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
      c = '\n';
    } else if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      switch (c) {
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;
        case 'b': c = '\b'; break;
        case 'r': c = '\r'; break;
        case 'f': c = '\f'; break;
        case 'a': c = '\a'; break;
        case '\n': continue;
        case '\\': case '"': case '\'': case '/': case '?': break;
        default: room[count++] = '\\';
      }
    }
    room[count++] = c;
  }
  return std::string_view(room.data(), count);
};

struct Vmf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct QUOTEDSTRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">,
                 cursiv::text_conv<unquote>> {};
  struct STRING : terminal<cr<R"(^"{} \t\r\n)" "\xEF\xBB\xBF"> +
                           many(cr<R"(^"{} \t\r\n)">)> {};
  static constexpr auto key =
      flag<&ast::KeyValue::keyQuoted>(assign<&ast::KeyValue::key>(ref<QUOTEDSTRING>)) |
      assign<&ast::KeyValue::key>(ref<STRING>);
  static constexpr auto value =
      flag<&ast::KeyValue::valueQuoted>(assign<&ast::KeyValue::value>(ref<QUOTEDSTRING>)) |
      assign<&ast::KeyValue::value>(ref<STRING>);
  struct KeyValueRule
      : rule<key +
             (value |
              (lit<"{"> + many(append<&ast::KeyValue::children>(ref<KeyValueRule>)) + lit<"}">))> {};
  struct DocumentRule
      : rule<create<ast::Document> + option(lit<"\xEF\xBB\xBF">) +
             many(append<&ast::Document::entries>(ref<KeyValueRule>))> {};
  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace vmf
