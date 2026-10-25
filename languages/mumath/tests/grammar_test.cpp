// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/ and tests/, check the tree, and
// probe the operator ladder's precedence.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "mumath/core/CoreServices.hpp"
#include "mumath/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "mumath: %s: %s\n", file.string().c_str(), what);
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

// Renders an expression fully parenthesised, e.g. `(2 + (3 * 4))`, so a
// probe's shape can be asserted as one string.
std::string render(const cursiv::AstNode* n) {
  if (const auto* const b = as<mumath::ast::BinaryExpression>(n);
      b != nullptr)
    return "(" + render(b->left) + " " + std::string(b->op) + " " +
           render(b->right) + ")";
  if (const auto* const neg = as<mumath::ast::NegateExpression>(n);
      neg != nullptr)
    return "(-" + render(neg->operand) + ")";
  if (const auto* const not_ = as<mumath::ast::NotExpression>(n);
      not_ != nullptr)
    return "(NOT " + render(not_->operand) + ")";
  if (const auto* const num = as<mumath::ast::NumberLiteral>(n);
      num != nullptr) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", num->value);
    return buf;
  }
  if (const auto* const name = as<mumath::ast::NameExpression>(n);
      name != nullptr)
    return std::string(name->name);
  return "?";
}

// Every pair of adjacent levels in the binding-power table,
// tightest to loosest: `^`, unary +/-, `*`/`/`, +/-, comparison, NOT, AND,
// OR -- plus the regression a flat level would get backwards.
bool check_precedence(const auto& language) {
  struct Probe {
    const char* stmt;
    const char* want;
  };
  static const Probe probes[] = {
      {"2+3*4", "(2 + (3 * 4))"},
      {"2*3^2", "(2 * (3 ^ 2))"},
      {"2^3^2", "(2 ^ (3 ^ 2))"},
      {"-2^2", "(-(2 ^ 2))"},
      {"2+3=5", "((2 + 3) = 5)"},
      {"NOT A=B", "(NOT (A = B))"},
      {"NOT A AND B", "((NOT A) AND B)"},
      {"A AND B OR C", "((A AND B) OR C)"},
      {"A<=B AND A>=B", "((A <= B) AND (A >= B))"},
  };
  bool ok = true;
  for (const Probe& p : probes) {
    const std::string text = std::string("X:") + p.stmt + ";";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const program = as<mumath::ast::Program>(result.value);
    const auto* const asg =
        (program != nullptr && !program->statements.empty())
            ? as<mumath::ast::Assignment>(program->statements[0])
            : nullptr;
    const std::string got = asg != nullptr ? render(asg->expr) : "no-parse";
    if (got != p.want) {
      std::fprintf(stderr, "mumath: %s: got %s want %s\n", p.stmt,
                   got.c_str(), p.want);
      ok = false;
    }
  }
  return ok;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      mumath::MumathCoreServices, mumath::MumathSharedCoreServices,
      mumath::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(file, "the document carries diagnostics");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mumath::ast::Program>)
      failures += fail(file, "the parse built no Program");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");

  // constructs.mu: my own document, exercising operators and constructs
  // example1.mu does not. Never counted in `total`: only example/ is the
  // upstream corpus.
  int testDocs = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".mu")
      continue;
    const std::filesystem::path& file = entry.path();
    ++testDocs;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(file, "the document carries diagnostics");
      continue;
    }
    if (as<mumath::ast::Program>(result.value) == nullptr)
      failures += fail(file, "the parse built no Program");
  }
  if (testDocs == 0) return fail(CURSIV_TEST_DIR, "no test documents");

  if (!check_precedence(language)) ++failures;

  if (failures > 0) return 1;

  std::printf("mumath: %d example(s), %d test document(s) parse\n", total,
             testDocs);
  return 0;
}
