#include "audit_regressions.hpp"
#include "modern_cases.hpp"
// The wren grammar end to end: every corpus example parses clean, real field
// values are read back out of one of them, and a handful of constructs the
// corpus does not exercise are covered here with short documents of our own.

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "wren/core/ast.hpp"
#include "wren/generated/Wren/Module.hpp"

namespace {

namespace ast = wren::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      wren::generated::Wren::create_parser();
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
    "binary_tree.wren", "binary_tree_gc.wren", "call.wren",
    "call_wren_root.wren", "delta_blue.wren", "error.wren",
    "get_variable.wren", "method_call.wren",
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
    ast::Script* const root = as<ast::Script>(r.value);
    CHECK(root != nullptr);
  }
}

// `class Tree { construct new(item, depth) { ... } check { ... } }`, then a
// run of top-level `var` declarations: the class's two members, and one
// declaration's name and value read back.
void binary_tree_values() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/binary_tree.wren");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() >= 2);
  if (root == nullptr || root->statements.size() < 2) return;

  ast::ClassDef* const tree = as<ast::ClassDef>(root->statements[0]);
  CHECK(tree != nullptr && tree->name == "Tree" && tree->members.size() == 2);
  if (tree != nullptr && tree->members.size() == 2) {
    ast::ConstructorDef* const ctor = as<ast::ConstructorDef>(tree->members[0]);
    CHECK(ctor != nullptr && ctor->name == "new" && ctor->params.size() == 2 &&
          ctor->params[0] == "item" && ctor->params[1] == "depth");

    ast::GetterDef* const check = as<ast::GetterDef>(tree->members[1]);
    CHECK(check != nullptr && check->name == "check");
  }

  ast::ExprStat* const minDepthStat = as<ast::ExprStat>(root->statements[1]);
  ast::AssignStat* const minDepth =
      minDepthStat != nullptr ? as<ast::AssignStat>(minDepthStat->expr) : nullptr;
  CHECK(minDepth != nullptr && minDepth->isVar && minDepth->op == "=");
  ast::NameExpr* const minDepthTarget =
      minDepth != nullptr ? as<ast::NameExpr>(minDepth->target) : nullptr;
  CHECK(minDepthTarget != nullptr && minDepthTarget->name == "minDepth");
  ast::NumExpr* const four =
      minDepth != nullptr ? as<ast::NumExpr>(minDepth->value) : nullptr;
  CHECK(four != nullptr && four->text == "4");
}

// `for (d in 0...maxDepth) { ... }`: an exclusive range read back as a
// `BinaryExpr` whose `op` is the operator's own spelling, `"..."`.
void exclusive_range_in_for() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_EXAMPLE_DIR) + "/binary_tree.wren");
  const cursiv::ParseResult r = parse(kept(text));
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr);
  if (root == nullptr) return;
  for (ast::Stat* const stat : root->statements) {
    ast::ForStat* const forStat = as<ast::ForStat>(stat);
    if (forStat == nullptr) continue;
    CHECK(forStat->var == "d");
    ast::BinaryExpr* const range = as<ast::BinaryExpr>(forStat->iterable);
    CHECK(range != nullptr && range->op == "...");
    return;
  }
  CHECK(false);  // no `for` statement found
}

