// The smoke test of the language: every document in example/ parses clean,
// and a few field reads confirm the converters do not silently swallow a
// value.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "gdscript/core/CoreServices.hpp"
#include "gdscript/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "gdscript: %s\n", what.c_str());
  return 1;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

#include "code_audit.hpp"

int main() {
  if (!code_audit::run()) return 1;
  {
    const auto audit_language =
        cursiv::create_language<gdscript::GdscriptCoreServices,
                                gdscript::GdscriptSharedCoreServices,
                                gdscript::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "func f():\n    var x: int = 1\n    var y := 2\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool typed = false, inferred = false;
    for (const auto* node : audit_result.nodes)
      if (const auto* local =
              cursiv::ast_cast<gdscript::ast::LocalVarDecl>(node)) {
        typed = typed || (local->name == "x" && local->typeHint == "int" &&
                          !local->isInferred);
        inferred = inferred || (local->name == "y" && local->isInferred);
      }
    if (!typed || !inferred) return 1;
  }

  const auto language =
      cursiv::create_language<gdscript::GdscriptCoreServices,
                              gdscript::GdscriptSharedCoreServices,
                              gdscript::install_core_module>();

  int checked = 0;
  // `custom_features.gd` lives beside this test, not in the corpus: it is
  // written here for the constructs the five upstream documents never reach.
  std::vector<std::filesystem::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& e : std::filesystem::directory_iterator(dir))
      docs.push_back(e.path());

  for (const auto& entry : docs) {
    if (!std::filesystem::is_regular_file(entry) || entry.extension() != ".gd")
      continue;
    ++checked;

    // The text must outlive every read of the tree: every string_view in
    // the AST points into it.
    const std::string text = readFile(entry);
    const cursiv::ParseResult result = language.language.parser->parse(text);

    if (result.cst().recovered())
      return fail(entry.string() + ": the document does not parse");
    if (!result.diagnostics.empty())
      return fail(entry.string() + ": the document carries a diagnostic");

    if (cursiv::ast_cast<gdscript::ast::Program>(result.value) == nullptr)
      return fail(entry.string() + ": the parse built no Program");
  }
  if (checked == 0) return fail("no .gd document found");

  // Field reads: a converter that silently refuses would still leave the
  // structural shape intact, so read a few actual values back out.
  {
    const std::string text = readFile(
        std::filesystem::path(CURSIV_EXAMPLE_DIR) / "issue2422_expr_order.gd");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("issue2422_expr_order.gd: unexpected diagnostics");
    const auto* const program =
        cursiv::ast_cast<gdscript::ast::Program>(result.value);
    if (program == nullptr || program->body.empty() ||
        !cursiv::ast_cast<gdscript::ast::Inheritance>(program->body[0]) ||
        cursiv::ast_cast<gdscript::ast::Inheritance>(program->body[0])->base !=
            "Object")
      return fail("issue2422_expr_order.gd: extends base misread");
    if (program->body.size() != 2)
      return fail("issue2422_expr_order.gd: expected one top-level func");
    const auto* const f =
        cursiv::ast_cast<gdscript::ast::MethodDecl>(program->body[1]);
    if (f == nullptr || f->name != "f")
      return fail("issue2422_expr_order.gd: first decl is not func f");
    if (f->body.empty())
      return fail("issue2422_expr_order.gd: func f has no statements");
  }

  {
    const std::string text =
        readFile(std::filesystem::path(CURSIV_TEST_DIR) / "custom_features.gd");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("custom_features.gd: unexpected diagnostics");
    const auto* const program =
        cursiv::ast_cast<gdscript::ast::Program>(result.value);
    if (program == nullptr) return fail("custom_features.gd: no Program");
    bool sawTool = false, sawSignal = false, sawConst = false;
    for (const gdscript::ast::TopLevelDecl* const decl : program->body) {
      if (cursiv::ast_cast<gdscript::ast::ToolDecl>(decl) != nullptr)
        sawTool = true;
      if (const auto* const sig =
              cursiv::ast_cast<gdscript::ast::SignalDecl>(decl);
          sig != nullptr && sig->name == "hit" && sig->params.size() == 2)
        sawSignal = true;
      if (const auto* const c =
              cursiv::ast_cast<gdscript::ast::ConstDecl>(decl);
          c != nullptr && c->name == "MAX_SPEED" && c->typeHint == "int")
        sawConst = true;
    }
    if (!sawTool) return fail("custom_features.gd: tool decl misread");
    if (!sawSignal) return fail("custom_features.gd: signal hit misread");
    if (!sawConst) return fail("custom_features.gd: const MAX_SPEED misread");
  }

  std::puts("gdscript: every example parses");
  return 0;
}
