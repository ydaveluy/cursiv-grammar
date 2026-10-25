
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "metric/core/CoreServices.hpp"
#include "metric/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

#include "audit_regressions.hpp"

namespace {

int fail(const std::filesystem::path &file, const char *what) {
  std::fprintf(stderr, "metric: %s: %s\n", file.string().c_str(), what);
  return 1;
}

} // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<metric::MetricCoreServices,
                              metric::MetricSharedCoreServices,
                              metric::install_core_module>();
  {
    const auto r = language.language.parser->parse("s^-2");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto *m = cursiv::ast_cast<metric::ast::PowerUom>(r.value);
    if (!m || m->exponent != "-2")
      return 1;
  }
  {
    const auto r = language.language.parser->parse("°C");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("Qg");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("yg");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("s^+2");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return 1;
    const auto *measure = cursiv::ast_cast<metric::ast::PowerUom>(r.value);
    if (!measure || measure->exponent != "+2")
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language =
      cursiv::create_language<metric::MetricCoreServices,
                              metric::MetricSharedCoreServices,
                              metric::install_core_module>();

  metric_regressions(language.language);

  int total = 0;
  int failures = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".metric")
        continue;
      const std::filesystem::path &file = entry.path();
      ++total;

      std::ifstream in(file);
      if (!in) {
        failures += fail(file, "the document is not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      // `recovered()` is set when the parser had to repair the input to keep
      // going: a clean document leaves it false.
      if (result.cst().recovered()) {
        failures += fail(file, "the document does not parse clean");
        continue;
      }

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          !root->type->derivesFrom(&cursiv::ast_type_tag<metric::ast::Uom>))
        failures += fail(file, "the parse built no Uom");
    }

  if (total == 0)
    return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0)
    return 1;

  std::printf("metric: %d example(s) parse\n", total);
  return cursiv::test::report("metric_grammar_test");
}
