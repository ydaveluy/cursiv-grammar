#pragma once

#include "interpolation_cases.hpp"

namespace v_interpolation_targeted {
namespace ast = v::ast;

// Additional exact V 0.4.12 parser-only sources, frozen 2026-11-01:
// native-targeted-results06 and native-extra-results07. Old 46 cases unchanged.
struct Case {
  const char* name;
  std::string_view source;
  bool accepted;
  const cursiv::AstTypeTag* expressionType;
  std::string_view tail;
};
inline const Case cases[] = {
    {"short-call-call-tail", R"VCASE(fn main(){s:='$f()()'}
)VCASE", true, &cursiv::ast_type_tag<ast::CallExpr>, R"VCASE(())VCASE"},
    {"short-method-call-call-tail", R"VCASE(fn main(){s:='$f().g()()'}
)VCASE", true, &cursiv::ast_type_tag<ast::CallExpr>, R"VCASE(())VCASE"},
    {"short-call-selector-continuation", R"VCASE(fn main(){s:='$f().g().h'}
)VCASE", true, &cursiv::ast_type_tag<ast::SelectorExpr>, R"VCASE()VCASE"},
    {"unsafe-empty-expression-body", R"VCASE(fn main(){s:='${unsafe {}}'}
)VCASE", false, nullptr, R"VCASE()VCASE"},
    {"unsafe-multiple-expression-body", R"VCASE(fn main(){s:='${unsafe {1;2}}'}
)VCASE", false, nullptr, R"VCASE()VCASE"},
    {"unsafe-single-expression-control", R"VCASE(fn main(){s:='${unsafe {1}}'}
)VCASE", true, &cursiv::ast_type_tag<ast::UnsafeExpr>, R"VCASE()VCASE"},
    {"unsafe-call-expression-control", R"VCASE(fn main(){s:='${unsafe {f()}}'}
)VCASE", true, &cursiv::ast_type_tag<ast::UnsafeExpr>, R"VCASE()VCASE"},
    {"short-call-name-argument", R"VCASE(fn main(){s:='$f(x)'}
)VCASE", false, nullptr, R"VCASE()VCASE"},
    {"short-call-bool-argument", R"VCASE(fn main(){s:='$f(true)'}
)VCASE", true, &cursiv::ast_type_tag<ast::CallExpr>, R"VCASE()VCASE"},
    {"short-call-parenthesized-argument", R"VCASE(fn main(){s:='$f((1))'}
)VCASE", false, nullptr, R"VCASE()VCASE"},
    {"short-call-string-argument", R"VCASE(fn main(){s:="$f('x')"}
)VCASE", false, nullptr, R"VCASE()VCASE"},
    {"unsafe-nested-expression-body", R"VCASE(fn main(){s:='${unsafe {unsafe {1}}}'}
)VCASE", false, nullptr, R"VCASE()VCASE"},
};

template <class Language> bool run(const Language& language) {
  bool ok = true;
  auto check = [&](bool pass, const char* name, const char* reason) {
    if (!pass) {
      std::fprintf(stderr, "v targeted interpolation: %s: %s\n", name, reason);
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
    if (!c.accepted || !clean) continue;
    check(v_interpolation::tree(language, parsed, nullptr, c.name), c.name,
          "AST ownership, parents and CST containment");
    check(validation.document && v_interpolation::tree(
              language, validation.document->parseResult, validation.document.get(), c.name),
          c.name, "validated AST ownership");
    const auto* text = cursiv::ast_cast<ast::InterpolatedString>(v_interpolation::value(parsed));
    check(text && text->texts.size() == 2 && text->interpolations.size() == 1,
          c.name, "one structured interpolation and two raw segments");
    if (!text || text->texts.size() != 2 || text->interpolations.size() != 1) continue;
    check(text->texts[0].empty() && text->texts[1] == c.tail,
          c.name, "native trailing text after the first non-selector call");
    const auto literal = c.source.substr(std::string_view("fn main(){s:=").size(),
        c.source.size() - std::string_view("fn main(){s:=").size() - 2);
    check(parsed.cst().text(text->cstNodeId) == literal, c.name, "complete quoted CST");
    const auto* hole = text->interpolations[0];
    const auto* expression = hole ? hole->expression : nullptr;
    check(expression && expression->type == c.expressionType,
          c.name, "native embedded expression kind");
    if (!expression) continue;
    const auto spelling = [&](const cursiv::AstNode* node) -> std::string_view {
      return node && node->cstNodeId != cursiv::kNoNode
          ? parsed.cst().text(node->cstNodeId) : std::string_view{};
    };
    if (std::string_view(c.name) == "short-call-call-tail") {
      const auto* call = cursiv::ast_cast<ast::CallExpr>(expression);
      const auto* callee = call ? cursiv::ast_cast<ast::Ident>(call->callee) : nullptr;
      check(call && call->args.empty() && callee && callee->name == "f",
            c.name, "exactly one call; second parentheses belong to text");
      check(spelling(callee) == "f" && spelling(call) == "f()",
            c.name, "exact identifier and first-call CST prefixes");
    }
    if (std::string_view(c.name) == "short-method-call-call-tail" ||
        std::string_view(c.name) == "short-call-selector-continuation") {
      const auto* outerField = cursiv::ast_cast<ast::SelectorExpr>(expression);
      const auto* method = cursiv::ast_cast<ast::CallExpr>(
          outerField ? outerField->object : expression);
      const auto* field = method ? cursiv::ast_cast<ast::SelectorExpr>(method->callee) : nullptr;
      const auto* call = field ? cursiv::ast_cast<ast::CallExpr>(field->object) : nullptr;
      const auto* callee = call ? cursiv::ast_cast<ast::Ident>(call->callee) : nullptr;
      check(method && method->args.empty() && field && field->field == "g" &&
                call && call->args.empty() && callee && callee->name == "f" &&
                (std::string_view(c.name) != "short-call-selector-continuation" ||
                 (outerField && outerField->field == "h")),
            c.name, "call-selector-call chain stops at the native boundary");
      check(spelling(callee) == "f" && spelling(call) == "f()",
            c.name, "exact identifier and first-call CST prefixes");
      check(spelling(field) == "f().g",
            c.name, "exact selector CST prefix before method call");
      check(spelling(method) == "f().g()",
            c.name, "exact method-call CST prefix");
      if (std::string_view(c.name) == "short-call-selector-continuation")
        check(spelling(outerField) == "f().g().h",
              c.name, "exact outer-selector CST prefix");
    }
    if (std::string_view(c.name) == "short-call-bool-argument") {
      const auto* call = cursiv::ast_cast<ast::CallExpr>(expression);
      const auto* argument = call && call->args.size() == 1
          ? cursiv::ast_cast<ast::BoolLit>(call->args[0]) : nullptr;
      check(argument && argument->value, c.name, "boolean argument stays valid");
    }
    if (c.expressionType == &cursiv::ast_type_tag<ast::UnsafeExpr>) {
      const cursiv::AstNode* value = nullptr;
      std::size_t children = 0;
      language.astReflection->visitMembers(*expression,
          [&](std::string_view member, std::uint32_t, cursiv::AstNode& child) {
            ++children;
            if (member == "value") value = &child;
          });
      check(children == 1 && value &&
                language.astReflection->isInstance(*value, &cursiv::ast_type_tag<ast::Expr>),
            c.name, "unsafe owns one expression value instead of a statement list");
      if (std::string_view(c.name) == "unsafe-single-expression-control") {
        const auto* number = cursiv::ast_cast<ast::IntLit>(value);
        check(number && number->text == "1", c.name, "unsafe literal expression retained");
      } else {
        const auto* call = cursiv::ast_cast<ast::CallExpr>(value);
        const auto* callee = call ? cursiv::ast_cast<ast::Ident>(call->callee) : nullptr;
        check(call && call->args.empty() && callee && callee->name == "f",
              c.name, "unsafe call expression retained");
      }
    }
  }
  return ok;
}
}  // namespace v_interpolation_targeted
