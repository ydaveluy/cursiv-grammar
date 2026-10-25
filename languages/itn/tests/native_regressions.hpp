#pragma once

#include <cstdio>
#include <utility>
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace native_regressions {
inline bool run() {
  auto language = cursiv::create_language<itn::ItnCoreServices,
      itn::ItnSharedCoreServices, itn::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "ITN regression: %s\n", message); ok = false; }
  };
  const auto result = language.language.parser->parse(
      "\"Itinéraire\"\n~metadata\n\ufeff+1|-2||4\r\n3|4|  café, λ | b  |2|\n");
  const auto* route = cursiv::ast_cast<itn::ast::Itinerary>(result.value);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "named Unicode itinerary and optional final separator");
  check(route && route->names.size() == 1 && route->names[0] == "\"Itinéraire\"" && route->lines.size() == 2, "name and ordered positions retained");
  if (route && route->lines.size() == 2) {
    check(route->lines[0]->longitude == 1 && route->lines[0]->latitude == -2 && route->lines[0]->descr.empty() && route->lines[0]->flag == "4", "signed coordinates and empty description");
    check(route->lines[1]->descr == "  café, λ | b  " && route->lines[1]->flag == "2", "description spaces, punctuation and embedded pipe retained");
  }
  const auto inlineResult = language.language.parser->parse("1|2|a|0|3|4|b|2|");
  const auto* inlineRoute = cursiv::ast_cast<itn::ast::Itinerary>(inlineResult.value);
  check(inlineResult.accepted && !inlineResult.cst().recovered() && inlineRoute && inlineRoute->lines.size() == 1 && inlineRoute->lines[0]->descr == "a|0|3|4|b", "one physical line is one position");
  const auto rawFlag = language.language.parser->parse("1|2|a|00|");
  const auto* flagged = cursiv::ast_cast<itn::ast::Itinerary>(rawFlag.value);
  check(flagged && flagged->lines.size() == 1 && flagged->lines[0]->flag == "00", "invalid flag spelling remains available to validation");
  for (const auto& [text, code] : {
      std::pair{"1|2|a|00|", "itn.flag"},
      std::pair{"18000001|2|a|0|", "itn.longitude"},
      std::pair{"1|-9000001|a|0|", "itn.latitude"}}) {
    const auto parsed = language.language.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "invalid values retain their AST");
    const auto validation = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == code) found = true;
    check(found, code);
  }
  return ok;
}
}  // namespace native_regressions
