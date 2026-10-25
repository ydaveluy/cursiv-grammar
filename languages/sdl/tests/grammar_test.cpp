// The sdl grammar end to end: every document under `example/` parses clean
// (`recovered()` false, no diagnostic), builds a `Program`, and one document's
// AST is read back field by field so a converter that silently mis-values a
// field cannot pass this lane.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "sdl/core/CoreServices.hpp"
#include "sdl/core/ast.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = sdl::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "sdl: %s\n", what.c_str());
  ++cursiv::test::failures;
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      sdl::SdlCoreServices, sdl::SdlSharedCoreServices,
      sdl::install_core_module>();

  const fs::path exampleDir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  int parsed = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(exampleDir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".sdl")
      continue;
    const std::string path = entry.path().string();
    std::ifstream file(path);
    if (!file) {
      fail(path + ": not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      fail(path + ": the document does not parse clean");
    else if (!result.diagnostics.empty())
      fail(path + ": " + std::to_string(result.diagnostics.size()) +
           " diagnostic(s) on a document expected clean");
    else if (result.value == nullptr ||
             result.value->type != &cursiv::ast_type_tag<ast::Program>)
      fail(path + ": the parse built no Program");
    else
      ++parsed;
  }
  if (parsed == 0) fail("no example document was found");

  // Field-level readback on custom_constructs.sdl: every construct the
  // language accepts that no corpus file exercises.
  std::ifstream custom(
      (fs::path(CURSIV_TEST_DIR) / "custom_constructs.sdl").string());
  const std::string customText((std::istreambuf_iterator<char>(custom)),
                               std::istreambuf_iterator<char>());
  const cursiv::ParseResult r = language.language.parser->parse(customText);
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  auto* const program = r.value_as<ast::Program>();
  CHECK(program != nullptr);
  if (program != nullptr) {
    CHECK(program->items.size() == 1);
    const ast::Item* moduleItem =
        program->items.empty() ? nullptr : program->items[0];
    CHECK(moduleItem != nullptr &&
          moduleItem->type == &cursiv::ast_type_tag<ast::Module>);
    if (moduleItem != nullptr &&
        moduleItem->type == &cursiv::ast_type_tag<ast::Module>) {
      const auto& mod = *static_cast<const ast::Module*>(moduleItem);
      CHECK(mod.name == "custom_constructs");
      CHECK(mod.ident == "1.0");

      bool sawDeclare = false, sawInclude = false, sawRead = false,
           sawAggregate = false, sawEntry = false, sawIfSymbol = false,
           sawConditional = false, sawLiteral = false;
      for (const ast::Item* item : mod.body) {
        if (item == nullptr) continue;
        if (item->type == &cursiv::ast_type_tag<ast::DeclareStmt>) {
          sawDeclare = true;
          const auto& d = *static_cast<const ast::DeclareStmt*>(item);
          CHECK(d.name == "my_int");
          CHECK(d.prefix == "mi_");
          CHECK(d.tag == "tag1");
        } else if (item->type == &cursiv::ast_type_tag<ast::IncludeStmt>) {
          sawInclude = true;
          CHECK(static_cast<const ast::IncludeStmt*>(item)->path ==
                "other.sdl");
        } else if (item->type == &cursiv::ast_type_tag<ast::ReadStmt>) {
          sawRead = true;
          CHECK(static_cast<const ast::ReadStmt*>(item)->path ==
                "included.sdl");
        } else if (item->type == &cursiv::ast_type_tag<ast::AggregateStmt>) {
          sawAggregate = true;
          const auto& agg = *static_cast<const ast::AggregateStmt*>(item);
          CHECK(agg.name == "rec");
          CHECK(agg.kind == "STRUCTURE");
          CHECK(agg.body.size() == 3);
        } else if (item->type == &cursiv::ast_type_tag<ast::EntryDecl>) {
          sawEntry = true;
          const auto& e = *static_cast<const ast::EntryDecl*>(item);
          CHECK(e.name == "my_proc");
          CHECK(e.opts != nullptr);
          if (e.opts != nullptr) {
            CHECK(e.opts->params.size() == 2);
            CHECK(e.opts->aliases.size() == 1 &&
                  e.opts->aliases[0] == "MY_PROC");
            CHECK(e.opts->linkages.size() == 1 && e.opts->linkages[0] == "C");
            CHECK(e.opts->returnNames.size() == 1 &&
                  e.opts->returnNames[0] == "result");
          }
        } else if (item->type == &cursiv::ast_type_tag<ast::IfSymbolStmt>) {
          sawIfSymbol = true;
          const auto& i = *static_cast<const ast::IfSymbolStmt*>(item);
          CHECK(i.symbol == "flag");
          CHECK(i.thenBody.size() == 1);
          CHECK(i.elseIfs.size() == 1);
          CHECK(i.hasElse);
          CHECK(i.elseBody.size() == 1);
        } else if (item->type == &cursiv::ast_type_tag<ast::ConditionalStmt>) {
          sawConditional = true;
          const auto& c = *static_cast<const ast::ConditionalStmt*>(item);
          CHECK(c.languages.size() == 2);
        } else if (item->type == &cursiv::ast_type_tag<ast::LiteralStmt>) {
          sawLiteral = true;
          CHECK(static_cast<const ast::LiteralStmt*>(item)->strings.size() ==
                2);
        } else if (item->type == &cursiv::ast_type_tag<ast::ConstantStmt>) {
          const auto& c = *static_cast<const ast::ConstantStmt*>(item);
          for (const ast::ConstantPhrase* phrase : c.phrases) {
            if (phrase == nullptr) continue;
            if (phrase->names.size() == 1 && phrase->names[0] == "hexc")
              CHECK(phrase->value != nullptr);
          }
        }
      }
      CHECK(sawDeclare);
      CHECK(sawInclude);
      CHECK(sawRead);
      CHECK(sawAggregate);
      CHECK(sawEntry);
      CHECK(sawIfSymbol);
      CHECK(sawConditional);
      CHECK(sawLiteral);
    }
  }

  const cursiv::ParseResult twice = language.language.parser->parse(
      "MODULE m; ITEM i LONGWORD PREFIX \"a_\" TAG t PREFIX \"b_\"; "
      "END_MODULE;");
  CHECK(!twice.cst().recovered() && twice.diagnostics.empty());
  const auto* const twiceProgram = twice.value_as<ast::Program>();
  const auto* const twiceModule =
      twiceProgram != nullptr && twiceProgram->items.size() == 1 &&
              twiceProgram->items[0]->type == &cursiv::ast_type_tag<ast::Module>
          ? static_cast<const ast::Module*>(twiceProgram->items[0])
          : nullptr;
  const auto* const item =
      twiceModule != nullptr && twiceModule->body.size() == 1 &&
              twiceModule->body[0]->type == &cursiv::ast_type_tag<ast::ItemDecl>
          ? static_cast<const ast::ItemDecl*>(twiceModule->body[0])
          : nullptr;
  CHECK(item != nullptr && item->prefixes.size() == 2 &&
        item->prefixes[0] == "a_" && item->prefixes[1] == "b_" &&
        item->tags.size() == 1);

  return cursiv::test::report("sdl_grammar_test") + (parsed == 0 ? 1 : 0);
}
