#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "kirikiritjs/core/CoreServices.hpp"
#include "kirikiritjs/core/ast.hpp"

namespace tjs_octet_regexp_cases {
struct Case { const char* name; std::string_view source; std::string_view literal; bool valid; bool regexp; };
inline constexpr Case cases[] = {
  {"octet-raw-empty", "var value = <%%>;", "<%%>", true, false},
  {"octet-raw-basic", "var value = <%41 42%>;", "<%41 42%>", true, false},
  {"octet-raw-odd-nibble", "var value = <%f%>;", "<%f%>", true, false},
  {"octet-raw-comma", "var value = <%a,b%>;", "<%a,b%>", true, false},
  {"octet-raw-block-close", "var value = <%41 /* %> */ 42%>;", "<%41 /* %> */ 42%>", true, false},
  {"octet-raw-nested-block-close", "var value = <%41 /* outer /* %> */ end */ 42%>;", "<%41 /* outer /* %> */ end */ 42%>", true, false},
  {"octet-raw-line-LF", "var value = <%41 // %>\n42%>;", "<%41 // %>\n42%>", true, false},
  {"octet-raw-line-CRLF", "var value = <%41 // %>\r\n42%>;", "<%41 // %>\r\n42%>", true, false},
  {"octet-raw-line-CR-is-content", "var value = <%41 // %>\r42\n43%>;", "<%41 // %>\r42\n43%>", true, false},
  {"octet-raw-split-nibble", "var value = <%a/* %> */b%>;", "<%a/* %> */b%>", true, false},
  {"octet-raw-comment-before-comma", "var value = <%a/* %> */,b%>;", "<%a/* %> */,b%>", true, false},
  {"octet-raw-ignored-nonhex", "var value = <%Z!@%>;", "<%Z!@%>", true, false},
  {"octet-raw-missing-close", "var value = <%41;", "<%41", false, false},
  {"octet-raw-unclosed-comment", "var value = <%41 /* %>;", "<%41 /* %>", false, false},
  {"octet-raw-unterminated-line", "var value = <%41 // %>;", "<%41 // %>", false, false},
  {"octet-raw-comment-at-EOF", "var value = <%41 /**/;", "<%41 /**/", false, false},
  {"octet-normalized-CR-line-boundary", "var value = <%41 // %>\r42\n43%>;", "<%41 // %>\r42\n43%>", true, false},
  {"octet-normalized-CRLF-line-boundary", "var value = <%41 // %>\r\n42%>;", "<%41 // %>\r\n42%>", true, false},
  {"octet-normalized-CR-block-comment", "var value = <%41 /* %>\r ignored */42%>;", "<%41 /* %>\r ignored */42%>", true, false},
  {"regexp-plain", "var value = /ab/g;", "/ab/g", true, true},
  {"regexp-escaped-slash", "var value = /a\\/b/ig;", "/a\\/b/ig", true, true},
  {"regexp-even-backslashes", "var value = /a\\\\/g;", "/a\\\\/g", true, true},
  {"regexp-raw-LF", "var value = /a\nb/m;", "/a\nb/m", true, true},
  {"regexp-raw-CRLF", "var value = /a\r\nb/g;", "/a\r\nb/g", true, true},
  {"regexp-leading-LF", "var value = /\n/g;", "/\n/g", true, true},
  {"regexp-ordinary-brackets", "var value = /[ab]/g;", "/[ab]/g", true, true},
  {"regexp-slash-inside-brackets", "var value = /[a/b]/;", "/[a/b]/", false, true},
  {"regexp-uppercase-flag", "var value = /a/G;", "/a/G", false, true},
  {"regexp-mixedcase-flag", "var value = /a/gI;", "/a/gI", false, true},
  {"regexp-missing-close", "var value = /abc;", "/abc", false, true},
  {"regexp-escaped-final-slash", "var value = /ab\\/;", "/ab\\/", false, true},
  {"octet-CR-closes-before-EOF", "var value = <%41 // ignored\r42%>;", "<%41 // ignored\r42%>", true, false},
};
inline bool run(const kirikiritjs::KirikiriTjsCoreServices& services) {
  namespace a = kirikiritjs::ast;
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* what) {
    if (!ok) { std::fprintf(stderr, "tjs octet/regexp %s: %s\n", name, what); ++failures; }
  };
  for (const auto& test : cases) {
    if (std::string_view(test.name).find("CR") != std::string_view::npos)
      check(test.source.find('\r') != std::string_view::npos &&
            test.literal.find('\r') != std::string_view::npos,
            test.name, "runtime input really retains physical CR, not compiler-normalized LF");
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.valid, test.name, "complete source verdict, not helper prefix acceptance");
    if (!test.valid || !clean) continue;
    check(parsed.cst().text() == test.source, test.name, "all physical source bytes retained");
    unsigned octets = 0, regexps = 0;
    const a::Expr* literal = nullptr;
    std::string_view text;
    for (const auto* node : parsed.nodes) {
      if (const auto* value = cursiv::ast_cast<a::OctetLit>(node)) { ++octets; literal = value; text = value->text; }
      if (const auto* value = cursiv::ast_cast<a::RegexLit>(node)) { ++regexps; literal = value; text = value->text; }
    }
    const auto* owner = literal ? cursiv::ast_cast<a::Declarator>(literal->parent) : nullptr;
    check(octets == (test.regexp ? 0u : 1u) && regexps == (test.regexp ? 1u : 0u) &&
          text == test.literal && literal && parsed.cst().text(literal->cstNodeId) == test.literal,
          test.name, "one correctly typed literal with exact delimiters/body/flags");
    check(owner && owner->id && owner->id->name == "value" && owner->init == literal,
          test.name, "literal belongs to the variable initializer");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique AST owner"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "immediate parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "complete tree reachability");
  }
  // NUL ends the native lexer input. It cannot hide an unfinished octet
  // behind a line comment. This is a primary-source control, not a fresh
  // native helper execution on embedded-NUL input.
  constexpr char nul[] = "var value = <%//\0\n%>;";
  check(std::string_view(nul, sizeof(nul) - 1).find('\0') != std::string_view::npos,
        "octet-NUL-comment", "runtime input really contains NUL");
  const auto broken = services.parser->parse(std::string_view(nul, sizeof(nul) - 1));
  check(!broken.accepted || broken.cst().recovered() || !broken.diagnostics.empty(),
        "octet-NUL-comment", "line comment does not carry source past native terminator");
  constexpr char blockNul[] = "var value = <%/*\0*/%>;";
  const std::string_view blockInput(blockNul, sizeof(blockNul) - 1);
  check(blockInput.find('\0') != std::string_view::npos,
        "octet-NUL-block-comment", "runtime input really contains NUL inside the block comment");
  const auto brokenBlock = services.parser->parse(blockInput);
  check(!brokenBlock.accepted || brokenBlock.cst().recovered() || !brokenBlock.diagnostics.empty(),
        "octet-NUL-block-comment", "block comment does not carry source past native terminator");
  return !failures;
}
} // namespace tjs_octet_regexp_cases
