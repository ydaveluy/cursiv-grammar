#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xyz/core/ast.hpp"
#include "xyz/core/Number.hpp"

namespace xyz {

struct Xyz : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<eol> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct COUNT : terminal<some(cr<"0-9_+-.eE">)> {};
  static constexpr auto digits = d + many(option(lit<"_">) + d);
  static constexpr auto field_end = &(cr<R"( \t\r\n)"> | eof);
  static constexpr auto finite =
      ((digits + option(lit<"."> + option(digits))) | (lit<"."> + digits)) +
      option(cr<"eE"> + option(cr<"+-">) + digits);
  struct NUM
      : terminal<as<coordinate_value>(
            option(cr<"+-">) +
            (finite |
             cursiv::Lit<"nan", true>{} |
             (cursiv::Lit<"inf", true>{} +
              option(cursiv::Lit<"inity", true>{}))) + field_end)> {};
  struct ATOMNAME : terminal<some(cr<"A-Za-z">) + &cr<R"( \t)">> {};
  struct EXTRA : terminal<some(cr<R"(^ \t\r\n)">)> {};
  struct DESC : terminal<some(cr<R"(^\r\n)">)> {};

  struct Atom
      : rule<assign<&ast::Atom::element>(ref<ATOMNAME>) +
             assign<&ast::Atom::x>(ref<NUM>) +
             assign<&ast::Atom::y>(ref<NUM>) +
             assign<&ast::Atom::z>(ref<NUM>) +
             many(append<&ast::Atom::extra>(ref<EXTRA>)) + line_end> {};

  struct Frame
      : rule<assign<&ast::Frame::count>(ref<COUNT>) + ref<EOL> +
             option(assign<&ast::Frame::description>(ref<DESC>)) + line_end +
             many(append<&ast::Frame::atoms>(ref<Atom>))> {};

  struct Doc
      : rule<create<ast::Document> +
             many(append<&ast::Document::frames>(ref<Frame>))> {};

  using entry = Doc;
  using ignored = WS;
};

}  // namespace xyz
