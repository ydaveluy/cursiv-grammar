#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "v/core/ast.hpp"

namespace v_interpolation {
namespace ast = v::ast;

// Exact V 0.4.12 parser-only sources: native results02 (classification03),
// boundary-results04 and expression-results05, frozen 2026-11-01.
// `${}` normalizes the native missing trailing empty segment to one tail.
struct Case {
  const char* name;
  std::string_view source;
  bool accepted;
  bool interpolated;
  std::vector<std::string_view> texts;
  std::vector<const cursiv::AstTypeTag*> expressionTypes;
};
inline const Case cases[] = {
    {"short-name", R"VCASE(fn main(){s:='hi $name!'}
)VCASE", true, true, {R"VCASE(hi )VCASE", R"VCASE(!)VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"braced-binary", R"VCASE(fn main(){s:="${ x + 1 }"}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::BinaryExpr>}},
    {"adjacent", R"VCASE(fn main(){s:='$a$b'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>, &cursiv::ast_type_tag<ast::Ident>}},
    {"short-selector", R"VCASE(fn main(){s:='$u.name'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::SelectorExpr>}},
    {"short-call", R"VCASE(fn main(){s:='$f()'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::CallExpr>}},
    {"braced-index", R"VCASE(fn main(){s:='${a[0] + 1}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::BinaryExpr>}},
    {"inner-double", R"VCASE(fn main(){s:='${f("x")}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::CallExpr>}},
    {"inner-single", R"VCASE(fn main(){s:='${f('x')}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::CallExpr>}},
    {"nested-interpolation", R"VCASE(fn main(){s:='${f('$x')}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::CallExpr>}},
    {"format-zero", R"VCASE(fn main(){s:='${n:04d}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"format-plus", R"VCASE(fn main(){s:="${n:+10.2f}"}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"format-negative", R"VCASE(fn main(){s:='${n:-8x}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"raw-single", R"VCASE(fn main(){s:=r'${x} $x'}
)VCASE", true, false, {}, {}},
    {"escaped-dollar", R"VCASE(fn main(){s:='\$x'}
)VCASE", true, false, {}, {}},
    {"literal-dollar", R"VCASE(fn main(){s:='$9 $ !'}
)VCASE", true, false, {}, {}},
    {"raw-double", R"VCASE(fn main(){s:=r"${x}"}
)VCASE", true, false, {}, {}},
    {"short-index-tail", R"VCASE(fn main(){s:='$a[0]'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE([0])VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"even-backslashes", R"VCASE(fn main(){s:='\\$x'}
)VCASE", true, true, {R"VCASE(\\)VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"short-keyword-at-close", R"VCASE(fn main(){s:='$true'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"short-keyword-before-text", R"VCASE(fn main(){s:='$true!'}
)VCASE", false, false, {}, {}},
    {"empty-braced", R"VCASE(fn main(){s:='${}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::StringLit>}},
    {"empty-format", R"VCASE(fn main(){s:='${x:}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"invalid-format-name", R"VCASE(fn main(){s:='${x:long}'}
)VCASE", false, false, {}, {}},
    {"invalid-escape", R"VCASE(fn main(){s:='\q $x'}
)VCASE", false, false, {}, {}},
    {"unfinished-brace", R"VCASE(fn main(){s:='${x'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"missing-number", R"VCASE(fn main(){s:='${1'}
)VCASE", false, false, {}, {}},
    {"missing-parenthesized-name", R"VCASE(fn main(){s:='${(x)'}
)VCASE", false, false, {}, {}},
    {"missing-binary-name", R"VCASE(fn main(){s:='${a + b'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::BinaryExpr>}},
    {"missing-selector-name", R"VCASE(fn main(){s:='${u.name'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::SelectorExpr>}},
    {"missing-call", R"VCASE(fn main(){s:='${f()'}
)VCASE", false, false, {}, {}},
    {"missing-format-code", R"VCASE(fn main(){s:='${x:d'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"missing-format-number", R"VCASE(fn main(){s:='${x:4'}
)VCASE", false, false, {}, {}},
    {"missing-after-nested-string", R"VCASE(fn main(){s:='${f("x") + y'}
)VCASE", false, false, {}, {}},
    {"missing-with-gap", R"VCASE(fn main(){s:='${x '}
)VCASE", false, false, {}, {}},
    {"missing-after-rune", R"VCASE(fn main(){s:='${`a` + x'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::BinaryExpr>}},
    {"short-call-argument", R"VCASE(fn main(){s:='$f(1)'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::CallExpr>}},
    {"short-call-gap", R"VCASE(fn main(){s:='$f (1)'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE( (1))VCASE"}, {&cursiv::ast_type_tag<ast::Ident>}},
    {"short-selector-gap", R"VCASE(fn main(){s:='$u. field'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::SelectorExpr>}},
    {"short-trailing-dot", R"VCASE(fn main(){s:='$u.'}
)VCASE", false, false, {}, {}},
    {"short-dot-number", R"VCASE(fn main(){s:='$u.0'}
)VCASE", false, false, {}, {}},
    {"short-dotdot", R"VCASE(fn main(){s:='$a..b'}
)VCASE", false, false, {}, {}},
    {"short-reserved-selector-close", R"VCASE(fn main(){s:='$u.type'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::SelectorExpr>}},
    {"short-reserved-selector-tail", R"VCASE(fn main(){s:='$u.type!'}
)VCASE", false, false, {}, {}},
    {"braced-if-expression", R"VCASE(fn main(){s:='${if true {1} else {2}}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::IfExpr>}},
    {"braced-unsafe-expression", R"VCASE(fn main(){s:='${unsafe {1}}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::UnsafeExpr>}},
    {"braced-match-expression", R"VCASE(fn main(){s:='${match 1 {1 {2} else {3}}}'}
)VCASE", true, true, {R"VCASE()VCASE", R"VCASE()VCASE"}, {&cursiv::ast_type_tag<ast::MatchExpr>}},
};

template <class Language>
bool tree(const Language& language, const cursiv::ParseResult& result,
          const cursiv::AstNode* parent = nullptr, const char* label = "base46") {
  const auto dump = [&](const char* role, const cursiv::AstNode* node) {
    if (!node) { std::fprintf(stderr, " %s=null", role); return; }
    const auto name = node->type ? node->type->name : std::string_view("?");
    const auto span = node->cstNodeId != cursiv::kNoNode && node->cstNodeId < result.cst().size()
        ? result.cst()[node->cstNodeId] : cursiv::CstNode{};
    std::fprintf(stderr, " %s=%.*s@%p id=%u span=[%u,%u) parent=%p", role,
                 static_cast<int>(name.size()), name.data(), static_cast<const void*>(node),
                 static_cast<unsigned>(node->cstNodeId), static_cast<unsigned>(span.begin),
                 static_cast<unsigned>(span.end), static_cast<const void*>(node->parent));
  };
  if (!result.value || result.value->parent != parent) {
    std::fprintf(stderr, "v tree diagnostic: %s root-parent mismatch", label);
    dump("root", result.value); dump("expected", parent); std::fputc('\n', stderr);
    return false;
  }
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) {
      std::fprintf(stderr, "v tree diagnostic: %s duplicate", label);
      dump("node", &node); std::fputc('\n', stderr);
      valid = false; return;
    }
    language.astReflection->visitMembers(node,
        [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
          const bool parents = child.parent == &node;
          const bool contains = result.cst().contains(node.cstNodeId, child.cstNodeId);
          if (!parents || !contains) {
            std::fprintf(stderr, "v tree diagnostic: %s document=%d member=%.*s index=%u parents=%d contains=%d",
                         label, parent != nullptr, static_cast<int>(member.size()), member.data(),
                         static_cast<unsigned>(index), parents, contains);
            dump("owner", &node); dump("child", &child); dump("actual-parent", child.parent);
            std::fputc('\n', stderr);
          }
          valid &= parents && contains;
          self(self, child);
        });
  };
  visit(visit, *result.value);
  if (!valid || seen.size() != result.nodes.size()) {
    std::fprintf(stderr, "v tree diagnostic: %s seen=%zu arena-nodes=%zu\n",
                 label, seen.size(), result.nodes.size());
    for (const auto* node : result.nodes)
      if (!seen.contains(node)) {
        std::fprintf(stderr, "v tree diagnostic: %s unreachable", label);
        dump("node", node); std::fputc('\n', stderr);
      }
  }
  return valid && seen.size() == result.nodes.size();
}

inline const ast::Expr* value(const cursiv::ParseResult& result) {
  const auto* file = cursiv::ast_cast<ast::SourceFile>(result.value);
  const auto* fn = file && file->decls.size() == 1
                       ? cursiv::ast_cast<ast::FunctionDecl>(file->decls[0]) : nullptr;
  const auto* variable = fn && fn->body && fn->body->stmts.size() == 1
      ? cursiv::ast_cast<ast::VarDecl>(fn->body->stmts[0]) : nullptr;
  return variable && variable->values.size() == 1 ? variable->values[0] : nullptr;
}

template <class Language> bool run(const Language& language) {
  bool ok = true;
  auto check = [&](bool pass, const char* name, const char* reason) {
    if (!pass) {
      std::fprintf(stderr, "v interpolation: %s: %s\n", name, reason);
      ok = false;
    }
  };
  for (const auto& c : cases) {
    const std::string source(c.source);
    const auto parsed = language.parser->parse(source);
    const auto validation = cursiv::test::validationHelper(language, source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty();
    check((clean && validation.diagnostics.empty()) == c.accepted,
          c.name, "native acceptance");
    if (clean && validation.document &&
        (std::string_view(c.name) == "missing-parenthesized-name" ||
         std::string_view(c.name) == "unfinished-brace")) {
      const auto& document = *validation.document;
      const auto* text = cursiv::ast_cast<ast::InterpolatedString>(value(document.parseResult));
      const auto* hole = text && text->interpolations.size() == 1
          ? text->interpolations[0] : nullptr;
      const auto* name = hole ? cursiv::ast_cast<ast::Ident>(hole->expression) : nullptr;
      check(name != nullptr, c.name, "terminal name retained after adoption");
      if (name) {
        auto sink = [](cursiv::Diagnostic&&) {};
        cursiv::ValidationAcceptor accept(sink, document, "v");
        const auto exact = accept.spanOfName(name->name, *name);
        const auto extent = accept.spanOf(*hole);
        const bool parenthesized = std::string_view(c.name) == "missing-parenthesized-name";
        check(exact.end - exact.begin == name->name.size() &&
                  (parenthesized ? exact.end < extent.end : exact.end == extent.end),
              c.name, "borrowed name extent distinguishes parentheses");
      }
    }
    if (!c.accepted || !clean) continue;
    check(tree(language, parsed), c.name, "AST ownership, parents and CST containment");
    check(validation.document &&
              tree(language, validation.document->parseResult, validation.document.get()),
          c.name, "validated AST ownership");
    const auto* expression = value(parsed);
    const auto literal = c.source.substr(std::string_view("fn main(){s:=").size(),
        c.source.size() - std::string_view("fn main(){s:=").size() - 2);
    if (!c.interpolated) {
      const auto* plain = cursiv::ast_cast<ast::StringLit>(expression);
      check(plain && plain->text == literal, c.name, "ordinary quoted spelling");
      continue;
    }
    const auto* text = cursiv::ast_cast<ast::InterpolatedString>(expression);
    check(text != nullptr, c.name, "structured string node");
    if (!text) continue;
    check(parsed.cst().text(text->cstNodeId) == literal, c.name, "complete quoted CST");
    check(text->texts.size() == c.texts.size() &&
              text->interpolations.size() == c.expressionTypes.size(),
          c.name, "segment and expression counts");
    if (text->texts.size() == c.texts.size())
      for (std::size_t i = 0; i < c.texts.size(); ++i)
        check(text->texts[i] == c.texts[i], c.name, "raw text segment");
    if (text->interpolations.size() != c.expressionTypes.size()) continue;
    for (std::size_t i = 0; i < c.expressionTypes.size(); ++i) {
      const auto* hole = text->interpolations[i];
      check(hole && hole->expression && hole->expression->type == c.expressionTypes[i],
            c.name, "embedded expression kind");
      if (!hole) continue;
      const bool braced = literal.find("${") != std::string_view::npos;
      const bool missing = std::string_view(c.name).starts_with("missing-") ||
                           std::string_view(c.name) == "unfinished-brace";
      check(hole->braced == braced && hole->hasClosingBrace == (braced && !missing),
            c.name, "interpolation delimiter flags");
      if (std::string_view(c.name) == "empty-braced") {
        const auto* empty = cursiv::ast_cast<ast::StringLit>(hole->expression);
        check(empty && empty->text.empty(), c.name, "native empty StringLiteral");
      }
      if (std::string_view(c.name) == "format-zero")
        check(hole->format && hole->format->sign.empty() &&
                  hole->format->number == "04" && hole->format->code == "d",
              c.name, "zero-padded format spelling");
      if (std::string_view(c.name) == "format-plus")
        check(hole->format && hole->format->sign == "+" &&
                  hole->format->number == "10.2" && hole->format->code == "f",
              c.name, "signed precision format spelling");
      if (std::string_view(c.name) == "format-negative")
        check(hole->format && hole->format->sign == "-" &&
                  hole->format->number == "8" && hole->format->code == "x",
              c.name, "negative-width format spelling");
      if (std::string_view(c.name) == "empty-format")
        check(hole->format && hole->format->sign.empty() &&
                  hole->format->number.empty() && hole->format->code.empty(),
              c.name, "inactive colon-only format");
    }
    if (std::string_view(c.name) == "nested-interpolation") {
      const auto* call = cursiv::ast_cast<ast::CallExpr>(text->interpolations[0]->expression);
      const auto* inner = call && call->args.size() == 1
          ? cursiv::ast_cast<ast::InterpolatedString>(call->args[0]) : nullptr;
      check(inner && inner->interpolations.size() == 1 && inner->texts.size() == 2,
            c.name, "nested interpolation is an owned expression");
    }
  }
  // Adding expression wrappers must preserve existing statement AST shapes.
  const auto statement = language.parser->parse("fn main(){if true {1} else {2}}\n");
  const auto* file = cursiv::ast_cast<ast::SourceFile>(statement.value);
  const auto* fn = file && file->decls.size() == 1
      ? cursiv::ast_cast<ast::FunctionDecl>(file->decls[0]) : nullptr;
  check(statement.accepted && !statement.cst().recovered() && fn && fn->body &&
            fn->body->stmts.size() == 1 &&
            cursiv::ast_cast<ast::IfStmt>(fn->body->stmts[0]),
        "if-statement", "existing statement shape");
  return ok;
}
}  // namespace v_interpolation
