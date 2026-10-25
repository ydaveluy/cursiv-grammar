// AOL FDO atom source.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fdo91/core/ast.hpp"

namespace fdo91 {

inline constexpr auto unquote = [](std::string_view text,
                                  std::pmr::memory_resource* memory) {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size()) {
      c = text[++i];
      switch (c) {
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case '"': break;
        default: room[size++] = '\\'; break;
      }
    }
    room[size++] = c;
  }
  return std::string_view(room.data(), size);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Fdo91 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<(lit<"#"> | lit<";">) + many(cr<R"(^\r\n)">)> {};

  struct ID
      : terminal<!(cr<"a-fA-F"> + many(cr<"0-9a-fA-F">) + lit<"x"> +
                   !cr<"a-zA-Z_0-9">) +
                 cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">)> {};
  struct NUMBER : terminal<option(lit<"-">) + some(d)> {};
  struct GID : terminal<option(lit<"-">) + some(d) + lit<"-"> + some(d) +
                        many(d | lit<"-">)> {};
  struct HEX
      : terminal<(option(lit<"-">) + d + many(cr<"0-9a-fA-F">) + cr<"xX">) |
                 (cr<"a-fA-F"> + many(cr<"0-9a-fA-F">) + lit<"x">)> {};
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};

  struct IdentRule : rule<assign<&ast::Ident::text>(ref<ID>)> {};
  struct NumberRule : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct HexRule : rule<assign<&ast::Hex::text>(ref<HEX>)> {};
  struct GidRule : rule<assign<&ast::Gid::text>(ref<GID>)> {};

  struct AtomRule;
  struct AtomGroupRule
      : rule<some(append<&ast::AtomGroup::atoms>(ref<AtomRule>))> {};
  struct NestedGroupRule
      : rule<lit<"<"> + many(append<&ast::AtomGroup::atoms>(ref<AtomRule>)) +
             lit<">">> {};

  static constexpr auto literal =
      ref<IdentRule> | ref<NumberRule> | ref<STRING> | ref<GidRule> | ref<HexRule>;
  struct PipedRule
      : rule<append<&ast::Piped::parts>(literal) +
             some(lit<"|"> + append<&ast::Piped::parts>(literal))> {};
  static constexpr auto arg =
      ref<PipedRule> | literal | ref<NestedGroupRule> | ref<AtomGroupRule>;
  static constexpr auto arguments =
      flag<&ast::Atom::bracketed>(lit<"<">) +
      option(some(append<&ast::Atom::args>(arg), lit<",">)) + lit<">">;
  static constexpr auto command = assign<&ast::Atom::command>(ref<ID>);

  struct AtomRule
      : rule<(flag<&ast::Atom::atomPrefix>(lit<"atom$">) + command +
              option(arguments | some(append<&ast::Atom::args>(literal)))) |
             (command + option(arguments))> {};

  struct DocRule : rule<many(append<&ast::Doc::atoms>(ref<AtomRule>))> {};

  using entry = DocRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace fdo91
