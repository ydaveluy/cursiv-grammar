// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Program at the root; a few
// field values are read back to catch a silently-wrong converter, and one
// authored document (tests/extras.bicep, never counted as example corpus)
// exercises constructs no upstream file reaches. A deliberately malformed
// document, kept as a string literal here rather than a file, must be
// refused.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "bicep/core/CoreServices.hpp"
#include "bicep/core/ast.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = bicep::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "bicep: %s\n", what.c_str());
  return 1;
}

std::string read(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(node)
             : nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      bicep::BicepCoreServices, bicep::BicepSharedCoreServices,
      bicep::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  int failures = 0;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR "/example")) {
    if (!entry.is_regular_file() || entry.path().extension() != ".bicep")
      continue;
    ++count;

    const std::string text = read(entry.path());
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path().string() + ": the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path().string() + ": the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Program>) {
      failures += fail(entry.path().string() + ": the parse built no Program");
      continue;
    }
  }
  if (count == 0) {
    std::fprintf(stderr, "bicep: no .bicep file found under example/\n");
    return 1;
  }

  // -- Field readback: a converter that silently yields the wrong value
  // passes every structural check above. ----

  {
    const std::string text = read(fs::path(CURSIV_EXAMPLE_DIR) / "example" / "targetScope.bicep");
    const cursiv::ParseResult result = parser.parse(text);
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr || program->body.size() != 1)
      failures += fail("targetScope.bicep: expected exactly one statement");
    else {
      const auto* decl = as<ast::TargetScopeDecl>(program->body[0]);
      const auto* value = decl != nullptr ? as<ast::InterpString>(decl->value) : nullptr;
      if (value == nullptr || value->quasis.size() != 1 || value->quasis[0] != "test")
        failures += fail("targetScope.bicep: value should read back as 'test'");
    }
  }

  {
    const std::string text = read(fs::path(CURSIV_EXAMPLE_DIR) / "example" / "program.bicep");
    const cursiv::ParseResult result = parser.parse(text);
    const auto* program = as<ast::Program>(result.value);
    const auto* first = program != nullptr && !program->body.empty()
                             ? as<ast::ParameterDecl>(program->body[0])
                             : nullptr;
    if (first == nullptr || first->name != "name")
      failures += fail("program.bicep: first statement should be param 'name'");
  }

  {
    const std::string text = read(fs::path(CURSIV_EXAMPLE_DIR) / "example" / "typeDecl.bicep");
    const cursiv::ParseResult result = parser.parse(text);
    const auto* program = as<ast::Program>(result.value);
    const auto* first = program != nullptr && !program->body.empty()
                             ? as<ast::TypeDecl>(program->body[0])
                             : nullptr;
    const auto* literal = first != nullptr ? as<ast::LiteralType>(first->type) : nullptr;
    const auto* number = literal != nullptr ? as<ast::NumberLit>(literal->value) : nullptr;
    if (number == nullptr || number->text != "98765432123456789")
      failures += fail("typeDecl.bicep: the huge integer literal should read back byte for byte");
  }

  // -- The authored document, exercising constructs no example/ file
  // reaches: bool/union/object/tuple/paren/named types, a lambda, string
  // escapes, a multiline string, the member ':' form. ----
  {
    const std::string text = read(fs::path(CURSIV_TEST_DIR) / "extras.bicep");
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      failures += fail("extras.bicep: should parse clean");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr || program->body.size() != 13)
      failures += fail("extras.bicep: expected 13 top-level statements");

    // `a + b * c == d && e || f ?? g`: multiplicative > additive >
    // equality > && > || > ?? (Microsoft Learn, "Bicep operators",
    // Operator precedence and associativity).
    const auto* prec =
        program != nullptr && program->body.size() == 13
            ? as<ast::VariableDecl>(program->body[12])
            : nullptr;
    const auto* coalesce = prec != nullptr ? as<ast::BinaryExpr>(prec->value) : nullptr;
    const auto* orExpr =
        coalesce != nullptr ? as<ast::BinaryExpr>(coalesce->left) : nullptr;
    const auto* andExpr =
        orExpr != nullptr ? as<ast::BinaryExpr>(orExpr->left) : nullptr;
    const auto* eqExpr =
        andExpr != nullptr ? as<ast::BinaryExpr>(andExpr->left) : nullptr;
    const auto* addExpr =
        eqExpr != nullptr ? as<ast::BinaryExpr>(eqExpr->left) : nullptr;
    const auto* mulExpr =
        addExpr != nullptr ? as<ast::BinaryExpr>(addExpr->right) : nullptr;
    if (coalesce == nullptr || coalesce->op != "??" || orExpr == nullptr ||
        orExpr->op != "||" || andExpr == nullptr || andExpr->op != "&&" ||
        eqExpr == nullptr || eqExpr->op != "==" || addExpr == nullptr ||
        addExpr->op != "+" || mulExpr == nullptr || mulExpr->op != "*")
      failures += fail("extras.bicep: && / || / ?? precedence lost its levels");
  }

  // -- A malformed document (no NL+ before an object's first property):
  // the CST must report recovery or a diagnostic. ----
  {
    const std::string bad = "var x = {\n  a: 1\n}\nvar y = { a: 1 }\n";
    const cursiv::ParseResult result = parser.parse(bad);
    if (!result.cst().recovered() && result.diagnostics.empty())
      failures += fail("a single-line object with a property should be refused");
  }

  if (failures > 0) return 1;
  std::printf("bicep: %d example documents parse clean\n", count);
  return 0;
}
