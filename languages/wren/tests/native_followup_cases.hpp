#pragma once

#include <cstdio>
#include <string_view>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "wren/core/CoreServices.hpp"
#include "wren/core/ast.hpp"

namespace wren_followup {
inline bool run() {
  const auto language = cursiv::create_language<
      wren::WrenCoreServices, wren::WrenSharedCoreServices,
      wren::install_core_module>();
  const auto& parser = *language.language.parser;
  constexpr std::string_view invalid[] = {
      R"(var s = "\q")", R"(var s = "\x1")", R"(var s = "\u123")",
      R"(var s = "\U0001234")", R"(var s = "\xGG")",
      R"(var s = "a % b")", R"(var s = "a %")",
      "var s = \"", "import \"", "var s = \"\"\"",
  };
  for (const auto source : invalid) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty()) {
      std::fprintf(stderr, "wren: invalid native string accepted: %.*s\n",
                   int(source.size()), source.data());
      return false;
    }
  }
  // Escaped percent is text; raw strings keep escapes and interpolation raw.
  const struct {
    std::string_view source, expected;
    bool raw;
  } valid[] = {
      {R"(var s = "a \% b")", "a % b", false},
      {R"(var s = """a %(1) \q""")", R"(a %(1) \q)", true},
      // The selected native reader accepts these and drops values > 10FFFF.
      {R"(var s = "\U00110000")", "", false},
      {"var s = \"\"\" \n \"\"\"\n", "", true},
      {"var s = \"\"\" \n \n \"\"\"\n", " ", true},
  };
  for (const auto& test : valid) {
    const auto result = parser.parse(test.source);
    const auto* root = result.value_as<wren::ast::Script>();
    const auto* statement = root && root->statements.size() == 1
        ? cursiv::ast_cast<wren::ast::ExprStat>(root->statements[0]) : nullptr;
    const auto* declaration = statement
        ? cursiv::ast_cast<wren::ast::AssignStat>(statement->expr) : nullptr;
    const auto* string = declaration
        ? cursiv::ast_cast<wren::ast::StringExpr>(declaration->value) : nullptr;
    const auto* text = string && string->parts.size() == 1
        ? cursiv::ast_cast<wren::ast::StringText>(string->parts[0]) : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !text || text->value != test.expected ||
        string->raw != test.raw || text->parent != string) {
      std::fprintf(stderr, "wren: native string payload or parent mismatch\n");
      return false;
    }
  }
  return true;
}
}  // namespace wren_followup
