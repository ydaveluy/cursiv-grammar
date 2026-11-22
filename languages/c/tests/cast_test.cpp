#include <string>

#include "c/ast.hpp"
#include "c/generated/C/Module.hpp"
#include "cursiv/Check.hpp"

int main() {
  constexpr unsigned depth = 8;
  std::string source = "void f(void) { x = ";
  for (unsigned i = 0; i < depth; ++i) source += "(a)(";
  source += 'b';
  source.append(depth, ')');
  source += "; }";

  const auto parser = c::generated::C::createParser();
  const auto result = parser->parse(source);
  CHECK(result.accepted && result.diagnostics.empty());
  unsigned casts = 0;
  for (const auto* node : result.nodes()) {
    if (const auto* cast = cursiv::ast_cast<c::ast::CastExpr>(node)) {
      ++casts;
      CHECK(cast->parent && result.cst().contains(cast->parent->cstNodeId, cast->cstNodeId));
    }
  }
  CHECK(casts == depth);
  return cursiv::test::report("c_cast_test");
}
