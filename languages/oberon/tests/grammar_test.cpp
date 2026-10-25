
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "oberon/core/CoreServices.hpp"
#include "oberon/core/ast.hpp"
// The smoke test of the language: every corpus document parses clean,
// features.mod (the constructs the one-file corpus never reaches) parses
// clean too, and hello.mod is read back field by field to catch a
// converter that silently mismaps its value.

#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "oberon: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "hello.mod",
};

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<oberon::OberonCoreServices,
                              oberon::OberonSharedCoreServices,
                              oberon::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "MODULE M; VAR T: INTEGER; PROCEDURE p(x: INTEGER); BEGIN END p; BEGIN "
        "p(T) END M.");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      oberon::OberonCoreServices, oberon::OberonSharedCoreServices,
      oberon::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<oberon::ast::Module>)
      return fail((std::string(name) + ": built no Module").c_str());
  }

  // features.mod covers what hello.mod never reaches: CONST/TYPE/VAR
  // sections, record extension, arrays, pointers, procedure types, nested
  // procedures, IF/ELSIF/ELSE, CASE with ranges, WHILE with ELSIF, REPEAT,
  // FOR BY, sets, IN/IS, hex literals, nested comments, type guards.
  const std::string featuresText = read_file(CURSIV_FEATURES_SAMPLE);
  if (featuresText.empty()) return fail("features.mod: empty or unreadable");
  const cursiv::ParseResult featuresResult =
      language.language.parser->parse(featuresText);
  if (featuresResult.cst().recovered())
    return fail("features.mod: does not parse clean");
  if (!featuresResult.diagnostics.empty())
    return fail("features.mod: has diagnostics");
  if (featuresResult.value == nullptr ||
      featuresResult.value->type != &cursiv::ast_type_tag<oberon::ast::Module>)
    return fail("features.mod: built no Module");

  // Read back the converters that a structural check alone would not catch:
  // hex integer, hex char string and a scaled real.
  const auto& features =
      *static_cast<const oberon::ast::Module*>(featuresResult.value);
  if (features.decls.size() < 5) return fail("features.mod: too few declarations");

  const auto& hexDecl =
      *static_cast<const oberon::ast::ConstDecl*>(features.decls[2]);
  if (hexDecl.name != "Hex") return fail("features.mod: expected Hex at decls[2]");
  const auto& hexValue = *static_cast<const oberon::ast::IntLit*>(hexDecl.value);
  if (hexValue.value != 255) return fail("features.mod: 0FFH did not convert to 255");

  const auto& chDecl =
      *static_cast<const oberon::ast::ConstDecl*>(features.decls[3]);
  if (chDecl.name != "Ch") return fail("features.mod: expected Ch at decls[3]");
  const auto& chValue = *static_cast<const oberon::ast::StringLit*>(chDecl.value);
  if (chValue.value != "A") return fail("features.mod: 41X did not convert to \"A\"");

  const auto& msgDecl =
      *static_cast<const oberon::ast::ConstDecl*>(features.decls[4]);
  if (msgDecl.name != "Msg") return fail("features.mod: expected Msg at decls[4]");
  const auto& msgValue = *static_cast<const oberon::ast::StringLit*>(msgDecl.value);
  if (msgValue.value != "hi") return fail("features.mod: wrong quoted string value");

  if (features.body.size() < 3) return fail("features.mod: too few statements");
  const auto& rAssign =
      *static_cast<const oberon::ast::AssignStmt*>(features.body[2]);
  const auto& rValue = *static_cast<const oberon::ast::RealLit*>(rAssign.value);
  if (rValue.value != 150.0) return fail("features.mod: 1.5E2 did not convert to 150");

  // hello.mod, read back field by field.
  const std::string helloText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/hello.mod");
  const cursiv::ParseResult result = language.language.parser->parse(helloText);
  const auto& module = *static_cast<const oberon::ast::Module*>(result.value);
  if (module.name != "Hello") return fail("hello.mod: wrong module name");
  if (module.endName != "Hello")
    return fail("hello.mod: wrong module end name");
  if (module.imports.size() != 2)
    return fail("hello.mod: expected two imports");
  if (module.imports[0]->name != "Oberon" || !module.imports[0]->actual.empty())
    return fail("hello.mod: wrong first import");
  if (module.imports[1]->name != "Texts")
    return fail("hello.mod: wrong second import");
  if (module.decls.size() != 2)
    return fail("hello.mod: expected two declarations");

  if (module.decls[0]->type != &cursiv::ast_type_tag<oberon::ast::VarDecl>)
    return fail("hello.mod: first declaration is not a VarDecl");

  if (module.decls[1]->type != &cursiv::ast_type_tag<oberon::ast::ProcDecl>)
    return fail("hello.mod: second declaration is not a ProcDecl");
  const auto& world = *static_cast<const oberon::ast::ProcDecl*>(module.decls[1]);
  if (world.name != "World*") return fail("hello.mod: wrong procedure name");
  if (world.params != nullptr)
    return fail("hello.mod: procedure should take no parameters");
  // A `;` before END leaves an empty statement behind it.
  if (world.body.size() != 4)
    return fail("hello.mod: expected three statements and an empty one");
  if (world.body[3]->type != &cursiv::ast_type_tag<oberon::ast::EmptyStmt>)
    return fail("hello.mod: the last statement of World is not empty");

  const auto& call0 = *static_cast<const oberon::ast::ExprStmt*>(world.body[0]);
  if (call0.value->type != &cursiv::ast_type_tag<oberon::ast::ApplyExpr>)
    return fail("hello.mod: first statement is not a call");
  const auto& write = *static_cast<const oberon::ast::ApplyExpr*>(call0.value);
  if (write.operand->type != &cursiv::ast_type_tag<oberon::ast::NameExpr>)
    return fail("hello.mod: call callee is not a NameExpr");
  const auto& writeCallee =
      *static_cast<const oberon::ast::NameExpr*>(write.operand);
  if (writeCallee.name != "Texts.WriteString")
    return fail("hello.mod: wrong callee name");
  if (write.args.size() != 2) return fail("hello.mod: expected two arguments");
  if (write.args[1]->type != &cursiv::ast_type_tag<oberon::ast::StringLit>)
    return fail("hello.mod: second argument is not a string literal");
  const auto& greeting = *static_cast<const oberon::ast::StringLit*>(write.args[1]);
  if (greeting.value != "Hello World!")
    return fail("hello.mod: wrong string literal value");

  if (module.body.size() != 2)
    return fail("hello.mod: expected a top-level statement and an empty one");
  const auto& open = *static_cast<const oberon::ast::ExprStmt*>(module.body[0]);
  const auto& openCall =
      *static_cast<const oberon::ast::ApplyExpr*>(open.value);
  const auto& openCallee =
      *static_cast<const oberon::ast::NameExpr*>(openCall.operand);
  if (openCallee.name != "Texts.OpenWriter")
    return fail("hello.mod: wrong top-level call");

  // An empty statement is a statement, an empty CASE arm an arm: `BEGIN END`
  // holds one, a module with no BEGIN holds none.
  const auto parse = [&](const char* text) {
    return language.language.parser->parse(text);
  };
  const cursiv::ParseResult bare = parse("MODULE M; END M.");
  const cursiv::ParseResult empty = parse("MODULE M; BEGIN END M.");
  const cursiv::ParseResult arms =
      parse("MODULE M; BEGIN CASE x OF | 1: END END M.");
  const auto* const bareModule = bare.value_as<oberon::ast::Module>();
  const auto* const emptyModule = empty.value_as<oberon::ast::Module>();
  const auto* const armsModule = arms.value_as<oberon::ast::Module>();
  if (bareModule == nullptr || emptyModule == nullptr || armsModule == nullptr)
    return fail("empty.mod: built no Module");
  if (!bareModule->body.empty() || emptyModule->body.size() != 1 ||
      emptyModule->body[0]->type !=
          &cursiv::ast_type_tag<oberon::ast::EmptyStmt>)
    return fail("BEGIN END: expected one empty statement");
  if (armsModule->body.size() != 1 ||
      armsModule->body[0]->type != &cursiv::ast_type_tag<oberon::ast::CaseStmt>)
    return fail("CASE: built no CaseStmt");
  const auto& caseStmt =
      *static_cast<const oberon::ast::CaseStmt*>(armsModule->body[0]);
  if (caseStmt.arms.size() != 2 || !caseStmt.arms[0]->labels.empty() ||
      caseStmt.arms[1]->labels.size() != 1)
    return fail("CASE: expected an empty arm and a labelled one");

  std::puts("oberon: the corpus parses");
  return 0;
}
