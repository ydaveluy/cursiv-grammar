// Turing end to end: every document under example/ parses clean, and a few
// field values are read back out of the AST -- a converter that silently
// yields the wrong value passes every structural check.

#include <memory>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "turing/core/CoreServices.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "turing/core/ast.hpp"
#include "turing/generated/Turing/Module.hpp"
#include "three_type_cases.hpp"

namespace {

namespace ast = turing::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      turing::generated::Turing::create_parser();
  return *p;
}

// The AST holds views into `text`, so every caller keeps it alive for as
// long as it reads the result.
cursiv::ParseResult parse(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  CHECK(r.value != nullptr &&
        r.value->type == &cursiv::ast_type_tag<ast::Program>);
  return r;
}

std::string load(const char* name) {
  const std::string path =
      std::string(CURSIV_EXAMPLE_DIR) + "/example/" + name;
  const std::string src = cursiv::test::read_file(path);
  CHECK(!src.empty());
  return src;
}

std::string load_test(const char* name) {
  const std::string src =
      cursiv::test::read_file(std::string(CURSIV_TEST_DIR) + "/" + name);
  CHECK(!src.empty());
  return src;
}

void every_example_parses_clean() {
  for (const char* name : {"example1.t", "example2.t"}) {
    const std::string src = load(name);
    parse(src);
  }
  parse(load_test("showcase.t"));
}

// example2.t: two variable declarations, then a run of put/get statements.
void reads_the_grammars_v4_sample() {
  const std::string src = load("example2.t");
  const cursiv::ParseResult r = parse(src);
  auto* const program = static_cast<ast::Program*>(r.value);
  CHECK(program->items.size() == 8);
  if (program->items.size() < 2) return;

  auto* const nameDecl = static_cast<ast::VarDecl*>(program->items[0]);
  CHECK(nameDecl->type == &cursiv::ast_type_tag<ast::VarDecl>);
  CHECK(nameDecl->names.size() == 1 && nameDecl->names[0] == "name");
  // "string" is a standardType keyword; only "nat" below falls through to a
  // plain (unresolved) named type.
  CHECK(nameDecl->typeSpec != nullptr &&
        nameDecl->typeSpec->type == &cursiv::ast_type_tag<ast::StandardType>);

  auto* const ageDecl = static_cast<ast::VarDecl*>(program->items[1]);
  CHECK(ageDecl->names.size() == 1 && ageDecl->names[0] == "age");
  auto* const ageType = static_cast<ast::NamedType*>(ageDecl->typeSpec);
  CHECK(ageType->name == "nat");  // not a standardType keyword: plain text
}

// showcase.t: declarations, a function header, and the expression grammar.
void reads_the_showcase() {
  const std::string src = load_test("showcase.t");
  const cursiv::ParseResult r = parse(src);
  auto* const program = static_cast<ast::Program*>(r.value);

  ast::ConstDecl* pi = nullptr;
  ast::SubprogramDecl* square = nullptr;
  for (ast::Item* const item : program->items) {
    if (item->type == &cursiv::ast_type_tag<ast::ConstDecl>) {
      auto* const c = static_cast<ast::ConstDecl*>(item);
      if (c->name == "Pi") pi = c;
    } else if (item->type == &cursiv::ast_type_tag<ast::SubprogramDecl>) {
      auto* const s = static_cast<ast::SubprogramDecl*>(item);
      if (s->name == "Square") square = s;
    }
  }

  CHECK(pi != nullptr);
  if (pi != nullptr) {
    CHECK(std::holds_alternative<ast::Expr*>(pi->value));
    ast::Expr* const v = std::get<ast::Expr*>(pi->value);
    CHECK(v != nullptr && v->type == &cursiv::ast_type_tag<ast::RealLit>);
    if (v != nullptr && v->type == &cursiv::ast_type_tag<ast::RealLit>)
      CHECK(static_cast<ast::RealLit*>(v)->value > 3.14 &&
            static_cast<ast::RealLit*>(v)->value < 3.15);
  }

  CHECK(square != nullptr);
  if (square != nullptr) {
    CHECK(square->isFunction);
    CHECK(square->params.size() == 1);
    CHECK(square->returnType != nullptr &&
          square->returnType->type ==
              &cursiv::ast_type_tag<ast::StandardType>);
    CHECK(square->endName == "Square");
  }
}

}  // namespace

ast::BinaryExpr* rhs_of(cursiv::ParseResult& r) {
  auto* const program = static_cast<ast::Program*>(r.value);
  auto* const assign = static_cast<ast::AssignStmt*>(program->items.back());
  return static_cast<ast::BinaryExpr*>(assign->value);
}

