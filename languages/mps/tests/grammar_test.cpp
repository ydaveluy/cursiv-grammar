#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "mps/core/CoreServices.hpp"
#include "mps/core/ast.hpp"
#include "quadratic_header_cases.hpp"

namespace {

int fail(const char* sample, const char* what) {
  std::fprintf(stderr, "mps: %s: %s\n", sample, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      mps::MpsCoreServices,
      mps::MpsSharedCoreServices,
      mps::install_core_module>();
  if (!audit_regressions::run(language.language, *language.shared)) return 1;
  if (!quadratic_header_cases::run(language.language)) return 1;

  {
    const std::string source =
        "NAME TEST FREE\nOBJSENSE MAX\nROWS\n N COST\n L LIMIT\n"
        "COLUMNS\n X COST 1D+2 LIMIT 2\nRHS\n LIMIT 4\nENDATA\n";
    const auto result = language.language.parser->parse(source);
    const auto* model = result.value_as<mps::ast::Model>();
    const auto* column = model && model->columns.size() == 1
                             ? cursiv::ast_cast<mps::ast::ColumnCard>(model->columns[0]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !model || !model->free || model->name != "TEST" ||
        model->objectiveSenses.size() != 1 || model->objectiveSenses[0] != "MAX" ||
        !column || column->entries.size() != 2 || column->entries[0]->value != "1D+2" ||
        model->rhs.size() != 1 || model->rhs[0]->fields.size() != 2 ||
        model->rhs[0]->fields[0] != "LIMIT" || model->rhs[0]->fields[1] != "4")
      return fail("native profile", "FREE, objective sense and raw coefficient/card fields changed");
  }

  const char* const samples[] = {
      CURSIV_EXAMPLE_DIR "/example1.mps",
      CURSIV_EXAMPLE_DIR "/sample1.mps",
      CURSIV_EXAMPLE_DIR "/sample2.mps",
      CURSIV_TEST_DIR "/integer.mps",
  };

  for (const char* const sample : samples) {
    std::ifstream file(sample);
    if (!file) return fail(sample, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(sample, "does not parse clean");
    if (!result.diagnostics.empty())
      return fail(sample, "carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mps::ast::Model>)
      return fail(sample, "the parse built no Model");

    const auto& model = *static_cast<const mps::ast::Model*>(root);
    if (model.rows.empty() || model.columns.empty() || model.rhs.empty())
      return fail(sample, "the model is missing a mandatory section");
  }

  std::puts("mps: every sample parses");
  return 0;
}
