#pragma once

#include <algorithm>
#include <cstdio>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreServices.hpp"
#include "morsecode/core/ast.hpp"

inline int morse_table_regressions(const cursiv::CoreServices& services) {
  struct Case {
    std::string_view source;
    std::vector<std::uint8_t> decoded;
  };
  int failures = 0;
  for (const Case& c :
       std::vector<Case>{{".-.-.- --..-- ..--.. .--.-.", {'.', ',', '?', '@'}},
                         {"-.--. -.--.- -..-. -...-", {'(', ')', '/', '='}},
                         {"..-- .-.- ---.", {0xDC, 0xC4, 0xD6}},
                         {"...-.- -...-.- ...-. .-... -.-.- .-.-..",
                          {'<', '>', '!', '&', '^', '~'}},
                         {". ........ ..-..", {'e', '\b', 0xC9}}}) {
    const auto result = services.parser->parse(c.source);
    const auto* message =
        cursiv::ast_cast<morsecode::ast::Message>(result.value);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !message || message->parent ||
        result.nodes.size() != 1 || result.cst().text() != c.source ||
        !std::equal(message->letters.begin(), message->letters.end(),
                    c.decoded.begin(), c.decoded.end())) {
      ++failures;
    }
  }
  if (failures)
    std::fprintf(stderr, "morsecode: %d decoded table regression(s)\n",
                 failures);
  return failures;
}
