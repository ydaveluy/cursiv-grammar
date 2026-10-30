#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gedcom/core/ast.hpp"
#include <string_view>

namespace gedcom {

inline constexpr auto strip_at = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};

struct Gedcom : cursiv::GrammarSpec {
  static constexpr auto tag_character = cr<"a-zA-Z_0-9"> | lit<"-">;
  static constexpr auto pointer = lit<"@"> + some(cr<R"(^@\r\n)">) + lit<"@">;
  static constexpr auto tag_head =
      many(lit<" ">) + option(pointer + many(lit<" ">)) + some(tag_character);
  struct LEADING : terminal<some(cr<R"( \t\v\f)">)> {};
  struct SPACE : terminal<some(lit<" ">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct LEVEL : terminal<some(d) + &tag_head> {};
  struct TAG : terminal<some(tag_character)> {};
  struct POINTER : terminal<as<strip_at>(pointer)> {};
  struct VALUE : terminal<some(cr<R"(^\r\n)">)> {};

  struct Line
      : rule<create<ast::Line>(
            assign<&ast::Line::level>(ref<LEVEL>) +
            option(ref<SPACE>) +
            option(assign<&ast::Line::name>(ref<POINTER>) +
                   option(ref<SPACE>)) +
            assign<&ast::Line::tag>(ref<TAG>) +
            option(flag<&ast::Line::hasValue>(lit<" ">) +
                   option(assign<&ast::Line::value>(ref<VALUE>))) +
            (ref<EOL> | eof))> {};

  struct Doc
      : rule<create<ast::Document>(
            option(flag<&ast::Document::bom>(lit<"\xEF\xBB\xBF">)) +
            many(ref<LEADING> | ref<EOL>) +
            some(append<&ast::Document::lines>(ref<Line>) +
                 many(ref<LEADING> | ref<EOL>)))> {};

  using entry = Doc;
};

} // namespace gedcom
