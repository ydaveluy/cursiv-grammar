#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "smiles/core/ast.hpp"

namespace smiles {

inline constexpr auto bond_kind = [](std::string_view t) -> ast::Bond {
  if (t == "->") return ast::Bond::DativeRight;
  if (t == "<-") return ast::Bond::DativeLeft;
  switch (t[0]) {
    case '~':
      return ast::Bond::Any;
    case '=':
      return ast::Bond::Double;
    case '#':
      return ast::Bond::Triple;
    case '$':
      return ast::Bond::Quad;
    case ':':
      return ast::Bond::Aromatic;
    case '/':
      return ast::Bond::Up;
    case '\\':
      return ast::Bond::Down;
    default:
      return ast::Bond::Single;
  }
};

struct Smiles : cursiv::GrammarSpec {
  struct BOND
      : terminal<as<bond_kind>(lit<"-"> | lit<"="> | lit<"#"> | lit<"$"> |
                               lit<":"> | lit<"/"> | lit<"\\"> | lit<"\\\\"> |
                               lit<"->"> | lit<"<-"> | lit<"~">)> {};

  struct OrganicAtomSymbol
      : terminal<lit<"B"> | lit<"C"> | lit<"N"> | lit<"O"> | lit<"S"> |
                     lit<"P"> | lit<"F"> | lit<"Cl"> | lit<"Br"> | lit<"I"> |
                     lit<"b"> | lit<"c"> | lit<"n"> | lit<"o"> | lit<"s"> |
                     lit<"p"> | lit<"*">> {};

  struct ElementSymbol : terminal<
      (cr<"A-Z"> + repeat<0, 2>(cr<"a-z">)) |
      (lit<"'"> + cr<"A-Z"> + repeat<0, 2>(cr<"a-z">) + lit<"'">)> {};
  struct AromaticSymbol : terminal<cr<"a-z"> + option(cr<"a-z">)> {};
  struct AtomicNumber : terminal<lit<"#"> + some(d)> {};

  static constexpr auto bracket_symbol =
      ref<ElementSymbol> | ref<AromaticSymbol> | ref<AtomicNumber> | lit<"*">;

  struct NUMBER : terminal<some(d)> {};
  struct CHIRAL : terminal<lit<"@@"> | (lit<"@"> + option(
      many(cr<" '">) +
      (lit<"TH"> | lit<"AL"> | lit<"SP"> | lit<"TB"> | lit<"OH">) + many(d)))> {};
  struct HCOUNT : terminal<lit<"H"> + many(d)> {};
  struct CHARGE : terminal<cr<"+-"> + many(cr<"+-"> | d)> {};
  struct RINGNUM : terminal<d | (lit<"%"> + d + d) |
                           (lit<"%("> + some(d) + lit<")">)> {};

  struct Atom
      : rule<assign<&ast::Atom::symbol>(ref<OrganicAtomSymbol>) |
             (flag<&ast::Atom::bracket>(lit<"[">) +
              option(assign<&ast::Atom::isotope>(ref<NUMBER>)) +
              assign<&ast::Atom::symbol>(bracket_symbol) +
              option(assign<&ast::Atom::chiral>(ref<CHIRAL>)) +
              option(assign<&ast::Atom::hcount>(ref<HCOUNT>)) +
              option(assign<&ast::Atom::charge>(ref<CHARGE>)) +
              option(lit<":"> + assign<&ast::Atom::atomClass>(ref<NUMBER>)) +
              lit<"]">)> {};

  struct RingBond
      : rule<option(assign<&ast::RingBond::bond>(ref<BOND>)) +
             assign<&ast::RingBond::number>(ref<RINGNUM>)> {};

  struct Chain;

  struct Branch
      : rule<lit<"("> +
             option(assign<&ast::Branch::bond>(ref<BOND> | as<ast::Bond::Dot>(lit<".">))) +
             assign<&ast::Branch::chain>(ref<Chain>) + lit<")">> {};

  struct BranchedAtom
      : rule<assign<&ast::BranchedAtom::atom>(ref<Atom>) +
             many(append<&ast::BranchedAtom::suffixes>(
                 ref<RingBond> | ref<Branch>))> {};

  struct ChainLink
      : rule<option(assign<&ast::ChainLink::bond>(ref<BOND> | as<ast::Bond::Dot>(lit<".">))) +
             assign<&ast::ChainLink::atom>(ref<BranchedAtom>)> {};

  struct Chain
      : rule<assign<&ast::Chain::first>(ref<BranchedAtom>) +
             many(append<&ast::Chain::rest>(ref<ChainLink>))> {};

  struct TERMINATOR : terminal<some(cr<R"( \t\r\n)">)> {};

  struct SmilesRule
      : rule<create<ast::Smiles> + option(ref<TERMINATOR>) +
             option(assign<&ast::Smiles::chain>(ref<Chain>) +
                    option(flag<&ast::Smiles::hasTerminator>(ref<TERMINATOR>)))> {
  };

  using entry = SmilesRule;
};

}  // namespace smiles
