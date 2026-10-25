#include "audit_regressions.hpp"
// The smoke test of the language: every corpus document parses clean, a few
// are read back field by field against the corpus's own recorded parse
// trees, and a coverage document of our own exercises what the corpus never
// does (isotopes, charges, ring-closure digits, chirality tags, the
// remaining bond kinds, the wildcard atom, disconnection, and the
// terminator).

#include <cstdio>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "smiles/core/CoreServices.hpp"
#include "smiles/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "smiles: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "biphenyl.smi",   "dinitrogen.smi", "hydronium.smi", "hydroxide.smi",
    "imidazole.smi",  "mequitazine.smi", "methane.smi",   "MIC.smi",
    "nicotine.smi",   "scandium.smi",   "sulfercarbon.smi", "uranium.smi",
};

// One (bond-before, atom) pair per position of a flat chain with no
// branches: `first` carries no bond.
struct Flat {
  std::vector<std::optional<smiles::ast::Bond>> bonds;
  std::vector<smiles::ast::Atom*> atoms;
};

Flat flatten(const smiles::ast::Chain& chain) {
  Flat flat;
  flat.bonds.push_back(std::nullopt);
  flat.atoms.push_back(chain.first->atom);
  for (const smiles::ast::ChainLink* const link : chain.rest) {
    flat.bonds.push_back(link->bond);
    flat.atoms.push_back(link->atom->atom);
  }
  return flat;
}

