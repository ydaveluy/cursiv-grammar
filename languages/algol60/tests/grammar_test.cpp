// The grammar end to end: every corpus document parses clean (no repair, no
// diagnostic, a Block root), and one document's AST is read back field by
// field, so a converter that silently drifted would fail here.

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "algol60/core/CoreServices.hpp"
#include "algol60/core/ast.hpp"
#include "statement_families.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = algol60::ast;

const auto& language() {
  static const auto language = cursiv::create_language<
      algol60::Algol60CoreServices, algol60::Algol60SharedCoreServices,
      algol60::install_core_module>();
  return language.language;
}

const cursiv::Parser& engine() { return *language().parser; }

template <class T>
T* as(cursiv::AstNode* n) {
  return n != nullptr ? const_cast<T*>(cursiv::ast_cast<T>(n)) : nullptr;
}

bool names_are(const cursiv::vector<std::string_view>& got,
              std::initializer_list<std::string_view> want) {
  if (got.size() != want.size()) return false;
  std::size_t i = 0;
  for (std::string_view w : want) {
    if (got[i++] != w) return false;
  }
  return true;
}

// Every corpus document parses clean: no repair, no diagnostic, a Block
// root -- except primes.a60, whose `div` is not part of this language.
void corpus_parses_clean() {
  std::vector<fs::path> files;
  for (const auto& entry : fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) files.push_back(entry.path());
  CHECK(!files.empty());
  for (const fs::path& path : files) {
    if (path.filename() == "primes.a60") continue;
    const std::string text = cursiv::test::read_file(path.string());
    CHECK(!text.empty());
    const cursiv::ParseResult result = engine().parse(text);
    CHECK(result.accepted);
    CHECK(!result.cst().recovered());
    CHECK(result.diagnostics.empty());
    CHECK(as<ast::Block>(result.value) != nullptr);
  }
}

