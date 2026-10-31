#pragma once
#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "haskell/core/ast.hpp"

namespace haskell_native_followup {
inline bool run(const cursiv::Parser& parser) {
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "haskell native: %s\n", message); ok = false; }
  };
  for (const std::string& source : {
      std::string("x\u00a0=\u20031\n"),
      std::string("x\v=\v1\n"),
      std::string("{-# custom ignored body #-}\nmodule Audit where x = 1\n"),
      std::string("x = 1\f"),
      std::string("x = 1 -- comment\fy = 2\n"),
      std::string("s = \"left\\\n  \\right\\&\\SOH\"\n"),
      std::string("f :: [] a -> (->) a a\n"),
      std::string("f :: (,) a b -> ( )\n"),
      std::string("f = (1 `mod`)\ng = (`mod` 2)\nh = (Data.List.++ [1])\n")}) {
    const auto result = parser.parse(source);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value,
          "report lexical form, builtin type constructor or qualified/backtick section");
  }
  for (const char* source : {"x = '\\q'\n", "x = '\\&'\n", "x = '\\^a'\n",
                             "s = \"a\nb\"\n", "s = \"\\unknown\"\n"}) {
    const auto result = parser.parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "invalid escape or literal newline must not parse clean");
  }
  {
    const std::string source =
        "{-# options_ghc -Wall #-}\n"
        "module Audit (Data.List.map, (Data.List.++)) where { "
        "type role T nominal _; data T a b = T a; "
        "type Applied = Proxy @Type Int; "
        "{-# INLINE [1] f #-}; f = 1 }\n";
    const auto result = parser.parse(source);
    const auto* module = result.value_as<haskell::ast::Module>();
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && module &&
          module->exports.size() == 2 && module->exports[0]->name == "Data.List.map" &&
          module->exports[1]->name == "(Data.List.++)", "qualified export spellings");
    bool role = false, visible = false, options = false, inlinePragma = false;
    for (const auto* node : result.nodes) {
      if (const auto* declaration = cursiv::ast_cast<haskell::ast::RoleDecl>(node)) {
        const auto* name = cursiv::ast_cast<haskell::ast::TyCon>(declaration->header);
        role |= name && name->name == "T" && declaration->roles.size() == 2 &&
                declaration->roles[0] == "nominal" && declaration->roles[1] == "_";
      }
      if (const auto* argument = cursiv::ast_cast<haskell::ast::TyVisibleArg>(node))
        visible |= argument->type && argument->type->parent == argument;
      if (const auto* pragma = cursiv::ast_cast<haskell::ast::PragmaDecl>(node)) {
        options |= pragma->kind == "options_ghc" && pragma->body == " -Wall #-}";
        inlinePragma |= pragma->kind == "INLINE" && pragma->body == " [1] f #-}";
      }
    }
    check(role && visible && options && inlinePragma,
          "role lists, explicit kind arguments and pragma payloads survive in AST");
  }
  return ok;
}
} // namespace haskell_native_followup
