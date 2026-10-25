// The grammar end to end: every document under example/ (the upstream
// corpus) and tests/ (constructs the corpus never reaches) parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of two documents' ASTs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "bcpl/core/ast.hpp"
#include "bcpl/generated/Bcpl/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "bcpl: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      bcpl::generated::Bcpl::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const bcpl::ast::FunctionDef* find_function(const bcpl::ast::Program& program,
                                            std::string_view name) {
  for (const bcpl::ast::Decl* const decl : program.declarations) {
    if (decl->type != &cursiv::ast_type_tag<bcpl::ast::LetDecl>) continue;
    const auto& let = *static_cast<const bcpl::ast::LetDecl*>(decl);
    for (const bcpl::ast::Definition* const def : let.definitions) {
      if (def->type != &cursiv::ast_type_tag<bcpl::ast::FunctionDef>) continue;
      const auto& fn = *static_cast<const bcpl::ast::FunctionDef*>(def);
      if (fn.name != nullptr && fn.name->name == name) return &fn;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".bcpl")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path fact_doc;
  fs::path match_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bcpl::ast::Program>)
      return fail(doc.string() + ": the parse built no program");
    if (doc.filename() == "fact.bcpl") fact_doc = doc;
    if (doc.filename() == "match.bcpl") match_doc = doc;
  }
  if (fact_doc.empty()) return fail("fact.bcpl was not found among the examples");
  if (match_doc.empty()) return fail("match.bcpl was not found under tests/");

  // Read a few real field values out of fact.bcpl: a converter or a
  // structural mistake would still leave `recovered()` false and the root
  // type right, so only reading the fields back catches it.
  {
    const std::string text = read_file(fact_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& program = *static_cast<const bcpl::ast::Program*>(result.value);
    if (program.directives.size() != 1 ||
        program.directives[0]->kw != "GET")
      return fail("fact.bcpl: wrong directive");
    const bcpl::ast::FunctionDef* const factorial =
        find_function(program, "factorial");
    if (factorial == nullptr)
      return fail("fact.bcpl: no `factorial` definition");
    const auto& fn = *factorial;
    if (fn.params.size() != 1 || fn.params[0]->name != "n")
      return fail("fact.bcpl: `factorial` has the wrong parameters");
    if (fn.body->type != &cursiv::ast_type_tag<bcpl::ast::CondExpr>)
      return fail("fact.bcpl: `factorial`'s body is not a conditional");
  }

  // ...and from match.bcpl, that a pattern clause and its result survive.
  {
    const std::string text = read_file(match_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& program = *static_cast<const bcpl::ast::Program*>(result.value);
    const bcpl::ast::FunctionDef* const classify =
        find_function(program, "classify");
    if (classify == nullptr)
      return fail("match.bcpl: no `classify` definition");
    const auto& fn = *classify;
    if (fn.body->type != &cursiv::ast_type_tag<bcpl::ast::ValofExpr>)
      return fail("match.bcpl: `classify`'s body is not VALOF");
  }

  std::printf("bcpl: %zu documents parse clean\n", docs.size());
  return 0;
}
