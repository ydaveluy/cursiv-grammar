#pragma once

#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "racketisl/core/ast.hpp"

namespace racketisl_reader_boundaries {
namespace ast = racketisl::ast;
inline void run(const auto& language) {
  struct Case { const char* source; bool valid; std::size_t forms; };
  // Racket 8.18 read-syntax, with the actual reader profile; no expansion.
  for (const Case& test : std::initializer_list<Case>{
      {"#t|name|", false, 0},
      {"#false\\name", false, 0},
      {"#true| |", false, 0},
      {"#t #f", true, 2},
      {"#t(name)", true, 2},
      {"#f\"x\"", true, 2},
      {"#\\aͅ", false, 0},
      {"#\\ͅa", false, 0},
      {"#\\ͅ", true, 1},
      {"#\\a1", true, 2},
      {"#\\u03bbZZ", true, 2},
      {"#\\newline1", true, 2},
      {"#<<\nbody\n\n", true, 1},
      {"#<<END\nbody\nEND\n", true, 1},
      {"a ;hidden\rb\nc", true, 2},
      {"a ;hiddenb\nc", true, 2},
      {"a ;hidden b\nc", true, 2},
      {"a ;hidden b\nc", true, 2},
      {"a ;hidden\r\nb", true, 2},
      {"#t\\", false, 0},
  }) {
    const std::string source = test.source;
    const auto parsed = language.parser->parse(source);
    const auto validation = cursiv::test::validationHelper(language, source);
    const bool valid = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty() && validation.diagnostics.empty();
    if (valid != test.valid) std::fprintf(stderr, "reader boundary: %s\n", test.source);
    CHECK(valid == test.valid);
    if (!test.valid) continue;
    const auto* root = parsed.template value_as<ast::Program>();
    CHECK(root && root->forms.size() == test.forms);
    if (!root) continue;
    for (const auto* item : root->forms) {
      CHECK(item && item->parent == root);
      CHECK(item && parsed.cst().contains(root->cstNodeId, item->cstNodeId));
    }
  }

  {
    const std::string source = "#<<\nbody\n\n42";
    const auto parsed = language.parser->parse(source);
    const auto* root = parsed.template value_as<ast::Program>();
    CHECK(parsed.accepted && !parsed.cst().recovered() && root && root->forms.size() == 2);
    const auto* value = root && !root->forms.empty() ? cursiv::ast_cast<ast::String>(root->forms[0]) : nullptr;
    CHECK(value && value->text == "#<<\nbody\n" && value->parent == root);
  }

}
}  // namespace racketisl_reader_boundaries
