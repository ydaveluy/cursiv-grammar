// `option(commit(kw<"else">, …))` on C's dangling `else`: the token in hand
// binds it to the nearest `if`, so no level of a nest forks.

#include <cstdio>
#include <string>

#include "c/core/ast.hpp"
#include "c/generated/C/Module.hpp"
#include "c/generated/C/Parser.hpp"
#include "c/generated/C/Table.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/engines/lr/runtime/LrParseStats.hpp"
#include "cursiv/engines/lr/runtime/LrParser.hpp"
#include "cursiv/test/Check.hpp"

namespace {

template <class T>
T* as(cursiv::AstNode* n, const cursiv::AstReflection& reflection) {
  return n != nullptr && reflection.isInstance(*n, &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

void inner_else(const cursiv::Parser& p) {
  const auto reflection = c::generated::C::create_ast_reflection();
  const cursiv::ParseResult r =
      p.parse("int f(int x) { if (x) if (x) x = 1; else x = 2; }");
  CHECK(r.accepted);
  CHECK(r.diagnostics.empty());
  const auto* unit = r.value_as<c::ast::TranslationUnit>();
  CHECK(unit != nullptr && unit->items.size() == 1);
  if (unit == nullptr || unit->items.size() != 1) return;
  const auto* fn = as<c::ast::FunctionDef>(unit->items[0], *reflection);
  CHECK(fn != nullptr && fn->body != nullptr && fn->body->items.size() == 1);
  if (fn == nullptr || fn->body == nullptr || fn->body->items.size() != 1)
    return;
  const auto* outer = as<c::ast::IfStmt>(fn->body->items[0], *reflection);
  CHECK(outer != nullptr);
  if (outer == nullptr) return;
  const auto* inner = as<c::ast::IfStmt>(outer->thenBranch, *reflection);
  CHECK(outer->elseBranch == nullptr);
  CHECK(inner != nullptr);
  if (inner != nullptr)
    CHECK(as<c::ast::ExprStmt>(inner->elseBranch, *reflection) != nullptr);
}

std::string nested(unsigned n) {
  std::string doc = "int f(int x) {\n";
  for (unsigned i = 0; i < n; ++i) doc += "  if (x) ";
  doc += "x = 1;";
  for (unsigned i = 0; i < n; ++i) doc += " else x = 2;";
  return doc + "\n}\n";
}

}  // namespace

int main() {
  const auto p = c::generated::C::create_parser();
  inner_else(*p);
  for (const unsigned n : {64u, 256u, 1024u}) {
    const std::string doc = nested(n);
    cursiv::Cst cst;
    cursiv::LrParseStats stats;
    CHECK(cursiv::lr_parser<c::generated::C::kTable>.parse(
        doc, cst, &stats, c::generated::C::kDrivePtr));
    std::printf("  %4u deep, %6zu bytes, %u splits\n", n, doc.size(),
                stats.splits);
    CHECK(cst.accepted() && cst.diagnostics().empty());
    CHECK(stats.splits == 0);
  }
  return cursiv::test::report("c_commit_test");
}
