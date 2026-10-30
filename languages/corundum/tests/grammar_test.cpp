// The grammar lane: the upstream sample and a fixture exercising constructs
// the sample does not use both parse clean, and a few field values come back
// as written -- not merely a shape check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "corundum/core/CoreServices.hpp"
#include "corundum/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "corundum: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

template <class Language>
const corundum::ast::Block* parse_clean(Language& language,
                                        const std::string& text,
                                        const char* label,
                                        cursiv::ParseResult& result) {
  result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "corundum: %s does not parse cleanly\n", label);
    return nullptr;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "corundum: %s left %zu diagnostic(s)\n", label,
                 result.diagnostics.size());
    return nullptr;
  }
  const corundum::ast::Block* const root = as<corundum::ast::Block>(result.value);
  if (root == nullptr)
    std::fprintf(stderr, "corundum: %s built no Block root\n", label);
  return root;
}

}  // namespace

int main() {
  const std::string sample = read_file(CURSIV_SAMPLE);
  if (sample.empty()) return fail("the sample document is not readable");

  const std::string fixture =
      read_file(CURSIV_TEST_DIR "/fixtures/constructs.rb");
  if (fixture.empty()) return fail("the fixture document is not readable");

  auto language = cursiv::create_language<
      corundum::CorundumCoreServices, corundum::CorundumSharedCoreServices,
      corundum::install_core_module>();

  {
    // The author's comparison_list groups its right-recursive conditions
    // independently of the general rvalue operator ladder.
    const std::string source =
        "if a < b | c < d & e < f\n  x = 1\nend\n";
    const auto parsed = language.language.parser->parse(source);
    const auto* root = as<corundum::ast::Block>(parsed.value);
    const auto* conditional = root && root->statements.size() == 1
        ? as<corundum::ast::Conditional>(root->statements[0]) : nullptr;
    const auto* outer = conditional
        ? as<corundum::ast::BinaryExpr>(conditional->cond) : nullptr;
    const auto* right = outer
        ? as<corundum::ast::BinaryExpr>(outer->right) : nullptr;
    const auto* left = outer
        ? as<corundum::ast::BinaryExpr>(outer->left) : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !outer ||
        outer->op != corundum::ast::BinOp::logical_or || !right ||
        right->op != corundum::ast::BinOp::logical_and || !left ||
        left->op != corundum::ast::BinOp::lt)
      return fail("native condition list lost its right-recursive grouping");
  }

  cursiv::ParseResult sampleResult;
  const corundum::ast::Block* const model =
      parse_clean(language, sample, "the sample", sampleResult);
  if (model == nullptr) return 1;
  if (model->statements.empty())
    return fail("the sample built an empty program");

  // Line 1 of the sample: `a = 1.1+2.5*2` -- read the assignment's shape and
  // the multiply's precedence back out, not merely that something parsed.
  const auto* const first = as<corundum::ast::Assign>(model->statements[0]);
  if (first == nullptr) return fail("the first statement is not an assign");
  const auto* const target = as<corundum::ast::Ident>(first->target);
  if (target == nullptr || target->name != "a")
    return fail("the first assign does not target the name 'a'");
  const auto* const sum = as<corundum::ast::BinaryExpr>(first->value);
  if (sum == nullptr || sum->op != corundum::ast::BinOp::add)
    return fail("1.1+2.5*2 should split on the '+', the loosest operator");

  cursiv::ParseResult fixtureResult;
  const corundum::ast::Block* const constructs =
      parse_clean(language, fixture, "the fixture", fixtureResult);
  if (constructs == nullptr) return 1;
  if (constructs->statements.size() < 20)
    return fail("the fixture parsed fewer statements than it declares");

  std::puts("corundum: the sample and the fixture parse");
  return 0;
}
