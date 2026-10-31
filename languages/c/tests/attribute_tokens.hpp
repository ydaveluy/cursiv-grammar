#pragma once

#include <cstdio>
#include <string_view>

#include "c/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace c_attribute_tokens {

struct Case {
  const char* name;
  bool clean;
  std::string_view source;
};

inline constexpr Case cases[] = {
  {"brackets", true, R"C23ATTR([[vendor::probe([1, (2)])]] int n;
)C23ATTR"},
  {"braces", true, R"C23ATTR([[vendor::probe({1, (2)})]] int n;
)C23ATTR"},
  {"mixed_groups", true, R"C23ATTR([[vendor::probe(({[1]}), [({2})])]] int n;
)C23ATTR"},
  {"punctuators", true, R"C23ATTR([[vendor::probe(a->b, ++a, --b, a / b % 2, a << 1, a >> 1, a | b, a ^ b, a && b, a || b, a <= b, a != b, a += b, a /= b, a <<= 1, a ? b : c;)]] int n;
)C23ATTR"},
  {"digraph_groups", true, R"C23ATTR([[vendor::probe(<:1, (2):>, <%3%>)]] int n;
)C23ATTR"},
  {"quoted_delimiters", true, R"C23ATTR([[vendor::probe(")]}", ')', '[', "/* ] } */", L'{')]] int n;
)C23ATTR"},
  {"comment_delimiters", true, R"C23ATTR([[vendor::probe([1 /* ) ] } */], {2 // ) ] }
})]] int n;
)C23ATTR"},
  {"gnu_existing_attribute", true, R"C23ATTR(__attribute__((aligned(8), unused)) int n;
)C23ATTR"},
  {"mismatched_bracket", false, R"C23ATTR([[vendor::probe([1})]] int n;
)C23ATTR"},
  {"unclosed_brace", false, R"C23ATTR([[vendor::probe({1)]] int n;
)C23ATTR"},
  {"unclosed_string", false, R"C23ATTR([[vendor::probe(")]})]] int n;
)C23ATTR"},
  {"unclosed_comment", false, R"C23ATTR([[vendor::probe(/* )]] int n;
)C23ATTR"},
};

inline bool run(const cursiv::Parser& parser) {
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    const auto* unit = cursiv::ast_cast<c::ast::TranslationUnit>(result.value);
    const auto* declaration = unit && unit->items.size() == 1
        ? cursiv::ast_cast<c::ast::Declaration>(unit->items[0]) : nullptr;
    const auto* item = declaration && declaration->declarators.size() == 1
        ? declaration->declarators[0] : nullptr;
    const auto* declarator = item ? item->declarator : nullptr;
    const bool tree = declaration && declaration->parent == unit &&
                      declaration->specifiers &&
                      declaration->specifiers->parent == declaration &&
                      item && item->parent == declaration && declarator &&
                      declarator->name == "n" && declarator->parent == item;
    if (clean != test.clean || (test.clean && !tree)) {
      std::fprintf(stderr,
                   "c attribute tokens: %s clean=%d expected=%d tree=%d "
                   "accepted=%d recovered=%d diagnostics=%zu source=[%.*s]\n",
                   test.name, clean, test.clean, tree, result.accepted,
                   result.cst().recovered(), result.diagnostics.size(),
                   int(test.source.size()), test.source.data());
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "c attribute diagnostic: kind=%d begin=%u end=%u\n",
                     int(diagnostic.kind), unsigned(diagnostic.begin),
                     unsigned(diagnostic.end));
      return false;
    }
  }
  return true;
}

}  // namespace c_attribute_tokens
