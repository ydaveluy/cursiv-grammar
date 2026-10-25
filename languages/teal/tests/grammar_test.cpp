#include "audit_regressions.hpp"
// The teal grammar end to end: every example parses clean, and a few real
// field values are read back so a converter that silently misfires would
// not pass unnoticed.

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "teal/core/ast.hpp"
#include "teal/generated/Teal/Module.hpp"

namespace {

namespace ast = teal::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      teal::generated::Teal::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

// Every `typ` is a `UnionType`, even a single base type: unwrap the trivial
// case to read the base type a test cares about.
ast::Type* base(ast::Type* t) {
  ast::UnionType* const u = as<ast::UnionType>(t);
  return (u != nullptr && u->alternatives.size() == 1) ? u->alternatives[0]
                                                        : t;
}

// The document outlives the tree: every AST member is a `string_view` into
// the source, so a composed document must live past the `ParseResult` that
// points into it.
const std::string& kept(std::string text) {
  static std::deque<std::string> documents;
  documents.push_back(std::move(text));
  return documents.back();
}

cursiv::ParseResult parse(std::string_view text) { return engine().parse(text); }

const char* const kExamples[] = {
    "arrays.tl", "enums.tl", "functions.tl",
};

// This port's own documents: nested record types, map/array/union types,
// generics, `is`/`as`, `global`, attribs, `goto`, bitwise, a shebang --
// none of it the corpus exercises.
const char* const kOwnDocs[] = {
    "types.tl", "kitchen_sink.tl", "shebang.tl",
};

void every_example_parses_clean() {
  for (const char* const name : kExamples) {
    const std::string text = cursiv::test::read_file(
        std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(kept(text));
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(r.diagnostics.empty());
    ast::Block* const root = as<ast::Block>(r.value);
    CHECK(root != nullptr);
  }
  for (const char* const name : kOwnDocs) {
    const std::string text = cursiv::test::read_file(
        std::string(CURSIV_TEST_DIR) + "/" + name);
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(kept(text));
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(r.diagnostics.empty());
    ast::Block* const root = as<ast::Block>(r.value);
    CHECK(root != nullptr);
  }
}

// `local values: {number}` then `local names = {"John", ...}`: an array
// type read back, and a string table read back.
void arrays_example_values() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/arrays.tl");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;

  ast::LocalAttrAssignStat* const values =
      as<ast::LocalAttrAssignStat>(root->statements[0]);
  CHECK(values != nullptr && values->vars.size() == 1 &&
        values->vars[0]->name == "values");
  CHECK(values != nullptr && values->types.size() == 1);
  ast::UnionType* const typ =
      values != nullptr && !values->types.empty()
          ? as<ast::UnionType>(values->types[0])
          : nullptr;
  CHECK(typ != nullptr && typ->alternatives.size() == 1);
  ast::ArrayType* const arr =
      typ != nullptr && !typ->alternatives.empty()
          ? as<ast::ArrayType>(typ->alternatives[0])
          : nullptr;
  ast::PrimitiveType* const elem =
      arr != nullptr ? as<ast::PrimitiveType>(base(arr->element)) : nullptr;
  CHECK(elem != nullptr && elem->kind == "number");

  ast::LocalAttrAssignStat* const names =
      as<ast::LocalAttrAssignStat>(root->statements[1]);
  CHECK(names != nullptr && names->vars.size() == 1 &&
        names->vars[0]->name == "names");
  CHECK(names != nullptr && names->values.size() == 1);
  ast::TableExpr* const table =
      names != nullptr && !names->values.empty()
          ? as<ast::TableExpr>(names->values[0])
          : nullptr;
  CHECK(table != nullptr && table->fields.size() == 4);
  if (table != nullptr && table->fields.size() == 4) {
    const char* const want[4] = {"John", "Paul", "George", "Ringo"};
    for (std::size_t i = 0; i < 4; ++i) {
      ast::StringExpr* const s = as<ast::StringExpr>(table->fields[i]->value);
      CHECK(s != nullptr && s->value == want[i]);
    }
  }
}

// `local Direction = enum "north" "south" "east" "west" end`: the string
// values, unquoted, in order.
void enums_example_values() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/enums.tl");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;

  ast::LocalNewTypeStat* const stat =
      as<ast::LocalNewTypeStat>(root->statements[0]);
  CHECK(stat != nullptr && stat->name == "Direction");
  ast::EnumNewType* const en =
      stat != nullptr ? as<ast::EnumNewType>(stat->value) : nullptr;
  CHECK(en != nullptr && en->values.size() == 4);
  if (en != nullptr && en->values.size() == 4) {
    CHECK(en->values[0] == "north" && en->values[1] == "south" &&
          en->values[2] == "east" && en->values[3] == "west");
  }
}

