#include "audit_regressions.hpp"
#include "native_regressions.hpp"
#include "native_followup_cases.hpp"
// The smoke test of the language: every file under example/ parses clean (no
// recovery) and roots at ast::File.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "swift/core/CoreServices.hpp"
#include "swift/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "swift: %s\n", what.c_str());
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

// Swift's default operator precedence is ordinary Swift source
// (stdlib/public/core/Policy.swift's `precedencegroup`s), not a grammar
// rule, and a file may declare its own -- so `BinaryChainExpr` stays flat,
// left to right, same as this campaign's `haskell` port. This
// probe pins that shape down: it must not silently start grouping by
// precedence (or stop being flat) without this assertion being revisited.
bool check_precedence_stays_flat(const cursiv::Parser& parser) {
  const std::string text = read_file(CURSIV_TEST_DIR "/precedence.swift");
  const cursiv::ParseResult result = parser.parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "swift: precedence.swift: did not parse clean\n");
    return false;
  }
  const auto* file = as<swift::ast::File>(result.value);
  const auto* decl =
      file && !file->statements.empty()
          ? as<swift::ast::VariableDecl>(file->statements[0])
          : nullptr;
  const auto* chain =
      decl && !decl->bindings.empty()
          ? as<swift::ast::BinaryChainExpr>(decl->bindings[0]->initializer)
          : nullptr;
  if (chain == nullptr || chain->items.size() != 2) {
    std::fprintf(stderr,
                  "swift: precedence.swift: expected a flat two-item chain "
                  "for `1 + 2 * 3`\n");
    return false;
  }
  const auto* plus = as<swift::ast::BinaryOp>(chain->items[0]);
  const auto* times = as<swift::ast::BinaryOp>(chain->items[1]);
  if (plus == nullptr || plus->op != "+" || times == nullptr ||
      times->op != "*") {
    std::fprintf(stderr,
                  "swift: precedence.swift: expected items [+, *], not "
                  "precedence-grouped\n");
    return false;
  }
  return true;
}

// `(Int)` is `Int` in parentheses, a tuple type having none or at least two
// elements (Types.md), and a bare `catch` has no pattern.
bool check_parens_and_bare_catch(const cursiv::Parser& parser) {
  const cursiv::ParseResult result =
      parser.parse("var t: (Int) = 3\ndo {\n} catch {\n}\n");
  if (result.cst().recovered()) {
    std::fprintf(stderr, "swift: `(Int)` and `catch {}`: did not parse clean\n");
    return false;
  }
  const auto* file = as<swift::ast::File>(result.value);
  if (file == nullptr || file->statements.size() != 2) {
    std::fprintf(stderr, "swift: expected two statements\n");
    return false;
  }
  const auto* decl = as<swift::ast::VariableDecl>(file->statements[0]);
  const auto* pattern =
      decl && !decl->bindings.empty()
          ? as<swift::ast::IdentifierPattern>(decl->bindings[0]->pattern)
          : nullptr;
  if (pattern == nullptr || as<swift::ast::NamedType>(pattern->type) == nullptr) {
    std::fprintf(stderr, "swift: expected `(Int)` to read as the NamedType\n");
    return false;
  }
  const auto* stmt = as<swift::ast::DoStmt>(file->statements[1]);
  if (stmt == nullptr || stmt->catches.size() != 1 ||
      !stmt->catches[0]->patterns.empty()) {
    std::fprintf(stderr, "swift: expected one catch clause, with no pattern\n");
    return false;
  }
  return true;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      swift::SwiftCoreServices,
      swift::SwiftSharedCoreServices,
      swift::install_core_module>();

  if (!native_regressions::run(language.language)) return 1;
  if (!native_followup::run(language.language)) return 1;

  int seen = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".swift")
      continue;
    ++seen;
    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": did not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<swift::ast::File>)
      return fail(entry.path().string() + ": wrong root type");
  }
  if (seen == 0) return fail("no .swift file found under " CURSIV_EXAMPLE_DIR);

  if (!check_precedence_stays_flat(*language.language.parser))
    return fail("precedence.swift: AST shape check failed");
  if (!check_parens_and_bare_catch(*language.language.parser))
    return fail("`(Int)` and `catch {}`: AST shape check failed");

  std::printf("swift: %d example(s) parse clean\n", seen);
  return 0;
}
