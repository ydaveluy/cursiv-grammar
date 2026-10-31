#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "scala/core/CoreServices.hpp"
#include "scala/core/UnicodeSource.hpp"
#include "scala/core/ast.hpp"
namespace scala_unicode_source {
inline bool run(const scala::ScalaCoreServices& services,
                const scala::ScalaSharedCoreServices& shared) {
  namespace ast = scala::ast;
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
    {"keyword-val", "object P { v\\u0061l x=1 }", true},
    {"keyword-object", "\\u006fbject P { val x=1 }", true},
    {"escaped-delimiters", "object P \\u007b val x=1\\u003b val y=2 \\u007d", true},
    {"keyword-reserved-name", "object P { val \\u0069f=1 }", false},
    {"keyword-prefix-name", "object P { val \\u0069fx=1 }", true},
    {"repeated-u", "object P { v\\uuuu0061l x=1 }", true},
    {"unicode-operator", "object P { val x=1 \\u002b 2 }", true},
    {"unicode-line-comment-close", "object P { // skipped\\u000aval x=1 }", true},
    {"unicode-line-comment-open", "object P { \\u002f\\u002f comment\\u000aval x=1 }", true},
    {"unicode-block-comment", "object P { \\u002f\\u002a skipped \\u002a\\u002f val x=1 }", true},
    {"string-unicode-000a", "object P { val x=\"a\\u000ab\" }", true},
    {"char-unicode-000a", "object P { val x='\\u000a' }", true},
    {"interpolated-unicode-000a", "object P { val x=s\"a\\u000ab\" }", true},
    {"backquoted-unicode-000a", "object P { val `a\\u000ab`=1 }", true},
    {"string-unicode-000d", "object P { val x=\"a\\u000db\" }", true},
    {"char-unicode-000d", "object P { val x='\\u000d' }", true},
    {"interpolated-unicode-000d", "object P { val x=s\"a\\u000db\" }", true},
    {"backquoted-unicode-000d", "object P { val `a\\u000db`=1 }", true},
    {"physical-string-lf", "object P { val x=\"a\nb\" }", false},
    {"physical-char-lf", "object P { val x='\n' }", false},
    {"physical-interpolated-lf", "object P { val x=s\"a\nb\" }", false},
    {"physical-backquoted-lf", "object P { val `a\nb`=1 }", false},
    {"physical-string-cr", "object P { val x=\"a\rb\" }", false},
    {"physical-char-cr", "object P { val x='\r' }", false},
    {"physical-interpolated-cr", "object P { val x=s\"a\rb\" }", false},
    {"physical-backquoted-cr", "object P { val `a\rb`=1 }", false},
    {"triple-physical-lf", "object P { val x=\"\"\"a\nb\"\"\" }", true},
    {"triple-unicode-lf", "object P { val x=\"\"\"a\\u000ab\"\"\" }", true},
    {"interpolation-hole-physical-lf", "object P { val x=s\"${\n 1\n}\" }", true},
    {"string-escaped-quote", "object P { val x=\"a\\u0022b\" }", false},
    {"string-escaped-backslash-n", "object P { val x=\"\\u005cn\" }", true},
    {"string-even-slashes", "object P { val x=\"\\\\u0041\" }", true},
    {"string-odd-slashes", "object P { val x=\"\\\\\\u0041\" }", true},
    {"string-nonrecursive-backslash", "object P { val x=\"\\u005cu0041\" }", false},
    {"malformed-short", "object P { val x=\\u123 }", false},
    {"malformed-hex", "object P { val x=\"\\u00xz\" }", false},
    {"malformed-comment", "object P { //\\u00xz\n}", false},
    {"upper-U-not-unicode", "object P { val x=\"\\U00000041\" }", false},
    {"utf16-high-char", "object P { val x='\\uD800' }", true},
    {"utf16-low-char", "object P { val x='\\uDC00' }", true},
    {"utf16-pair-char", "object P { val x='\\uD83D\\uDE00' }", false},
    {"utf16-pair-string", "object P { val x=\"\\uD83D\\uDE00\" }", true},
    {"utf16-high-string", "object P { val x=\"\\uD800\" }", true},
    {"unicode-crlf-char", "object P { val x='\\u000d\\u000a' }", false},
    {"unicode-crlf-string", "object P { val x=\"\\u000d\\u000a\" }", true},
    {"unicode-crlf-triple", "object P { val x=\"\"\"\\u000d\\u000a\"\"\" }", true},
    {"unquoted-high", "object P { val \\uD800=1 }", false},
    {"unquoted-low", "object P { val \\uDC00=1 }", false},
    {"unquoted-repeated-u-high", "object P { val \\uuD800=1 }", false},
    {"unquoted-pair-letter", "object P { val \\uD801\\uDC00=1 }", false},
    {"quoted-high", "object P { val `a\\uD800b`=1 }", true},
    {"quoted-low", "object P { val `a\\uDC00b`=1 }", true},
    {"quoted-pair", "object P { val `a\\uD801\\uDC00b`=1 }", true},
    {"quoted-equivalent-spellings", "object P { val `a\\uD800b`=1; val x=`a\\uuD800b` }", true},
    {"member-high", "object P { val x=obj.\\uD800 }", false},
    {"quoted-lf-class", "class `A\\u000aB` {}", true},
    {"quoted-lf-type", "object P { type `A\\u000aB`=Int }", true},
    {"quoted-lf-member", "object P { val x=obj.`a\\u000ab` }", true},
    {"quoted-lf-infix", "object P { val x=a `b\\u000ac` d }", true},
    {"generated-surrogate-string", "object P { val x=\"\\u005cuD800\" }", false},
    {"generated-surrogate-char", "object P { val x='\\u005cuD800' }", false},
    {"generated-surrogate-quoted-name", "object P { val `a\\u005cuD800b`=1 }", false},
    {"generated-surrogate-triple", "object P { val x=\"\"\"\\u005cuD800\"\"\" }", true},
    {"generated-surrogate-interpolated", "object P { val x=s\"\\u005cuD800\" }", true},
    {"generated-surrogate-interpolated-triple", "object P { val x=s\"\"\"\\u005cuD800\"\"\" }", true},
    {"generated-slash-infix-name", "object P { val x=a \\u005cuD800 }", true},
    {"generated-slash-then-real-unit", "object P { val x=\"\\u005c\\uD800\" }", false},
    {"escaped-slash-then-real-unit", "object P { val x=\"\\\\\\uD800\" }", true},
    {"two-generated-slashes-then-letters", "object P { val x=\"\\u005c\\u005cuD800\" }", true},
    {"generated-plus-raw-slashes", "object P { val x=\"\\u005c\\\\uD800\" }", false},
    {"quoted-generated-then-real", "object P { val `a\\u005c\\uD800b`=1 }", false},
    {"interpolated-generated-then-real", "object P { val x=s\"\\u005c\\uD800\" }", true},
    {"quote-char-deprecated", "object P { val x=''' }", true},
    {"unicode-quote-char", "object P { val x='\\u0027' }", true},
    {"raw-char-octal-three-high", "object P { val x='\\777' }", false},
    {"generated-char-octal-three-high", "object P { val x='\\u005c777' }", false},
    {"raw-char-octal-three-low", "object P { val x='\\377' }", true},
    {"raw-char-octal-two", "object P { val x='\\77' }", true},
    {"raw-string-octal-three-high", "object P { val x=\"\\777\" }", true},
    {"generated-slash-quote-char", "object P { val x='\\u005c\\u0027' }", true},
    {"unicode-nul-char", "object P { val x='\\u0000' }", true},
    {"unicode-nul-string", "object P { val x=\"\\u0000\" }", true},
  };
  for (const auto& test : cases) {
    auto document = shared.documentFactory->fromString(test.source, "file:///unicode-source.scala");
    const auto& r = document->parseResult;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && diagnostics.empty();
    const auto fail = [&](const char* reason) {
      std::fprintf(stderr, "scala Unicode source: %s %s expected=%d clean=%d\n%s\n",
                   test.id, reason, test.valid, clean, test.source);
      for (const auto& d : r.diagnostics)
        std::fprintf(stderr, "  parse kind=%d bytes=%zu..%zu\n", int(d.kind),
                     std::size_t(d.begin), std::size_t(d.end));
      for (const auto& d : diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return false;
    };
    if (clean != test.valid) return fail("native classification");
    const std::string_view id(test.id);
    if (!test.valid) {
      if (id.starts_with("physical-")) {
        bool precise = false;
        const auto code = id.starts_with("physical-backquoted")
            ? "scala.physical-name-newline" : "scala.physical-literal-newline";
        for (const auto& d : diagnostics)
          precise |= d.code && d.code->isString() && d.code->string() == code;
        if (!r.accepted || r.cst().recovered() || !precise) return fail("physical spelling diagnostic");
      }
      continue;
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = cursiv::ast_cast<ast::CompilationUnit>(r.value) != nullptr;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    if (!parents || seen.size() != r.nodes.size()) return fail("AST ownership/CST");
    if (id == "keyword-val" || id == "unicode-line-comment-close" ||
        id == "unicode-line-comment-open" || id == "unicode-block-comment") {
      bool named = false;
      for (const auto* node : r.nodes)
        if (const auto* p = cursiv::ast_cast<ast::VarPattern>(node)) named |= p->name == "x";
      if (!named) return fail("decoded declaration survives");
    }
    if (id == "keyword-val") {
      bool raw = false;
      for (const auto* node : r.nodes)
        if (cursiv::ast_cast<ast::PatDef>(node))
          raw |= r.cst().text(node->cstNodeId).find("v\\u0061l") != std::string_view::npos;
      if (!raw) return fail("original source span");
    }
    if (id == "keyword-prefix-name") {
      bool canonical = false;
      for (const auto* node : r.nodes)
        if (const auto* p = cursiv::ast_cast<ast::VarPattern>(node)) canonical |= p->name == "ifx";
      if (!canonical) return fail("translated name");
    }
    if (id == "generated-slash-infix-name") {
      bool shape = false;
      for (const auto* node : r.nodes)
        if (const auto* p = cursiv::ast_cast<ast::InfixExpr>(node)) {
          const auto* rhs = cursiv::ast_cast<ast::PathExpr>(p->right);
          shape |= p->op == "\\" && rhs && rhs->path && rhs->path->root == "uD800";
        }
      if (!shape) return fail("generated slash is an operator, not Unicode");
    }
    if (id == "utf16-high-char") {
      bool raw = false;
      for (const auto* node : r.nodes)
        if (const auto* p = cursiv::ast_cast<ast::CharLit>(node)) raw |= p->text == "'\\uD800'";
      if (!raw) return fail("single UTF16 Char spelling");
    }
    if (id == "unicode-nul-char" || id == "unicode-nul-string") {
      bool nul = false;
      for (const auto* node : r.nodes) {
        if (const auto* p = cursiv::ast_cast<ast::CharLit>(node))
          nul |= p->text.size() == 3 && p->text[1] == '\0';
        if (const auto* p = cursiv::ast_cast<ast::StringLit>(node))
          nul |= p->text.size() == 3 && p->text[1] == '\0';
      }
      if (!nul) return fail("embedded NUL width/value");
    }
    if (id == "unicode-crlf-string" || id == "unicode-crlf-triple") {
      bool pair = false;
      for (const auto* node : r.nodes)
        if (const auto* p = cursiv::ast_cast<ast::StringLit>(node))
          pair |= p->text.find("\r\n") != std::string_view::npos;
      if (!pair) return fail("CRLF pair not coalesced");
    }
  }
  struct Translation { const char* source; const char* text; bool error; };
  for (const auto& test : {
      Translation{"v\\u0061l", "val", false},
      Translation{"\\uuuu0061", "a", false},
      Translation{"\\\\u0061", "\\\\u0061", false},
      Translation{"\\u005cu0041", "\\u0041", false},
      Translation{"\\uD800", "\\uD800", false},
      Translation{"\\u000d\\u000a", "\r\n", false},
      Translation{"\\u00xz", "\\u00xz", true}}) {
    cursiv::SourceTranslation translation(test.source);
    scala::translate_unicode_source(test.source, translation);
    translation.finish();
    if (translation.text() != test.text || (!translation.errors().empty()) != test.error) {
      std::fprintf(stderr, "scala Unicode source: direct translation invariant: %s\n", test.source);
      return false;
    }
  }
  {
    cursiv::SourceTranslation nul("\\u0000");
    scala::translate_unicode_source("\\u0000", nul);
    nul.finish();
    if (nul.text().size() != 1 || nul.text()[0] != '\0' || !nul.errors().empty()) {
      std::fprintf(stderr, "scala Unicode source: NUL is one byte, not an empty C string\n");
      return false;
    }
  }
  return true;
}
}  // namespace scala_unicode_source
