
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pf/core/CoreServices.hpp"
#include "pf/core/ast.hpp"
// The smoke test of the language: every corpus document parses clean, the
// constructs the corpus never exercises parse from documents of our own,
// and one document is read back field by field.

#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "pf: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "example1.pf", "example2.pf",  "example3.pf",  "example4.pf",
    "example5.pf", "example6.pf",  "example7.pf",  "example8.pf",
    "example9.pf", "example10.pf", "example11.pf",
};

const char* const kFeatures[] = {
    "options.pf",
    "filteropts.pf",
    "tables_queues_anchors.pf",
};

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<pf::PfCoreServices, pf::PfSharedCoreServices,
                              pf::install_core_module>();
  {
    const auto r = language.language.parser->parse("pass from ::1 to any");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      pf::PfCoreServices, pf::PfSharedCoreServices, pf::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<pf::ast::File>)
      return fail(std::string(name) + ": built no File");

    const auto& file = *static_cast<const pf::ast::File*>(root);
    if (file.lines.empty())
      return fail(std::string(name) + ": File holds no line");
  }

  for (const char* const name : kFeatures) {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<pf::ast::File>)
      return fail(std::string(name) + ": built no File");
  }

  // example1.pf, read back field by field.
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example1.pf");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& file = *static_cast<const pf::ast::File*>(result.value);
  if (file.lines.size() != 3) return fail("example1.pf: expected three lines");

  const auto* const* const varPtr = std::get_if<pf::ast::Var*>(&file.lines[0]);
  if (varPtr == nullptr || *varPtr == nullptr)
    return fail("example1.pf: first line is not a Var");
  if ((*varPtr)->name != "ext_if" || (*varPtr)->value != "kue0")
    return fail("example1.pf: wrong var name/value");

  const auto* const* const rulePtr =
      std::get_if<pf::ast::Rule*>(&file.lines[1]);
  if (rulePtr == nullptr || *rulePtr == nullptr)
    return fail("example1.pf: second line is not a Rule");
  const pf::ast::Rule& scrubRule = **rulePtr;
  if (scrubRule.action != "match") return fail("example1.pf: wrong action");
  if (scrubRule.hosts == nullptr || !scrubRule.hosts->all)
    return fail("example1.pf: wrong hosts");
  if (scrubRule.filterOpts.size() != 1 ||
      scrubRule.filterOpts[0]->kind != "scrub")
    return fail("example1.pf: wrong filter opts");
  if (scrubRule.filterOpts[0]->scrubOpts.size() != 2)
    return fail("example1.pf: wrong scrub opts");

  const auto* const* const blockPtr =
      std::get_if<pf::ast::Rule*>(&file.lines[2]);
  if (blockPtr == nullptr || *blockPtr == nullptr)
    return fail("example1.pf: third line is not a Rule");
  const pf::ast::Rule& blockRule = **blockPtr;
  if (blockRule.action != "block" || blockRule.blockReturn != "return")
    return fail("example1.pf: wrong block/return");
  if (!blockRule.log) return fail("example1.pf: expected log");
  if (blockRule.onIf == nullptr || blockRule.onIf->ifaces.size() != 1 ||
      blockRule.onIf->ifaces[0]->name != "$ext_if")
    return fail("example1.pf: wrong onspec");

  std::puts("pf: the corpus parses");
  return 0;
}
