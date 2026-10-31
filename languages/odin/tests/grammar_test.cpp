#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "odin/core/CoreServices.hpp"
#include "odin/core/ast.hpp"
#include "native_followup.hpp"
// The smoke test of the language: every corpus document parses clean with no
// diagnostics, one is read back field by field to catch a converter that
// silently mismaps its value, and the coverage document (constructs the
// two-file corpus never reaches) parses clean too.

#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "odin: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

void parse_failure(const cursiv::ParseResult& result, const std::string& source) {
  std::fprintf(stderr, "accepted=%d recovered=%d diagnostics=%zu\n", result.accepted,
               result.cst().recovered(), result.diagnostics.size());
  for (const auto& diagnostic : result.diagnostics) {
    const auto start = diagnostic.begin > 80 ? diagnostic.begin - 80 : 0;
    const auto context = std::string_view(source).substr(start, 160);
    std::fprintf(stderr, "kind=%u begin=%zu end=%zu element=%u message=%.*s\n"
                         "context starts at %zu: %.*s\n",
                 static_cast<unsigned>(diagnostic.kind), std::size_t(diagnostic.begin),
                 std::size_t(diagnostic.end), static_cast<unsigned>(diagnostic.element),
                 int(diagnostic.message.size()), diagnostic.message.data(), std::size_t(start),
                 int(context.size()), context.data());
  }
}

