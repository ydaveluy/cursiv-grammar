#include "native_regressions.hpp"
// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "tnt/core/CoreServices.hpp"
#include "tnt/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "tnt: %s: %s\n", file.string().c_str(), what);
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

// Renders an expression fully parenthesised, e.g. `(a+(b*c))`, so a probe's
// shape can be asserted as one string.
std::string render(const cursiv::AstNode* n) {
  if (const auto* const b = as<tnt::ast::BinaryExpression>(n); b != nullptr)
    return "(" + render(b->left) +
           (b->op == tnt::ast::BinaryOp::add ? "+" : "*") + render(b->right) +
           ")";
  if (const auto* const v = as<tnt::ast::Variable>(n); v != nullptr) {
    std::string s(v->letter);
    s.append(v->primes, '\'');
    return s;
  }
  if (const auto* const num = as<tnt::ast::NumberLiteral>(n); num != nullptr)
    return "0";
  if (const auto* const g = as<tnt::ast::GroupedExpression>(n); g != nullptr)
    return render(g->expression);
  if (const auto* successor = as<tnt::ast::SuccessorExpression>(n))
    return std::string(successor->count, 'S') + render(successor->expression);
  return "?";
}

// Unparenthesized arithmetic keeps ordinary precedence during recovery.
bool check_precedence(const auto& language,
                      const std::filesystem::path& test_dir) {
  bool ok = true;

  const cursiv::ParseResult probe =
      language.language.parser->parse("a+b*c=a");
  const auto* const eq = as<tnt::ast::Equation>(probe.value);
  const std::string got = eq != nullptr ? render(eq->left) : "no-parse";
  if (got != "(a+(b*c))") {
    std::fprintf(stderr, "tnt: a+b*c: got %s want (a+(b*c))\n", got.c_str());
    ok = false;
  }

  const std::filesystem::path path = test_dir / "unparenthesized.tnt";
  std::ifstream file(path);
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto* const eq2 = as<tnt::ast::Equation>(result.value);
  if (eq2 == nullptr) {
    fail(path, "the parse built no formula");
    return false;
  }
  const std::string left = render(eq2->left);
  const std::string right = render(eq2->right);
  if (left != "(Sa'+(b*c))" || right != "((b*c)+Sa')") {
    std::fprintf(stderr, "tnt: %s: got %s = %s\n", path.string().c_str(),
                 left.c_str(), right.c_str());
    ok = false;
  }
  return ok;
}

}  // namespace

int main() {
  if (!native_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      tnt::TntCoreServices, tnt::TntSharedCoreServices,
      tnt::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const auto& entry :
      std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        !cursiv::ast_cast<tnt::ast::Formula>(root)) {
      fail(path, "the parse built no formula");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "tnt: no example files found\n");
    return 1;
  }

  if (!check_precedence(language, CURSIV_TEST_DIR)) ++failures;

  if (failures > 0) return 1;
  std::printf("tnt: %d example(s) parse\n", total);
  return 0;
}
