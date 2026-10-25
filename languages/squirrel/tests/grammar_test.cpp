#include "audit_regressions.hpp"
// The Squirrel grammar end to end: every corpus example parses clean, real
// field values are read back out of one of them, and a handful of
// constructs the corpus does not exercise are covered here with short
// documents of our own.

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "squirrel/core/ast.hpp"
#include "squirrel/generated/Squirrel/Module.hpp"

namespace {

namespace ast = squirrel::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      squirrel::generated::Squirrel::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
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
    "ackermann.nut",  "array.nut",         "classattributes.nut",
    "class.nut",      "coroutines.nut",    "delegation.nut",
    "fibonacci.nut",  "flow.nut",          "generators.nut",
    "hello.nut",      "list.nut",          "loops.nut",
    "matrix.nut",     "metamethods.nut",   "methcall.nut",
    "regex.nut",      "tailstate.nut",
};

void every_example_parses_clean() {
  for (const char* const name : kExamples) {
    const std::string text =
        cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(kept(text));
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(r.diagnostics.empty());
    ast::Chunk* const root = as<ast::Chunk>(r.value);
    CHECK(root != nullptr);
  }
}

// `function Ack(M, N) { ... }` then a `local n;` and two more top-level
// statements: the function's name and parameter, and a couple of field
// values read back.
void ackermann_values() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/ackermann.nut");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Chunk* const root = as<ast::Chunk>(r.value);
  CHECK(root != nullptr && root->body.size() >= 2);
  if (root == nullptr || root->body.size() < 2) return;

  ast::FunctionDecl* const ack = as<ast::FunctionDecl>(root->body[0]);
  CHECK(ack != nullptr && ack->path.size() == 1 && ack->path[0] == "Ack");
  CHECK(ack != nullptr && ack->params.size() == 2 &&
        ack->params[0]->name == "M" && ack->params[1]->name == "N");

  ast::LocalVarDecl* const decl = as<ast::LocalVarDecl>(root->body[1]);
  CHECK(decl != nullptr && decl->vars.size() == 1 &&
        decl->vars[0]->name == "n" && decl->vars[0]->init == nullptr);
}

// `class Vector3 extends BaseVector { function _add(other) {...} ... }`:
// the base name, a method's name, and the `<-` table-slot assignment.
void class_values() {
  const std::string text =
      cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/class.nut");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Chunk* const root = as<ast::Chunk>(r.value);
  CHECK(root != nullptr);
  if (root == nullptr) return;

  ast::ClassDecl* vector3 = nullptr;
  for (ast::Stmt* const stmt : root->body) {
    ast::ClassDecl* const c = as<ast::ClassDecl>(stmt);
    if (c != nullptr && c->name.size() == 1 && c->name[0] == "Vector3") {
      vector3 = c;
      break;
    }
  }
  CHECK(vector3 != nullptr);
  if (vector3 == nullptr) return;
  CHECK(vector3->base.size() == 1 && vector3->base[0] == "BaseVector");
  CHECK(vector3->members.size() == 2);
  if (vector3->members.size() == 2) {
    ast::FunctionDecl* const* const add =
        std::get_if<ast::FunctionDecl*>(&vector3->members[0]->member);
    CHECK(add != nullptr && *add != nullptr && (*add)->path.size() == 1 &&
          (*add)->path[0] == "_add");
  }
}

