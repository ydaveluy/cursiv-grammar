#include "code_audit.hpp"
#include "audit_regressions.hpp"
// The grammar end to end: every document under `example/`, subdirectories
// included, parses clean -- no recovery, no diagnostics -- and builds a File
// at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "terraform/core/CoreServices.hpp"
#include "terraform/core/ast.hpp"
#include "native_followup_cases.hpp"
#include "expression_layout_cases.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "terraform: %s: %s\n", file.string().c_str(), what);
  return 1;
}

int failMsg(const char* what) {
  std::fprintf(stderr, "terraform: %s\n", what);
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !code_audit::run()) return 1;
  const auto language = cursiv::create_language<
      terraform::TerraformCoreServices, terraform::TerraformSharedCoreServices,
      terraform::install_core_module>();
  if (!terraform_native_followup::run(language)) return failMsg("native syntax/AST followup failed");
  if (!terraform_expression_layout::run(language)) return failMsg("expression layout followup failed");

  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".tf")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<terraform::ast::File>) {
      failures += fail(entry.path(), "the parse built no File");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "terraform: no .tf file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("terraform: %d example documents parse clean\n", count);

  // Unary operators bind tighter than every infix level (HCL native syntax
  // spec, "Operator Precedence"): -var.a * 2 is (-var.a) * 2.
  {
    const cursiv::ParseResult r = language.language.parser->parse(
        "locals {\n  x = -var.a * 2\n}\n");
    if (r.cst().recovered() || !r.diagnostics.empty())
      return failMsg("-var.a * 2 does not parse clean");
    const auto* file = as<terraform::ast::File>(r.value);
    const auto* locals =
        file != nullptr && !file->items.empty()
            ? as<terraform::ast::Block>(file->items[0])
            : nullptr;
    const auto* arg =
        locals != nullptr && !locals->body.empty()
            ? as<terraform::ast::Argument>(locals->body[0])
            : nullptr;
    const auto* mul =
        arg != nullptr ? as<terraform::ast::BinaryExpr>(arg->value) : nullptr;
    const auto* neg =
        mul != nullptr ? as<terraform::ast::UnaryExpr>(mul->left) : nullptr;
    if (mul == nullptr || mul->op != terraform::ast::Operator::multiply ||
        neg == nullptr || neg->op != terraform::ast::UnaryOp::negate ||
        as<terraform::ast::MemberExpr>(neg->operand) == nullptr)
      return failMsg("-var.a * 2 did not group (-var.a) * 2");
  }

  std::puts("terraform: unary operators bind tightest");
  return 0;
}
