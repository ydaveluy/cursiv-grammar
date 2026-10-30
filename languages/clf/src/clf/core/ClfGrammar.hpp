#pragma once

#include <memory_resource>
#include <string_view>

#include "clf/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace clf {

inline constexpr auto unescape = [](std::string_view text,
                                    std::pmr::memory_resource* resource) {
  if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
    text = text.substr(1, text.size() - 2);
  if (text.find('\\') == std::string_view::npos) return text;
  auto* output = static_cast<char*>(resource->allocate(text.size()));
  const auto hex = [](char c) {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
  };
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      switch (c) {
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'v':
          c = '\v';
          break;
        case 'x':
          if (i + 2 < text.size() && hex(text[i + 1]) >= 0 &&
              hex(text[i + 2]) >= 0) {
            c = static_cast<char>(hex(text[i + 1]) * 16 + hex(text[i + 2]));
            i += 2;
          }
          break;
      }
    }
    output[size++] = c;
  }
  return std::string_view(output, size);
};

struct Clf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct BLANK : terminal<some(cr<R"( \t)">) + &(cr<R"(\r\n)"> | eof)> {};
  static constexpr auto escape =
      lit<"\\"> +
      ((lit<"x"> + repeat<2, 2>(cr<"0-9a-fA-F">)) | cr<R"(^x\r\n)">);
  struct WORD : terminal<as<unescape>(some(cr<R"(^"\\ \t\r\n)"> | escape))> {};
  struct WordValue : rule<option(ref<WORD>)> {};
  struct USER
      : terminal<as<unescape>((lit<"\"\""> | some(cr<R"(^"\\\r\n)"> | escape)) +
                              &(lit<" [">))> {};
  struct UserValue : rule<option(ref<USER>)> {};
  struct LITERAL
      : terminal<as<unescape>(lit<"\""> + many(cr<R"(^"\\\r\n)"> | escape) +
                              lit<"\"">)> {};
  struct DATE
      : terminal<some(d) + lit<"/"> + some(cr<"A-Za-z">) + lit<"/"> + some(d)> {
  };
  struct TIME : terminal<some(d) + lit<":"> + some(d) + lit<":"> + some(d)> {};
  struct TZ : terminal<cr<"+-"> + some(d)> {};

  struct Line
      : rule<assign<&ast::Line::host>(ref<WordValue>) + lit<" "> +
             assign<&ast::Line::logname>(ref<WordValue>) + lit<" "> +
             assign<&ast::Line::username>(ref<UserValue>) + lit<" ["> +
             assign<&ast::Line::date>(ref<DATE>) + lit<":"> +
             assign<&ast::Line::time>(ref<TIME>) + option(ref<WS>) +
             assign<&ast::Line::tz>(ref<TZ>) + lit<"] "> +
             assign<&ast::Line::request>(ref<LITERAL>) + lit<" "> +
             assign<&ast::Line::statuscode>(ref<WORD>) + lit<" "> +
             assign<&ast::Line::bytes>(ref<WORD>) +
             option(lit<" "> + assign<&ast::Line::referer>(ref<LITERAL>) +
                    lit<" "> + assign<&ast::Line::useragent>(ref<LITERAL>))> {};

  static constexpr auto line = append<&ast::Log::lines>(ref<Line>);
  struct Log
      : rule<create<ast::Log> + many(option(line | ref<BLANK>) + ref<EOL>) +
             option(line | ref<BLANK>)> {};

  using entry = Log;
};

}  // namespace clf
