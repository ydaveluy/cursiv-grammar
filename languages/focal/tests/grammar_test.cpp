// The smoke test of the language: parse every file under example/ and check
// each one comes out clean -- no repaired input, no diagnostics, the right
// root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "focal/core/CoreServices.hpp"
#include "focal/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "focal: %s\n", what);
  return 1;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      focal::FocalCoreServices, focal::FocalSharedCoreServices,
      focal::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("01.01 SET X=-1.5\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto* program = audit_result.value_as<focal::ast::Program>();
    if (!program || program->statements.size() != 1) return 1;
    const auto* command = cursiv::ast_cast<focal::ast::SetCommand>(program->statements[0]->command);
    const auto* value = command ? cursiv::ast_cast<focal::ast::NumberLiteral>(command->value) : nullptr;
    if (!value || value->value != -1.5) return 1;
  }


  const auto language = cursiv::create_language<
      focal::FocalCoreServices, focal::FocalSharedCoreServices,
      focal::install_core_module>();

  int checked = 0;
  // extras.focal lives beside this test, not in the corpus: it exercises
  // the abbreviations, builtins and grouping forms example1.focal does not.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".focal")
        continue;

      std::ifstream file(entry.path());
      if (!file) return fail("an example is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result =
          language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail("an example does not parse clean");
      if (!result.diagnostics.empty())
        return fail("an example parses with diagnostics");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<focal::ast::Program>)
        return fail("the parse built no program");

      ++checked;
    }
  if (checked == 0) return fail("no example was found");

  std::printf("focal: %d examples parse\n", checked);
  return 0;
}
