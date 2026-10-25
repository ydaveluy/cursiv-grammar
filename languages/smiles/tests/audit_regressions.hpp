#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "smiles/core/CoreServices.hpp"
#include "smiles/core/ast.hpp"
#include "smiles/generated/Smiles/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = smiles::generated::Smiles::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = smiles::ast;
  auto r = parser->parse("C\t");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "smiles valid syntax regression");

  const auto language = cursiv::create_language<smiles::SmilesCoreServices,
      smiles::SmilesSharedCoreServices, smiles::install_core_module>();
  for (const char* symbol : {"Nh", "Mc", "Ts", "Og", "Uut", "b", "si", "te", "#6"}) {
    const std::string text = "[" + std::string(symbol) + "]";
    auto parsed = parser->parse(text);
    auto* root = as<ast::Smiles>(parsed.value);
    check(parsed.accepted && !parsed.cst().recovered() && root && root->chain &&
              root->chain->first->atom->symbol == symbol,
          "bracket symbols retain their source spelling");
    check(cursiv::test::validationHelper(language.language, text).diagnostics.empty(),
          "known bracket symbol validates");
  }
  for (const char* text : {"", " C  ", "C(C)1CC1", "C%(12345)CC%(12345)",
                           "N->[Cu]", "[Cu]<-N", "C~C", "[CH12+0]", "[C-0]",
                           "N[C@ TH1](C)(O)F", "C/1CC\\1"}) {
    const auto parsed = parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
          "native SMILES syntax parses");
    check(cursiv::test::validationHelper(language.language, text).diagnostics.empty(),
          "native SMILES syntax validates");
  }
  {
    const auto parsed = parser->parse("C(C)1CC1");
    const auto* root = as<ast::Smiles>(parsed.value);
    const auto* first = root && root->chain ? root->chain->first : nullptr;
    check(first && first->suffixes.size() == 2 &&
              cursiv::ast_cast<ast::Branch>(first->suffixes[0]) &&
              cursiv::ast_cast<ast::RingBond>(first->suffixes[1]),
          "branch and ring suffix order is retained");
  }
  {
    const auto parsed = parser->parse("[13CH12+0:42]");
    const auto* root = as<ast::Smiles>(parsed.value);
    const auto* atom = root && root->chain ? root->chain->first->atom : nullptr;
    check(atom && atom->isotope && *atom->isotope == "13" && atom->hcount &&
              *atom->hcount == "H12" && atom->charge && *atom->charge == "+0" &&
              atom->atomClass && *atom->atomClass == "42",
          "numeric atom attributes retain their exact spelling");
  }
  struct Invalid { std::string text; std::string_view code; std::string_view span; };
  for (const auto& item : {
      Invalid{"[Xx]", "smiles.symbol", "Xx"},
      Invalid{"[65536C]", "smiles.isotope", "65536"},
      Invalid{"[CH256]", "smiles.hcount", "H256"},
      Invalid{"[C+1000]", "smiles.charge", "+1000"},
      Invalid{"[C:2147483648]", "smiles.atomClass", "2147483648"},
      Invalid{"[C@SP4]", "smiles.chiral", "@SP4"},
      Invalid{"C%(123456)", "smiles.ringNumber", "%(123456)"},
      Invalid{"C1", "smiles.ringUnclosed", "1"},
      Invalid{"C11", "smiles.ringSelf", "1"},
      Invalid{"C1C1", "smiles.ringDuplicate", "1"},
      Invalid{"[C+++]", "smiles.charge", "+++"}}) {
    const auto parsed = parser->parse(item.text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
          "invalid molecular data stays parseable");
    const auto validation = cursiv::test::validationHelper(language.language, item.text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == item.code &&
          std::string_view(item.text).substr(diagnostic.range.begin,
              diagnostic.range.end - diagnostic.range.begin) == item.span) found = true;
    check(found, "runtime molecular diagnostic points to the invalid attribute");
  }
  return ok;
}
} // namespace audit_regressions
