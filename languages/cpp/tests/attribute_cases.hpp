#pragma once
#include <cstdio>
#include <string_view>
#include <vector>

#include "cpp/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
namespace cpp_attribute_cases {
namespace ast = cpp::ast;

template <class T>
bool owner_is(const cursiv::AstNode* n) {
  return n && cursiv::ast_cast<T>(n);
}
struct Case {
  const char* id;
  const char* source;
  bool positive;
  bool (*owner)(const cursiv::AstNode*);
  ast::AttributeKind kind;
  const char* name;
  int argument;
};
inline const Case cases[] = {
    {"declaration", "[[v::one]] int n;", true, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"specifier", "int [[v::one]] n;", true, owner_is<ast::Specifiers>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"pointer", "int * [[v::one]] n;", true, owner_is<ast::Pointer>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"reference", "int n; int & [[v::one]] r = n;", true,
     owner_is<ast::Pointer>, ast::AttributeKind::Standard, "v::one", 0},
    {"member-pointer", "struct S {}; int S::* [[v::one]] p;", true,
     owner_is<ast::Pointer>, ast::AttributeKind::Standard, "v::one", 0},
    {"identifier", "int n [[v::one]] [2];", true, owner_is<ast::Declarator>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"array-suffix", "int n[2] [[v::one]];", true, owner_is<ast::ArraySuffix>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"function-suffix", "int f() [[v::one]];", true,
     owner_is<ast::FunctionSuffix>, ast::AttributeKind::Standard, "v::one", 0},
    {"parameter", "void f([[v::one]] int x);", true, owner_is<ast::Param>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"class", "struct [[v::one]] S {};", true, owner_is<ast::ClassType>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"elaborated", "struct S; struct [[v::one]] S *p;", true,
     owner_is<ast::ElaboratedType>, ast::AttributeKind::Standard, "v::one", 0},
    {"base", "struct B {}; struct D : [[v::one]] B {};", true,
     owner_is<ast::BaseSpecifier>, ast::AttributeKind::Standard, "v::one", 0},
    {"bitfield", "struct S { int x [[v::one]] : 3; };", true,
     owner_is<ast::MemberDeclarator>, ast::AttributeKind::Standard, "v::one",
     0},
    {"alias", "using Name [[v::one]] = int;", true, owner_is<ast::AliasDecl>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"member", "struct S { [[v::one]] int x; };", true,
     owner_is<ast::MemberDecl>, ast::AttributeKind::Standard, "v::one", 0},
    {"enum", "enum [[v::one]] E { A };", true, owner_is<ast::EnumType>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"opaque-enum", "enum [[v::one]] E : int;", true, owner_is<ast::EnumType>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"function", "[[v::one]] int f() { return 0; }", true,
     owner_is<ast::FunctionDef>, ast::AttributeKind::Standard, "v::one", 0},
    {"pure-function", "struct S { virtual void f [[v::one]] () = 0; };", true,
     owner_is<ast::Declarator>, ast::AttributeKind::Standard, "v::one", 0},
    {"lambda", "void f() { auto x = []() [[v::one]] {}; }", true,
     owner_is<ast::LambdaExpr>, ast::AttributeKind::Standard, "v::one", 0},
    {"condition", "void f() { if ([[v::one]] int x = 1) {} }", true,
     owner_is<ast::Condition>, ast::AttributeKind::Standard, "v::one", 0},
    {"for-init", "void f() { for ([[v::one]] int n = 0; n < 1; ++n) {} }", true,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 0},
    {"for-range", "void f() { int a[1]; for ([[v::one]] int n : a) {} }", true,
     owner_is<ast::ForRangeStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"compound", "void f() { [[v::one]] {} }", true,
     owner_is<ast::CompoundStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"if", "void f() { [[v::one]] if (true) return; }", true,
     owner_is<ast::IfStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"while", "void f() { [[v::one]] while (false) {} }", true,
     owner_is<ast::WhileStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"for", "void f() { [[v::one]] for (;;) {} }", true, owner_is<ast::ForStmt>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"return", "int f() { [[v::one]] return 0; }", true,
     owner_is<ast::ReturnStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"case", "void f(int x) { switch (x) { [[v::one]] case 0: break; } }", true,
     owner_is<ast::CaseLabel>, ast::AttributeKind::Standard, "v::one", 0},
    {"label", "void f() { [[v::one]] label: return; }", true,
     owner_is<ast::Label>, ast::AttributeKind::Standard, "v::one", 0},
    {"catch", "void f() { try {} catch ([[v::one]] int x) {} }", true,
     owner_is<ast::CatchClause>, ast::AttributeKind::Standard, "v::one", 0},
    {"using", "namespace N {} [[v::one]] using namespace N;", true,
     owner_is<ast::UsingDirective>, ast::AttributeKind::Standard, "v::one", 0},
    {"standalone", "[[v::one]];", true, owner_is<ast::AttributeDecl>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"alignment-value", "alignas(8) int n;", true, owner_is<ast::Declaration>,
     ast::AttributeKind::Alignment, "", 3},
    {"alignment-type", "alignas(int) int n;", true, owner_is<ast::Declaration>,
     ast::AttributeKind::Alignment, "", 4},
    {"empty-group", "[[v::one()]] int n;", true, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "v::one", 2},
    {"empty-specifier", "[[]] int n;", true, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "", 0},
    {"ordered-tokens", "[[v::one(a /* keep */ b, [1], {2}, \")]}\")]] int n;",
     true, owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one",
     5},
    {"deep-groups",
     "[[v::one((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((("
     "((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((x))))))))"
     ")))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))"
     "))))))))))))))))))))))))))))))))))))))))))))))))]] int n;",
     true, owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one",
     6},
    {"mismatch", "[[v::one([1})]] int n;", false, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"unclosed-comment", "[[v::one(/* )]] int n;", false,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 0},
    {"unclosed-string", "[[v::one(\"x)]] int n;", false,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 0},
    {"compound-punctuators",
     "[[v::one(a >> 1, b <<= 2, x ->* p, a += b)]] int n;", true,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 7},
    {"raw-string", "[[v::one(R\"tag()]} /*)tag\")]] int n;", true,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 8},
    {"alignment-empty", "alignas() int n;", false, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"alignment-junk", "alignas(a b) int n;", false, owner_is<ast::Declaration>,
     ast::AttributeKind::Standard, "v::one", 0},
};
inline bool run(const cursiv::Parser& parser) {
  bool good = true;
  for (const auto& c : cases) {
    const auto parsed = parser.parse(c.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty();
    bool ok = clean == c.positive;
    if (clean && c.positive) {
      ok &= parsed.cst().text() == c.source;
      for (const auto* n : parsed.nodes)
        if (!n || (n != parsed.value && !n->parent)) ok = false;
      unsigned seen = 0;
      const ast::AttributeGroup* arguments = nullptr;
      const ast::AttributeSpecifier* chosen = nullptr;
      for (const auto* n : parsed.nodes) {
        const auto* spec = cursiv::ast_cast<ast::AttributeSpecifier>(n);
        if (!spec || spec->kind != c.kind || !c.owner(spec->parent)) continue;
        if (*c.name) {
          for (const auto* a : spec->attributes) {
            if (a && a->name == c.name) {
              ++seen;
              chosen = spec;
              arguments = a->arguments;
              ok &= a->parent == spec;
            }
          }
        } else {
          ++seen;
          chosen = spec;
          (void)arguments;
        }
      }
      ok &= seen == 1;
      if (!*c.name && c.kind == ast::AttributeKind::Standard)
        ok &= chosen && chosen->attributes.empty();
      if (*c.name && c.argument == 0) ok &= arguments == nullptr;
      if (c.argument == 1 || c.argument == 2 || c.argument >= 5)
        ok &= arguments != nullptr;
      if (arguments) {
        if (c.argument == 2) ok &= arguments->tokens.empty();
        std::vector<const ast::AttributeToken*> work{arguments};
        std::vector<std::string_view> atoms;
        unsigned groups = 0;
        while (!work.empty()) {
          const auto* token = work.back();
          work.pop_back();
          if (const auto* group =
                  cursiv::ast_cast<ast::AttributeGroup>(token)) {
            ++groups;
            const bool paren = group->opening == "(" && group->closing == ")";
            const bool square =
                (group->opening == "[" || group->opening == "<:") &&
                (group->closing == "]" || group->closing == ":>");
            const bool brace =
                (group->opening == "{" || group->opening == "<%") &&
                (group->closing == "}" || group->closing == "%>");
            ok &= paren || square || brace;
            for (std::size_t i = group->tokens.size(); i-- > 0;) {
              ok &= group->tokens[i] && group->tokens[i]->parent == group;
              if (group->tokens[i]) work.push_back(group->tokens[i]);
            }
          } else if (const auto* atom =
                         cursiv::ast_cast<ast::AttributeAtom>(token))
            atoms.push_back(atom->text);
          else
            ok = false;
        }
        if (c.argument == 5) {
          ok &= atoms.size() == 8 && atoms[0] == "a" && atoms[1] == "b" &&
                atoms[2] == "," && atoms[3] == "1" && atoms[4] == "," &&
                atoms[5] == "2" && atoms[6] == "," && atoms[7] == "\")]}\"";
          // The quoted token retains its exact contents (without inserting
          // spaces).
        }
        if (c.argument == 6)
          ok &= groups == 129 && atoms.size() == 1 && atoms[0] == "x";
        if (c.argument == 7) {
          const std::vector<std::string_view> expected{
              "a", ">>",  "1", ",", "b", "<<=", "2", ",",
              "x", "->*", "p", ",", "a", "+=",  "b"};
          ok &= atoms == expected;
        }
        if (c.argument == 8)
          ok &= atoms.size() == 1 && atoms[0] == "R\"tag()]} /*)tag\"";
      }
      if (chosen && c.argument == 3)
        ok &= chosen->alignmentExpression && !chosen->alignmentType &&
              chosen->alignmentExpression->parent == chosen;
      if (chosen && c.argument == 4)
        ok &= chosen->alignmentType && !chosen->alignmentExpression &&
              chosen->alignmentType->parent == chosen;
      if (std::string_view(c.id) == "pure-function") {
        bool pure = false;
        for (const auto* n : parsed.nodes)
          if (const auto* m = cursiv::ast_cast<ast::MemberDeclarator>(n))
            pure |= m->isPure;
        ok &= pure;
      }
    }
    if (!ok) {
      std::fprintf(stderr,
                   "attribute owner/token case %s: expected clean=%d, "
                   "actual=%d, recovered=%d, diagnostics=%zu\n%s\n",
                   c.id, c.positive, clean, parsed.cst().recovered(),
                   parsed.diagnostics.size(), c.source);
      for (const auto& d : parsed.diagnostics)
        std::fprintf(stderr, "diag kind=%u bytes=%u..%u\n", unsigned(d.kind),
                     unsigned(d.begin), unsigned(d.end));
      good = false;
    }
  }
  return good;
}

}  // namespace cpp_attribute_cases
