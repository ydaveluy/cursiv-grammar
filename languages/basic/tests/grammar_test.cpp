// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Program at the root. One
// document's values are also read back, so a converter that silently drops
// or mis-converts a value would still fail this test.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "basic/core/CoreServices.hpp"
#include "basic/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = basic::ast;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "basic: %s: %s\n", file.string().c_str(), what);
  return 1;
}

// hello.bas: `100 PRINT "Hello world"` -- one line, one PRINT statement with
// one string value, nothing more.
int check_hello_values(const fs::path& path, const cursiv::ParseResult& result) {
  const auto& program = *static_cast<const ast::Program*>(result.value);
  if (program.lines.size() != 1) return fail(path, "expected one line");
  const ast::Line& line = *program.lines[0];
  if (line.number != 100 || line.items.size() != 1)
    return fail(path, "line did not read back");
  const ast::Stmt* const stmt = line.items[0]->stmt;
  if (stmt == nullptr || stmt->type != &cursiv::ast_type_tag<ast::PrintStmt>)
    return fail(path, "expected a PRINT statement");
  const auto& print = *static_cast<const ast::PrintStmt*>(stmt);
  if (print.items.size() != 1 ||
      print.items[0]->value->type != &cursiv::ast_type_tag<ast::StringLit>)
    return fail(path, "expected one string value");
  const auto& str = *static_cast<const ast::StringLit*>(print.items[0]->value);
  if (str.value != "Hello world")
    return fail(path, "the printed string did not read back");
  return 0;
}

// strings.bas: LET, string functions and a naked assignment's RHS all read
// back to the values the source spells.
int check_strings_values(const fs::path& path,
                         const cursiv::ParseResult& result) {
  const auto& program = *static_cast<const ast::Program*>(result.value);
  if (program.lines.empty()) return fail(path, "no lines");
  const ast::Line& first = *program.lines[0];
  const ast::Stmt* const stmt = first.items[0]->stmt;
  if (stmt == nullptr || stmt->type != &cursiv::ast_type_tag<ast::AssignStmt>)
    return fail(path, "expected an assignment on the first line");
  const auto& assign = *static_cast<const ast::AssignStmt*>(stmt);
  if (!assign.explicitLet || assign.target == nullptr ||
      assign.target->name != "S" ||
      assign.target->suffix != ast::VarSuffix::dollar)
    return fail(path, "LET S$ did not read back");
  if (assign.values.size() != 1 ||
      assign.values[0]->type != &cursiv::ast_type_tag<ast::StringLit>)
    return fail(path, "S$ = ... did not read back as a string literal");
  const auto& str = *static_cast<const ast::StringLit*>(assign.values[0]);
  if (str.value != "abc123") return fail(path, "S$ value did not read back");
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      basic::BasicCoreServices, basic::BasicSharedCoreServices,
      basic::install_core_module>();

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse("10 PRINT 1,,2;\n");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    const basic::ast::PrintStmt *print = nullptr;
    for (const auto *node : result.nodes)
      if (const auto *value = cursiv::ast_cast<basic::ast::PrintStmt>(node))
        print = value;
    if (!print || print->items.size() != 4 || !print->items[0]->value ||
        print->items[1]->value || print->items[1]->separator != "," ||
        !print->items[2]->value || print->items[3]->value ||
        print->items[3]->separator != ";")
      return 1;
  }

  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".bas")
      continue;
    ++count;
    const fs::path path = entry.path();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
      failures += fail(path, "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(path, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(path, "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Program>) {
      failures += fail(path, "the parse built no Program");
      continue;
    }

    if (path.filename() == "hello.bas")
      failures += check_hello_values(path, result);
    if (path.filename() == "strings.bas")
      failures += check_strings_values(path, result);
  }

  if (count == 0) {
    std::fprintf(stderr, "basic: no .bas file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("basic: %d example documents parse clean\n", count);
  return 0;
}
