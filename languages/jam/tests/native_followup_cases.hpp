#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "jam/core/ast.hpp"
namespace jam_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = jam::ast;
  const auto result = language.language.parser->parse(
      "# intro\n[3/4] F#dim/G# Esus4/Db G+ | C C7 | %%\n# middle\n| Dm7-5");
  const auto* root = result.template value_as<a::Jam>();
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !root || root->sections.size() != 1 || root->sections[0]->measures.size() != 4) {
    std::fprintf(stderr, "jam native: shape/comments/sharp source rejected\n");return false;
  }
  const auto& measures = root->sections[0]->measures;
  if (!measures[0]->meter || measures[0]->meter->numerator != 3 ||
      measures[0]->meter->denominator != 4 || measures[0]->items.size() != 3 ||
      measures[2]->items.size() != 1 || measures[3]->items.size() != 1) return false;
  const auto* sharp = cursiv::ast_cast<a::Chord>(measures[0]->items[0]);
  const auto* sus = cursiv::ast_cast<a::Chord>(measures[0]->items[1]);
  const auto* aug = cursiv::ast_cast<a::Chord>(measures[0]->items[2]);
  const auto* repeat = cursiv::ast_cast<a::RepeatMeasure>(measures[2]->items[0]);
  const auto* altered = cursiv::ast_cast<a::Chord>(measures[3]->items[0]);
  if (!sharp || !sharp->note || sharp->note->key != "F" ||
      sharp->note->accidental != a::Accidental::sharp || sharp->quality != a::Quality::dim ||
      !sharp->bass || sharp->bass->key != "G" || sharp->bass->accidental != a::Accidental::sharp ||
      !sus || sus->quality != a::Quality::sus || sus->extension != 4 ||
      !sus->bass || sus->bass->key != "D" || sus->bass->accidental != a::Accidental::flat ||
      !aug || aug->quality != a::Quality::aug_plus ||
      !repeat || repeat->count != 2 || repeat->parent != measures[2] ||
      !altered || altered->quality != a::Quality::m || altered->extension != 7 ||
      altered->alterationSign != a::Sign::minus || altered->alteration != 5) {
    std::fprintf(stderr, "jam native: musical AST payload lost\n");return false;
  }
  const auto beats = language.language.parser->parse("C / G");
  const auto* beatRoot = beats.template value_as<a::Jam>();
  if (!beats.accepted || beats.cst().recovered() || !beats.diagnostics.empty() ||
      !beatRoot || beatRoot->sections.size() != 1 || beatRoot->sections[0]->measures.size() != 1 ||
      beatRoot->sections[0]->measures[0]->items.size() != 3 ||
      !cursiv::ast_cast<a::RepeatChord>(beatRoot->sections[0]->measures[0]->items[1])) {
    std::fprintf(stderr, "jam native: separated beats became a slash bass\n");return false;
  }
  const auto bareComments = language.language.parser->parse("#\nF#\n# end");
  if (!bareComments.accepted || bareComments.cst().recovered() || !bareComments.diagnostics.empty()) return false;
  for (const auto* source : {"[0/4] C", "[4/257] C"}) {
    const auto checked = cursiv::test::validationHelper(language.language, source);
    if (checked.diagnostics.size() != 1 || checked.diagnostics[0].code != "jam.meter") return false;
  }
  return true;
}
} // namespace jam_native_followup
