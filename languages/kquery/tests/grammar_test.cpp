// KQuery end to end: the corpus accepted clean, a fixture of this port's own
// exercising the operator families no corpus file reaches, and the shapes a
// document may not take.

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstMetaData.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "kquery/core/ast.hpp"
#include "kquery/generated/Kquery/AstReflection.hpp"
#include "kquery/generated/Kquery/Module.hpp"

namespace {

namespace ast = kquery::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      kquery::generated::Kquery::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      kquery::generated::Kquery::create_ast_reflection();
  return *r;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  return r;
}

std::string read_file(std::string_view path) {
  return cursiv::test::read_file(std::string(path));
}

void accepted_and_refused_texts() {
  CHECK(accepts("array A[2] : w32 -> w8 = [5,6]\n(query [] false)"));
  CHECK(accepts(""));  // queryStatements is `(ktranslationUnit)*`
  CHECK(accepts("array a[1] : w32 -> w8 = symbolic"));
  CHECK(accepts("(query [] false)"));
  CHECK(accepts("(query [(Eq 1 2)] false)"));

  CHECK(!accepts("(query [] false"));         // unclosed
  CHECK(!accepts("array a[1] : w32 -> w8"));  // no initializer
  CHECK(!accepts("(query [] )"));             // no query expression
  CHECK(!accepts("(Add w8 1 2)"));  // a bare expression is not a translation unit
  CHECK(!accepts("N0:(Add w8 1 2)"));  // ...labelled or not
  CHECK(!accepts("(query [] false))"));       // a stray extra paren
}

// A named abbreviation and a bare reference to the same spelling read as
// the same class either way, so a chain of array versions does not fork at
// every step.
void named_and_bare_are_one_class() {
  cursiv::ParseResult r = build("(query [] N0:(Add w8 1 2))");
  ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->units.size() == 1);
  ast::QueryCommand* const q = as<ast::QueryCommand>(doc->units[0]);
  CHECK(q != nullptr);
  ast::NamedExpr* const named = q != nullptr ? as<ast::NamedExpr>(q->query) : nullptr;
  CHECK(named != nullptr && named->name == "N0");
  ast::BinaryExpr* const add = named != nullptr ? as<ast::BinaryExpr>(named->value) : nullptr;
  CHECK(add != nullptr && add->op == "Add" && add->width == "w8");

  cursiv::ParseResult bare = build("(query [] N0)");
  ast::QueryCommand* const q2 =
      as<ast::QueryCommand>(bare.value_as<ast::Document>()->units[0]);
  CHECK(q2 != nullptr && as<ast::VarRef>(q2->query) != nullptr &&
        as<ast::VarRef>(q2->query)->name == "N0");
}

void array_declarations() {
  cursiv::ParseResult r = build(
      "array sym[4] : w32 -> w8 = symbolic\n"
      "array lit[3] : w32 -> w8 = [1 2 3]\n");
  ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->units.size() == 2);
  ast::ArrayDecl* const sym = as<ast::ArrayDecl>(doc->units[0]);
  CHECK(sym != nullptr && sym->name == "sym" && sym->size == "4" &&
        sym->domain == "w32" && sym->range == "w8" && sym->symbolic &&
        sym->values.empty());
  ast::ArrayDecl* const lit = as<ast::ArrayDecl>(doc->units[1]);
  CHECK(lit != nullptr && !lit->symbolic && lit->values.size() == 3 &&
        lit->values[0]->text == "1" && lit->values[2]->text == "3");
}

void update_chain() {
  cursiv::ParseResult r =
      build("(query [] (Read w8 0 U0:[1=2,0=3] @ base))");
  ast::QueryCommand* const q =
      as<ast::QueryCommand>(r.value_as<ast::Document>()->units[0]);
  ast::ReadExpr* const read = q != nullptr ? as<ast::ReadExpr>(q->query) : nullptr;
  CHECK(read != nullptr && read->op == "Read" && read->width == "w8");
  ast::NamedExpr* const named = read != nullptr ? as<ast::NamedExpr>(read->array) : nullptr;
  CHECK(named != nullptr && named->name == "U0");
  ast::UpdateExpr* const upd = named != nullptr ? as<ast::UpdateExpr>(named->value) : nullptr;
  CHECK(upd != nullptr && upd->updates.size() == 2);
  CHECK(upd != nullptr && as<ast::VarRef>(upd->base) != nullptr &&
        as<ast::VarRef>(upd->base)->name == "base");
  // An empty update list is legal, and it carries no wrapping parentheses of
  // its own -- `expression`'s other bracketed forms all open on `(`.
  CHECK(accepts("(query [] [] @ base)"));
}