// jensen.a60, read back field by field: a procedure with a value parameter
// and a specifier, a nested block, a for/step/until loop, and a call whose
// argument is itself an infix expression.
void jensen_ast() {
  const std::string path =
      std::string(CURSIV_EXAMPLE_DIR) + "/jensen.a60";
  const std::string text = cursiv::test::read_file(path);
  CHECK(!text.empty());
  const cursiv::ParseResult result = engine().parse(text);
  CHECK(result.accepted && !result.cst().recovered());
  CHECK(result.diagnostics.empty());

  ast::Block* root = as<ast::Block>(result.value);
  CHECK(root != nullptr);
  if (root == nullptr) return;
  CHECK(root->decls.size() == 2);
  CHECK(root->statements.size() == 2);
  if (root->decls.size() != 2 || root->statements.size() != 2) return;
  CHECK(as<ast::EmptyStmt>(root->statements[1]->stmt) != nullptr);

  ast::ProcDecl* sigma = as<ast::ProcDecl>(root->decls[0]);
  CHECK(sigma != nullptr);
  if (sigma != nullptr) {
    CHECK(sigma->name == "SIGMA");
    CHECK(sigma->type_name == "integer");
    CHECK(names_are(sigma->params, {"x", "i", "n"}));
    CHECK(names_are(sigma->value_params, {"n"}));
    CHECK(sigma->specs.size() == 1);
    if (sigma->specs.size() == 1) {
      CHECK(sigma->specs[0]->kind == "integer");
      CHECK(names_are(sigma->specs[0]->names, {"x", "i", "n"}));
    }

    ast::Block* body = as<ast::Block>(sigma->body);
    CHECK(body != nullptr);
    if (body != nullptr) {
      CHECK(body->decls.size() == 1);
      CHECK(body->statements.size() == 4);
      if (body->decls.size() == 1) {
        ast::TypeDecl* sum_decl = as<ast::TypeDecl>(body->decls[0]);
        CHECK(sum_decl != nullptr && sum_decl->type_name == "integer");
        CHECK(sum_decl != nullptr && names_are(sum_decl->names, {"sum"}));
      }
      if (body->statements.size() == 4) {
        CHECK(as<ast::EmptyStmt>(body->statements[3]->stmt) != nullptr);
        ast::AssignStmt* init =
            as<ast::AssignStmt>(body->statements[0]->stmt);
        CHECK(init != nullptr && init->targets.size() == 1);
        CHECK(init != nullptr && !init->targets.empty() &&
              as<ast::NameRef>(init->targets[0]) != nullptr &&
              as<ast::NameRef>(init->targets[0])->name == "sum");
        CHECK(init != nullptr &&
              as<ast::NumberLiteral>(init->value) != nullptr &&
              as<ast::NumberLiteral>(init->value)->value == 0.0);

        ast::ForStmt* loop = as<ast::ForStmt>(body->statements[1]->stmt);
        CHECK(loop != nullptr && loop->var == "i");
        CHECK(loop != nullptr && loop->items.size() == 1);
        if (loop != nullptr && loop->items.size() == 1) {
          auto* su = std::get_if<ast::ForStepUntil*>(&loop->items[0]);
          CHECK(su != nullptr && *su != nullptr);
          if (su != nullptr && *su != nullptr) {
            CHECK(as<ast::NumberLiteral>((*su)->from) != nullptr &&
                  as<ast::NumberLiteral>((*su)->from)->value == 1.0);
            CHECK(as<ast::NumberLiteral>((*su)->step) != nullptr &&
                  as<ast::NumberLiteral>((*su)->step)->value == 1.0);
            CHECK(as<ast::NameRef>((*su)->to) != nullptr &&
                  as<ast::NameRef>((*su)->to)->name == "n");
          }
          ast::AssignStmt* accum = as<ast::AssignStmt>(loop->body);
          CHECK(accum != nullptr);
          if (accum != nullptr) {
            ast::BinaryExpr* sum_plus_x = as<ast::BinaryExpr>(accum->value);
            CHECK(sum_plus_x != nullptr &&
                  sum_plus_x->op == ast::BinOp::add);
            CHECK(sum_plus_x != nullptr &&
                  as<ast::NameRef>(sum_plus_x->left) != nullptr &&
                  as<ast::NameRef>(sum_plus_x->left)->name == "sum");
            CHECK(sum_plus_x != nullptr &&
                  as<ast::NameRef>(sum_plus_x->right) != nullptr &&
                  as<ast::NameRef>(sum_plus_x->right)->name == "x");
          }
        }

        ast::AssignStmt* ret = as<ast::AssignStmt>(body->statements[2]->stmt);
        CHECK(ret != nullptr && !ret->targets.empty() &&
              as<ast::NameRef>(ret->targets[0]) != nullptr &&
              as<ast::NameRef>(ret->targets[0])->name == "SIGMA");
        CHECK(ret != nullptr && as<ast::NameRef>(ret->value) != nullptr &&
              as<ast::NameRef>(ret->value)->name == "sum");
      }
    }
  }

  ast::TypeDecl* q_decl = as<ast::TypeDecl>(root->decls[1]);
  CHECK(q_decl != nullptr && q_decl->type_name == "integer");
  CHECK(q_decl != nullptr && names_are(q_decl->names, {"q"}));

  ast::CallStmt* print = as<ast::CallStmt>(root->statements[0]->stmt);
  CHECK(print != nullptr && print->callee == "printnln");
  CHECK(print != nullptr && print->args.size() == 1);
  if (print != nullptr && print->args.size() == 1) {
    ast::CallExpr* call = as<ast::CallExpr>(print->args[0]);
    CHECK(call != nullptr && call->callee == "SIGMA");
    CHECK(call != nullptr && call->args.size() == 3);
    if (call != nullptr && call->args.size() == 3) {
      ast::BinaryExpr* q2m1 = as<ast::BinaryExpr>(call->args[0]);
      CHECK(q2m1 != nullptr && q2m1->op == ast::BinOp::sub);
      if (q2m1 != nullptr) {
        ast::BinaryExpr* q2 = as<ast::BinaryExpr>(q2m1->left);
        CHECK(q2 != nullptr && q2->op == ast::BinOp::mul);
        CHECK(q2 != nullptr && as<ast::NameRef>(q2->left) != nullptr &&
              as<ast::NameRef>(q2->left)->name == "q");
        CHECK(q2 != nullptr &&
              as<ast::NumberLiteral>(q2->right) != nullptr &&
              as<ast::NumberLiteral>(q2->right)->value == 2.0);
        CHECK(as<ast::NumberLiteral>(q2m1->right) != nullptr &&
              as<ast::NumberLiteral>(q2m1->right)->value == 1.0);
      }
      CHECK(as<ast::NameRef>(call->args[1]) != nullptr &&
            as<ast::NameRef>(call->args[1])->name == "q");
      CHECK(as<ast::NumberLiteral>(call->args[2]) != nullptr &&
            as<ast::NumberLiteral>(call->args[2])->value == 7.0);
    }
  }
}

