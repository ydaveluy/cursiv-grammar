#include "audit_regressions.hpp"
// The smoke test of the language: parse every example, read a few field values
// back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "smalltalk/core/CoreServices.hpp"
#include "smalltalk/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "smalltalk: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language =
      cursiv::create_language<smalltalk::SmalltalkCoreServices,
                              smalltalk::SmalltalkSharedCoreServices,
                              smalltalk::install_core_module>();

  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".st")
        files.push_back(entry.path());
  if (files.empty()) return fail("no example documents found");

  // Keep every document's text AND its ParseResult alive for the whole run:
  // the CST and AST hold string_views into the text, and the AST nodes
  // themselves live in the ParseResult's arena.
  std::vector<std::string> texts;
  texts.reserve(files.size());
  std::vector<cursiv::ParseResult> results;
  results.reserve(files.size());

  const smalltalk::ast::Sequence* sample = nullptr;
  for (const auto& path : files) {
    texts.push_back(read_file(path));
    const std::string& text = texts.back();

    results.push_back(language.language.parser->parse(text));
    const cursiv::ParseResult& result = results.back();
    if (result.cst().recovered())
      return fail(path.string() + ": does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<smalltalk::ast::Sequence>)
      return fail(path.string() + ": built no Sequence root");

    if (path.filename() == "temps-and-blocks.st")
      sample = static_cast<const smalltalk::ast::Sequence*>(root);
  }

  if (sample == nullptr) return fail("temps-and-blocks.st was not read");

  if (sample->temps.size() != 2 || sample->temps[0] != "sum" ||
      sample->temps[1] != "items")
    return fail("temps-and-blocks.st: wrong temps");
  if (sample->statements.size() != 3)
    return fail("temps-and-blocks.st: wrong statement count");
  if (sample->answer == nullptr ||
      sample->answer->type != &cursiv::ast_type_tag<smalltalk::ast::VarRef>)
    return fail("temps-and-blocks.st: wrong answer");
  if (static_cast<const smalltalk::ast::VarRef*>(sample->answer)->name != "sum")
    return fail("temps-and-blocks.st: answer names the wrong variable");

  const cursiv::AstNode* const first = sample->statements[0];
  if (first->type != &cursiv::ast_type_tag<smalltalk::ast::Assignment>)
    return fail("temps-and-blocks.st: first statement is not an assignment");
  const auto& assign = *static_cast<const smalltalk::ast::Assignment*>(first);
  if (assign.name != "sum") return fail("temps-and-blocks.st: wrong lhs");
  if (assign.value->type !=
      &cursiv::ast_type_tag<smalltalk::ast::NumberLiteral>)
    return fail("temps-and-blocks.st: wrong rhs type");
  if (static_cast<const smalltalk::ast::NumberLiteral*>(assign.value)->value !=
      0.0)
    return fail("temps-and-blocks.st: wrong rhs value");

  const cursiv::AstNode* const third = sample->statements[2];
  if (third->type != &cursiv::ast_type_tag<smalltalk::ast::MessageSend>)
    return fail("temps-and-blocks.st: third statement is not a send");
  const auto& send = *static_cast<const smalltalk::ast::MessageSend*>(third);
  if (send.selectorParts.size() != 1 || send.selectorParts[0] != "do:")
    return fail("temps-and-blocks.st: wrong selector");
  if (send.args.size() != 1 ||
      send.args[0]->type != &cursiv::ast_type_tag<smalltalk::ast::Block>)
    return fail("temps-and-blocks.st: wrong argument");
  const auto& block = *static_cast<const smalltalk::ast::Block*>(send.args[0]);
  if (block.params.size() != 1 || block.params[0] != "each")
    return fail("temps-and-blocks.st: wrong block params");

  std::puts("smalltalk: every sample parses and reads back");
  return 0;
}
