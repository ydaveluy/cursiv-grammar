#include "audit_regressions.hpp"
// The smoke test of the language: every file in example/ and tests/ parses
// without recovery and builds a TranslationUnit.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "c/core/CoreServices.hpp"
#include "c/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "c: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      c::CCoreServices,
      c::CSharedCoreServices,
      c::install_core_module>();

  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    const std::string ext = entry.path().extension().string();
    if (!entry.is_regular_file() || (ext != ".c" && ext != ".h")) continue;
    ++seen;
    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<c::ast::TranslationUnit>)
      return fail(entry.path().string() + ": built no translation unit");
  }
  if (seen == 0) return fail("no examples found");

  // The parser retains the unresolved shape; runtime typedef scope selects its meaning.
  const auto want_ambiguous_stmt = [&](const std::string& text,
                                  std::string_view label) -> int {
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(label) + ": does not parse clean");
    const auto* const tu =
        cursiv::ast_cast<c::ast::TranslationUnit>(result.value);
    if (tu == nullptr || tu->items.empty())
      return fail(std::string(label) + ": expected a top-level item");
    const auto* const fn =
        cursiv::ast_cast<c::ast::FunctionDef>(tu->items[tu->items.size() - 1]);
    if (fn == nullptr || fn->body == nullptr || fn->body->items.size() != 1)
      return fail(std::string(label) + ": expected one block item");
    if (cursiv::ast_cast<c::ast::AmbiguousStmt>(fn->body->items[0]) == nullptr)
      return fail(std::string(label) +
                  ": expected an unresolved declaration/expression shape");
    return 0;
  };
  if (int rc = want_ambiguous_stmt("void g(void) { f(x); }\n", "f(x), no typedef"))
    return rc;
  if (int rc = want_ambiguous_stmt("typedef int T;\nvoid g(void) { T(x); }\n",
                              "T(x), T typedef'd"))
    return rc;
  if (int rc = want_ambiguous_stmt("typedef int T;\nvoid g(void) { T * x; }\n",
                              "T * x, T typedef'd"))
    return rc;
  if (int rc =
          want_ambiguous_stmt("void g(void) { f * x; }\n", "f * x, no typedef"))
    return rc;

  // Everything else a name can open stays a declaration: only the two
  // minimal shapes above are held back.
  const auto want_declaration = [&](const std::string& text,
                                    std::string_view label) -> int {
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(label) + ": does not parse clean");
    const auto* const tu =
        cursiv::ast_cast<c::ast::TranslationUnit>(result.value);
    if (tu == nullptr || tu->items.empty())
      return fail(std::string(label) + ": expected a top-level item");
    const auto* const fn =
        cursiv::ast_cast<c::ast::FunctionDef>(tu->items[tu->items.size() - 1]);
    if (fn == nullptr || fn->body == nullptr || fn->body->items.size() != 1)
      return fail(std::string(label) + ": expected one block item");
    if (cursiv::ast_cast<c::ast::Declaration>(fn->body->items[0]) == nullptr)
      return fail(std::string(label) +
                  ": expected a declaration, read something else");
    return 0;
  };
  if (int rc = want_declaration("void g(void) { T x; }\n", "T x"))
    return rc;
  if (int rc = want_declaration("void g(void) { T * x = y; }\n",
                                "T * x = y"))
    return rc;

  std::printf("c: %d examples parse\n", seen);
  return 0;
}
