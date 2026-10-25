// The grammar's smoke test: every document under example/ (the upstream
// corpus) parses clean, so do the documents of my own under tests/ that
// exercise constructs the corpus never reaches, and a few real field values
// read back correctly -- a converter that silently gave the wrong value
// would still pass a structural-only check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "apex/core/CoreServices.hpp"
#include "apex/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "apex: %s\n", what.c_str());
  return 1;
}

std::string read(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

int check_clean(const cursiv::Parser& parser, const std::filesystem::path& path,
                std::string* text_out) {
  const std::string text = read(path);
  const cursiv::ParseResult result = parser.parse(text);
  if (result.cst().recovered())
    return fail(path.filename().string() + " does not parse clean");
  if (!result.diagnostics.empty())
    return fail(path.filename().string() + " carries a diagnostic");
  if (result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<apex::ast::CompilationUnit>)
    return fail(path.filename().string() + " built no compilation unit");
  if (text_out != nullptr) *text_out = text;
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      apex::ApexCoreServices, apex::ApexSharedCoreServices, apex::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cls") continue;
    if (const int rc = check_clean(parser, entry.path(), nullptr); rc != 0) return rc;
    ++count;
  }
  if (count == 0) return fail("no example files found");

  if (const int rc = check_clean(
          parser, std::filesystem::path(CURSIV_TEST_DIR) / "PackageAndGenerics.cls", nullptr);
      rc != 0)
    return rc;

  // GrammarShowcase.cls, kept alive in this scope: every string_view in the
  // tree it builds points into `showcaseText`, so the text must outlive
  // every read of the parsed tree.
  std::string showcaseText;
  if (const int rc = check_clean(
          parser, std::filesystem::path(CURSIV_TEST_DIR) / "GrammarShowcase.cls", &showcaseText);
      rc != 0)
    return rc;

  // Read real values back out of it: a converter that silently yields the
  // wrong value passes every structural check above.
  const cursiv::ParseResult showcaseResult = parser.parse(showcaseText);
  const auto* unit = as<apex::ast::CompilationUnit>(showcaseResult.value);
  if (unit == nullptr) return fail("GrammarShowcase.cls built no compilation unit");
  if (unit->types.size() != 1) return fail("GrammarShowcase.cls has more than one type");

  const auto* classDecl = as<apex::ast::ClassDecl>(unit->types[0]);
  if (classDecl == nullptr) return fail("GrammarShowcase.cls's type is not a class");
  if (classDecl->name != "GrammarShowcase") return fail("wrong class name");

  bool sawPublic = false, sawWithSharing = false;
  for (const std::string_view m : classDecl->modifiers) {
    if (m == "public") sawPublic = true;
    if (m == "with sharing") sawWithSharing = true;
  }
  if (!sawPublic) return fail("class modifiers lost 'public'");
  if (!sawWithSharing) return fail("class modifiers lost 'with sharing'");

  const apex::ast::FieldDecl* valueField = nullptr;
  const apex::ast::MethodDecl* runMethod = nullptr;
  for (const apex::ast::Item* const member : classDecl->members) {
    if (const auto* field = as<apex::ast::FieldDecl>(member);
        field != nullptr && !field->declarators.empty() &&
        field->declarators[0]->name == "value")
      valueField = field;
    if (const auto* method = as<apex::ast::MethodDecl>(member);
        method != nullptr && method->name == "run")
      runMethod = method;
  }
  if (valueField == nullptr) return fail("field 'value' not found");
  const auto* valueType = as<apex::ast::ClassTypeRef>(valueField->type);
  if (valueType == nullptr || valueType->name != "Integer")
    return fail("field 'value' does not have type Integer");

  if (runMethod == nullptr) return fail("method 'run' not found");
  if (runMethod->params.size() != 1 || runMethod->params[0]->name != "obj")
    return fail("method 'run' does not take a single parameter 'obj'");

  // `a ?? b ?? c` is left-associative: the outer `??`'s
  // left operand is itself a `??` BinaryExpr, not its right operand.
  if (runMethod->body == nullptr) return fail("method 'run' has no body");
  const apex::ast::Expr* chainedInit = nullptr;
  for (const apex::ast::Item* const item : runMethod->body->items) {
    const auto* decl = as<apex::ast::LocalVarDecl>(item);
    if (decl == nullptr) continue;
    for (const apex::ast::VarDeclarator* const d : decl->declarators)
      if (d->name == "chained") {
        const auto* init = as<apex::ast::ExprInit>(d->init);
        if (init != nullptr) chainedInit = init->value;
      }
  }
  const auto* outer = as<apex::ast::BinaryExpr>(chainedInit);
  if (outer == nullptr || outer->op != "??")
    return fail("'chained' is not a null-coalescing BinaryExpr");
  const auto* outerLeft = as<apex::ast::BinaryExpr>(outer->left);
  if (outerLeft == nullptr || outerLeft->op != "??")
    return fail("'a ?? b ?? c' does not bind null-coalescing left-associatively");

  // `-` is never part of a name, and a letter past ASCII is.
  const std::string minusText =
      "class A { Integer m(Integer a, Integer b) { Integer caf\u00e9 = a-b; return-1; } }";
  const cursiv::ParseResult minusResult = parser.parse(minusText);
  if (minusResult.cst().recovered() || !minusResult.diagnostics.empty())
    return fail("'a-b; return-1;' does not parse clean");
  const auto* minusUnit = as<apex::ast::CompilationUnit>(minusResult.value);
  const auto* minusClass =
      minusUnit != nullptr && minusUnit->types.size() == 1
          ? as<apex::ast::ClassDecl>(minusUnit->types[0])
          : nullptr;
  const auto* minusMethod =
      minusClass != nullptr && minusClass->members.size() == 1
          ? as<apex::ast::MethodDecl>(minusClass->members[0])
          : nullptr;
  if (minusMethod == nullptr || minusMethod->body == nullptr ||
      minusMethod->body->items.size() != 2)
    return fail("'a-b; return-1;' built the wrong statements");
  const auto* minusDecl = as<apex::ast::LocalVarDecl>(minusMethod->body->items[0]);
  if (minusDecl == nullptr || minusDecl->declarators.size() != 1 ||
      minusDecl->declarators[0]->name != "caf\u00e9")
    return fail("a non-ASCII identifier is not a name");
  const auto* minusInit = as<apex::ast::ExprInit>(minusDecl->declarators[0]->init);
  const auto* minusBinary =
      minusInit != nullptr ? as<apex::ast::BinaryExpr>(minusInit->value) : nullptr;
  if (minusBinary == nullptr || minusBinary->op != "-" ||
      as<apex::ast::Ident>(minusBinary->left) == nullptr ||
      as<apex::ast::Ident>(minusBinary->right) == nullptr)
    return fail("'a-b' is not a subtraction of two names");
  const auto* minusReturn = as<apex::ast::ReturnStmt>(minusMethod->body->items[1]);
  const auto* minusNegation =
      minusReturn != nullptr ? as<apex::ast::PrefixExpr>(minusReturn->value) : nullptr;
  if (minusNegation == nullptr || minusNegation->op != "-")
    return fail("'return-1' is not a return of a negation");

  std::printf("apex: %d examples parse clean\n", count);
  return 0;
}
