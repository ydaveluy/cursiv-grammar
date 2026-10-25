// The smoke test of the language: parse every document under `example/`,
// require a clean parse, and read a few real field values back out of one
// of them so a converter that silently yields the wrong value is caught.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "asl/core/CoreServices.hpp"
#include "asl/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "asl: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      asl::AslCoreServices, asl::AslSharedCoreServices, asl::install_core_module>();

  const std::filesystem::path example_dir = CURSIV_EXAMPLE_DIR;
  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".asl")
        files.push_back(entry.path());
  if (files.empty()) return fail("no example documents found");

  for (const auto& path : files) {
    // Both the text and the ParseResult must outlive every read of the
    // tree: its string_views point into the text, and its nodes live in
    // the arena the ParseResult owns.
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the document does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the document carries diagnostics");

    const auto* model = as<asl::ast::Model>(result.value);
    if (model == nullptr) return fail(path.string() + ": the parse built no model");
    if (model->statements.empty())
      return fail(path.string() + ": the model holds no statement");
  }

  // Re-parsed on its own, kept alive for the rest of main: a converter that
  // silently yields the wrong value passes every check above.
  const std::string assignments_text = read_file(example_dir / "assignments.asl");
  const cursiv::ParseResult assignments_result =
      language.language.parser->parse(assignments_text);
  const auto* assignments_model = as<asl::ast::Model>(assignments_result.value);
  if (assignments_model == nullptr) return fail("assignments.asl was not found");

  // Read a few real values back: a converter that silently yields the
  // wrong value passes every structural check above.
  const auto& stmts = assignments_model->statements;
  if (stmts.size() != 24)
    return fail("assignments.asl: expected 24 statements, got " +
                std::to_string(stmts.size()));

  const auto* a0 = as<asl::ast::Assignment>(stmts[0]);
  if (a0 == nullptr) return fail("assignments.asl[0]: not an Assignment");
  const auto* a0_target = as<asl::ast::VariableRef>(a0->target);
  if (a0_target == nullptr || a0_target->name != "variable1")
    return fail("assignments.asl[0]: target is not variable1");
  const auto* a0_value = as<asl::ast::IntegerConstant>(a0->value);
  if (a0_value == nullptr || a0_value->value != 5)
    return fail("assignments.asl[0]: value is not the integer 5");

  const auto* a4 = as<asl::ast::Assignment>(stmts[4]);
  const auto* a4_value = a4 != nullptr ? as<asl::ast::IntegerConstant>(a4->value) : nullptr;
  if (a4_value == nullptr || a4_value->value != -100)
    return fail("assignments.asl[4]: value is not the integer -100");

  const auto* a6 = as<asl::ast::Assignment>(stmts[6]);
  const auto* a6_target = a6 != nullptr ? as<asl::ast::ObjectAttribute>(a6->target) : nullptr;
  const auto* a6_value = a6 != nullptr ? as<asl::ast::RealConstant>(a6->value) : nullptr;
  if (a6_target == nullptr || a6_target->base != "variable4" ||
      a6_target->attribute != "Real_attribute")
    return fail("assignments.asl[6]: target is not variable4.Real_attribute");
  if (a6_value == nullptr || a6_value->value < 1.229 || a6_value->value > 1.231)
    return fail("assignments.asl[6]: value is not the real 1.23");

  const auto* a7 = as<asl::ast::Assignment>(stmts[7]);
  const auto* a7_value = a7 != nullptr ? as<asl::ast::BooleanConstant>(a7->value) : nullptr;
  if (a7_value == nullptr || a7_value->value != true)
    return fail("assignments.asl[7]: value is not the boolean TRUE");

  const auto* a14 = as<asl::ast::Assignment>(stmts[14]);
  const auto* a14_value = a14 != nullptr ? as<asl::ast::TextConstant>(a14->value) : nullptr;
  if (a14_value == nullptr || a14_value->value != "Hello")
    return fail("assignments.asl[14]: value is not the text \"Hello\"");

  std::puts("asl: every example parses, and assignments.asl reads back clean");
  return 0;
}