void corpus() {
  const char* const names[] = {"test1.kquery", "test2.kquery", "test3.kquery",
                               "test4.kquery", "test5.kquery", "test6.kquery",
                               "test7.kquery", "test8.kquery"};
  for (const char* const name : names) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/example/" + name;
    const std::string text = read_file(path);
    CHECK(!text.empty());
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(r.diagnostics.empty());
    ast::Document* const doc = r.value_as<ast::Document>();
    CHECK(doc != nullptr && !doc->units.empty());
  }

  // Real values, read back out of one document: test6's three arrays, then
  // its single query command's constraints and the labelled reads chained
  // through them.
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example/test6.kquery");
  const cursiv::ParseResult r = engine().parse(text);
  ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->units.size() == 4);
  if (doc == nullptr || doc->units.size() != 4) return;

  ast::ArrayDecl* const a0 = as<ast::ArrayDecl>(doc->units[0]);
  CHECK(a0 != nullptr && a0->name == "sample1_pse" && a0->size == "4" &&
        a0->domain == "w32" && a0->range == "w8" && a0->symbolic);

  ast::QueryCommand* const q = as<ast::QueryCommand>(doc->units[3]);
  CHECK(q != nullptr && q->constraints.size() == 6);
  if (q == nullptr || q->constraints.size() != 6) return;

  ast::NamedExpr* const first = as<ast::NamedExpr>(q->constraints[0]);
  CHECK(first == nullptr);  // the first constraint is a bare Ule, unlabelled
  ast::BinaryExpr* const ule0 = as<ast::BinaryExpr>(q->constraints[0]);
  CHECK(ule0 != nullptr && ule0->op == "Ule");
  ast::NamedExpr* const n0 = as<ast::NamedExpr>(ule0->left);
  CHECK(n0 != nullptr && n0->name == "N0");
  ast::ReadExpr* const read0 = as<ast::ReadExpr>(n0->value);
  CHECK(read0 != nullptr && read0->op == "ReadLSB" && read0->width == "w32");
  ast::NumberLit* const off0 = as<ast::NumberLit>(read0->index);
  CHECK(off0 != nullptr && off0->text == "0");
  ast::VarRef* const arr0 = as<ast::VarRef>(read0->array);
  CHECK(arr0 != nullptr && arr0->name == "sample1_pse");
  ast::NumberLit* const bound = as<ast::NumberLit>(ule0->right);
  CHECK(bound != nullptr && bound->text == "4294967295");

  // The query expression itself: `false`.
  ast::NumberLit* const query = as<ast::NumberLit>(q->query);
  CHECK(query != nullptr && query->text == "false");
  CHECK(q->evalExprs.empty() && q->evalArrayNames.empty());
}

// The operator families and literal forms no corpus file reaches: every
// bitwise and arithmetic keyword beyond `And`/`Or`/`Add`/`SDiv`, every
// comparison beyond `Eq`/`Ule`/`Sle`/`Ult`, `Concat`, `Not`, `Neg` with and
// without its width, the size-query form, the numeric bases, and the
// trailing eval lists of a query command.
void operator_fixture() {
  const std::string text = read_file(
      std::string(CURSIV_TEST_DIR) + "/fixtures/operators.kquery");
  CHECK(!text.empty());
  const cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->units.size() == 2);
  if (doc == nullptr || doc->units.size() != 2) return;

  ast::QueryCommand* const q = as<ast::QueryCommand>(doc->units[1]);
  CHECK(q != nullptr);
  if (q == nullptr) return;
  CHECK(q->constraints.size() == 25);
  CHECK(q->evalExprs.size() == 1);
  CHECK(q->evalArrayNames.size() == 2 && q->evalArrayNames[0] == "x" &&
        q->evalArrayNames[1] == "y");

  // Every "(Eq false (OP ...))" constraint's inner operator, in the order
  // written.
  const std::string_view wantOps[] = {
      "Not", "Shl", "LShr", "AShr", "Xor", "Sub",  "Mul", "UDiv",
      "URem", "SRem", "Ne",  "Ugt", "Uge", "Slt", "Sgt", "Sge", "Concat"};
  for (std::size_t i = 0; i < std::size(wantOps); ++i) {
    ast::BinaryExpr* const outer = as<ast::BinaryExpr>(q->constraints[i]);
    CHECK(outer != nullptr && outer->op == "Eq");
    if (outer == nullptr) continue;
    ast::UnaryExpr* const un = as<ast::UnaryExpr>(outer->right);
    ast::BinaryExpr* const bin = as<ast::BinaryExpr>(outer->right);
    CHECK((un != nullptr && un->op == wantOps[i]) ||
          (bin != nullptr && bin->op == wantOps[i]));
  }
  ast::UnaryExpr* const negBare =
      as<ast::UnaryExpr>(as<ast::BinaryExpr>(q->constraints[17])->right);
  CHECK(negBare != nullptr && negBare->op == "Neg" && negBare->width.empty());
  ast::UnaryExpr* const negWide =
      as<ast::UnaryExpr>(as<ast::BinaryExpr>(q->constraints[18])->right);
  CHECK(negWide != nullptr && negWide->op == "Neg" && negWide->width == "w8");

  ast::SizeExpr* const size = as<ast::SizeExpr>(q->constraints[19]);
  CHECK(size != nullptr && size->width == "w8" && size->value->text == "5");
  CHECK(as<ast::NumberLit>(q->constraints[20])->text == "+3");
  CHECK(as<ast::NumberLit>(q->constraints[21])->text == "-4");
  CHECK(as<ast::NumberLit>(q->constraints[22])->text == "0x1F");
  CHECK(as<ast::NumberLit>(q->constraints[23])->text == "0o17");
  CHECK(as<ast::NumberLit>(q->constraints[24])->text == "0b101");

  ast::ReadExpr* const msb = as<ast::ReadExpr>(q->query);
  CHECK(msb != nullptr && msb->op == "ReadMSB");
}

}  // namespace

int main() {
  accepted_and_refused_texts();
  named_and_bare_are_one_class();
  array_declarations();
  update_chain();
  corpus();
  operator_fixture();
  return cursiv::test::report("kquery_grammar_test");
}
