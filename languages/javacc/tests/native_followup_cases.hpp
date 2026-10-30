#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <variant>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "javacc/core/ast.hpp"
#include "unicode_input_cases.hpp"
namespace javacc_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = javacc::ast;
  if (!unicode_input(language)) {
    std::fprintf(stderr, "javacc native: Unicode input, original CST or diagnostics mismatch\n");
    return false;
  }
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "javacc native: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"jj(options { TOKEN_EXTENDS = "base\n\12"; LOOKAHEAD = 1; }
PARSER_BEGIN(P)
class P {
  int x = 1_000;
  int [/* [ */] dimension;
  String s = "\12";
  void f() { object.<String>method(); if (a) if (b) x(); else y(); }
}
PARSER_END(P)
JAVACODE public void code() {}
private void S() : {} { <EOF> }
TOKEN : { <X: ["\n"-"\r", "é", "\377"]> }
)jj";
  const auto result = language.language.parser->parse(source);
  const auto* root = result.template value_as<a::JavaccInput>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && root,
        "native modifiers, literals, generic selectors and dimensions parse clean");
  if (result.cst().recovered() || !result.diagnostics.empty())
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "javacc kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  bool option = false, java = false, bnf = false, number = false, generic = false;
  bool dims = false, innerElse = false, outerElse = false, range = false, accent = false, octal = false;
  for (const auto* node : result.nodes) {
    if (const auto* p = cursiv::ast_cast<a::OptionBinding>(node))
      if (const auto* text = std::get_if<std::string_view>(&p->value)) option |= p->name == "TOKEN_EXTENDS" && *text == "base\n\n";
    if (const auto* p = cursiv::ast_cast<a::JavaCodeProduction>(node)) java |= p->name == "code" && p->accessModifier == "public";
    if (const auto* p = cursiv::ast_cast<a::BnfProduction>(node)) bnf |= p->name == "S" && p->accessModifier == "private";
    if (const auto* p = cursiv::ast_cast<a::IntLit>(node)) number |= p->text == "1_000";
    if (const auto* p = cursiv::ast_cast<a::GenericMemberExpr>(node)) generic |= p->field == "method" && p->typeArgs.size() == 1 && p->typeArgs[0]->parent == p;
    if (const auto* p = cursiv::ast_cast<a::ArrayType>(node)) dims |= p->dims == 1;
    if (const auto* p = cursiv::ast_cast<a::IfStmt>(node)) {
      innerElse |= p->elseBranch && cursiv::ast_cast<a::IfStmt>(p->parent);
      outerElse |= !p->elseBranch && cursiv::ast_cast<a::IfStmt>(p->thenBranch);
    }
    if (const auto* p = cursiv::ast_cast<a::CharDescriptor>(node)) {
      range |= p->range && p->from == "\n" && p->to == "\r";
      accent |= !p->range && p->from == "é";
      octal |= !p->range && p->from == "ÿ";
    }
  }
  check(option && java && bnf && number && generic && dims && innerElse && outerElse && range && accent && octal,
        "native values, modifier/type-argument captures and nearest-if ownership retained");
  if (root && !result.cst().recovered()) {
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
    walk(walk, *root);
    check(owned && seen.size() == result.nodes.size(), "complete AST ownership");
  }
  const auto jjt = language.language.parser->parse(
      R"jjt(PARSER_BEGIN(P) class P {} PARSER_END(P) void S() #Node(")".equals("(")) : {} { <EOF> })jjt");
  bool argument = false;
  for (const auto* node : jjt.nodes)
    if (const auto* p = cursiv::ast_cast<a::NodeDescriptor>(node)) argument |= p->args == R"jjt(")".equals("("))jjt";
  check(jjt.accepted && !jjt.cst().recovered() && jjt.diagnostics.empty() && argument,
        "JJTree condition strings do not change parenthesis balancing");
  std::string many = "PARSER_BEGIN(P) class P { int ";
  for (int i = 0; i < 300; ++i) many += "[]";
  many += " dimension; } PARSER_END(P) void S():{} { <EOF> }";
  const auto large = language.language.parser->parse(many);
  bool exact = false;
  for (const auto* node : large.nodes) if (const auto* p = cursiv::ast_cast<a::ArrayType>(node)) exact |= p->dims == 300;
  check(large.accepted && !large.cst().recovered() && large.diagnostics.empty() && exact,
        "dimension count does not wrap modulo256");
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{R"jj(PARSER_BEGIN(P) class P {} PARSER_END(P) TOKEN:{<X:["ab"]>} void S():{}{<EOF>})jj", "javacc.character"},
      Invalid{R"jj(PARSER_BEGIN(P) class P {} PARSER_END(P) TOKEN:{<X:["a"-""]>} void S():{}{<EOF>})jj", "javacc.character"},
      Invalid{R"jj(PARSER_BEGIN(P) class P {} PARSER_END(P) TOKEN:{<X:["😀"]>} void S():{}{<EOF>})jj", "javacc.character"},
      Invalid{R"jj(PARSER_BEGIN(P) class P {} PARSER_END(P) TOKEN:{<X:["z"-"a"]>} void S():{}{<EOF>})jj", "javacc.character-order"},
      Invalid{"options{LOOKAHEAD=2147483648;} PARSER_BEGIN(P)class P{} PARSER_END(P)void S():{}{<EOF>}", "javacc.integer-range"},
      Invalid{"PARSER_BEGIN(P)class P{}PARSER_END(P)void S():{}{LOOKAHEAD(2147483648)<EOF>}", "javacc.integer-range"},
      Invalid{"PARSER_BEGIN(P)class P{}PARSER_END(P)void S():{}{try{<EOF>}}", "javacc.try-handler"}}) {
    const auto validation = cursiv::test::validationHelper(language.language, c.source);
    check(validation.document->parseResult.accepted && !validation.document->parseResult.cst().recovered() &&
          validation.diagnostics.size() == 1 && validation.diagnostics[0].code == c.code, c.code);
  }
  for (const auto bad : {"anything{LOOKAHEAD=1;} PARSER_BEGIN(P)class P{}PARSER_END(P)void S():{}{<EOF>}",
                          "PARSER_BEGIN(P)class P{int 😀;}PARSER_END(P)void S():{}{<EOF>}",
                          "PARSER_BEGIN(P)class P{int null;}PARSER_END(P)void S():{}{<EOF>}",
                          R"jj(PARSER_BEGIN(P)class P{String s="\u1";}PARSER_END(P)void S():{}{<EOF>})jj",
                          "options{LOOKAHEAD=9223372036854775808;}PARSER_BEGIN(P)class P{}PARSER_END(P)void S():{}{<EOF>}"}) {
    const auto r = language.language.parser->parse(bad);
    check(!r.accepted || r.cst().recovered() || !r.diagnostics.empty(), "invalid introducer/name/escape/overflow cannot parse clean");
  }
  return ok;
}
} // namespace javacc_native_followup
