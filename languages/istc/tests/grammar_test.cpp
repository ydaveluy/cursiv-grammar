// The smoke test of the language: build the container the way a workspace
// does, parse every file under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "istc/core/CoreServices.hpp"
#include "istc/core/ast.hpp"

namespace {

int fail(const std::string& file, const char* what) {
  std::fprintf(stderr, "istc: %s: %s\n", file.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      istc::IstcCoreServices, istc::IstcSharedCoreServices,
      istc::install_core_module>();

  // Independent examples from ISO 21047:2009, clauses 4.1 and Annex A.
  for (const auto source : {"ISTC 0A9-2002-12B4A105-7",
                            "ISTC 0A9 2002 1223F332 0",
                            "ISTC 0A920021223F3320",
                            "ISTC  0A9  2002  1223F332  0",
                            "ISTC A02-2009-000004BE-A"}) {
    auto result = cursiv::test::validationHelper(language.language, source);
    if (!result.document->parseResult.accepted ||
        result.document->parseResult.cst().recovered() ||
        !result.diagnostics.empty())
      return fail(source, "valid ISTC rejected");
    result.dispose();
  }
  {
    const auto result = language.language.parser->parse("ISTC 0A920021223F3320");
    const auto* code = result.value_as<istc::ast::Istc>();
    if (!code || code->registration != "0A9" || code->year != "2002" ||
        code->work != "1223F332" || code->check != "0")
      return fail("compact ISTC", "adjacent component captures lost");
  }
  for (const auto source : {"ISTC 0A9-2002-12B4A105-8",
                            "ISTC 0A9-20A2-12B4A105-7"}) {
    auto result = cursiv::test::validationHelper(language.language, source);
    if (!result.document->parseResult.accepted ||
        result.document->parseResult.cst().recovered() ||
        result.diagnostics.empty())
      return fail(source, "invalid year or check digit was not diagnosed");
    result.dispose();
  }
  for (const auto source : {"ISTC A0\t2-2009-000004BE-A",
                            "ISTC A02-20\n09-000004BE-A",
                            "ISTC A02-2009-0000\t04BE-A"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty())
      return fail(source, "a component must contain adjacent digits");
  }

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".istc")
        continue;
      const std::string path = entry.path().string();

      std::ifstream file(entry.path());
      if (!file) return fail(path, "the file is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) return fail(path, "does not parse clean");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<istc::ast::Istc>)
        return fail(path, "the parse built no Istc");

      const auto& node = *static_cast<const istc::ast::Istc*>(root);
      if (node.registration.size() != 3 || node.year.size() != 4 ||
          node.work.size() != 8 || node.check.size() != 1)
        return fail(path, "a field does not hold its expected width");

      ++count;
    }

  if (count == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  std::printf("istc: %d example(s) parse\n", count);
  return 0;
}