// `local Comparator = functiontype<T>(T, T): boolean` then a generic
// function reading a generic type back as its parameter.
void functions_example_values() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/functions.tl");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;

  ast::LocalNewTypeStat* const cmp =
      as<ast::LocalNewTypeStat>(root->statements[0]);
  CHECK(cmp != nullptr && cmp->name == "Comparator");
  ast::FuncNewType* const fn =
      cmp != nullptr ? as<ast::FuncNewType>(cmp->value) : nullptr;
  CHECK(fn != nullptr && fn->signature != nullptr);
  if (fn != nullptr && fn->signature != nullptr) {
    ast::FunctionSigType* const sig = fn->signature;
    CHECK(sig->typeParams != nullptr && sig->typeParams->names.size() == 1 &&
          sig->typeParams->names[0] == "T");
    CHECK(sig->params.size() == 2);
    CHECK(sig->rets != nullptr && sig->rets->types.size() == 1);
    ast::PrimitiveType* const ret =
        sig->rets != nullptr && !sig->rets->types.empty()
            ? as<ast::PrimitiveType>(base(sig->rets->types[0]))
            : nullptr;
    CHECK(ret != nullptr && ret->kind == "boolean");
  }

  ast::LocalFuncStat* const mysort =
      as<ast::LocalFuncStat>(root->statements[1]);
  CHECK(mysort != nullptr && mysort->name == "mysort");
  ast::FuncBody* const body = mysort != nullptr ? mysort->body : nullptr;
  CHECK(body != nullptr && body->typeParams != nullptr &&
        body->typeParams->names.size() == 1 &&
        body->typeParams->names[0] == "A");
  CHECK(body != nullptr && body->params.size() == 2);
  if (body != nullptr && body->params.size() == 2) {
    CHECK(body->params[0]->name == "arr");
    ast::ArrayType* const argArr =
        as<ast::ArrayType>(base(body->params[0]->type));
    ast::NamedType* const elem =
        argArr != nullptr ? as<ast::NamedType>(base(argArr->element))
                          : nullptr;
    CHECK(elem != nullptr && elem->name == "A");

    CHECK(body->params[1]->name == "cmp");
    ast::NamedType* const cmpType =
        as<ast::NamedType>(base(body->params[1]->type));
    CHECK(cmpType != nullptr && cmpType->name == "Comparator");
    CHECK(cmpType != nullptr && cmpType->typeArgs != nullptr &&
          cmpType->typeArgs->names.size() == 1 &&
          cmpType->typeArgs->names[0] == "A");
  }
  // The body is one comment and nothing else.
  CHECK(body != nullptr && body->body != nullptr &&
        body->body->statements.empty());
}

// `^` binds tighter than a unary operator, but a unary operand may itself
// hold a `^` chain: `-2^2` is `-(2^2)`.
void pow_against_unary() {
  const cursiv::ParseResult r = parse(kept("local x = -2^2\n"));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;
  ast::LocalAttrAssignStat* const s =
      as<ast::LocalAttrAssignStat>(root->statements[0]);
  CHECK(s != nullptr && s->values.size() == 1);
  ast::UnaryExpr* const neg =
      s != nullptr && !s->values.empty() ? as<ast::UnaryExpr>(s->values[0])
                                          : nullptr;
  CHECK(neg != nullptr && neg->op == ast::UnOp::neg);
  ast::BinaryExpr* const pow =
      neg != nullptr ? as<ast::BinaryExpr>(neg->operand) : nullptr;
  CHECK(pow != nullptr && pow->op == ast::BinOp::pow);
}

// `x is number` reads a bare name and a type; `x as number` wraps the
// expression it follows and binds tighter than every other operator.
void is_and_as() {
  const cursiv::ParseResult r =
      parse(kept("local y = x is number\nlocal z = 1 + x as number\n"));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;

  ast::LocalAttrAssignStat* const y =
      as<ast::LocalAttrAssignStat>(root->statements[0]);
  ast::IsExpr* const isExpr =
      y != nullptr && !y->values.empty() ? as<ast::IsExpr>(y->values[0])
                                          : nullptr;
  CHECK(isExpr != nullptr && isExpr->name == "x");
  ast::PrimitiveType* const isType =
      isExpr != nullptr ? as<ast::PrimitiveType>(base(isExpr->type))
                        : nullptr;
  CHECK(isType != nullptr && isType->kind == "number");

  ast::LocalAttrAssignStat* const z =
      as<ast::LocalAttrAssignStat>(root->statements[1]);
  ast::BinaryExpr* const add =
      z != nullptr && !z->values.empty() ? as<ast::BinaryExpr>(z->values[0])
                                          : nullptr;
  CHECK(add != nullptr && add->op == ast::BinOp::add);
  ast::AsExpr* const asExpr =
      add != nullptr ? as<ast::AsExpr>(add->right) : nullptr;
  CHECK(asExpr != nullptr);
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  every_example_parses_clean();
  arrays_example_values();
  enums_example_values();
  functions_example_values();
  pow_against_unary();
  is_and_as();
  return cursiv::test::report("teal_grammar_test");
}
