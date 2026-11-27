#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/parser/Parser.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "cursiv/syntax-tree/AstNode.hpp"
#include "mps/CoreServices.hpp"
#include "mps/ast.hpp"
#include "quadratic_header_cases.hpp"

namespace {

int fail(const char* sample, const char* what) {
  std::fprintf(stderr, "mps: %s: %s\n", sample, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::createLanguage<
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
    const auto* model = result.valueAs<mps::ast::Model>();
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

  {
    const std::string padding(28, ' ');
    const std::string source = "NAME TEST\nROWS\n N COST\n L LIMIT\nCOLUMNS\n"
        "    X         COST    " + padding + "-3\nRHS\n    R         LIMIT   " +
        padding + "4.0\nENDATA\n";
    const auto result = language.language.parser->parse(source);
    const auto* model = result.valueAs<mps::ast::Model>();
    const auto* column = model && model->columns.size() == 1
                             ? cursiv::ast_cast<mps::ast::ColumnCard>(model->columns[0]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !column || column->entries.size() != 1 || column->entries[0]->row != "COST" ||
        column->entries[0]->value != "-3" || model->rhs.size() != 1 ||
        model->rhs[0]->fields.size() != 3 || model->rhs[0]->fields[1] != "LIMIT" ||
        model->rhs[0]->fields[2] != "4.0")
      return fail("free-format spacing", "a coefficient outside the fixed field changed the card");
  }
  for (const std::string spaces : {"  ", "       ", "                 ", " \t "}) {
    const std::string pairs = "    12        COST" + spaces + "1   LIMIT  2";
    const std::string source = "NAME TEST\nROWS\n N COST\n L LIMIT\nCOLUMNS\n" + pairs +
                               "\nRHS\n" + pairs + "\nBOUNDS\n MI  DE                   12\n"
                               " UP             DE       12  +2.75\nENDATA\n\t";
    const auto result = language.language.parser->parse(source);
    const auto* model = result.valueAs<mps::ast::Model>();
    const auto* column = model && model->columns.size() == 1
                             ? cursiv::ast_cast<mps::ast::ColumnCard>(model->columns[0]) : nullptr;
    if (!result.accepted || !result.diagnostics.empty() || !column || column->entries.size() != 2 ||
        column->entries[0]->row != "COST" || column->entries[0]->value != "1" ||
        column->entries[1]->row != "LIMIT" || column->entries[1]->value != "2" ||
        model->rhs.size() != 1 || model->rhs[0]->fields.size() != 5 ||
        model->rhs[0]->fields[3] != "LIMIT" || model->rhs[0]->fields[4] != "2" ||
        model->bounds.size() != 2 || model->bounds[0]->fields.size() != 2 ||
        model->bounds[0]->fields[0] != "DE" || model->bounds[0]->fields[1] != "12" ||
        model->bounds[1]->fields.size() != 3 || model->bounds[1]->fields[0] != "DE" ||
        model->bounds[1]->fields[1] != "12" || model->bounds[1]->fields[2] != "+2.75")
      return fail("free-format fields", "coefficient pairs, bounds or trailing whitespace changed");
  }

  for (const bool fixed : {true, false}) {
    const std::string row = fixed ? "A 4 B" : "OBJ";
    const std::string columnName = fixed ? "X 2" : "X";
    const std::string boundSet = fixed ? "B 2" : "B";
    const auto field = [](std::string s) { s.resize(8, ' '); return s; };
    const std::string source = "NAME          MIXED\nROWS\n N  " + row +
        "\n L  LIMIT\nCOLUMNS\n    " + field(columnName) + "  " + field(row) +
        "             1   LIMIT                2\nBOUNDS\n" +
        (fixed ? " UP " + field(boundSet) + "  " + field(columnName) + "             5"
               : " PL            B    X") + "\nENDATA\n";
    const auto result = language.language.parser->parse(source);
    const auto* model = result.valueAs<mps::ast::Model>();
    const auto* column = model && model->columns.size() == 1
                             ? cursiv::ast_cast<mps::ast::ColumnCard>(model->columns[0]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !column || column->name != columnName || column->entries.size() != 2 ||
        column->entries[0]->row != row || column->entries[0]->value != "1" ||
        column->entries[1]->row != "LIMIT" || column->entries[1]->value != "2" ||
        model->bounds.size() != 1 || model->bounds[0]->fields.size() != (fixed ? 3u : 2u) ||
        model->bounds[0]->fields[0] != boundSet || model->bounds[0]->fields[1] != columnName ||
        (fixed && model->bounds[0]->fields[2] != "5"))
      return fail("document format", "aligned columns and late bounds use inconsistent fields");
  }

  for (const bool free : {false, true}) {
    const std::string source = std::string("NAME          TEST") + (free ? " FREE" : "") +
        "\nROWS\n N  OBJ\n L  DE\n G  F\nCOLUMNS\n"
        "    X         OBJ                  1   DE                   2\n"
        "    Y         F                    1\nRHS\n    34  DE 8    F        6\nENDATA\n";
    const auto result = language.language.parser->parse(source);
    const auto* model = result.valueAs<mps::ast::Model>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !model || model->rhs.size() != 1 || model->rhs[0]->fields.size() != (free ? 5u : 3u) ||
        model->rhs[0]->fields[0] != (free ? "34" : "34  DE 8") ||
        model->rhs[0]->fields[1] != (free ? "DE" : "F"))
      return fail("ambiguous model", "the NAME card selects the wrong RHS fields");
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
        root->type != &cursiv::kAstTypeTag<mps::ast::Model>)
      return fail(sample, "the parse built no Model");

    const auto& model = *static_cast<const mps::ast::Model*>(root);
    if (model.rows.empty() || model.columns.empty() || model.rhs.empty())
      return fail(sample, "the model is missing a mandatory section");
  }

  std::puts("mps: every sample parses");
  return 0;
}
