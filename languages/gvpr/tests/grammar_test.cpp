// The smoke test of the language: every document under example/ parses
// clean, a few hand-written documents exercise constructs the corpus does
// not, and one parsed tree is read back field by field.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "gvpr/core/CoreServices.hpp"
#include "gvpr/core/ast.hpp"

namespace {

using Language =
    cursiv::CoreLanguage<gvpr::GvprCoreServices, gvpr::GvprSharedCoreServices>;

int fail(const std::string& what) {
  std::fprintf(stderr, "gvpr: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

bool parse_clean(const Language& language, const std::filesystem::path& path) {
  const std::string text = read_file(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "gvpr: %s did not parse clean\n", path.string().c_str());
    return false;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "gvpr: %s carries diagnostics\n", path.string().c_str());
    return false;
  }
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<gvpr::ast::Preds>) {
    std::fprintf(stderr, "gvpr: %s did not build a Preds\n", path.string().c_str());
    return false;
  }
  return true;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      gvpr::GvprCoreServices,
      gvpr::GvprSharedCoreServices,
      gvpr::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("BEGIN { int x = 1; }");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto* node : audit_result.nodes)
      if (const auto* d = cursiv::ast_cast<gvpr::ast::DclItem>(node)) found = d->name == "x" && d->initOp.has_value() && *d->initOp == gvpr::ast::AssignOp::set;
    if (!found) return 1;
  }

  const auto language = cursiv::create_language<
      gvpr::GvprCoreServices,
      gvpr::GvprSharedCoreServices,
      gvpr::install_core_module>();

  bool ok = true;
  int corpusCount = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (entry.path().extension() != ".gvpr") continue;
    ++corpusCount;
    ok = parse_clean(language, entry.path()) && ok;
  }
  if (corpusCount == 0) return fail("no corpus document found");

  static constexpr const char* kExtras[] = {
      CURSIV_TEST_DIR "/uncommon.gvpr",
  };
  for (const char* const path : kExtras) ok = parse_clean(language, path) && ok;
  if (!ok) return fail("some document did not parse clean");

  // Read real values back out of t1.gvpr: N[layer=="(*a*)"], a guarded pred
  // with no action body. The document text must outlive every read: the
  // tree's string_views are slices of it, not owned copies.
  const std::string t1Text =
      read_file(std::filesystem::path(CURSIV_EXAMPLE_DIR) / "t1.gvpr");
  const cursiv::ParseResult t1 = language.language.parser->parse(t1Text);
  if (t1.cst().recovered() || !t1.diagnostics.empty() || t1.value == nullptr ||
      t1.value->type != &cursiv::ast_type_tag<gvpr::ast::Preds>)
    return fail("t1.gvpr did not parse clean");

  const auto& preds = *static_cast<const gvpr::ast::Preds*>(t1.value);
  if (preds.items.size() != 1) return fail("t1.gvpr: expected one pred");

  const gvpr::ast::Pred* const pred = preds.items[0];
  if (pred == nullptr ||
      pred->type != &cursiv::ast_type_tag<gvpr::ast::GuardedPred>)
    return fail("t1.gvpr: expected a guarded pred");
  const auto& guarded = *static_cast<const gvpr::ast::GuardedPred*>(pred);
  if (guarded.kind != gvpr::ast::GuardKind::node_)
    return fail("t1.gvpr: expected N, not E");
  if (guarded.body != nullptr) return fail("t1.gvpr: expected no action body");
  if (guarded.guard == nullptr || guarded.guard->statements.size() != 1)
    return fail("t1.gvpr: expected one guard statement");

  const gvpr::ast::Stmt* const stmt = guarded.guard->statements[0];
  if (stmt == nullptr ||
      stmt->type != &cursiv::ast_type_tag<gvpr::ast::ExprStmt>)
    return fail("t1.gvpr: expected the guard to be one expression statement");
  const auto& exprStmt = *static_cast<const gvpr::ast::ExprStmt*>(stmt);
  if (exprStmt.expr == nullptr ||
      exprStmt.expr->type != &cursiv::ast_type_tag<gvpr::ast::Binary>)
    return fail("t1.gvpr: expected the guard to compare with ==");
  const auto& eq = *static_cast<const gvpr::ast::Binary*>(exprStmt.expr);
  if (eq.op != gvpr::ast::BinOp::eq) return fail("t1.gvpr: expected ==");
  if (eq.left == nullptr || eq.left->type != &cursiv::ast_type_tag<gvpr::ast::Var>)
    return fail("t1.gvpr: expected a variable on the left of ==");
  const auto& layer = *static_cast<const gvpr::ast::Var*>(eq.left);
  if (layer.name != "layer") return fail("t1.gvpr: expected the name 'layer'");
  if (eq.right == nullptr ||
      eq.right->type != &cursiv::ast_type_tag<gvpr::ast::StringLit>)
    return fail("t1.gvpr: expected a string literal on the right of ==");
  const auto& str = *static_cast<const gvpr::ast::StringLit*>(eq.right);
  if (str.text != "\"(*a*)\"") return fail("t1.gvpr: expected the literal text");

  // Only ARGV is a predefined array (`ArrayCall`); a declared array is read
  // with `Var::index`, whose index is a full expression, comma included
  // (Graphviz exparse.y: `ARRAY '[' args ']'`, `DYNAMIC index members`).
  {
    namespace ast = gvpr::ast;
    const std::string text =
        "BEGIN { int a[int]; int x; x = a[1]; x = ARGV[0]; x = a[1, 2]; }";
    const cursiv::ParseResult r = language.language.parser->parse(text);
    if (r.cst().recovered() || !r.diagnostics.empty() || r.value == nullptr ||
        r.value->type != &cursiv::ast_type_tag<ast::Preds>)
      return fail("arrays: did not parse clean");
    const auto& arrays = *static_cast<const ast::Preds*>(r.value);
    if (arrays.items.size() != 1 ||
        arrays.items[0]->type != &cursiv::ast_type_tag<ast::SimplePred>)
      return fail("arrays: expected one BEGIN pred");
    const auto& begin = *static_cast<const ast::SimplePred*>(arrays.items[0]);
    if (begin.body == nullptr || begin.body->statements.size() != 5)
      return fail("arrays: expected five statements");
    const auto assigned = [&](std::size_t i) -> const ast::Expr* {
      const ast::Stmt* const st = begin.body->statements[i];
      if (st == nullptr || st->type != &cursiv::ast_type_tag<ast::ExprStmt>)
        return nullptr;
      const ast::Expr* const e = static_cast<const ast::ExprStmt*>(st)->expr;
      if (e == nullptr || e->type != &cursiv::ast_type_tag<ast::Assign>)
        return nullptr;
      return static_cast<const ast::Assign*>(e)->value;
    };
    const ast::Expr* const declared = assigned(2);
    if (declared == nullptr ||
        declared->type != &cursiv::ast_type_tag<ast::Var> ||
        static_cast<const ast::Var*>(declared)->index == nullptr)
      return fail("arrays: `a[1]` is not a Var with an index");
    const ast::Expr* const argv = assigned(3);
    if (argv == nullptr ||
        argv->type != &cursiv::ast_type_tag<ast::ArrayCall> ||
        static_cast<const ast::ArrayCall*>(argv)->name != "ARGV")
      return fail("arrays: `ARGV[0]` is not an ArrayCall");
    const ast::Expr* const pair = assigned(4);
    const ast::Expr* const pairIndex =
        pair != nullptr && pair->type == &cursiv::ast_type_tag<ast::Var>
            ? static_cast<const ast::Var*>(pair)->index
            : nullptr;
    if (pairIndex == nullptr ||
        pairIndex->type != &cursiv::ast_type_tag<ast::Binary>)
      return fail("arrays: `a[1, 2]` is not one comma index");
  }

  {
    namespace ast = gvpr::ast;
    const auto result = language.language.parser->parse(R"gvpr(BEGIN {
      ; int a[]; int x=7;
      a["key"]=1;
      int n=#a;
      x %= 4; x |= 8; x &= 15; x ^= 3; x <<= 1; x >>= 1;
      int shifted=1 << 2;
      string empty=''; string word='hello'; string identity="a\q";
      if (1); else print("yes");
      switch (1) {case 1:;}
      int f(void) {return 1;}
    })gvpr");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native operators/arrays/empty statements: unclean parse");
    bool array=false, cardinality=false, empty=false, shift=false, void_formals=false;
    unsigned compounds=0;
    bool empty_string=false, single_string=false, identity_escape=false;
    for (const auto* node : result.nodes) {
      if (const auto* literal=cursiv::ast_cast<ast::StringLit>(node)) {
        empty_string |= literal->text == "''";
        single_string |= literal->text == "'hello'";
        identity_escape |= literal->text == "\"a\\q\"";
      }
      if (const auto* declaration=cursiv::ast_cast<ast::DclItem>(node))
        if (declaration->name=="a") array=declaration->isArray && !declaration->keyType;
      if (const auto* count=cursiv::ast_cast<ast::Cardinality>(node)) cardinality |= count->name=="a";
      if (cursiv::ast_cast<ast::EmptyStmt>(node)) empty=true;
      if (const auto* binary=cursiv::ast_cast<ast::Binary>(node)) shift |= binary->op==ast::BinOp::lsh;
      if (const auto* function=cursiv::ast_cast<ast::FuncDecl>(node))
        if (function->name=="f") void_formals=function->formals.empty();
      if (const auto* assignment=cursiv::ast_cast<ast::Assign>(node))
        if (assignment->op==ast::AssignOp::mod || assignment->op==ast::AssignOp::band ||
            assignment->op==ast::AssignOp::bor || assignment->op==ast::AssignOp::bxor ||
            assignment->op==ast::AssignOp::lsh || assignment->op==ast::AssignOp::rsh) ++compounds;
    }
    if (!array || !cardinality || !empty || !shift || !void_formals || compounds!=6 ||
        !empty_string || !single_string || !identity_escape)
      return fail("native operators/array distinction/void formals were lost in the AST");
  }

  std::printf("gvpr: %d corpus documents parse clean\n", corpusCount);
  return 0;
}
