// The smoke test of the language: parse every file under example/, and
// fail on a recovered parse or a wrong root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "lisa/core/CoreServices.hpp"
#include "lisa/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& path, const char* what) {
  std::fprintf(stderr, "lisa: %s: %s\n", path.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      lisa::LisaCoreServices, lisa::LisaSharedCoreServices,
      lisa::install_core_module>();

  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : fs::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".lisa")
        files.push_back(entry.path());
  if (files.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example files found");

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<lisa::ast::Program>)
      return fail(path, "the parse built no Program");
    if (!result.diagnostics.empty())
      return fail(path, "the parse carries diagnostics");
  }

  {
    const auto result = language.language.parser->parse(
        "declare{int x;}program{x= - //sign\n2; x=123456789012345678901234567890; x=2=3;}");
    const auto* program = result.value_as<lisa::ast::Program>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !program || program->statements.size() != 3)
      return fail("native-followup", "native signed/unbounded assignment forms");
    const auto assignment = [&](std::size_t i) {
      const auto* stmt = cursiv::ast_cast<lisa::ast::ExpressionStatement>(program->statements[i]);
      return stmt ? cursiv::ast_cast<lisa::ast::AssignExpr>(stmt->expression) : nullptr;
    };
    const auto* first = assignment(0);
    const auto* second = assignment(1);
    const auto* chain = assignment(2);
    const auto* signedNumber = first ? cursiv::ast_cast<lisa::ast::IntegerLiteral>(first->value) : nullptr;
    const auto* huge = second ? cursiv::ast_cast<lisa::ast::IntegerLiteral>(second->value) : nullptr;
    const auto* last = chain && chain->continuations.size() == 1
        ? cursiv::ast_cast<lisa::ast::IntegerLiteral>(chain->continuations[0]->value) : nullptr;
    if (!signedNumber || signedNumber->value != "-2" || !huge ||
        huge->value != "123456789012345678901234567890" || !last || last->value != "3")
      return fail("native-followup", "integer or assignment-chain AST lost");
  }
  for (const auto source : {"declare{int WHILE;}program{break;}",
                            "declare{int x;}program{x=-0;}"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-followup", "native reserved/zero rejection");
  }
  std::printf("lisa: %zu example files parse clean\n", files.size());
  return 0;
}
