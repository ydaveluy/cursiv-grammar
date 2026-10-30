#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "trapc/core/ast.hpp"

namespace trapc_followup {
template <class Language> bool run(const Language& language) {
  struct Case { const char* name; std::string_view source; bool accepted; };
  const Case cases[] = {
      {"enum-fixed", R"TC(enum E : unsigned int { A=1 }; enum E e;)TC", true},
      {"enum-forward", R"TC(enum E : unsigned int; enum E : unsigned int { A };)TC", true},
      {"enum-missing-colon", R"TC(enum E unsigned int { A };)TC", false},
      {"enum-plain", R"TC(enum E { A, B=2 }; enum E e;)TC", true},
      {"static-assert-file", R"TC(static_assert(1);)TC", true},
      {"static-assert-block", R"TC(int f(void) { static_assert(1); return 1; })TC", true},
      {"static-assert-legacy", R"TC(int f(void) { _Static_assert(1,"yes"); return 1; })TC", true},
      {"hex-float-integer", R"TC(double x=0x1p2;)TC", true},
      {"hex-float-trailing", R"TC(double x=0X1.P+3;)TC", true},
      {"hex-float-leading", R"TC(double x=0x.8p-1;)TC", true},
      {"hex-float-suffix", R"TC(float x=0x1.2p-3f;)TC", true},
      {"hex-float-no-exponent", R"TC(double x=0x1.2;)TC", false},
      {"hex-float-empty", R"TC(double x=0xp1;)TC", false},
      {"hex-float-empty-exponent", R"TC(double x=0x1p;)TC", false},
      {"char-u8", R"TC(unsigned char c=u8'a';)TC", true},
      {"char-multichar", R"TC(int c='ab';)TC", true},
      {"string-escape-good", R"TC(char* s="a\n\x41";)TC", true},
      {"string-escape-bad", R"TC(char* s="a\q";)TC", false},
      {"char-escape-bad", R"TC(int c='\q';)TC", false},
      {"keyword-return", R"TC(int return;)TC", false},
      {"keyword-if", R"TC(int if;)TC", false},
      {"keyword-true", R"TC(int true;)TC", false},
      {"keyword-alignof", R"TC(int alignof;)TC", false},
      {"keyword-prefix", R"TC(int returning;)TC", true},
      {"ordinary-cast", R"TC(typedef int T; int f(int n) { return (T)n; })TC", true},
      {"function-pointer", R"TC(int (*f)(int);)TC", true},
      {"designators", R"TC(struct S {int n;}; struct S x={.n=3};)TC", true},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = language.language.parser->parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
    if (clean != test.accepted) {
      std::fprintf(stderr, "trapc C23 baseline: %s expected=%d accepted=%d recovered=%d diagnostics=%zu\n", test.name, test.accepted, result.accepted, result.cst().recovered(), result.diagnostics.size());
      ok = false;
    }
  }
  constexpr std::string_view source = R"TC(enum E : unsigned int { A=1 };
static_assert(1);
int f(void) { static_assert(1,"yes"); double h=0x1p2; unsigned char c=u8'a'; return A; }
)TC";
  const auto result = language.language.parser->parse(source);
  namespace ast = trapc::ast;
  unsigned assertions = 0;
  bool underlying = false, hex = false, character = false;
  for (const auto* node : result.nodes) {
    if (cursiv::ast_cast<ast::StaticAssertItem>(node)) ++assertions;
    if (const auto* e = cursiv::ast_cast<ast::EnumType>(node)) {
      const auto* type = e->underlying ? cursiv::ast_cast<ast::BuiltinType>(e->underlying->type) : nullptr;
      underlying |= type && type->keywords.size() == 2 && type->keywords[0] == "unsigned" && type->keywords[1] == "int";
    }
    if (const auto* f = cursiv::ast_cast<ast::FloatLit>(node)) hex |= f->text == "0x1p2";
    if (const auto* c = cursiv::ast_cast<ast::CharLit>(node)) character |= c->text == "u8'a'";
  }
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  if (result.value) walk(walk, *result.value);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || assertions != 2 || !underlying || !hex || !character || !owned || seen.size() != result.nodes.size()) {
    std::fprintf(stderr, "trapc: C23 source captures assertions=%u enum=%d hex=%d char=%d owned=%d\n", assertions, underlying, hex, character, owned);
    ok = false;
  }
  // The proposal's trap syntax is separate from the common-C compiler oracle.
  const auto trap = language.language.parser->parse("int f(void) { int x=1; trap { return 1; } return x; }");
  bool body = false;
  for (const auto* node : trap.nodes)
    if (const auto* t = cursiv::ast_cast<ast::TrapStmt>(node)) body |= t->body && t->body->parent == t;
  if (!trap.accepted || trap.cst().recovered() || !trap.diagnostics.empty() || !body) {
    std::fprintf(stderr, "trapc: proposal trap block AST\n");
    ok = false;
  }
  return ok;
}
} // namespace trapc_followup