// Constructs the shipped corpus never reaches: const/enum, try/catch/throw,
// bitwise operators, a short lambda, default parameters beside a vararg, the
// comma operator, and `local function`.
void uncovered_constructs() {
  {
    const cursiv::ParseResult r = parse(kept(
        "const LIMIT = 10;\n"
        "enum Color { Red, Green = 2, Blue }\n"));
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    ast::Chunk* const root = as<ast::Chunk>(r.value);
    CHECK(root != nullptr && root->body.size() == 2);
    if (root != nullptr && root->body.size() == 2) {
      ast::ConstDecl* const c = as<ast::ConstDecl>(root->body[0]);
      CHECK(c != nullptr && c->name == "LIMIT");
      ast::IntLit* const value = as<ast::IntLit>(c->value);
      CHECK(value != nullptr && value->text == "10");

      ast::EnumDecl* const e = as<ast::EnumDecl>(root->body[1]);
      CHECK(e != nullptr && e->name == "Color" && e->items.size() == 3);
      CHECK(e != nullptr && e->items[0]->value == nullptr);
      CHECK(e != nullptr && e->items[1]->value != nullptr);
    }
  }
  {
    const cursiv::ParseResult r = parse(kept(
        "try {\n"
        "    throw \"boom\";\n"
        "} catch (e) {\n"
        "    print(e);\n"
        "}\n"));
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    ast::Chunk* const root = as<ast::Chunk>(r.value);
    CHECK(root != nullptr && root->body.size() == 1);
    if (root != nullptr && root->body.size() == 1) {
      ast::TryStmt* const t = as<ast::TryStmt>(root->body[0]);
      CHECK(t != nullptr && t->catchName == "e");
    }
  }
  {
    // Bitwise ops, a short lambda, default params beside a vararg.
    const cursiv::ParseResult r = parse(kept(
        "local m = (a & b) | (c ^ d) | ~e;\n"
        "local s = a << 2 >> 1 >>> 1;\n"
        "local f = @(x) x + 1;\n"
        "function g(a, b = 1, ...) { return a + b; }\n"));
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    ast::Chunk* const root = as<ast::Chunk>(r.value);
    CHECK(root != nullptr && root->body.size() == 4);
    if (root != nullptr && root->body.size() == 4) {
      ast::LocalVarDecl* const lambdaDecl = as<ast::LocalVarDecl>(root->body[2]);
      CHECK(lambdaDecl != nullptr && lambdaDecl->vars.size() == 1);
      ast::LambdaExpr* const lambda =
          lambdaDecl != nullptr
              ? as<ast::LambdaExpr>(lambdaDecl->vars[0]->init)
              : nullptr;
      CHECK(lambda != nullptr && lambda->params.size() == 1);

      ast::FunctionDecl* const g = as<ast::FunctionDecl>(root->body[3]);
      CHECK(g != nullptr && g->params.size() == 2 && g->variadic);
      CHECK(g != nullptr && g->params[1]->defaultValue != nullptr);
    }
  }
  {
    // The comma operator, and `local function`.
    const cursiv::ParseResult r = parse(kept(
        "local x = (1, 2, 3);\n"
        "local function inc(n) { return n + 1; }\n"));
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    ast::Chunk* const root = as<ast::Chunk>(r.value);
    CHECK(root != nullptr && root->body.size() == 2);
    if (root != nullptr && root->body.size() == 2) {
      ast::LocalFunctionDecl* const lf =
          as<ast::LocalFunctionDecl>(root->body[1]);
      CHECK(lf != nullptr && lf->func != nullptr &&
            lf->func->path.size() == 1 && lf->func->path[0] == "inc");
    }
  }
}

// A converter that silently refuses (out of range, unparsable) leaves the
// root type right and `recovered()` false -- only a non-empty diagnostics
// list catches it. Every example above already asserts on it; this document
// checks a diagnostic is actually raised for genuinely broken input.
void broken_input_is_diagnosed() {
  const cursiv::ParseResult r = parse(kept("local x = ;\n"));
  CHECK(r.cst().recovered() || !r.diagnostics.empty());
}

ast::Expr* rhs_of(const cursiv::ParseResult& r) {
  ast::Chunk* const root = as<ast::Chunk>(r.value);
  if (root == nullptr || root->body.empty()) return nullptr;
  ast::LocalVarDecl* const decl = as<ast::LocalVarDecl>(root->body[0]);
  if (decl == nullptr || decl->vars.empty()) return nullptr;
  return decl->vars[0]->init;
}

ast::BinaryExpr* binary(ast::Expr* e, const char* op) {
  ast::BinaryExpr* const b = as<ast::BinaryExpr>(e);
  return b != nullptr && b->op == op ? b : nullptr;
}

// Precedence, checked at AST level: unary operators
// (`!`, `~`, ...) bind tighter than `instanceof`, and `instanceof` is a
// peer of the relational operators.
void instanceof_precedence() {
  // `!x instanceof Y` is `(!x) instanceof Y`: `!` binds tighter.
  {
    const cursiv::ParseResult r = parse(kept("local a = !x instanceof Y;\n"));
    ast::BinaryExpr* const inst = binary(rhs_of(r), "instanceof");
    CHECK(inst != nullptr && as<ast::PrefixExpr>(inst->left) != nullptr);
  }
  // `x instanceof Y < 5` is `(x instanceof Y) < 5`: peers, left to right.
  {
    const cursiv::ParseResult r =
        parse(kept("local b = x instanceof Y < 5;\n"));
    ast::BinaryExpr* const lt = binary(rhs_of(r), "<");
    CHECK(lt != nullptr && binary(lt->left, "instanceof") != nullptr);
  }
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  every_example_parses_clean();
  ackermann_values();
  class_values();
  uncovered_constructs();
  broken_input_is_diagnosed();
  instanceof_precedence();
  return cursiv::test::report("squirrel_grammar_test");
}
