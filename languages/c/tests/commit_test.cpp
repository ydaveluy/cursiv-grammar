#include <string>

#include "c/ast.hpp"
#include "c/generated/C/Module.hpp"
#include "cursiv/Check.hpp"

int main() {
  const auto parser = c::generated::C::createParser();
  for (const unsigned depth : {2u, 64u, 256u, 1024u}) {
    std::string source = "int f(int x) { ";
    for (unsigned i = 0; i < depth; ++i) source += "if (x) ";
    source += "x = 1;";
    for (unsigned i = 1; i < depth; ++i) source += " else x = 2;";
    source += " }";

    const auto result = parser->parse(source);
    CHECK(result.accepted && result.diagnostics.empty());
    const auto* unit = result.valueAs<c::ast::TranslationUnit>();
    const auto* fn = unit && unit->items.size() == 1
                         ? cursiv::ast_cast<c::ast::FunctionDef>(unit->items[0])
                         : nullptr;
    const auto* stmt = fn && fn->body && fn->body->items.size() == 1
                           ? cursiv::ast_cast<c::ast::IfStmt>(fn->body->items[0])
                           : nullptr;
    for (unsigned i = 0; i < depth; ++i) {
      CHECK(stmt != nullptr);
      if (!stmt) break;
      CHECK(i == 0 ? stmt->elseBranch == nullptr
                   : cursiv::ast_cast<c::ast::ExprStmt>(stmt->elseBranch) != nullptr);
      if (i + 1 == depth) CHECK(cursiv::ast_cast<c::ast::ExprStmt>(stmt->thenBranch));
      stmt = cursiv::ast_cast<c::ast::IfStmt>(stmt->thenBranch);
    }
  }
  return cursiv::test::report("c_commit_test");
}
