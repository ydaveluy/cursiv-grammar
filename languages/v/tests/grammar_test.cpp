#include "audit_regressions.hpp"
// The smoke test of the language: every document in example/ parses clean
// (no repair, no diagnostic), and one of them is read back for the values
// its declarations actually hold -- a converter that silently drops or
// misreads a value would still pass a structural-only check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "v/core/CoreServices.hpp"
#include "v/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "v: %s\n", what.c_str());
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      v::VCoreServices, v::VSharedCoreServices, v::install_core_module>();

  std::vector<fs::path> samples;
  // `features.v` lives beside this test, not in the corpus: the one upstream
  // document is a hello world, so everything else is exercised from here.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : fs::directory_iterator(dir))
      if (entry.path().extension() == ".v") samples.push_back(entry.path());
  if (samples.empty()) return fail("no .v sample found");

  for (const fs::path& path : samples) {
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the sample does not parse clean");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the sample raised a diagnostic");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<v::ast::SourceFile>)
      return fail(path.string() + ": the parse built no SourceFile");
  }

  // helloworld.v, read back: `areas := [...]` then `for area in areas {...}`.
  const fs::path hello = fs::path(CURSIV_EXAMPLE_DIR) / "helloworld.v";
  const std::string text = read_file(hello);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& file = *result.value_as<v::ast::SourceFile>();
  if (file.decls.size() != 1) return fail("helloworld.v: wrong decl count");

  const auto* const fn = as<v::ast::FunctionDecl>(file.decls[0]);
  if (fn == nullptr || fn->name != "main" || fn->body == nullptr)
    return fail("helloworld.v: main was not read back");
  if (fn->body->stmts.size() != 2)
    return fail("helloworld.v: main should hold two statements");

  const auto* const areas = as<v::ast::VarDecl>(fn->body->stmts[0]);
  if (areas == nullptr || areas->names.size() != 1 ||
      areas->names[0] != "areas" || areas->values.size() != 1)
    return fail("helloworld.v: areas was not read back");
  const auto* const list = as<v::ast::ArrayLit>(areas->values[0]);
  if (list == nullptr || list->elements.size() != 8)
    return fail("helloworld.v: areas should hold eight elements");
  const auto* const first = as<v::ast::StringLit>(list->elements[0]);
  if (first == nullptr || first->text != "'game'")
    return fail("helloworld.v: areas[0] should read 'game'");

  const auto* const loop = as<v::ast::ForStmt>(fn->body->stmts[1]);
  if (loop == nullptr || loop->inIterable != "areas" ||
      loop->inTargets.size() != 1)
    return fail("helloworld.v: the for-in loop was not read back");
  const auto* const target = as<v::ast::Ident>(loop->inTargets[0]);
  if (target == nullptr || target->name != "area")
    return fail("helloworld.v: the for-in target should read 'area'");

  std::puts("v: every sample parses, and helloworld.v reads back clean");
  return 0;
}
