#include "audit_regressions.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "unrealangelscript/core/CoreServices.hpp"
#include "unrealangelscript/core/ast.hpp"

namespace {

namespace ast = unrealangelscript::ast;

int fail(const char* what) {
  std::fprintf(stderr, "unrealangelscript: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

// Every example under example/, all of which come out clean.
const char* const kCleanExamples[] = {
    CURSIV_EXAMPLE_DIR "/Example_Actor.as",
    CURSIV_TEST_DIR "/declarations.as",
    CURSIV_TEST_DIR "/statements.as",
    CURSIV_TEST_DIR "/expressions.as",
    CURSIV_TEST_DIR "/preprocessor.as",
};

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      unrealangelscript::UnrealAngelscriptCoreServices,
      unrealangelscript::UnrealAngelscriptSharedCoreServices,
      unrealangelscript::install_core_module>();

  for (const char* const path : kCleanExamples) {
    const std::string text = read(path);
    if (text.empty()) return fail(path);

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      std::fprintf(stderr, "unrealangelscript: %s does not parse clean\n",
                   path);
      return 1;
    }
    if (!result.diagnostics.empty()) {
      std::fprintf(stderr, "unrealangelscript: %s carries a diagnostic\n",
                   path);
      return 1;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Script>) {
      std::fprintf(stderr, "unrealangelscript: %s built no Script\n", path);
      return 1;
    }
    const auto& script = *static_cast<const ast::Script*>(root);
    if (script.items.empty()) {
      std::fprintf(stderr, "unrealangelscript: %s built an empty Script\n",
                   path);
      return 1;
    }
  }

  // Read real field values back out of Example_Actor.as: a converter that
  // silently yields the wrong value passes every structural check above.
  {
    const std::string text = read(CURSIV_EXAMPLE_DIR "/Example_Actor.as");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& script = *result.value_as<ast::Script>();

    if (script.items.size() != 1) return fail("Example_Actor: item count");

    const auto* const decl = static_cast<const ast::VarDecl*>(script.items[0]);
    if (decl->type != &cursiv::ast_type_tag<ast::VarDecl>)
      return fail("Example_Actor: root item is not a VarDecl");
    if (decl->specifiers == nullptr || decl->specifiers->parts.size() != 1)
      return fail("Example_Actor: expected one type specifier");

    const auto* const classDef =
        static_cast<const ast::ClassDef*>(decl->specifiers->parts[0]);
    if (classDef->type != &cursiv::ast_type_tag<ast::ClassDef>)
      return fail("Example_Actor: expected a ClassDef");
    if (classDef->name != "AExampleActor")
      return fail("Example_Actor: wrong class name");
    if (classDef->keyword != "class") return fail("Example_Actor: keyword");
    if (classDef->bases.size() != 1 || classDef->bases[0]->name != "AActor")
      return fail("Example_Actor: base class");

    // ExampleUPropertyBool = false; -- a PropertyDef with a UPROPERTY
    // annotation and no decl-specifier-seq, reached through its bare
    // assignmentExpression alternative.
    bool sawExampleBool = false;
    bool sawUFunctionCallable = false;
    for (const ast::Item* const member : classDef->members) {
      if (member->type != &cursiv::ast_type_tag<ast::PropertyDef>) continue;
      const auto& prop = *static_cast<const ast::PropertyDef*>(member);
      if (prop.expr == nullptr) continue;
      if (prop.expr->type != &cursiv::ast_type_tag<ast::AssignExpr>) continue;
      const auto& assign = *static_cast<const ast::AssignExpr*>(prop.expr);
      if (assign.target->type != &cursiv::ast_type_tag<ast::IdExpr>) continue;
      const auto& name = *static_cast<const ast::IdExpr*>(assign.target);
      if (name.name == "ExampleUPropertyBool") {
        sawExampleBool = true;
        if (prop.uproperty == nullptr)
          return fail("ExampleUPropertyBool: missing UPROPERTY");
      }
    }
    if (!sawExampleBool) return fail("Example_Actor: ExampleUPropertyBool not found");

    for (const ast::Item* const member : classDef->members) {
      if (member->type != &cursiv::ast_type_tag<ast::FunctionDef>) continue;
      const auto& fn = *static_cast<const ast::FunctionDef*>(member);
      if (fn.declarator != nullptr && fn.declarator->name == "ExampleUFunction") {
        sawUFunctionCallable = true;
        if (fn.ufunction == nullptr)
          return fail("ExampleUFunction: missing UFUNCTION");
      }
    }
    if (!sawUFunctionCallable)
      return fail("Example_Actor: ExampleUFunction not found");
  }

  // Binary precedence: bitwise `&`/`^`/`|` bind tighter than relational and
  // equality here, matching AngelScript's own table; `^^` (logical xor)
  // sits at equality's own step, not fused with bitwise `^`.
  {
    const struct {
      const char* text;
      const char* top;
      const char* left;
    } kPrecedence[] = {
        {"a & b == c", "==", "&"},
        {"a ^ b == c", "==", "^"},
        {"a | b < c", "<", "|"},
        {"a ^^ b == c", "==", "^^"},
    };
    for (const auto& probe : kPrecedence) {
      const std::string src =
          std::string("void f(){return ") + probe.text + ";}";
      const cursiv::ParseResult result = language.language.parser->parse(src);
      if (result.cst().recovered() || !result.diagnostics.empty())
        return fail(probe.text);
      const auto& script = *result.value_as<ast::Script>();
      const auto* const fn = static_cast<const ast::FunctionDef*>(script.items[0]);
      const auto* const ret =
          static_cast<const ast::ReturnStmt*>(fn->body->items[0]);
      if (ret->type != &cursiv::ast_type_tag<ast::ReturnStmt>)
        return fail(probe.text);
      const ast::Expr* const top = ret->value;
      if (top == nullptr || top->type != &cursiv::ast_type_tag<ast::BinaryExpr>)
        return fail(probe.text);
      const auto& topBin = *static_cast<const ast::BinaryExpr*>(top);
      if (topBin.op != probe.top) return fail(probe.text);
      if (topBin.left->type != &cursiv::ast_type_tag<ast::BinaryExpr>)
        return fail(probe.text);
      if (static_cast<const ast::BinaryExpr*>(topBin.left)->op != probe.left)
        return fail(probe.text);
    }
  }

  // `enum Name;` is the forward declaration (as_parser.cpp, `ParseEnumeration`),
  // where `class X;` stays a declaration of a tag.
  {
    const std::string src = "enum EFwd;\nenum EBased : int32;\nclass X;\n";
    const cursiv::ParseResult result = language.language.parser->parse(src);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("forward declarations: do not parse clean");
    const auto& script = *result.value_as<ast::Script>();
    if (script.items.size() != 3)
      return fail("forward declarations: item count");
    if (script.items[0]->type != &cursiv::ast_type_tag<ast::EnumForwardDecl>)
      return fail("`enum EFwd;` is not an EnumForwardDecl");
    if (script.items[1]->type != &cursiv::ast_type_tag<ast::EnumForwardDecl>)
      return fail("`enum EBased : int32;` is not an EnumForwardDecl");
    if (script.items[2]->type != &cursiv::ast_type_tag<ast::VarDecl>)
      return fail("`class X;` is not a VarDecl");
  }

  std::puts("unrealangelscript: every example parses clean");
  return 0;
}
