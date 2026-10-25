// The molecule language: its grammar. A Hill formula -- symbol runs made of
// elements, parenthesised groups and bracketed ions, each optionally
// counted, several formulas joined by a middle dot for a hydrate.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "molecule/core/ast.hpp"

namespace molecule {

struct Molecule : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct COUNT : terminal<some(d), std::uint32_t> {};

  // Every symbol of the periodic table up to element 118 (Og); the longest
  // alternative wins so "He" is read whole and not as "H" then a stray "e".
  struct ELEMENT
      : terminal<lit<"He"> | lit<"Li"> | lit<"Be"> | lit<"Ne"> | lit<"Na"> |
                 lit<"Mg"> | lit<"Al"> | lit<"Si"> | lit<"Cl"> | lit<"Ar"> |
                 lit<"Ca"> | lit<"Sc"> | lit<"Ti"> | lit<"Cr"> | lit<"Mn"> |
                 lit<"Fe"> | lit<"Co"> | lit<"Ni"> | lit<"Cu"> | lit<"Zn"> |
                 lit<"Ga"> | lit<"Ge"> | lit<"As"> | lit<"Se"> | lit<"Br"> |
                 lit<"Kr"> | lit<"Rb"> | lit<"Sr"> | lit<"Zr"> | lit<"Nb"> |
                 lit<"Mo"> | lit<"Tc"> | lit<"Ru"> | lit<"Rh"> | lit<"Pd"> |
                 lit<"Ag"> | lit<"Cd"> | lit<"In"> | lit<"Sn"> | lit<"Sb"> |
                 lit<"Te"> | lit<"Xe"> | lit<"Cs"> | lit<"Ba"> | lit<"La"> |
                 lit<"Ce"> | lit<"Pr"> | lit<"Nd"> | lit<"Pm"> | lit<"Sm"> |
                 lit<"Eu"> | lit<"Gd"> | lit<"Tb"> | lit<"Dy"> | lit<"Ho"> |
                 lit<"Er"> | lit<"Tm"> | lit<"Yb"> | lit<"Lu"> | lit<"Hf"> |
                 lit<"Ta"> | lit<"Re"> | lit<"Os"> | lit<"Ir"> | lit<"Pt"> |
                 lit<"Au"> | lit<"Hg"> | lit<"Tl"> | lit<"Pb"> | lit<"Bi"> |
                 lit<"Po"> | lit<"At"> | lit<"Rn"> | lit<"Fr"> | lit<"Ra"> |
                 lit<"Ac"> | lit<"Th"> | lit<"Pa"> | lit<"Np"> | lit<"Pu"> |
                 lit<"Am"> | lit<"Cm"> | lit<"Bk"> | lit<"Cf"> | lit<"Es"> |
                 lit<"Fm"> | lit<"Md"> | lit<"No"> | lit<"Lr"> | lit<"Rf"> |
                 lit<"Db"> | lit<"Sg"> | lit<"Bh"> | lit<"Hs"> | lit<"Mt"> |
                 lit<"Ds"> | lit<"Rg"> | lit<"Cn"> | lit<"Nh"> | lit<"Fl"> |
                 lit<"Mc"> | lit<"Lv"> | lit<"Ts"> | lit<"Og"> | lit<"H"> |
                 lit<"B"> | lit<"C"> | lit<"N"> | lit<"O"> | lit<"F"> |
                 lit<"P"> | lit<"S"> | lit<"K"> | lit<"V"> | lit<"Y"> |
                 lit<"I"> | lit<"W"> | lit<"U">> {};

  struct ElementRule : rule<assign<&ast::Element::name>(ref<ELEMENT>)> {};

  struct GroupRule;
  struct IonRule;

  static constexpr auto symbol =
      ref<ElementRule> | ref<GroupRule> | ref<IonRule>;

  struct StructureRule
      : rule<assign<&ast::Structure::symbol>(symbol) +
             option(assign<&ast::Structure::count>(ref<COUNT>))> {};

  struct GroupRule
      : rule<lit<"("> +
             some(append<&ast::Group::structures>(ref<StructureRule>)) +
             lit<")">> {};

  struct IonRule
      : rule<lit<"["> +
             some(append<&ast::Ion::structures>(ref<StructureRule>)) +
             lit<"]">> {};

  struct TermRule
      : rule<option(assign<&ast::Term::multiplier>(ref<COUNT>)) +
             assign<&ast::Term::structure>(ref<StructureRule>)> {};

  struct PartRule : rule<some(append<&ast::Part::terms>(ref<TermRule>))> {};

  struct MoleculeRule
      : rule<some(append<&ast::Molecule::parts>(ref<PartRule>), lit<"·">)> {
  };

  using entry = MoleculeRule;
  using ignore = WS;
};

}  // namespace molecule
