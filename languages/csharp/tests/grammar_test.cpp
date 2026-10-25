// The smoke test of the language: parse every file under example/, the way a
// workspace would.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "csharp/core/CoreServices.hpp"
#include "csharp/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "modern_cases.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "csharp: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      csharp::CSharpCoreServices, csharp::CSharpSharedCoreServices,
      csharp::install_core_module>();

  if (!csharp_audit::modern_cases(*language.language.parser))
    return 1;

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "ref struct S { A<X>.B<Y> x; int?[] y; int[]? z; string a = $\"{1}\"; "
        "string b = $@\"{2}\"; }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    bool generic = false, nullableArray = false, arrayNullable = false;
    bool regular = false, verbatim = false, refStruct = false;
    for (const auto *node : result.nodes) {
      if (const auto *type = cursiv::ast_cast<csharp::ast::Type>(node)) {
        if (type->segments.size() == 2 && type->segments[0]->name == "A" &&
            type->segments[1]->name == "B")
          generic = type->segments[0]->arguments.size() == 1 &&
                    type->segments[1]->arguments.size() == 1;
        if (type->suffixes.size() == 2) {
          nullableArray |=
              type->suffixes[0] == "?" && type->suffixes[1] == "[]";
          arrayNullable |=
              type->suffixes[0] == "[]" && type->suffixes[1] == "?";
        }
      }
      if (const auto *str =
              cursiv::ast_cast<csharp::ast::InterpolatedStringExpr>(node)) {
        regular |= !str->verbatim;
        verbatim |= str->verbatim;
      }
      if (const auto *decl = cursiv::ast_cast<csharp::ast::StructDecl>(node))
        for (auto modifier : decl->modifiers)
          refStruct |= modifier == "ref";
    }
    if (!generic || !nullableArray || !arrayNullable || !regular || !verbatim ||
        !refStruct)
      return 1;
  }

  int count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cs")
      continue;

    std::ifstream file(entry.path());
    if (!file) return fail("cannot read " + entry.path().string());
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().filename().string() + " does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<csharp::ast::CompilationUnit>)
      return fail(entry.path().filename().string() +
                   " built no compilation unit");
    ++count;
  }
  if (count == 0) return fail("no example files found");

  std::printf("csharp: %d examples parse clean\n", count);
  return 0;
}
