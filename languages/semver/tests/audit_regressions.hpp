#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "semver/core/CoreServices.hpp"
#include "semver/core/ast.hpp"
#include "semver/generated/Semver/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = semver::generated::Semver::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = semver::ast;

  auto r = parser->parse("18446744073709551616.0.0-alpha3.001+build-42.0");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "large SemVer with mixed identifiers");
  auto *v = as<ast::Semver>(r.value);
  check(v && v->core && v->core->major == "18446744073709551616",
        "unbounded core text");
  check(v && v->preRelease && v->preRelease->identifiers.size() == 2 &&
            v->preRelease->identifiers[0] == "alpha3" &&
            v->preRelease->identifiers[1] == "001",
        "identifier boundaries and spelling");
  check(v && v->build && v->build->identifiers.size() == 2 &&
            v->build->identifiers[0] == "build-42",
        "build identifiers");

  {
    auto language = cursiv::create_language<semver::SemverCoreServices,
                                            semver::SemverSharedCoreServices,
                                            semver::install_core_module>();
    const std::string input = "01.2.3-01";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:test", "semver", 0, input));
    doc.parseResult = parser->parse(input);
    check(doc.parseResult.accepted && !doc.parseResult.cst().recovered(),
          "semantic error remains syntactically parseable");
    int errors = 0;
    auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, input);
    if (doc.parseResult.value)
      language.language.validator->checkVersion(
          *as<ast::Semver>(doc.parseResult.value), accept);
    check(errors == 2, "runtime validator reports semantic constraints");
  }
  return ok;
}
} // namespace audit_regressions
