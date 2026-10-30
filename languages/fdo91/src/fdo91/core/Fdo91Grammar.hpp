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
  struct STRING : terminal<as<unquote>(
                      lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) +
                      lit<"\"">)> {};

  struct Ident : rule<assign<&ast::Ident::text>(ref<ID>)> {};
  struct Number : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct Hex : rule<assign<&ast::Hex::text>(ref<HEX>)> {};
  struct Gid : rule<assign<&ast::Gid::text>(ref<GID>)> {};

  struct Atom;
  struct AtomGroup
      : rule<some(append<&ast::AtomGroup::atoms>(ref<Atom>))> {};
  struct NestedGroup
      : rule<lit<"<"> + many(append<&ast::AtomGroup::atoms>(ref<Atom>)) +
             lit<">">> {};

  static constexpr auto literal =
      ref<Ident> | ref<Number> | ref<STRING> | ref<Gid> | ref<Hex>;
  struct Piped
      : rule<append<&ast::Piped::parts>(literal) +
             some(lit<"|"> + append<&ast::Piped::parts>(literal))> {};
  static constexpr auto arg =
      ref<Piped> | literal | ref<NestedGroup> | ref<AtomGroup>;
  static constexpr auto arguments =
      flag<&ast::Atom::bracketed>(lit<"<">) +
      option(some(append<&ast::Atom::args>(arg), lit<",">)) + lit<">">;
  static constexpr auto command = assign<&ast::Atom::command>(ref<ID>);

  struct Atom
      : rule<(flag<&ast::Atom::atomPrefix>(lit<"atom$">) + command +
              option(arguments | some(append<&ast::Atom::args>(literal)))) |
             (command + option(arguments))> {};

  struct Doc : rule<many(append<&ast::Doc::atoms>(ref<Atom>))> {};

  using entry = Doc;
  using ignored = list<WS, COMMENT>;
};

}  // namespace fdo91
