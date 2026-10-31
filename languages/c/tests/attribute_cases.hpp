#pragma once
#include <cstdio>
#include <string_view>
#include <vector>

#include "c/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
namespace c_attribute_cases {
namespace ast = c::ast;

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
    {"tag", "struct [[v::one]] S { int x; };", true, owner_is<ast::TagType>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"enum", "enum [[v::one]] E { A };", true, owner_is<ast::EnumType>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"enumerator", "enum E { A [[v::one]] };", true, owner_is<ast::Enumerator>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"pointer", "int * [[v::one]] n;", true, owner_is<ast::Pointer>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"qualified-pointer", "int * [[v::one]] const n;", true,
     owner_is<ast::Pointer>, ast::AttributeKind::Standard, "v::one", 0},
    {"identifier", "int n [[v::one]] [2];", true, owner_is<ast::Declarator>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"array-suffix", "int n[2] [[v::one]];", true, owner_is<ast::ArraySuffix>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"function-suffix", "int f(void) [[v::one]];", true,
     owner_is<ast::FunctionSuffix>, ast::AttributeKind::Standard, "v::one", 0},
    {"abstract-array", "int f(int [2] [[v::one]]);", true,
     owner_is<ast::ArraySuffix>, ast::AttributeKind::Standard, "v::one", 0},
    {"abstract-pointer", "int f(int * [[v::one]]);", true,
     owner_is<ast::Pointer>, ast::AttributeKind::Standard, "v::one", 0},
    {"parameter", "int f([[v::one]] int x);", true, owner_is<ast::TypeName>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"member", "struct S { [[v::one]] int x; };", true,
     owner_is<ast::Declaration>, ast::AttributeKind::Standard, "v::one", 0},
    {"function", "[[v::one]] int f(void) { return 0; }", true,
     owner_is<ast::FunctionDef>, ast::AttributeKind::Standard, "v::one", 0},
    {"compound", "void f(void) { [[v::one]] {} }", true,
     owner_is<ast::CompoundStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"if", "void f(void) { [[v::one]] if (1) return; }", true,
     owner_is<ast::IfStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"while", "void f(void) { [[v::one]] while (0) {} }", true,
     owner_is<ast::WhileStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"do", "void f(void) { [[v::one]] do {} while (0); }", true,
     owner_is<ast::DoStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"for", "void f(void) { [[v::one]] for (;;) {} }", true,
     owner_is<ast::ForStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"case", "void f(int x) { switch (x) { [[v::one]] case 0: break; } }", true,
     owner_is<ast::CaseLabel>, ast::AttributeKind::Standard, "v::one", 0},
    {"default", "void f(int x) { switch (x) { [[v::one]] default: break; } }",
     true, owner_is<ast::DefaultLabel>, ast::AttributeKind::Standard, "v::one",
     0},
    {"label", "void f(void) { [[v::one]] label: return; }", true,
     owner_is<ast::Label>, ast::AttributeKind::Standard, "v::one", 0},
    {"return", "int f(void) { [[v::one]] return 0; }", true,
     owner_is<ast::ReturnStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"expression", "void f(void) { int n; [[v::one]] n = 1; }", true,
     owner_is<ast::ExprStmt>, ast::AttributeKind::Standard, "v::one", 0},
    {"standalone", "[[v::one]];", true, owner_is<ast::AttributeDecl>,
     ast::AttributeKind::Standard, "v::one", 0},
    {"gnu-prefix", "__attribute__((unused)) int n;", true,
     owner_is<ast::Declaration>, ast::AttributeKind::Gnu, "", 1},
    {"gnu-tail", "int n __attribute__((unused));", true,
     owner_is<ast::Declarator>, ast::AttributeKind::Gnu, "", 1},
    {"declspec", "__declspec(align(8)) int n;", true, owner_is<ast::Specifiers>,
     ast::AttributeKind::Declspec, "", 1},
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
          arguments = spec->arguments;
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

}  // namespace c_attribute_cases