// Turing's own precedence table.
void operator_precedence() {
  {
    cursiv::ParseResult r = parse("var x : int\nx := a + b * c\n");
    ast::BinaryExpr* const add = rhs_of(r);
    CHECK(add != nullptr && add->op == ast::BinOp::add);
    CHECK(add != nullptr &&
          static_cast<ast::BinaryExpr*>(add->right)->op == ast::BinOp::multiply);
  }
  {
    cursiv::ParseResult r = parse("var x : int\nx := a < b and c < d\n");
    ast::BinaryExpr* const andExpr = rhs_of(r);
    CHECK(andExpr != nullptr && andExpr->op == ast::BinOp::logicalAnd);
    CHECK(andExpr != nullptr &&
          static_cast<ast::BinaryExpr*>(andExpr->left)->op == ast::BinOp::less);
  }
  {
    cursiv::ParseResult r = parse("var x : int\nx := a and b or c\n");
    ast::BinaryExpr* const orExpr = rhs_of(r);
    CHECK(orExpr != nullptr && orExpr->op == ast::BinOp::logicalOr);
    CHECK(orExpr != nullptr &&
          static_cast<ast::BinaryExpr*>(orExpr->left)->op == ast::BinOp::logicalAnd);
  }
  {
    // The report groups exponentiation from left to right.
    cursiv::ParseResult r = parse("var x : int\nx := 2 ** 3 ** 2\n");
    ast::BinaryExpr* const outer = rhs_of(r);
    CHECK(outer != nullptr && outer->op == ast::BinOp::exponent);
    CHECK(outer != nullptr &&
          static_cast<ast::BinaryExpr*>(outer->left)->op == ast::BinOp::exponent);
  }
}

void native_report_regressions() {
  for (const auto text : {"", "var x : int; x := 1;",
                          "proc p begin return; end; end p;", "x := 0.", "x := .1", "x := 2E4", "x := - 56.1e+27",
                          "var a : array 1..10 of int", "x := 2 ** - 3 ** 2",
                          "proc p end p", "fcn f : int result 1 end f",
                          "x := a in b", "x := a not in b", "x := a -> b",
                          "/* comment */ put \"ok\" % EOF"}) parse(text);
  {
    auto r = parse("x := -2 ** 2");
    const auto* p = static_cast<ast::Program*>(r.value);
    const auto* value = static_cast<ast::AssignStmt*>(p->items[0])->value;
    CHECK(value->type == &cursiv::ast_type_tag<ast::UnaryExpr>);
    if (value->type == &cursiv::ast_type_tag<ast::UnaryExpr>) {
      const auto* u = static_cast<const ast::UnaryExpr*>(value);
      CHECK(u->op == ast::UnOp::minus);
      CHECK(u->operand->type == &cursiv::ast_type_tag<ast::BinaryExpr>);
    }
  }
  {
    auto r = parse("x := not a = b");
    const auto* p = static_cast<ast::Program*>(r.value);
    const auto* value = static_cast<ast::AssignStmt*>(p->items[0])->value;
    CHECK(value->type == &cursiv::ast_type_tag<ast::UnaryExpr>);
    if (value->type == &cursiv::ast_type_tag<ast::UnaryExpr>) {
      const auto* u = static_cast<const ast::UnaryExpr*>(value);
      CHECK(u->op == ast::UnOp::logicalNot);
      CHECK(u->operand->type == &cursiv::ast_type_tag<ast::BinaryExpr>);
    }
  }
  {
    auto r = parse(R"(x := "a\n\T\e\D\\\"")");
    const auto* p = static_cast<ast::Program*>(r.value);
    const auto* value = static_cast<ast::AssignStmt*>(p->items[0])->value;
    CHECK(value->type == &cursiv::ast_type_tag<ast::StringLit>);
    CHECK(static_cast<const ast::StringLit*>(value)->value == "a\n\t\x1b\x7f\\\"");
  }
  {
    auto r = parse("x := - 9223372036854775808");
    const auto* p = static_cast<ast::Program*>(r.value);
    const auto* value = static_cast<ast::AssignStmt*>(p->items[0])->value;
    CHECK(value->type == &cursiv::ast_type_tag<ast::IntLit>);
    if (value->type == &cursiv::ast_type_tag<ast::IntLit>)
      CHECK(static_cast<const ast::IntLit*>(value)->value == (-9223372036854775807LL - 1));
  }
  for (const auto text : {R"(put "raw
newline")", R"(put "bad\q")", "var if : int",
      "var aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa : int"}) {
    const auto r = engine().parse(text);
    CHECK(!r.accepted || r.cst().recovered() || !r.diagnostics.empty());
  }
}

void case_default_validation() {
  const auto language = cursiv::create_language<
      turing::TuringCoreServices, turing::TuringSharedCoreServices,
      turing::install_core_module>();
  for (const auto source : {"case x of label 1: label : end case",
                            "case x of label : label 1: end case",
                            "case x of label : label : end case"}) {
    cursiv::Document doc(cursiv::TextDocument::create(
        "memory:turing", "turing", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    const auto* program = static_cast<ast::Program*>(doc.parseResult.value);
    language.language.validator->checkCaseLabels(
        *static_cast<ast::CaseStmt*>(program->items[0]), accept);
    CHECK(errors == (std::string_view(source).starts_with("case x of label 1:") ? 0 : 1));
  }
}

int main() {
  const auto language = cursiv::create_language<turing::TuringCoreServices,
      turing::TuringSharedCoreServices, turing::install_core_module>();
  if (!turing_three_types::run(language.language)) return 1;
  every_example_parses_clean();
  reads_the_grammars_v4_sample();
  reads_the_showcase();
  operator_precedence();
  native_report_regressions();
  case_default_validation();
  return cursiv::test::report("turing_grammar_test");
}
