#pragma once

#include <cstddef>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "cmake/core/ast.hpp"

namespace cmake {

inline std::string_view argument_text(std::string_view text,
                                      std::pmr::memory_resource* memory,
                                      bool quoted = false) {
  const bool normalize = text.find("\r\n") != text.npos;
  const bool continuation = quoted && text.find("\\\n") != text.npos;
  if (!normalize && !continuation) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (quoted && text[i] == '\\' && i + 1 < text.size()) {
      if (text[i + 1] == '\n') { ++i; continue; }
      if (text[i + 1] == '\r' && i + 2 < text.size() &&
          text[i + 2] == '\n') { i += 2; continue; }
      room[n++] = text[i++];
    }
    if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
      ++i;
    room[n++] = text[i];
  }
  return {room.data(), n};
}

inline constexpr auto unquote = [](std::string_view t,
                                    std::pmr::memory_resource* memory) {
  return argument_text(t.substr(1, t.size() - 2), memory, true);
};

inline constexpr auto unbracket = [](std::string_view t,
                                      std::pmr::memory_resource* memory) {
  std::size_t n = 1;
  while (t[n] == '=') ++n;
  auto body = t.substr(n + 1, t.size() - 2 * (n + 1));
  if (body.starts_with("\r\n")) body.remove_prefix(2);
  else if (body.starts_with('\n')) body.remove_prefix(1);
  return argument_text(body, memory);
};

struct Cmake : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NEWLINE
      : terminal<some((lit<"\r"> + option(lit<"\n">)) | lit<"\n">)> {};

  struct BracketRun {};
  static constexpr auto bracketBody =
      through(lit<"["> + mark<BracketRun>(many(cr<"=">)) + lit<"[">, lit<"]"> + same<BracketRun> + lit<"]">);

  static constexpr auto bracketOpen = lit<"["> + many(cr<"=">) + lit<"[">;
  struct COMMENT
      : terminal<lit<"#"> +
                 (bracketBody | (!bracketOpen + many(cr<R"(^\n)">)))> {};

  struct ID : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_">)> {};

  static constexpr auto escape = lit<"\\"> + cr<R"(^\n)">;
  static constexpr auto unquotedChar = escape | cr<R"(^ \t\r\n()#"\\[=)">;
  static constexpr auto makeVariable =
      lit<"$("> + many(cr<"A-Za-z0-9_">) + lit<")">;
  static constexpr auto legacyQuoted =
      lit<"\""> + many(makeVariable | unquotedChar | cr<R"( \t[=)">) + lit<"\"">;
  static constexpr auto legacy = makeVariable | unquotedChar | legacyQuoted;
  struct UNQUOTED
      : terminal<as<[](std::string_view t, std::pmr::memory_resource* memory) {
        return argument_text(t, memory);
      }>(((makeVariable | unquotedChar | lit<"="> |
           (lit<"["> + many(cr<"=">) + legacy)) +
          many(legacy | cr<"[=">)) |
         lit<"[">)> {};

  struct QUOTED
      : terminal<as<unquote>(
            lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {
  };

  struct BRACKET : terminal<as<unbracket>(bracketBody)> {};

  struct UnquotedArgument
      : rule<assign<&ast::UnquotedArgument::text>(ref<UNQUOTED>)> {};
  struct QuotedArgument
      : rule<assign<&ast::QuotedArgument::text>(ref<QUOTED>)> {};
  struct BracketArgument
      : rule<assign<&ast::BracketArgument::text>(ref<BRACKET>)> {};

  struct CompoundArgument;
  static constexpr auto argument =
      ref<BracketArgument> | ref<UnquotedArgument> |
      ref<QuotedArgument> | ref<CompoundArgument>;

  struct CompoundArgument
      : rule<create<ast::CompoundArgument>(
            lit<"("> +
            many(append<&ast::CompoundArgument::arguments>(argument)) +
            lit<")">)> {};

  struct CommandInvocation
      : rule<create<ast::CommandInvocation>(
            assign<&ast::CommandInvocation::name>(ref<ID>) + lit<"("> +
            many(append<&ast::CommandInvocation::arguments>(argument)) +
            lit<")">)> {};

  struct File : rule<create<ast::File>(option(lit<"\xEF\xBB\xBF">) + many(
                    append<&ast::File::commands>(ref<CommandInvocation>)))> {};

  using entry = File;
  using ignored = list<WS, NEWLINE>;
  using hidden = COMMENT;
};

}  // namespace cmake
