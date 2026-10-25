// Matlab end to end: what the grammar takes and refuses at the CST, the
// whole example/ corpus parsed clean, and a few AST members read back so a
// converter that silently mis-reads a value would be caught too.

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "matlab/core/ast.hpp"
#include "matlab/generated/Matlab/Module.hpp"

namespace {

namespace ast = matlab::ast;
namespace fs = std::filesystem;

const cursiv::Parser &engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      matlab::generated::Matlab::create_parser();
  return *p;
}

const cursiv::AstReflection &reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      matlab::generated::Matlab::create_ast_reflection();
  return *r;
}

// The membership probe: the engine repairs by default, and what this suite
// pins is what the grammar refuses.
bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

template <class T> T *as(cursiv::AstNode *n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T *>(n)
             : nullptr;
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

void accepted_texts() {
  CHECK(accepts("x = 1\n"));
  CHECK(accepts("x = 1;\n"));
  CHECK(accepts("x = 1,\n"));
  CHECK(accepts("1 + 2\n"));
  CHECK(accepts("\n")); // a bare separator is a valid, empty statement
  CHECK(accepts(""));   // statement_list is optional
  // Literals.
  CHECK(accepts("x = 1.5\n"));
  CHECK(accepts("x = 1.5e10\n"));
  CHECK(accepts("x = 1.5E+10\n"));
  CHECK(accepts("x = 'a string'\n"));
  CHECK(accepts("x = 'it''s quoted'\n"));
  // Matrices, every separator, mixed and adjacent.
  CHECK(accepts("x = []\n"));
  CHECK(accepts("x = [1, 2, 3]\n"));
  CHECK(accepts("x = [1 2 3]\n"));
  CHECK(accepts("x = [1, 2; 3, 4]\n"));
  CHECK(accepts("x = [1, 2\n3, 4]\n"));
  // Indexing and calls share one syntax.
  CHECK(accepts("y = x(1)\n"));
  CHECK(accepts("y = x(1, 2)\n"));
  CHECK(accepts("y = x(:)\n"));
  CHECK(accepts("y = x(1:2, :)\n"));
  // The operator ladder, one of each.
  CHECK(accepts("x = 1 * 2 / 3 \\ 4 ^ 5\n"));
  CHECK(accepts("x = 1 .* 2 ./ 3 .\\ 4 .^ 5\n"));
  CHECK(accepts("x = 1 + 2 - 3\n"));
  CHECK(accepts("x = 1 < 2\n"));
  CHECK(accepts("x = 1 > 2\n"));
  CHECK(accepts("x = 1 <= 2\n"));
  CHECK(accepts("x = 1 >= 2\n"));
  CHECK(accepts("x = 1 == 2\n"));
  CHECK(accepts("x = 1 ~= 2\n"));
  CHECK(accepts("x = 1 & 2\n"));
  CHECK(accepts("x = 1 | 2\n"));
  CHECK(accepts("x = 1:5\n"));
  CHECK(accepts("x = 1:2:9\n"));
  // Unary and postfix.
  CHECK(accepts("x = -1\n"));
  CHECK(accepts("x = +1\n"));
  CHECK(accepts("x = ~1\n"));
  CHECK(accepts("x = a'\n"));
  CHECK(accepts("x = a .'\n"));
  // Statements.
  CHECK(accepts("global a b c\n"));
  CHECK(accepts("clear a b\n"));
  CHECK(accepts("if a\nb = 1;\nend\n"));
  CHECK(accepts("if a\nb = 1;\nelse\nc = 2;\nend\n"));
  CHECK(accepts("if a\nb = 1;\nelseif c\nd = 2;\nend\n"));
  CHECK(accepts("if a\nb = 1;\nelseif c\nd = 2;\nelse\ne = 3;\nend\n"));
  CHECK(accepts("while a\nb = 1;\nend\n"));
  CHECK(accepts("for i = 1:5\nb = i;\nend\n"));
  CHECK(accepts("for (i = 1:5)\nb = i;\nend\n"));
  CHECK(accepts("while a\nbreak;\nend\n"));
  CHECK(accepts("while a\nreturn;\nend\n"));
  // Assignment targets beyond a bare name.
  CHECK(accepts("x(1) = 2\n"));
  CHECK(accepts("(x) = 2\n"));
}

void refused_texts() {
  CHECK(!accepts("x = \n")); // no right-hand side
  // A line break right after the header is itself a valid (empty) body
  // statement, so an empty-looking body needs no separator at all to fail.
  CHECK(!accepts("if a end\n"));
  CHECK(!accepts("while a end\n"));
  CHECK(!accepts("x = (1\n")); // unbalanced parenthesis
  CHECK(!accepts("x()\n"));    // index_expression_list needs >= 1 item
  CHECK(!accepts("x = 'unterminated\n"));
  CHECK(!accepts("x = .5\n")); // NUMBER needs a leading digit, unlike C
}

void operator_precedence() {
  // `1 + 2 * 3` is `1 + (2 * 3)`: multiplicative is the tighter level.
  cursiv::ParseResult r = build("x = 1 + 2 * 3\n");
  ast::AssignStmt *const assign =
      as<ast::AssignStmt>(r.value_as<ast::File>()->statements[0]);
  CHECK(assign != nullptr);
  ast::BinaryExpr *const add =
      assign != nullptr ? as<ast::BinaryExpr>(assign->value) : nullptr;
  CHECK(add != nullptr && add->op == ast::Operator::add);
  ast::BinaryExpr *const mul =
      add != nullptr ? as<ast::BinaryExpr>(add->right) : nullptr;
  CHECK(mul != nullptr && mul->op == ast::Operator::multiply);

  // A left level folds left: `1 - 2 - 3` is `(1 - 2) - 3`.
  cursiv::ParseResult l = build("x = 1 - 2 - 3\n");
  ast::AssignStmt *const la =
      as<ast::AssignStmt>(l.value_as<ast::File>()->statements[0]);
  ast::BinaryExpr *const outer =
      la != nullptr ? as<ast::BinaryExpr>(la->value) : nullptr;
  CHECK(outer != nullptr && outer->op == ast::Operator::subtract &&
        as<ast::BinaryExpr>(outer->left) != nullptr &&
        as<ast::NumberLit>(outer->right) != nullptr);

  // The range operator binds looser than addition: `1:2+3` is `1:(2+3)`.
  cursiv::ParseResult ra = build("x = 1:2+3\n");
  ast::AssignStmt *const rassign =
      as<ast::AssignStmt>(ra.value_as<ast::File>()->statements[0]);
  ast::BinaryExpr *const range =
      rassign != nullptr ? as<ast::BinaryExpr>(rassign->value) : nullptr;
  CHECK(range != nullptr && range->op == ast::Operator::range);
  CHECK(range != nullptr && as<ast::BinaryExpr>(range->right) != nullptr);

  // Power binds tighter than multiplication: `2 + 3 * 2 ^ 2` is
  // `2 + (3 * (2 ^ 2))`.
  cursiv::ParseResult p = build("x = 2 + 3 * 2 ^ 2\n");
  ast::AssignStmt *const pAssign =
      as<ast::AssignStmt>(p.value_as<ast::File>()->statements[0]);
  ast::BinaryExpr *const pAdd =
      pAssign != nullptr ? as<ast::BinaryExpr>(pAssign->value) : nullptr;
  CHECK(pAdd != nullptr && pAdd->op == ast::Operator::add);
  ast::BinaryExpr *const pMul =
      pAdd != nullptr ? as<ast::BinaryExpr>(pAdd->right) : nullptr;
  CHECK(pMul != nullptr && pMul->op == ast::Operator::multiply);
  ast::BinaryExpr *const pPow =
      pMul != nullptr ? as<ast::BinaryExpr>(pMul->right) : nullptr;
  CHECK(pPow != nullptr && pPow->op == ast::Operator::power);

  // Colon binds tighter than a relational operator: `a < 1:5` is
  // `a < (1:5)`, never `(a < 1):5`.
  cursiv::ParseResult c = build("x = a < 1:5\n");
  ast::AssignStmt *const cAssign =
      as<ast::AssignStmt>(c.value_as<ast::File>()->statements[0]);
  ast::BinaryExpr *const cLess =
      cAssign != nullptr ? as<ast::BinaryExpr>(cAssign->value) : nullptr;
  CHECK(cLess != nullptr && cLess->op == ast::Operator::less);
  ast::BinaryExpr *const cRange =
      cLess != nullptr ? as<ast::BinaryExpr>(cLess->right) : nullptr;
  CHECK(cRange != nullptr && cRange->op == ast::Operator::range);

  // Relational and equality operators are one precedence tier, so they fold
  // left like any other same-tier run: `1 == 2 < 3` is `(1 == 2) < 3`.
  cursiv::ParseResult e = build("x = 1 == 2 < 3\n");
  ast::AssignStmt *const eAssign =
      as<ast::AssignStmt>(e.value_as<ast::File>()->statements[0]);
  ast::BinaryExpr *const eLess =
      eAssign != nullptr ? as<ast::BinaryExpr>(eAssign->value) : nullptr;
  CHECK(eLess != nullptr && eLess->op == ast::Operator::less);
  ast::BinaryExpr *const eEqual =
      eLess != nullptr ? as<ast::BinaryExpr>(eLess->left) : nullptr;
  CHECK(eEqual != nullptr && eEqual->op == ast::Operator::equal);
}

void index_args_and_colon() {
  cursiv::ParseResult r = build("y = x(1:2, :)\n");
  ast::AssignStmt *const assign =
      as<ast::AssignStmt>(r.value_as<ast::File>()->statements[0]);
  ast::IndexExpr *const idx =
      assign != nullptr ? as<ast::IndexExpr>(assign->value) : nullptr;
  CHECK(idx != nullptr && idx->name == "x" && idx->args.size() == 2);
  if (idx == nullptr || idx->args.size() != 2)
    return;
  CHECK(!idx->args[0]->all && idx->args[0]->value != nullptr);
  CHECK(idx->args[1]->all && idx->args[1]->value == nullptr);
}

// A line break not immediately after a header or before `end`/`elseif`/
// `else` closes the statement in front of it rather than standing as its
// own: written this way, every clause holds exactly the statement it names.
void if_elseif_else_shape() {
  cursiv::ParseResult r =
      build("if a b = 1; elseif c d = 2; elseif e f = 3; else g = 4; end\n");
  ast::IfStmt *const ifs =
      as<ast::IfStmt>(r.value_as<ast::File>()->statements[0]);
  CHECK(ifs != nullptr && ifs->thenBody.size() == 1 &&
        ifs->elseIfs.size() == 2 && ifs->elseBody.size() == 1);
}

std::string load_example(const fs::path &path) {
  return cursiv::test::read_file(path.string());
}

// Every document under example/ parses clean: no repair, no diagnostic, and
// the root is a File. A converter that silently refuses would still leave
// `recovered()` false and the root type right, so the diagnostics list is
// checked on its own.
void corpus() {
  std::size_t seen = 0;
  for (const fs::path &dir :
       {fs::path(CURSIV_EXAMPLE_DIR) / "example", fs::path(CURSIV_TEST_DIR)}) {
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (entry.path().extension() != ".matlab")
        continue;
      ++seen;
      const std::string src = load_example(entry.path());
      CHECK(!src.empty());
      const cursiv::ParseResult r = engine().parse(src);
      if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
        std::printf("FAIL %s did not parse clean\n",
                    entry.path().filename().string().c_str());
        ++failures;
        continue;
      }
      CHECK(r.value_as<ast::File>() != nullptr);
    }
  }
  CHECK(seen == 5);
}

