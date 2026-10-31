#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"

namespace typescript_yield_context_cases {
namespace a = typescript::ast;
inline bool run(const typescript::TypeScriptCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, const char* name, const char* message) {
    if (!value) { std::fprintf(stderr,"typescript yield-context %s: %s\n",name,message); ok=false; }
  };
  struct Case { const char* name; const char* source; bool syntax; bool validationOnly; };
  // Official5.9.3 observation04. Async-await is intentionally checked at local
  // validation rather than multiplying the two expression graphs by Await.
  // All original46 syntax/AST assertions remain unchanged in yield_cases.hpp.
  const Case cases[] = {
    {"generator-label-control", "function* g(){ outer: while(true){break outer;} }", true, false},
    {"generator-break-yield", "function* g(){ break yield; }", false, false},
    {"generator-continue-yield", "function* g(){ continue yield; }", false, false},
    {"generator-label-yield", "function* g(){ yield: ; }", false, false},
    {"generator-break-await", "function* g(){ break await; }", true, false},
    {"async-generator-break-await", "async function* g(){ break await; }", true, true},
    {"generator-import-yield", "function* g(){ import yield from \"x\"; }", false, false},
    {"generator-import-as-yield", "function* g(){ import {x as yield} from \"x\"; }", false, false},
    {"generator-export-yield", "function* g(){ export {yield}; }", true, false},
    {"generator-import-await", "function* g(){ import await from \"x\"; }", true, false},
    {"generator-import-equals", "function* g(){ import foo = require(\"x\"); }", true, false},
    {"generator-import-equals-yield", "function* g(){ import yield = require(\"x\"); }", false, false},
    {"ordinary-import-yield", "import yield from \"x\";", true, false},
    {"generator-debugger", "function* g(){ debugger; }", true, false},
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.syntax, test.name, "native syntax verdict (except explicitly separate Await validation stage)");
    if (!clean) continue;
    check(parsed.cst().text() == test.source, test.name, "complete source preserved");
    const auto validation = cursiv::test::validationHelper(services, std::string(test.source));
    bool contextual = false;
    for (const auto& diagnostic : validation.diagnostics)
      contextual |= diagnostic.code && *diagnostic.code == std::string_view("typescript.contextual-name");
    check(contextual == test.validationOnly, test.name, "local Await restriction; property/export names unaffected");
    const std::string_view name(test.name);
    bool field = false;
    for (const auto* node : parsed.nodes) {
      if (const auto* n=cursiv::ast_cast<a::BreakStmt>(node))
        field |= (name=="generator-label-control" && n->label=="outer") ||
                 ((name=="generator-break-await" || name=="async-generator-break-await") && n->label=="await");
      if (const auto* n=cursiv::ast_cast<a::ImportSpecifier>(node))
        field |= (name=="generator-import-await" && n->local=="await") ||
                 (name=="ordinary-import-yield" && n->local=="yield");
      if (const auto* n=cursiv::ast_cast<a::ExportSpecifier>(node))
        field |= name=="generator-export-yield" && n->local=="yield";
      if (const auto* n=cursiv::ast_cast<a::ImportEqualsDecl>(node))
        field |= name=="generator-import-equals" && n->name=="foo" && n->source=="\"x\"";
      if (cursiv::ast_cast<a::DebuggerStmt>(node)) field |= name=="generator-debugger";
    }
    check(field, test.name, "native accepted binding, label or export field retained");
  }
  return ok;
}
}  // namespace typescript_yield_context_cases