const char* const kExamples[] = {"test.odin", "demo.odin"};

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<odin::OdinCoreServices,
                                                odin::OdinSharedCoreServices,
                                                odin::install_core_module>();
  if (!odin_native_followup(*language.language.parser)) return fail("native follow-up");
  // A native Odin source file starts with a package, after comments/build tags.
  for (const char* source : {"// comment only\n", "/* comment only */",
                             "#!x\n", "#+build linux\n", "x :: 1\n",
                             "import \"core:fmt\"\n"}) {
    const auto parsed = language.language.parser->parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) {
      std::fprintf(stderr, "odin: missing package accepted: %s\n", source);
      return 1;
    }
  }
  for (const char* source : {"", " \t\r\n"}) {
    const auto parsed = language.language.parser->parse(source);
    const auto* unit = parsed.value_as<odin::ast::Unit>();
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !unit || !unit->packageName.empty() ||
        !unit->imports.empty() || !unit->items.empty())
      return fail("native empty file must stay empty");
  }
  for (const char* source : {"package main\n", "package main\nx :: 1\n",
                             "// comment\npackage main\nx :: 1\n",
                             "#+vet !using-stmt\npackage main\nx :: 1\n"}) {
    const auto parsed = language.language.parser->parse(source);
    const auto* unit = parsed.value_as<odin::ast::Unit>();
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !unit || unit->packageName != "main") {
      std::fprintf(stderr, "odin: valid package file rejected: %s\n", source);
      return 1;
    }
    if (std::string_view(source) == "package main\n") {
      if (!unit->items.empty()) return fail("package-only file has declarations");
    } else {
      const auto* decl = unit->items.size() == 1
          ? cursiv::ast_cast<odin::ast::NamedDecl>(unit->items[0]) : nullptr;
      const auto* number = decl
          ? cursiv::ast_cast<odin::ast::IntLit>(decl->value) : nullptr;
      if (!decl || decl->name != "x" || decl->parent != unit || !number ||
          number->text != "1" || number->parent != decl)
        return fail("package declaration readback/ownership");
    }
  }
  {
    const auto r = language.language.parser->parse(
        "package p\nS :: struct #simple #packed #align(4) #raw_union "
        "#all_or_none #min_field_align(2) { x: int }\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "package p\nS :: struct #packed #packed { x: int }\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "package p\nS :: struct #packed #packed { x: int }\n",
        "file:///audit.odin");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language =
      cursiv::create_language<odin::OdinCoreServices,
                              odin::OdinSharedCoreServices,
                              odin::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
      parse_failure(result, text);
      return fail((std::string(name) + ": does not parse clean").c_str());
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<odin::ast::Unit>)
      return fail((std::string(name) + ": built no Unit").c_str());

    const auto& unit = *static_cast<const odin::ast::Unit*>(root);
    if (unit.items.empty())
      return fail((std::string(name) + ": Unit holds no declaration").c_str());
  }

  // coverage.odin: a multi-pointer type, a dynamic array's fixed capacity,
  // a three-part slice, #row_major, #reverse, extra struct tags and a
  // `where` clause, an inferred #soa, #load/#defined, and a bare
  // `proc(...)->...` type -- none of which the two-file corpus reaches.
  {
    const std::string text = read_file(std::string(CURSIV_TEST_DIR) + "/coverage.odin");
    if (text.empty()) return fail("coverage.odin: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
      parse_failure(result, text);
      return fail("coverage.odin: does not parse clean");
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<odin::ast::Unit>)
      return fail("coverage.odin: built no Unit");

    const auto& unit = *static_cast<const odin::ast::Unit*>(root);
    const odin::ast::NamedDecl* multiPtr = nullptr;
    const odin::ast::NamedDecl* tight = nullptr;
    for (const odin::ast::Stmt* const item : unit.items) {
      if (item->type != &cursiv::ast_type_tag<odin::ast::NamedDecl>) continue;
      const auto& decl = *static_cast<const odin::ast::NamedDecl*>(item);
      if (decl.name == "multi_ptr") multiPtr = &decl;
      if (decl.name == "Tight") tight = &decl;
    }
    if (multiPtr == nullptr) return fail("coverage.odin: no multi_ptr declaration");
    if (multiPtr->value == nullptr ||
        multiPtr->value->type != &cursiv::ast_type_tag<odin::ast::ProcExpr>)
      return fail("coverage.odin: multi_ptr is not a proc");
    const auto& proc = *static_cast<const odin::ast::ProcExpr*>(multiPtr->value);
    if (proc.params.size() != 1 ||
        proc.params[0]->type == nullptr ||
        proc.params[0]->type->type != &cursiv::ast_type_tag<odin::ast::MultiPointerType>)
      return fail("coverage.odin: multi_ptr's parameter is not a multi-pointer");

    if (tight == nullptr) return fail("coverage.odin: no Tight declaration");
    if (tight->typeDecl == nullptr ||
        tight->typeDecl->type != &cursiv::ast_type_tag<odin::ast::StructType>)
      return fail("coverage.odin: Tight is not a struct");
    const auto& tightStruct = *static_cast<const odin::ast::StructType*>(tight->typeDecl);
    if (!tightStruct.simple || !tightStruct.allOrNone ||
        tightStruct.minFieldAlign.empty() ||
        tightStruct.whereConditions.size() != 1 ||
        tightStruct.fields.size() != 2)
      return fail(
          "coverage.odin: Tight lost one of its tags, its where clause "
          "or its fields");
  }

  // test.odin, read back field by field.
  const std::string testText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/test.odin");
  const cursiv::ParseResult result = language.language.parser->parse(testText);
  const auto& unit = *static_cast<const odin::ast::Unit*>(result.value);
  if (unit.packageName != "main") return fail("test.odin: wrong package name");
  if (unit.imports.size() != 1 || unit.imports[0]->path != "\"core:fmt\"")
    return fail("test.odin: wrong import");
  if (unit.items.size() != 1) return fail("test.odin: expected one declaration");

  if (unit.items[0]->type != &cursiv::ast_type_tag<odin::ast::NamedDecl>)
    return fail("test.odin: declaration is not a NamedDecl");
  const auto& mainDecl = *static_cast<const odin::ast::NamedDecl*>(unit.items[0]);
  if (mainDecl.name != "main") return fail("test.odin: wrong declaration name");
  if (mainDecl.value == nullptr ||
      mainDecl.value->type != &cursiv::ast_type_tag<odin::ast::ProcExpr>)
    return fail("test.odin: main is not a proc");

  const auto& mainProc = *static_cast<const odin::ast::ProcExpr*>(mainDecl.value);
  if (mainProc.body == nullptr ||
      mainProc.body->type != &cursiv::ast_type_tag<odin::ast::Block>)
    return fail("test.odin: main's body is not a block");
  const auto& body = *static_cast<const odin::ast::Block*>(mainProc.body);
  if (body.items.size() != 2)
    return fail("test.odin: expected two statements in main's body");
  if (body.items[1]->type != &cursiv::ast_type_tag<odin::ast::VarDecl>)
    return fail("test.odin: second statement is not a var_decl");
  const auto& varDecl = *static_cast<const odin::ast::VarDecl*>(body.items[1]);
  if (varDecl.names.size() != 2 || varDecl.names[0] != "i" || varDecl.names[1] != "j")
    return fail("test.odin: wrong variable names");
  if (varDecl.values.size() != 2)
    return fail("test.odin: wrong initializer count");
  if (varDecl.values[0]->type != &cursiv::ast_type_tag<odin::ast::IntLit>)
    return fail("test.odin: first initializer is not an int literal");
  if (static_cast<const odin::ast::IntLit*>(varDecl.values[0])->text != "1")
    return fail("test.odin: wrong first initializer value");

  // Precedence: `x := <expr>` at package scope, read back as one BinaryExpr
  // (or UnaryExpr) tree. Each check keeps its program text alive for as
  // long as the tree built from it is read.
  cursiv::ParseResult expressionResult;
  auto exprValue = [&](const std::string& text) -> const odin::ast::Expr* {
    expressionResult = language.language.parser->parse(text);
    const auto& r = expressionResult;
    if (r.cst().recovered() || !r.diagnostics.empty()) return nullptr;
    const auto& u = *static_cast<const odin::ast::Unit*>(r.value);
    if (u.items.size() != 1 ||
        u.items[0]->type != &cursiv::ast_type_tag<odin::ast::VarDecl>)
      return nullptr;
    const auto& v = *static_cast<const odin::ast::VarDecl*>(u.items[0]);
    return v.values.size() == 1 ? v.values[0] : nullptr;
  };
  auto asBinary = [](const odin::ast::Expr* e) -> const odin::ast::BinaryExpr* {
    return e != nullptr && e->type == &cursiv::ast_type_tag<odin::ast::BinaryExpr>
               ? static_cast<const odin::ast::BinaryExpr*>(e)
               : nullptr;
  };

  {  // `1 + 2 * 3` -> `1 + (2 * 3)`: `*` binds tighter than `+`.
    const std::string text = "package p\nx := 1 + 2 * 3\n";
    const auto* top = asBinary(exprValue(text));
    const auto* rhs = top != nullptr ? asBinary(top->right) : nullptr;
    if (top == nullptr || top->op != "+" || rhs == nullptr || rhs->op != "*")
      return fail("precedence: `1 + 2 * 3` did not read as `1 + (2 * 3)`");
  }
  {  // `a || b && c` -> `a || (b && c)`: `&&` binds tighter than `||`.
    const std::string text = "package p\nx := a || b && c\n";
    const auto* top = asBinary(exprValue(text));
    const auto* rhs = top != nullptr ? asBinary(top->right) : nullptr;
    if (top == nullptr || top->op != "||" || rhs == nullptr || rhs->op != "&&")
      return fail("precedence: `a || b && c` did not read as `a || (b && c)`");
  }
  {  // `x == y + 1` -> `x == (y + 1)`: `+` binds tighter than `==`.
    const std::string text = "package p\nz := x == y + 1\n";
    const auto* top = asBinary(exprValue(text));
    const auto* rhs = top != nullptr ? asBinary(top->right) : nullptr;
    if (top == nullptr || top->op != "==" || rhs == nullptr || rhs->op != "+")
      return fail("precedence: `x == y + 1` did not read as `x == (y + 1)`");
  }
  {  // `-a * b` -> `(-a) * b`: unary `-` binds tighter than any binary op.
    const std::string text = "package p\nz := -a * b\n";
    const auto* top = asBinary(exprValue(text));
    const bool leftIsUnary =
        top != nullptr && top->left != nullptr &&
        top->left->type == &cursiv::ast_type_tag<odin::ast::UnaryExpr>;
    if (top == nullptr || top->op != "*" || !leftIsUnary)
      return fail("precedence: `-a * b` did not read as `(-a) * b`");
  }

  {  // `switch v in val` binds `v` and reads its cases as types; with the
     // `in` inside parentheses the switch stays a plain one.
    const std::string text =
        "package p\nmain :: proc() {\n  switch v in val {\n  case int:\n"
        "  case ^Foo, nil:\n  case:\n  }\n  switch (a in b) {\n"
        "  case true:\n  }\n}\n";
    const cursiv::ParseResult r = language.language.parser->parse(text);
    if (r.cst().recovered() || !r.diagnostics.empty())
      return fail("type switch: does not parse clean");
    const auto& u = *static_cast<const odin::ast::Unit*>(r.value);
    const auto& decl = *static_cast<const odin::ast::NamedDecl*>(u.items[0]);
    const auto& proc = *static_cast<const odin::ast::ProcExpr*>(decl.value);
    const auto& block = *static_cast<const odin::ast::Block*>(proc.body);
    if (block.items.size() != 2 ||
        block.items[0]->type !=
            &cursiv::ast_type_tag<odin::ast::TypeSwitchStmt> ||
        block.items[1]->type != &cursiv::ast_type_tag<odin::ast::SwitchStmt>)
      return fail("type switch: `switch v in val` is not a TypeSwitchStmt");
    const auto& ts =
        *static_cast<const odin::ast::TypeSwitchStmt*>(block.items[0]);
    if (ts.name != "v" || ts.value == nullptr ||
        ts.value->type != &cursiv::ast_type_tag<odin::ast::Ident> ||
        ts.cases.size() != 3 || ts.cases[0]->values.size() != 1 ||
        !std::holds_alternative<odin::ast::Type*>(ts.cases[0]->values[0]) ||
        ts.cases[1]->values.size() != 2 ||
        !std::holds_alternative<odin::ast::Type*>(ts.cases[1]->values[0]) ||
        !std::holds_alternative<odin::ast::Expr*>(ts.cases[1]->values[1]) ||
        !ts.cases[2]->values.empty())
      return fail("type switch: wrong name, subject or cases");
    const auto& sw = *static_cast<const odin::ast::SwitchStmt*>(block.items[1]);
    if (asBinary(sw.tag) == nullptr || asBinary(sw.tag)->op != "in")
      return fail("type switch: a parenthesised `a in b` is not a plain tag");
  }

  std::puts("odin: the corpus, the coverage document and precedence parse");
  return 0;
}