// A few real field values, read back out of one document, so a converter
// silently returning the wrong value would fail this even though the
// structural checks above would not notice.
void field_values_from_control_flow() {
  const fs::path path = fs::path(CURSIV_TEST_DIR) / "control_flow.matlab";
  const std::string src = load_example(path);
  CHECK(!src.empty());
  const cursiv::ParseResult r = build(src);
  ast::File *const file = r.value_as<ast::File>();
  CHECK(file != nullptr);
  if (file == nullptr)
    return;

  ast::GlobalStmt *const g = as<ast::GlobalStmt>(file->statements[0]);
  CHECK(g != nullptr && g->names.size() == 3 && g->names[0] == "a" &&
        g->names[1] == "b" && g->names[2] == "c");

  ast::ClearStmt *const c = as<ast::ClearStmt>(file->statements[1]);
  CHECK(c != nullptr && c->names.size() == 2 && c->names[0] == "x" &&
        c->names[1] == "y");

  // The first `for`: the classic header, no parentheses.
  ast::ForStmt *forStmt = nullptr;
  for (ast::Statement *s : file->statements) {
    forStmt = as<ast::ForStmt>(s);
    if (forStmt != nullptr)
      break;
  }
  CHECK(forStmt != nullptr && forStmt->var == "i");
  CHECK(forStmt != nullptr && as<ast::BinaryExpr>(forStmt->range) != nullptr);
}

} // namespace

int main() {
  {
    const auto result =
        engine().parse("x = [1,2;3,4]\ny = [5 6\n7 8]\nt = x'\n");
    CHECK(result.accepted && !result.cst().recovered());
    int matrices = 0;
    for (const auto *node : result.nodes)
      if (const auto *a = cursiv::ast_cast<ast::ArrayLit>(node)) {
        CHECK(a->rows.size() == 2);
        CHECK(a->rows[0]->elements.size() == 2 &&
              a->rows[1]->elements.size() == 2);
        ++matrices;
      }
    CHECK(matrices == 2);
  }

  accepted_texts();
  refused_texts();
  operator_precedence();
  index_args_and_colon();
  if_elseif_else_shape();
  corpus();
  field_values_from_control_flow();
  return cursiv::test::report("matlab_grammar_test");
}
