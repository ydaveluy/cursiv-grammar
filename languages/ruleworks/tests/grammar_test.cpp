#include "native_followup.hpp"
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "ruleworks/core/CoreServices.hpp"
#include "ruleworks/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every example and the coverage document, and check the trees.

#include <array>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::string& file, const char* what) {
  std::fprintf(stderr, "ruleworks: %s: %s\n", file.c_str(), what);
  return 1;
}

std::string read_file(const std::string& path, bool& ok) {
  std::ifstream file(path);
  if (!file) {
    ok = false;
    return {};
  }
  ok = true;
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const ruleworks::ast::Program* as_program(const cursiv::ParseResult& result) {
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<ruleworks::ast::Program>)
    return nullptr;
  return static_cast<const ruleworks::ast::Program*>(root);
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<ruleworks::RuleworksCoreServices,
                              ruleworks::RuleworksSharedCoreServices,
                              ruleworks::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "(entry-block main)\n(on-entry (write "
        "(integer 1 2)))\n(end-block main)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "(entry-block main)\n(on-entry (write (integer 1 2)))\n(end-block "
        "main)",
        "file:///audit.rul");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      ruleworks::RuleworksCoreServices, ruleworks::RuleworksSharedCoreServices,
      ruleworks::install_core_module>();

  if (!native_followup::run(language)) return 1;

  const char* const examples[] = {
      CURSIV_EXAMPLE_DIR "/advent.rul",
      CURSIV_EXAMPLE_DIR "/count.rul",
      CURSIV_EXAMPLE_DIR "/tourney.rul",
  };

  // Keep every document's text alive for the whole run: every string_view in
  // a tree points into it, so a buffer that dies while its tree is still
  // read leaves every field reading freed memory.
  std::string texts[std::size(examples)];
  cursiv::ParseResult results[std::size(examples)];

  for (std::size_t i = 0; i < std::size(examples); ++i) {
    bool ok = false;
    texts[i] = read_file(examples[i], ok);
    if (!ok) return fail(examples[i], "the example is not readable");
    results[i] = language.language.parser->parse(texts[i]);
    if (results[i].cst().recovered())
      return fail(examples[i], "the example does not parse cleanly");
    if (!results[i].diagnostics.empty())
      return fail(examples[i], "the example raised a diagnostic");
    if (as_program(results[i]) == nullptr)
      return fail(examples[i], "the parse built no program");
  }

  // count.rul: read real field values back out of the tree, not just its
  // shape -- an entry-block name, an object-class name, a rule name and one
  // of its make actions.
  const auto& count = *as_program(results[1]);
  bool sawIterator = false;
  bool sawIncrementRule = false;
  for (const ruleworks::ast::ProgramConstruct* const construct :
       count.constructs) {
    for (const auto& line : construct->lines) {
      if (const auto* const* entry =
              std::get_if<ruleworks::ast::EntryBlock*>(&line)) {
        if ((*entry)->name != "main")
          return fail(examples[1], "the entry-block name is wrong");
      } else if (const auto* const* objectClass =
                     std::get_if<ruleworks::ast::ObjectClassDecl*>(&line)) {
        if ((*objectClass)->name == "iterator") sawIterator = true;
      } else if (const auto* const* rule =
                     std::get_if<ruleworks::ast::Rule*>(&line)) {
        if ((*rule)->name == "increment-rule") {
          sawIncrementRule = true;
          if ((*rule)->actions.empty())
            return fail(examples[1], "increment-rule has no actions");
        }
      }
    }
  }
  if (!sawIterator) return fail(examples[1], "the iterator class is missing");
  if (!sawIncrementRule)
    return fail(examples[1], "increment-rule is missing");

  // The coverage document: constructs the corpus never reaches (method,
  // generic-method, external-routine, catch, sql-* actions, ce_disjunction,
  // neg_ce, and SQL actions). Lives under tests/, not example/:
  // it is not part of the upstream corpus.
  bool ok = false;
  const std::string coverageText = read_file(CURSIV_TEST_DIR "/coverage.rul", ok);
  if (!ok) return fail("coverage.rul", "is not readable");
  const cursiv::ParseResult coverage =
      language.language.parser->parse(coverageText);
  if (coverage.cst().recovered())
    return fail("coverage.rul", "does not parse cleanly");
  if (!coverage.diagnostics.empty())
    return fail("coverage.rul", "raised a diagnostic");
  if (as_program(coverage) == nullptr)
    return fail("coverage.rul", "built no program");

  // A duplicate attribute is invalid and must be caught by the validator,
  // not merely accepted by the parser -- parse diagnostics alone would not
  // see it, so this runs the validation pass.
  const cursiv::test::ValidationResult duplicate =
      cursiv::test::validationHelper(language.language,
                                     "(object-class dup ^a ^a)\n");
  if (duplicate.diagnostics.empty())
    return fail("<duplicate attribute>", "the validator did not object");

  std::puts("ruleworks: the examples and the coverage document parse");
  return 0;
}
