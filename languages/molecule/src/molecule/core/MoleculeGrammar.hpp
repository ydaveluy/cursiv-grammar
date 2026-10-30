// A text formula: symbol runs made of elements, parenthesised groups and
// bracketed coordination entities, each optionally
// counted, several formulas joined by a middle dot for a hydrate.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "molecule/core/ast.hpp"

namespace molecule {

struct Molecule : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct COUNT : terminal<as<std::uint32_t>(some(d))> {};

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

  struct Element : rule<assign<&ast::Element::name>(ref<ELEMENT>)> {};

  struct Group;
  struct Ion;

  static constexpr auto symbol =
      ref<Element> | ref<Group> | ref<Ion>;

  struct Structure
      : rule<assign<&ast::Structure::symbol>(symbol) +
             option(assign<&ast::Structure::count>(ref<COUNT>))> {};

  struct Group
      : rule<lit<"("> +
             some(append<&ast::Group::structures>(ref<Structure>)) +
             lit<")">> {};

  struct Ion
      : rule<lit<"["> +
             some(append<&ast::Ion::structures>(ref<Structure>)) +
             lit<"]">> {};

  // A hydrate coefficient multiplies the whole component, including every
  // structure in H2O, rather than only its first element H2.
  struct Part
      : rule<option(assign<&ast::Part::multiplier>(ref<COUNT>)) +
             some(append<&ast::Part::structures>(ref<Structure>))> {};

  struct MoleculeRule
      : rule<some(append<&ast::Molecule::parts>(ref<Part>), lit<"·">)> {
  };

  using entry = MoleculeRule;
  using ignored = WS;
};

}  // namespace molecule