// `"%(iterations * 2) trees of depth %(depth) check: %(check)"`: the
// interpolation contains text and expression parts.
void interpolation_parts_are_structured() {
  const cursiv::ParseResult r =
      parse(kept(std::string("var s = \"a %(x) b\"\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[0]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  ast::StringExpr* const s = decl != nullptr ? as<ast::StringExpr>(decl->value) : nullptr;
  CHECK(s != nullptr && s->parts.size() == 3);
  CHECK(s != nullptr && s->parts.size() == 3 && as<ast::StringInterpolation>(s->parts[1]) != nullptr);
}

// `var x` with no initializer.
void var_without_init() {
  const cursiv::ParseResult r = parse(kept(std::string("var x\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[0]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  CHECK(decl != nullptr && decl->isVar && decl->value == nullptr);
}

// `b = a = 2`: chained assignment, right-recursive on the value.
void chained_assignment() {
  const cursiv::ParseResult r =
      parse(kept(std::string("var a = 1\nvar b\nb = a = 2\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 3);
  if (root == nullptr || root->statements.size() != 3) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[2]);
  ast::AssignStat* const outer = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  CHECK(outer != nullptr && !outer->isVar);
  ast::AssignStat* const inner =
      outer != nullptr ? as<ast::AssignStat>(outer->value) : nullptr;
  CHECK(inner != nullptr && inner->op == "=");
}

// `1 > 0 ? "yes" : "no"`.
void ternary() {
  const cursiv::ParseResult r =
      parse(kept(std::string("var picked = 1 > 0 ? \"yes\" : \"no\"\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[0]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  ast::CondExpr* const cond = decl != nullptr ? as<ast::CondExpr>(decl->value) : nullptr;
  CHECK(cond != nullptr);
}

// Comparison binds tighter than `is`, which binds tighter than equality
// (wren_compiler.c's own `rules[]` table): `a == b < c` is `a == (b < c)`,
// and `a < b is Num` is `(a < b) is Num`.
void comparison_is_equality_levels() {
  const cursiv::ParseResult r1 =
      parse(kept(std::string("var x = a == b < c\n")));
  CHECK(r1.accepted && !r1.cst().recovered() && r1.diagnostics.empty());
  ast::Script* const root1 = as<ast::Script>(r1.value);
  CHECK(root1 != nullptr && root1->statements.size() == 1);
  if (root1 != nullptr && !root1->statements.empty()) {
    ast::ExprStat* const es = as<ast::ExprStat>(root1->statements[0]);
    ast::AssignStat* const decl =
        es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
    ast::BinaryExpr* const eq =
        decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
    CHECK(eq != nullptr && eq->op == "==");
    ast::BinaryExpr* const cmp =
        eq != nullptr ? as<ast::BinaryExpr>(eq->right) : nullptr;
    CHECK(cmp != nullptr && cmp->op == "<");
  }

  const cursiv::ParseResult r2 =
      parse(kept(std::string("var y = a < b is Num\n")));
  CHECK(r2.accepted && !r2.cst().recovered() && r2.diagnostics.empty());
  ast::Script* const root2 = as<ast::Script>(r2.value);
  CHECK(root2 != nullptr && root2->statements.size() == 1);
  if (root2 != nullptr && !root2->statements.empty()) {
    ast::ExprStat* const es = as<ast::ExprStat>(root2->statements[0]);
    ast::AssignStat* const decl =
        es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
    ast::BinaryExpr* const is = decl != nullptr
                                     ? as<ast::BinaryExpr>(decl->value)
                                     : nullptr;
    CHECK(is != nullptr && is->op == "is");
    ast::BinaryExpr* const cmp =
        is != nullptr ? as<ast::BinaryExpr>(is->left) : nullptr;
    CHECK(cmp != nullptr && cmp->op == "<");
  }
}

// `g[0][1]`: subscript chaining -- a subscript can itself take a further
// suffix.
void chained_subscript() {
  const cursiv::ParseResult r = parse(
      kept(std::string("var g = [[1, 2], [3, 4]]\nvar x = g[0][1]\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[1]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  ast::IndexExpr* const outer = decl != nullptr ? as<ast::IndexExpr>(decl->value) : nullptr;
  CHECK(outer != nullptr && outer->indices.size() == 1);
  ast::IndexExpr* const inner =
      outer != nullptr ? as<ast::IndexExpr>(outer->object) : nullptr;
  CHECK(inner != nullptr && inner->indices.size() == 1);
}

// `import "mod" for A, B, C`: a three-name list -- not capped at two.
void import_with_three_names() {
  const cursiv::ParseResult r =
      parse(kept(std::string("import \"mod\" for A, B, C\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.empty()) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[0]);
  ast::ImportExpr* const imp = es != nullptr ? as<ast::ImportExpr>(es->expr) : nullptr;
  CHECK(imp != nullptr && imp->module == "mod" && imp->variables.size() == 3);
  if (imp != nullptr && imp->variables.size() == 3) {
    CHECK(imp->variables[0]->name == "A" && imp->variables[2]->name == "C");
  }
}

// A text block, a named setter, an operator overload pair, a subscript
// getter/setter pair, a foreign getter with no parens, and a class
// attribute -- none of which the corpus exercises.
void class_members_and_text_block() {
  const std::string src =
      "#doc\n"
      "class Ops {\n"
      "  foreign static bare\n"
      "  name { \"n\" }\n"
      "  name=(v) { \"set\" }\n"
      "  - { \"neg\" }\n"
      "  -(o) { \"sub\" }\n"
      "  [i, j] { i + j }\n"
      "  [i, j]=(v) { v }\n"
      "}\n"
      "var t = \"\"\"\n"
      "hi\n"
      "\"\"\"\n";
  const cursiv::ParseResult r = parse(kept(src));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;

  ast::ClassDef* const ops = as<ast::ClassDef>(root->statements[0]);
  CHECK(ops != nullptr && ops->name == "Ops" && ops->attributes.size() == 1 &&
        ops->members.size() == 7);
  if (ops != nullptr && ops->members.size() == 7) {
    ast::GetterDef* const bare = as<ast::GetterDef>(ops->members[0]);
    CHECK(bare != nullptr && bare->isForeign && bare->isStatic &&
          bare->name == "bare" && bare->body == nullptr);
    ast::SetterDef* const namedSetter = as<ast::SetterDef>(ops->members[2]);
    CHECK(namedSetter != nullptr && namedSetter->name == "name" &&
          namedSetter->param == "v");
    ast::GetterDef* const unaryOp = as<ast::GetterDef>(ops->members[3]);
    CHECK(unaryOp != nullptr && unaryOp->name == "-");
    ast::SetterDef* const binaryOp = as<ast::SetterDef>(ops->members[4]);
    CHECK(binaryOp != nullptr && binaryOp->name == "-" && binaryOp->param == "o");
    ast::SubscriptGetDef* const subGet = as<ast::SubscriptGetDef>(ops->members[5]);
    CHECK(subGet != nullptr && subGet->params.size() == 2);
    ast::SubscriptSetDef* const subSet = as<ast::SubscriptSetDef>(ops->members[6]);
    CHECK(subSet != nullptr && subSet->params.size() == 2 &&
          subSet->valueParam == "v");
  }

  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[1]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  ast::StringExpr* const block = decl != nullptr ? as<ast::StringExpr>(decl->value) : nullptr;
  CHECK(block != nullptr && block->raw && block->parts.size() == 1);
  CHECK(block != nullptr && block->parts.size() == 1 && as<ast::StringText>(block->parts[0]) != nullptr &&
        as<ast::StringText>(block->parts[0])->value == "hi");
}

// A setter carries its `=` (`rider=(value)`, wren.io/classes.html); a name
// and one parameter alone is an ordinary method, static or not.
void one_parameter_methods() {
  const cursiv::ParseResult r = parse(kept(std::string(
      "class A {\n"
      "  static s(a) { 1 }\n"
      "  m(a) { 2 }\n"
      "  v=(a) { 3 }\n"
      "}\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root == nullptr || root->statements.size() != 1) return;
  ast::ClassDef* const cls = as<ast::ClassDef>(root->statements[0]);
  CHECK(cls != nullptr && cls->members.size() == 3);
  if (cls == nullptr || cls->members.size() != 3) return;
  ast::MethodDef* const staticMethod = as<ast::MethodDef>(cls->members[0]);
  CHECK(staticMethod != nullptr && staticMethod->name == "s" &&
        staticMethod->isStatic && staticMethod->params.size() == 1);
  ast::MethodDef* const method = as<ast::MethodDef>(cls->members[1]);
  CHECK(method != nullptr && method->name == "m" && method->params.size() == 1);
  CHECK(as<ast::SetterDef>(cls->members[2]) != nullptr);
}

// Bitwise operators and literals.
void unary_bitwise_and_numerals() {
  const cursiv::ParseResult r = parse(kept(std::string(
      "var y = ~1 & 2 | 3 ^ 4 << 1 >> 1\n"
      "var h = 0x1F\n"
      "var f = 1.5e3\n"
      "var hf = 1.8e3\n"
      "var c = \"x\"\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 5);
  if (root == nullptr || root->statements.size() != 5) return;
  ast::ExprStat* const es = as<ast::ExprStat>(root->statements[0]);
  ast::AssignStat* const decl = es != nullptr ? as<ast::AssignStat>(es->expr) : nullptr;
  ast::BinaryExpr* const top = decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
  CHECK(top != nullptr && top->op == "|");
  ast::UnaryExpr* const notFound = top != nullptr ? as<ast::UnaryExpr>(top->left) : nullptr;
  CHECK(notFound == nullptr);  // `~1 & 2` is the `&` level, tighter than `|`
}

// Assignment expressions are allowed in conditions.
void while_with_assignment_condition() {
  const cursiv::ParseResult r = parse(
      kept(std::string("var n = 0\nwhile (n = n + 1) {\n break\n}\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Script* const root = as<ast::Script>(r.value);
  CHECK(root != nullptr && root->statements.size() == 2);
  if (root == nullptr || root->statements.size() != 2) return;
  ast::WhileStat* const w = as<ast::WhileStat>(root->statements[1]);
  ast::AssignStat* const cond = w != nullptr ? as<ast::AssignStat>(w->cond) : nullptr;
  CHECK(cond != nullptr && !cond->isVar);
}

// A bare `return`: the value is optional.
void bare_return() {
  const cursiv::ParseResult r =
      parse(kept(std::string("class A {\n  m() {\n    return\n  }\n}\n")));
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
}

}  // namespace

int main() {
  if (!wren_audit::modern_cases()) return 1;
  if (!audit_regressions::run()) return 1;
  every_example_parses_clean();
  binary_tree_values();
  exclusive_range_in_for();
  interpolation_parts_are_structured();
  var_without_init();
  chained_assignment();
  ternary();
  comparison_is_equality_levels();
  chained_subscript();
  import_with_three_names();
  class_members_and_text_block();
  one_parameter_methods();
  unary_bitwise_and_numerals();
  while_with_assignment_condition();
  bare_return();
  return cursiv::test::report("wren_grammar_test");
}