template <class T>
std::vector<const T*> suffixes(const smiles::ast::BranchedAtom& atom) {
  std::vector<const T*> values;
  for (const auto* suffix : atom.suffixes)
    if (const auto* value = cursiv::ast_cast<T>(suffix)) values.push_back(value);
  return values;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      smiles::SmilesCoreServices, smiles::SmilesSharedCoreServices,
      smiles::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<smiles::ast::Smiles>)
      return fail(std::string(name) + ": built no Smiles");

    const auto& doc = *static_cast<const smiles::ast::Smiles*>(root);
    if (doc.chain == nullptr || doc.chain->first == nullptr)
      return fail(std::string(name) + ": chain holds no atom");
  }

  // dinitrogen.smi: `N#N` -- reference tree
  // `(chain (branched_atom (atom N)) (bond #) (branched_atom (atom N)))`:
  // two atoms, one triple bond between them.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/dinitrogen.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const Flat flat = flatten(*doc.chain);
    if (flat.atoms.size() != 2) return fail("dinitrogen: expected 2 atoms");
    if (flat.atoms[0]->symbol != "N" || flat.atoms[1]->symbol != "N")
      return fail("dinitrogen: wrong symbols");
    if (!flat.bonds[1].has_value() ||
        *flat.bonds[1] != smiles::ast::Bond::Triple)
      return fail("dinitrogen: expected a triple bond");
  }

  // MIC.smi: `CN=C=O` -- reference tree has C, N, =, C, =, O: the C-N bond
  // is implicit (organic subset default), the other two are explicit `=`.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/MIC.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const Flat flat = flatten(*doc.chain);
    if (flat.atoms.size() != 4) return fail("MIC: expected 4 atoms");
    if (flat.bonds[1].has_value()) return fail("MIC: C-N bond should be implicit");
    if (!flat.bonds[2].has_value() ||
        *flat.bonds[2] != smiles::ast::Bond::Double)
      return fail("MIC: expected a double bond at position 2");
    if (!flat.bonds[3].has_value() ||
        *flat.bonds[3] != smiles::ast::Bond::Double)
      return fail("MIC: expected a double bond at position 3");
  }

  // hydronium.smi: `[OH3+]` -- one bracket atom, symbol O, hcount 3, charge
  // +1 (the bare sign).
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/hydronium.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const smiles::ast::Atom& atom = *doc.chain->first->atom;
    if (!atom.bracket) return fail("hydronium: expected a bracket atom");
    if (atom.symbol != "O") return fail("hydronium: wrong symbol");
    if (!atom.hcount.has_value() || *atom.hcount != "H3")
      return fail("hydronium: expected hcount 3");
    if (!atom.charge.has_value() || *atom.charge != "+")
      return fail("hydronium: expected charge +1");
  }

  // hydroxide.smi: `[OH-]` -- hcount with no digit (1), charge -1.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/hydroxide.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const smiles::ast::Atom& atom = *doc.chain->first->atom;
    if (!atom.hcount.has_value() || *atom.hcount != "H")
      return fail("hydroxide: expected hcount 1");
    if (!atom.charge.has_value() || *atom.charge != "-")
      return fail("hydroxide: expected charge -1");
  }

  // biphenyl.smi: `c1ccccc1-c2ccccc2` -- 12 atoms, one explicit `-` bond
  // between the two rings, four ring-closure digits total.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/biphenyl.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const Flat flat = flatten(*doc.chain);
    if (flat.atoms.size() != 12) return fail("biphenyl: expected 12 atoms");
    int explicitBonds = 0;
    for (const auto& b : flat.bonds)
      if (b.has_value()) ++explicitBonds;
    if (explicitBonds != 1) return fail("biphenyl: expected exactly one explicit bond");
    const smiles::ast::Chain& chain = *doc.chain;
    int ringbonds = static_cast<int>(suffixes<smiles::ast::RingBond>(*chain.first).size());
    for (const smiles::ast::ChainLink* const link : chain.rest)
      ringbonds += static_cast<int>(suffixes<smiles::ast::RingBond>(*link->atom).size());
    if (ringbonds != 4) return fail("biphenyl: expected 4 ring-closure digits");
  }

  // mequitazine.smi has one parenthesised branch: `C(C2)`. Confirm the
  // branch survives with its own chain.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/mequitazine.smi");
    const cursiv::ParseResult r = language.language.parser->parse(text);
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(r.value);
    const smiles::ast::Chain& chain = *doc.chain;
    bool sawBranch = false;
    if (!suffixes<smiles::ast::Branch>(*chain.first).empty()) sawBranch = true;
    for (const smiles::ast::ChainLink* const link : chain.rest)
      if (!suffixes<smiles::ast::Branch>(*link->atom).empty()) sawBranch = true;
    if (!sawBranch) return fail("mequitazine: expected a branch somewhere in the chain");
  }

  // The coverage document: constructs the 12-file corpus never exercises.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/coverage.smi");
    if (text.empty()) return fail("coverage.smi: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("coverage.smi: does not parse clean");
    if (!result.diagnostics.empty()) return fail("coverage.smi: has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<smiles::ast::Smiles>)
      return fail("coverage.smi: built no Smiles");
    const auto& doc = *static_cast<const smiles::ast::Smiles*>(root);

    if (!doc.hasTerminator) return fail("coverage.smi: expected a trailing terminator");

    const Flat flat = flatten(*doc.chain);
    if (flat.atoms.size() != 22)
      return fail("coverage.smi: expected 22 atoms, got " +
                  std::to_string(flat.atoms.size()));

    // [13CH4:1]: isotope, hcount and class on the first atom.
    const smiles::ast::Atom& first = *flat.atoms[0];
    if (!first.isotope.has_value() || *first.isotope != "13")
      return fail("coverage.smi: expected isotope 13");
    if (!first.hcount.has_value() || *first.hcount != "H4")
      return fail("coverage.smi: expected hcount 4");
    if (!first.atomClass.has_value() || *first.atomClass != "1")
      return fail("coverage.smi: expected class 1");

    // the `.` between the first atom and the wildcard: Bond::Dot at
    // position 1.
    if (!flat.bonds[1].has_value() || *flat.bonds[1] != smiles::ast::Bond::Dot)
      return fail("coverage.smi: expected a dot at position 1");

    // `*` and `[*]`: two wildcard atoms, one plain and one bracketed.
    if (flat.atoms[1]->symbol != "*" || flat.atoms[1]->bracket)
      return fail("coverage.smi: expected a plain wildcard atom");
    if (flat.atoms[2]->symbol != "*" || !flat.atoms[2]->bracket)
      return fail("coverage.smi: expected a bracket wildcard atom");

    // [Fe+2], [Fe++], [O--]: the three charge spellings.
    if (!flat.atoms[3]->charge.has_value() || *flat.atoms[3]->charge != "+2")
      return fail("coverage.smi: expected charge +2 (digit form)");
    if (!flat.atoms[4]->charge.has_value() || *flat.atoms[4]->charge != "++")
      return fail("coverage.smi: expected charge +2 (++)");
    if (!flat.atoms[5]->charge.has_value() || *flat.atoms[5]->charge != "--")
      return fail("coverage.smi: expected charge -2 (--)");

    // [se], [as]: aromatic symbols with no organic-subset spelling.
    if (flat.atoms[6]->symbol != "se") return fail("coverage.smi: expected symbol se");
    if (flat.atoms[7]->symbol != "as") return fail("coverage.smi: expected symbol as");

    // C%12C%12: both ring bonds read number 12.
    if (flat.atoms[8]->symbol != "C" || flat.atoms[9]->symbol != "C")
      return fail("coverage.smi: expected two plain carbons for the ring pair");
    const smiles::ast::BranchedAtom* ring1 = nullptr;
    const smiles::ast::BranchedAtom* ring2 = nullptr;
    {
      const smiles::ast::Chain& chain = *doc.chain;
      std::vector<const smiles::ast::BranchedAtom*> flatAtoms;
      flatAtoms.push_back(chain.first);
      for (const smiles::ast::ChainLink* const link : chain.rest)
        flatAtoms.push_back(link->atom);
      ring1 = flatAtoms[8];
      ring2 = flatAtoms[9];
    }
    if (suffixes<smiles::ast::RingBond>(*ring1).size() != 1 || suffixes<smiles::ast::RingBond>(*ring1)[0]->number != "%12")
      return fail("coverage.smi: expected ring bond 12 on the first ring atom");
    if (suffixes<smiles::ast::RingBond>(*ring2).size() != 1 || suffixes<smiles::ast::RingBond>(*ring2)[0]->number != "%12")
      return fail("coverage.smi: expected ring bond 12 on the second ring atom");

    // C$C:C/C=C\C (atoms 10-15, bonds[10] is the implicit link from the
    // ring pair before it): the four bond kinds the earlier checks never
    // used.
    using Bond = smiles::ast::Bond;
    if (flat.bonds[10].has_value())
      return fail("coverage.smi: expected an implicit bond before the bond-kind run");
    if (!flat.bonds[11].has_value() || *flat.bonds[11] != Bond::Quad)
      return fail("coverage.smi: expected a quadruple bond");
    if (!flat.bonds[12].has_value() || *flat.bonds[12] != Bond::Aromatic)
      return fail("coverage.smi: expected an aromatic bond");
    if (!flat.bonds[13].has_value() || *flat.bonds[13] != Bond::Up)
      return fail("coverage.smi: expected an up bond");
    if (!flat.bonds[14].has_value() || *flat.bonds[14] != Bond::Double)
      return fail("coverage.smi: expected a double bond");
    if (!flat.bonds[15].has_value() || *flat.bonds[15] != Bond::Down)
      return fail("coverage.smi: expected a down bond");

    // The six chirality tags, raw text kept as written.
    const char* const wantChiral[] = {"@@", "@TH1", "@AL2",
                                      "@SP3", "@TB5", "@OH77"};
    for (int i = 0; i < 6; ++i) {
      const smiles::ast::Atom& atom = *flat.atoms[16 + i];
      if (!atom.chiral.has_value() || *atom.chiral != wantChiral[i])
        return fail(std::string("coverage.smi: expected chiral ") + wantChiral[i]);
    }
  }

  std::puts("smiles: the corpus and the coverage document parse");
  return 0;
}
