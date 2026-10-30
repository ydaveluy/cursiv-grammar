#include "native_followup.hpp"
#include "audit_regressions.hpp"
#include "modern_cases.hpp"
// The grammar itself: every document in example/ parses clean (one is
// deliberately broken and must not), and one document's fields read back
// the values the source actually wrote.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "zig/core/CoreServices.hpp"
#include "zig/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "zig: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      zig::ZigCoreServices, zig::ZigSharedCoreServices,
      zig::install_core_module>();
  if (!zig_audit::modern_cases(language.language)) return 1;
  if (!zig_native_followup::run(language.language)) return 1;

  // decls.zig, control.zig and misc.zig live beside this test, not in the
  // corpus; invalid.zig is deliberately broken and read separately below.
  int total = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().extension() != ".zig") continue;
      if (entry.path() == std::filesystem::path(CURSIV_INVALID_SAMPLE)) continue;
      ++total;
      const std::string name = entry.path().filename().string();
      const std::string text = read_file(entry.path());
      const cursiv::ParseResult result = language.language.parser->parse(text);

      if (result.cst().recovered())
        return fail(name + " does not parse clean (recovered)");
      if (!result.diagnostics.empty())
        return fail(name + " carries an unexpected diagnostic");
      if (result.value == nullptr ||
          result.value->type != &cursiv::ast_type_tag<zig::ast::ContainerMembers>)
        return fail(name + " built no ContainerMembers root");
    }
  }
  if (total < 5) return fail("fewer example documents than expected");

  // A deliberately invalid document, kept beside this test rather than in
  // example/: its diagnostic is required, not merely tolerated, so a later
  // change that stops reporting it fails here too.
  {
    const std::string text = read_file(CURSIV_INVALID_SAMPLE);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid.zig should carry a diagnostic and does not");
  }

  // Field readback: f1.zig's first declaration is `///doc comment` over
  // `const v = "";` -- a converter that silently dropped a value would still
  // pass every check above.
  {
    const std::string text = read_file(
        std::filesystem::path(CURSIV_EXAMPLE_DIR) / "f1.zig");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("f1.zig does not parse clean");
    const auto* const root = result.value_as<zig::ast::ContainerMembers>();
    if (root == nullptr || root->members.size() != 3)
      return fail("f1.zig does not hold the three declarations it wrote");
    const auto* const first =
        static_cast<const zig::ast::Decl*>(root->members[0]);
    if (first->type != &cursiv::ast_type_tag<zig::ast::Decl>)
      return fail("f1.zig's first member is not a Decl");
    if (!first->docComment || *first->docComment != "///doc comment")
      return fail("f1.zig's doc comment did not read back");
    if (first->varDecl == nullptr)
      return fail("f1.zig's first declaration built no var decl");
    const zig::ast::VarDeclProto* const proto = first->varDecl->proto;
    if (proto == nullptr || proto->name != "v" || !proto->isConst)
      return fail("f1.zig's `const v` did not read back");
    if (first->varDecl->value == nullptr ||
        first->varDecl->value->type !=
            &cursiv::ast_type_tag<zig::ast::StringLiteral>)
      return fail("f1.zig's `v`'s initializer did not read back");
  }

  // f2.zig: a statement without `comptime` is no ComptimeStmt; the inner
  // while's body is the assignment, the for is a statement of its own.
  {
    const std::string text = read_file(
        std::filesystem::path(CURSIV_EXAMPLE_DIR) / "f2.zig");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto* const root = result.value_as<zig::ast::ContainerMembers>();
    const auto* const main = root == nullptr || root->members.empty()
                                 ? nullptr
                                 : static_cast<const zig::ast::Decl*>(root->members[0]);
    if (main == nullptr || main->body == nullptr ||
        main->body->statements.size() != 6)
      return fail("f2.zig's main does not hold its six statements");
    const auto& stmts = main->body->statements;
    for (std::size_t i = 0; i < 4; ++i)
      if (stmts[i]->type != &cursiv::ast_type_tag<zig::ast::VarDeclExprStmt>)
        return fail("f2.zig's declarations are not plain statements");
    const auto* const outer = static_cast<const zig::ast::Loop*>(stmts[4]);
    if (outer->type != &cursiv::ast_type_tag<zig::ast::Loop> ||
        outer->thenBranch->type != &cursiv::ast_type_tag<zig::ast::Block>)
      return fail("f2.zig's while is not a loop over a block");
    const auto& inner =
        static_cast<const zig::ast::Block*>(outer->thenBranch)->statements;
    if (inner.size() != 2 ||
        inner[1]->type != &cursiv::ast_type_tag<zig::ast::Loop> ||
        static_cast<const zig::ast::Loop*>(inner[1])->thenBranch->type !=
            &cursiv::ast_type_tag<zig::ast::AssignExpr>)
      return fail("f2.zig's inner while does not loop over its assignment");
    if (stmts[5]->type != &cursiv::ast_type_tag<zig::ast::Loop> ||
        static_cast<const zig::ast::Loop*>(stmts[5])->forPrefix == nullptr)
      return fail("f2.zig's for is not a statement of its own");
  }

  // An if-expression's else takes the whole expression after it.
  {
    const cursiv::ParseResult result = language.language.parser->parse(
        "const y = if (a) b else c + 1;");
    const auto* const root = result.value_as<zig::ast::ContainerMembers>();
    if (result.cst().recovered() || !result.diagnostics.empty() ||
        root == nullptr || root->members.size() != 1)
      return fail("the if-expression document does not parse clean");
    const auto* const decl = static_cast<const zig::ast::Decl*>(root->members[0]);
    const auto* const value =
        decl->varDecl == nullptr ? nullptr : decl->varDecl->value;
    if (value == nullptr || value->type != &cursiv::ast_type_tag<zig::ast::IfExpr> ||
        static_cast<const zig::ast::IfExpr*>(value)->elseBranch->type !=
            &cursiv::ast_type_tag<zig::ast::BinaryExpr>)
      return fail("`else c + 1` did not stay in the if-expression");
  }

  std::puts("zig: every example parses as expected");
  return 0;
}
