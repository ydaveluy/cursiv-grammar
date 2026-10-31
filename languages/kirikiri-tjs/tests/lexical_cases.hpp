#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "kirikiritjs/core/ast.hpp"

namespace tjs_lexical_cases {
// Same18 ASCII lexer witnesses as the root-run unchanged-primary helper.
// Cursiv is checked on token extent/raw AST, not native decoded string values
// or complete script semantics. The ordinary production grammar is used.
struct Case { const char* name; bool comment; std::string_view source;
              bool accepted; unsigned consumed; };
inline constexpr Case cases[] = {
  {"plain", true, std::string_view{"/* body */X", 11}, true, 10},
  {"nested", true, std::string_view{"/* outer /* inner */ tail */X", 29}, true, 28},
  {"deep", true, std::string_view{"/* a /* b /* c */ d */ e */X", 28}, true, 27},
  {"nested-string-markers", true, std::string_view{"/* \" /* inner */ \" */X", 22}, true, 21},
  {"overlap-open-close", true, std::string_view{"/*/*/ */X", 9}, true, 8},
  {"overlap-close-open", true, std::string_view{"/* /*x*/*y*/ */X", 16}, true, 15},
  {"empty", true, std::string_view{"/**/X", 5}, true, 4},
  {"unclosed-outer", true, std::string_view{"/* outer /* inner */X", 21}, false, 0},
  {"unclosed-inner", true, std::string_view{"/* outer /* inner", 17}, false, 0},
  {"not-comment", true, std::string_view{"x", 1}, false, 0},
  {"double-lf", false, std::string_view{"\"a\nb\"", 5}, true, 5},
  {"single-cr", false, std::string_view{"'a\rb'", 5}, true, 5},
  {"double-crlf", false, std::string_view{"\"a\r\nb\"", 6}, true, 6},
  {"escaped-lf", false, std::string_view{"\"a\\\nb\"", 6}, true, 6},
  {"ordinary-escape", false, std::string_view{"\"a\\nb\"", 6}, true, 6},
  {"adjacent-lf", false, std::string_view{"\"a\"\n\"b\"", 7}, true, 7},
  {"unclosed", false, std::string_view{"\"a\nb", 4}, false, 0},
  {"embedded-nul", false, std::string_view{"\"a\000b\"", 5}, false, 0},
};
inline bool run(const cursiv::Parser& parser) {
  namespace a = kirikiritjs::ast;
  bool good = true;
  const auto check = [&](bool ok, const char* name, const char* message) {
    if (!ok) { good = false; std::fprintf(stderr, "TJS lexical [%s]: %s\n",name,message); }
  };
  for (const auto& test : cases) {
    std::string source = test.comment ? "" : "var value=";
    source.append(test.source.data(), test.source.size());
    source += ';';
    const auto parsed = parser.parse(source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty();
    // x is a valid expression statement, but is not a comment. The native
    // helper's rejection is reflected by the absence of a comment CST token.
    const bool notComment = std::string_view(test.name) == "not-comment";
    check(clean == (test.accepted || notComment), test.name, "clean syntax expectation");
    if (clean != (test.accepted || notComment))
      for (const auto& d : parsed.diagnostics)
        std::fprintf(stderr,"kind=%u element=%u bytes=%u..%u\n",unsigned(d.kind),unsigned(d.element),unsigned(d.begin),unsigned(d.end));
    if (!clean) continue;
    const auto* program = parsed.value_as<a::Program>();
    check(program && program->body.size() == 1, test.name, "one following statement retained");
    if (test.comment) {
      unsigned comments = 0;
      for (cursiv::NodeId id = 0; id < parsed.cst().size(); ++id) {
        const auto& node = parsed.cst()[id];
        if (!node.hidden || !parsed.cst().text(id).starts_with("/*")) continue;
        ++comments;
        check(node.begin == 0 && node.end == test.consumed, test.name, "exact native comment extent");
      }
      check(comments == (test.accepted ? 1u : 0u), test.name, "one whole hidden comment or none");
      const auto* statement = program && program->body.size() == 1
          ? cursiv::ast_cast<a::ExprStmt>(program->body[0]) : nullptr;
      const auto* name = statement && statement->expressions.size() == 1
          ? cursiv::ast_cast<a::Ident>(statement->expressions[0]) : nullptr;
      check(name && name->name == (notComment ? "x" : "X"), test.name,
            "text following comment remains an independent identifier");
    } else {
      const auto* declaration = program && program->body.size() == 1
          ? cursiv::ast_cast<a::VarDecl>(program->body[0]) : nullptr;
      const auto* item = declaration && declaration->declarations.size() == 1
          ? declaration->declarations[0] : nullptr;
      const auto* literal = item ? cursiv::ast_cast<a::StringLit>(item->init) : nullptr;
      const bool adjacent = std::string_view(test.name) == "adjacent-lf";
      check(literal && literal->parent == item &&
                literal->parts.size() == (adjacent ? 2u : 1u),
            test.name, "literal shape, parent and concatenation count");
      if (literal && literal->parts.size() == (adjacent ? 2u : 1u))
        check(adjacent ? (literal->parts[0] == "\"a\"" && literal->parts[1] == "\"b\"")
                       : literal->parts[0] == test.source,
              test.name, "source spelling including CR/LF remains raw");
    }
    check(parsed.cst().text() == source, test.name, "CST source unchanged");
  }
  return good;
}
} // namespace tjs_lexical_cases
