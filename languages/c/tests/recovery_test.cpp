#include <algorithm>
#include <string>

#include "c/ast.hpp"
#include "c/generated/C/Module.hpp"
#include "cursiv/Check.hpp"

int main() {
  const auto parser = c::generated::C::createParser();
  const auto missing = parser->parse("int bad = ; int good;");
  const auto* unit = missing.valueAs<c::ast::TranslationUnit>();
  const auto* last = unit && unit->items.size() == 2 ? unit->items[1] : nullptr;
  CHECK(missing.accepted && last && missing.cst().text(last->cstNodeId) == "int good;");
  CHECK(missing.diagnostics.size() == 1 &&
        missing.diagnostics.front().kind == cursiv::DiagnosticKind::Inserted);

  const auto malformed = parser->parse("enum{{c t)(e*)t o*o t>w;t(");
  CHECK(malformed.cst().recovered());
  for (const auto& d : malformed.diagnostics)
    CHECK(d.begin <= d.end && d.end <= malformed.cst().text().size());

  const std::string comment = "int f() { int x; /* " + std::string(1'000'000, 'a');
  const auto open = parser->parse(comment);
  CHECK(open.accepted && open.valueAs<c::ast::TranslationUnit>());
  CHECK(std::ranges::count(open.diagnostics, cursiv::DiagnosticKind::Inserted,
                           &cursiv::ParseDiagnostic::kind) == 2);
  return cursiv::test::report("c_recovery_test");
}
