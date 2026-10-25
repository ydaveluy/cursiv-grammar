#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xyz/core/ast.hpp"
#include "xyz/core/Number.hpp"

namespace xyz {

struct Xyz : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct COUNT : terminal<some(cr<"0-9_+-.eE">)> {};
  struct NUM
      : terminal<option(cr<"+-">) +
                     (((some(d) + option(lit<"."> + many(d))) |
                       (lit<"."> + some(d))) +
                          option(cr<"eE"> + option(cr<"+-">) + some(d)) |
                      cursiv::Lit<"nan", true>{} |
                      (cursiv::Lit<"inf", true>{} +
                       option(cursiv::Lit<"inity", true>{}))),
                 cursiv::text_conv<coordinate_value>> {};
  struct ATOMNAME : terminal<some(cr<"A-Za-z">)> {};
  struct EXTRA : terminal<some(cr<R"(^ \t\r\n)">)> {};
  struct DESC : terminal<some(cr<R"(^\r\n)">)> {};

  struct AtomRule
      : rule<assign<&ast::Atom::element>(ref<ATOMNAME>) +
             assign<&ast::Atom::x>(ref<NUM>) +
             assign<&ast::Atom::y>(ref<NUM>) +
             assign<&ast::Atom::z>(ref<NUM>) +
             many(append<&ast::Atom::extra>(ref<EXTRA>)) + line_end> {};

  struct FrameRule
      : rule<assign<&ast::Frame::count>(ref<COUNT>) + ref<EOL> +
             option(assign<&ast::Frame::description>(ref<DESC>)) + line_end +
             many(append<&ast::Frame::atoms>(ref<AtomRule>))> {};

  struct DocRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::frames>(ref<FrameRule>))> {};

  using entry = DocRule;
  using ignore = WS;
};

}  // namespace xyz