// Constructs the language admits but no upstream document uses: `own` and
// `switch` declarations, a numeric label, `go`/`to` split across a line
// break, the report's own operator glyphs, an `if` used as an expression, a
// nesting nameless string, and a signed raised-ten exponent.
void gap_coverage() {
  const char* const names[] = {"own_switch_labels.a60",
                               "unicode_and_exponent.a60",
                               "parameter_delimiter.a60"};
  for (const char* name : names) {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
    const std::string text = cursiv::test::read_file(path);
    CHECK(!text.empty());
    const cursiv::ParseResult result = engine().parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    CHECK(result.diagnostics.empty());
    CHECK(as<ast::Block>(result.value) != nullptr);
  }

  // The signed exponent is unambiguous and must compute the raised-ten
  // value, not read `10` as more mantissa digits.
  const std::string path =
      std::string(CURSIV_TEST_DIR) + "/unicode_and_exponent.a60";
  const std::string text = cursiv::test::read_file(path);
  const cursiv::ParseResult result = engine().parse(text);
  ast::Block* root = as<ast::Block>(result.value);
  CHECK(root != nullptr && root->statements.size() >= 3);
  if (root != nullptr && root->statements.size() >= 3) {
    ast::AssignStmt* exp_stmt =
        as<ast::AssignStmt>(root->statements[2]->stmt);
    CHECK(exp_stmt != nullptr &&
          as<ast::NumberLiteral>(exp_stmt->value) != nullptr &&
          as<ast::NumberLiteral>(exp_stmt->value)->value == 2500000.0);
  }
}

void numeric_boundaries() {
  for (const std::string& literal : std::initializer_list<std::string>{
           "110+9999", "110-9999", std::string(400, '9')}) {
    const std::string source = "begin real x; x := " + literal + " end";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    CHECK(result.diagnostics.size() == 1);
    if (result.diagnostics.size() == 1)
      CHECK(result.diagnostics[0].kind == cursiv::DiagnosticKind::conversion_error);
  }
  for (const auto& [literal, expected] :
       {std::pair{".0110+310", 1e308}, std::pair{"010+9999", 0.0},
        std::pair{"2.510–3", .0025}}) {
    const std::string source = std::string("begin real x; x := ") + literal + " end";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    const ast::NumberLiteral* value = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* number = cursiv::ast_cast<ast::NumberLiteral>(node)) value = number;
    CHECK(value && value->value == expected);
  }
}

void native_empty_statements_and_division() {
  namespace ast = algol60::ast;
  for (const char* source : {"begin end", "begin ;;; end",
                            "begin integer x; end",
                            "begin if true then else end",
                            "begin for i:=1 do end",
                            "begin procedure p; ; p end"}) {
    const auto parsed = engine().parse(source);
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
  }
  const std::string source = "outer: 12: begin x:=7÷2; if true then else end";
  const auto parsed = engine().parse(source);
  const auto* block = parsed.value_as<ast::Block>();
  CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
  CHECK(block && block->labels.size() == 2 && block->labels[0] == "outer" &&
        block->labels[1] == "12" && block->statements.size() == 2);
  bool division = false, dummy_else = false;
  for (const auto* node : parsed.nodes) {
    if (const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(node)) {
      division |= binary->op == ast::BinOp::intdiv;
      if (binary->op != ast::BinOp::intdiv)
        std::fprintf(stderr, "algol60 division AST: op=%d expected=%d\n",
                     static_cast<int>(binary->op),
                     static_cast<int>(ast::BinOp::intdiv));
    }
    if (const auto* conditional = cursiv::ast_cast<ast::IfStmt>(node)) {
      const bool empty_then = as<ast::EmptyStmt>(conditional->then_branch);
      const bool empty_else = as<ast::EmptyStmt>(conditional->else_branch);
      dummy_else |= conditional->hasElse && empty_then && empty_else;
      if (!conditional->hasElse || !empty_then || !empty_else)
        std::fprintf(stderr,
                     "algol60 dummy else AST: hasElse=%d then=%d else=%d\n",
                     conditional->hasElse,
                     conditional->then_branch != nullptr,
                     conditional->else_branch != nullptr);
    }
  }
  CHECK(division);
  CHECK(dummy_else);
  CHECK(division && dummy_else);
}

}  // namespace

int main() {
  algol60_tests::statementFamilies(language());
  native_empty_statements_and_division();
  corpus_parses_clean();
  jensen_ast();
  gap_coverage();
  numeric_boundaries();
  return cursiv::test::report("algol60_grammar_test");
}
