#include "native_followup.hpp"
#include "extend_cases.hpp"
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "scss/core/CoreServices.hpp"
#include "scss/core/ast.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics, a Stylesheet at the root -- and a few real
// field values, not just structure, read back right from one of them.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "scss: %s: %s\n", file.string().c_str(), what);
  return 1;
}

template <class T>
bool is(const cursiv::AstNode* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<scss::ScssCoreServices,
                                                scss::ScssSharedCoreServices,
                                                scss::install_core_module>();
  // Keyword-shaped pseudo names must retain the selector AST, including
  // when another pseudo follows. A successful nested-property parse would
  // silently lose the selector even without a recovery diagnostic.
  const struct {
    const char* text;
    std::string_view firstName;
    std::size_t pseudos;
  } selectorCases[] = {
      {"a:not([href]):not([class]) {}", "not", 2},
      {"a:not([href]) { &, &:hover { color: inherit; } }", "not", 1},
      {"a:from:to {}", "from", 2},
      {"a:calc(1):hover {}", "calc", 2},
      {"a:true:hover {}", "true", 2}};
  for (const auto& c : selectorCases) {
    const auto r = language.language.parser->parse(c.text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !is<scss::ast::Stylesheet>(r.value))
      return 1;
    const auto* sheet = static_cast<const scss::ast::Stylesheet*>(r.value);
    if (sheet->statements.size() != 1 ||
        !is<scss::ast::Ruleset>(sheet->statements[0]))
      return 1;
    const auto* rule =
        static_cast<const scss::ast::Ruleset*>(sheet->statements[0]);
    if (rule->selectors == nullptr ||
        rule->selectors->selectors.size() != 1 ||
        rule->selectors->selectors[0]->sequences.size() != 1)
      return 1;
    const auto* sequence = rule->selectors->selectors[0]->sequences[0];
    if (sequence->items.size() != c.pseudos ||
        !is<scss::ast::PseudoSelector>(sequence->items[0]))
      return 1;
    const auto* pseudo =
        static_cast<const scss::ast::PseudoSelector*>(sequence->items[0]);
    if (pseudo->name != c.firstName) return 1;
  }
  {
    const auto r = language.language.parser->parse(
        ".a { margin: auto { bottom: 10px; } "
        "transition: { property: font-size; } }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !is<scss::ast::Stylesheet>(r.value))
      return 1;
    const auto* sheet = static_cast<const scss::ast::Stylesheet*>(r.value);
    if (sheet->statements.size() != 1 ||
        !is<scss::ast::Ruleset>(sheet->statements[0]))
      return 1;
    const auto* rule =
        static_cast<const scss::ast::Ruleset*>(sheet->statements[0]);
    if (rule->body == nullptr || rule->body->statements.size() != 2)
      return 1;
    for (const auto* statement : rule->body->statements) {
      if (!is<scss::ast::PropertyDeclaration>(statement)) return 1;
      const auto* property =
          static_cast<const scss::ast::PropertyDeclaration*>(statement);
      if (property->nestedBlock == nullptr ||
          property->nestedBlock->statements.size() != 1)
        return 1;
    }
  }
  {
    const auto r = language.language.parser->parse(
        "@for $i from $start + 1 through 5 { .x { width: $i; } }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "@function f() { @return 1; @debug 2; }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "@return 1;", "file:///audit.scss");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      scss::ScssCoreServices, scss::ScssSharedCoreServices,
      scss::install_core_module>();

  if (!native_followup::run(language)) return 1;
  if (!scss_extend_cases::run(*language.language.parser)) return 1;

  int failures = 0;
  int count = 0;
  const fs::path root = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".scss")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      for (const auto &diagnostic : result.diagnostics)
        std::fprintf(stderr, "scss parse diagnostic at byte %u..%u (kind %u)\n",
                     diagnostic.begin, diagnostic.end, unsigned(diagnostic.kind));
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries a diagnostic");
      continue;
    }
    if (!is<scss::ast::Stylesheet>(result.value)) {
      failures += fail(entry.path(), "the parse built no Stylesheet");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "scss: no .scss file found under example/\n");
    return 1;
  }

  // A structural pass alone would not catch a converter that silently
  // yields the wrong value, so read a few fields back out of one document.
  {
    const fs::path path = root / "basic.scss";
    std::ifstream file(path);
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& sheet =
        *static_cast<const scss::ast::Stylesheet*>(result.value);

    const scss::ast::VariableDeclaration* black = nullptr;
    const scss::ast::PropertyDeclaration* backgroundColor = nullptr;
    for (const scss::ast::Statement* stmt : sheet.statements) {
      if (is<scss::ast::VariableDeclaration>(stmt)) {
        const auto* decl =
            static_cast<const scss::ast::VariableDeclaration*>(stmt);
        if (decl->name == "black") black = decl;
      } else if (is<scss::ast::Ruleset>(stmt)) {
        const auto* ruleset = static_cast<const scss::ast::Ruleset*>(stmt);
        if (ruleset->body == nullptr) continue;
        for (const scss::ast::Statement* inner : ruleset->body->statements) {
          if (!is<scss::ast::PropertyDeclaration>(inner)) continue;
          const auto* prop =
              static_cast<const scss::ast::PropertyDeclaration*>(inner);
          if (prop->property == "background-color") backgroundColor = prop;
        }
      }
    }

    if (black == nullptr) {
      failures += fail(path, "no top-level $black variable declaration");
    } else if (!is<scss::ast::HexColorLit>(black->value)) {
      failures += fail(path, "$black's value is not a hex color");
    } else if (static_cast<const scss::ast::HexColorLit*>(black->value)
                   ->text != "#000") {
      failures += fail(path, "$black's hex color is not #000");
    } else if (!black->isDefault) {
      failures += fail(path, "$black does not carry !default");
    }

    if (backgroundColor == nullptr) {
      failures += fail(path, "no background-color declaration found");
    } else if (backgroundColor->value.size() != 1 ||
               !is<scss::ast::FunctionCall>(backgroundColor->value[0])) {
      failures += fail(path, "background-color's value is not one call");
    } else {
      const auto* call = static_cast<const scss::ast::FunctionCall*>(
          backgroundColor->value[0]);
      if (call->name != "var") {
        failures += fail(path, "background-color does not call var()");
      } else if (call->args.size() != 1 ||
                 !is<scss::ast::VariableRef>(call->args[0]->value)) {
        failures += fail(path, "var() does not take one variable reference");
      } else {
        const auto* ref =
            static_cast<const scss::ast::VariableRef*>(call->args[0]->value);
        if (!ref->isCustomProperty || ref->name != "app-background-color")
          failures +=
              fail(path, "var()'s argument is not --app-background-color");
      }
    }
  }

  // A trailing unit/`%`/bare variable glued right after `#{...}` in a
  // selector is a real construct (a keyframe step generated by a loop,
  // `#{$i}%`), not a nil-cost corner: read it back at the AST level, not
  // just structurally.
  {
    const std::string text =
        "@keyframes fade {\n"
        "  #{$i}% { opacity: 1; }\n"
        "}\n"
        "#{$sel}$y { color: red; }\n";
    const char* const label = "<keyframe interpolation>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      auto firstSequence =
          [](const scss::ast::Ruleset* r) -> const scss::ast::SimpleSelectorSequence* {
        if (r == nullptr || r->selectors == nullptr ||
            r->selectors->selectors.size() != 1 ||
            r->selectors->selectors[0]->sequences.size() != 1)
          return nullptr;
        return r->selectors->selectors[0]->sequences[0];
      };

      const scss::ast::KeyframesDeclaration* kf = nullptr;
      const scss::ast::Ruleset* bare = nullptr;
      for (const scss::ast::Statement* stmt : sheet.statements) {
        if (is<scss::ast::KeyframesDeclaration>(stmt))
          kf = static_cast<const scss::ast::KeyframesDeclaration*>(stmt);
        else if (is<scss::ast::Ruleset>(stmt))
          bare = static_cast<const scss::ast::Ruleset*>(stmt);
      }

      const scss::ast::Ruleset* step =
          (kf != nullptr && kf->body != nullptr &&
           kf->body->statements.size() == 1 &&
           is<scss::ast::Ruleset>(kf->body->statements[0]))
              ? static_cast<const scss::ast::Ruleset*>(kf->body->statements[0])
              : nullptr;
      const scss::ast::SimpleSelectorSequence* stepSeq = firstSequence(step);
      if (stepSeq == nullptr || stepSeq->items.size() != 1 ||
          !is<scss::ast::Interpolation>(stepSeq->items[0])) {
        failures += fail(label, "the keyframe step's selector is not one interpolation");
      } else if (static_cast<const scss::ast::Interpolation*>(stepSeq->items[0])
                     ->trailingUnit != "%") {
        failures += fail(label, "the interpolation's trailing % is lost");
      }

      const scss::ast::SimpleSelectorSequence* bareSeq = firstSequence(bare);
      if (bareSeq == nullptr || bareSeq->items.size() != 2 ||
          !is<scss::ast::Interpolation>(bareSeq->items[0]) ||
          !is<scss::ast::VariableRef>(bareSeq->items[1])) {
        failures += fail(label, "#{$sel}$y is not [interpolation, variable]");
      } else if (static_cast<const scss::ast::VariableRef*>(bareSeq->items[1])
                     ->name != "y") {
        failures += fail(label, "the glued variable's name is wrong");
      }
    }
  }

  // multiplicative > additive > relational > equality > and > or (Sass,
  // "Operators", order of operations): the first line is Sass's own
  // worked example, `1 + 2 * 3 == 1 + (2 * 3)`.
  {
    const std::string text =
        "$a: 1 + 2 * 3 == 1 + (2 * 3);\n"
        "$b: 1 < 2 and 3 == 4 or 5 > 6;\n";
    const char* const label = "<precedence>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      if (sheet.statements.size() != 2) {
        failures += fail(label, "wrong statement count");
      } else {
        const auto* const a = static_cast<const scss::ast::VariableDeclaration*>(
            sheet.statements[0]);
        const auto* const eq =
            is<scss::ast::BinaryExpr>(a->value)
                ? static_cast<const scss::ast::BinaryExpr*>(a->value)
                : nullptr;
        const auto* const add =
            eq != nullptr && is<scss::ast::BinaryExpr>(eq->left)
                ? static_cast<const scss::ast::BinaryExpr*>(eq->left)
                : nullptr;
        const auto* const mul =
            add != nullptr && is<scss::ast::BinaryExpr>(add->right)
                ? static_cast<const scss::ast::BinaryExpr*>(add->right)
                : nullptr;
        if (eq == nullptr || eq->op != scss::ast::Op::eq || add == nullptr ||
            add->op != scss::ast::Op::plus || mul == nullptr ||
            mul->op != scss::ast::Op::times)
          failures += fail(label, "* does not bind tighter than +, or + than ==");

        const auto* const b = static_cast<const scss::ast::VariableDeclaration*>(
            sheet.statements[1]);
        const auto* const orExpr =
            is<scss::ast::BinaryExpr>(b->value)
                ? static_cast<const scss::ast::BinaryExpr*>(b->value)
                : nullptr;
        const auto* const andExpr =
            orExpr != nullptr && is<scss::ast::BinaryExpr>(orExpr->left)
                ? static_cast<const scss::ast::BinaryExpr*>(orExpr->left)
                : nullptr;
        const auto* const lt =
            andExpr != nullptr && is<scss::ast::BinaryExpr>(andExpr->left)
                ? static_cast<const scss::ast::BinaryExpr*>(andExpr->left)
                : nullptr;
        if (orExpr == nullptr || orExpr->op != scss::ast::Op::or_ ||
            andExpr == nullptr || andExpr->op != scss::ast::Op::and_ ||
            lt == nullptr || lt->op != scss::ast::Op::less)
          failures += fail(label, "and/or/relational lost their levels");
      }
    }
  }

  // dart-sass, `_expression` and `_parentheses`: after an item, `+` and a
  // `-` before a blank or a variable are operators, a `-` glued to a number
  // behind a blank opens the next item; `(x)` is a value in parens.
  {
    const std::string text =
        "$a: $b +$c;\n"
        "$d: $e - $f;\n"
        "$g: 1 -2;\n"
        "$h: ($i);\n";
    const char* const label = "<signs>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto value =
          [&](std::size_t i) -> const scss::ast::ComponentValue* {
        return i < sheet.statements.size()
                   ? static_cast<const scss::ast::VariableDeclaration*>(
                         sheet.statements[i])
                         ->value
                   : nullptr;
      };
      const auto binary =
          [&](std::size_t i) -> const scss::ast::BinaryExpr* {
        return is<scss::ast::BinaryExpr>(value(i))
                   ? static_cast<const scss::ast::BinaryExpr*>(value(i))
                   : nullptr;
      };
      const auto* const sum = binary(0);
      const auto* const difference = binary(1);
      if (sum == nullptr || sum->op != scss::ast::Op::plus ||
          difference == nullptr || difference->op != scss::ast::Op::minus)
        failures += fail(label, "$b +$c or $e - $f is not one operation");
      if (!is<scss::ast::ListValue>(value(2)) ||
          static_cast<const scss::ast::ListValue*>(value(2))->items.size() !=
              2)
        failures += fail(label, "1 -2 is not a list of two");
      if (!is<scss::ast::ParenGroup>(value(3)))
        failures += fail(label, "($i) is not a value in parens");
    }
  }

  // A name glued to its `:` before a block is a pseudo-class, so the
  // statement is a style rule; a blank after the `:` makes it a declaration
  // with nested properties.
  {
    const std::string text =
        ".x {\n"
        "  a:hover, b::before { color: red; }\n"
        "  font: bold { family: x; }\n"
        "}\n";
    const char* const label = "<declaration or style rule>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto* const outer =
          sheet.statements.size() == 1 &&
                  is<scss::ast::Ruleset>(sheet.statements[0])
              ? static_cast<const scss::ast::Ruleset*>(sheet.statements[0])
              : nullptr;
      const scss::ast::Block* const body =
          outer != nullptr ? outer->body : nullptr;
      if (body == nullptr || body->statements.size() != 2) {
        failures += fail(label, "wrong statement count");
      } else {
        const auto* const rule =
            is<scss::ast::Ruleset>(body->statements[0])
                ? static_cast<const scss::ast::Ruleset*>(body->statements[0])
                : nullptr;
        if (rule == nullptr || rule->selectors == nullptr ||
            rule->selectors->selectors.size() != 2)
          failures += fail(label, "a:hover, b::before is not one style rule");

        const auto* const font =
            is<scss::ast::PropertyDeclaration>(body->statements[1])
                ? static_cast<const scss::ast::PropertyDeclaration*>(
                      body->statements[1])
                : nullptr;
        if (font == nullptr || font->value.size() != 1 ||
            font->nestedBlock == nullptr)
          failures += fail(label, "font: bold { ... } lost its nested block");
      }
    }
  }

  // A name glued from a run and interpolations is one declaration; `&`
  // glued to `-#{...}` is one compound selector; a stray `;` is an empty
  // statement.
  {
    const std::string text =
        ".x {\n"
        "  margin-#{$a}-#{$b}: 0;;\n"
        "  &-#{$i}:hover { color: red; }\n"
        "}\n";
    const char* const label = "<interpolated names>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto* const outer =
          sheet.statements.size() == 1 &&
                  is<scss::ast::Ruleset>(sheet.statements[0])
              ? static_cast<const scss::ast::Ruleset*>(sheet.statements[0])
              : nullptr;
      const scss::ast::Block* const body =
          outer != nullptr ? outer->body : nullptr;
      if (body == nullptr || body->statements.size() != 2) {
        failures += fail(label, "wrong statement count");
      } else {
        const auto* const margin =
            is<scss::ast::PropertyDeclaration>(body->statements[0])
                ? static_cast<const scss::ast::PropertyDeclaration*>(
                      body->statements[0])
                : nullptr;
        if (margin == nullptr || margin->property != "margin-" ||
            margin->suffix.size() != 3 ||
            !is<scss::ast::Interpolation>(margin->suffix[0]) ||
            !is<scss::ast::IdentValue>(margin->suffix[1]) ||
            !is<scss::ast::Interpolation>(margin->suffix[2]))
          failures += fail(label, "margin-#{$a}-#{$b} is not one name");

        const auto* const rule =
            is<scss::ast::Ruleset>(body->statements[1])
                ? static_cast<const scss::ast::Ruleset*>(body->statements[1])
                : nullptr;
        const scss::ast::SimpleSelectorSequence* const seq =
            rule != nullptr && rule->selectors != nullptr &&
                    rule->selectors->selectors.size() == 1 &&
                    rule->selectors->selectors[0]->sequences.size() == 1
                ? rule->selectors->selectors[0]->sequences[0]
                : nullptr;
        if (seq == nullptr || seq->type == nullptr ||
            !seq->type->hasParentRefPrefix || seq->items.size() != 2 ||
            !is<scss::ast::Interpolation>(seq->items[0]) ||
            !is<scss::ast::PseudoSelector>(seq->items[1]))
          failures += fail(label, "&-#{$i}:hover is not one compound selector");
      }
    }
  }

  // `@charset` builds nothing; `@import` takes a list; a parent suffix is any
  // identifier body; a `-` between interpolations stays in its compound.
  {
    const std::string text =
        "@charset \"UTF-8\";\n"
        "@import \"a\", \"b\";\n"
        ".x {\n"
        "  &--on { color: red; }\n"
        "  &-#{$a}-#{$b} { color: red; }\n"
        "}\n";
    const char* const label = "<parent suffixes>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto* const imports =
          sheet.statements.size() == 2 &&
                  is<scss::ast::ImportDeclaration>(sheet.statements[0])
              ? static_cast<const scss::ast::ImportDeclaration*>(
                    sheet.statements[0])
              : nullptr;
      if (imports == nullptr || imports->morePaths.size() != 1)
        failures += fail(label, "@import \"a\", \"b\" is not one import");

      const auto* const outer =
          sheet.statements.size() == 2 &&
                  is<scss::ast::Ruleset>(sheet.statements[1])
              ? static_cast<const scss::ast::Ruleset*>(sheet.statements[1])
              : nullptr;
      const scss::ast::Block* const body =
          outer != nullptr ? outer->body : nullptr;
      auto sequence = [&](unsigned i)
          -> const scss::ast::SimpleSelectorSequence* {
        if (body == nullptr || body->statements.size() != 2 ||
            !is<scss::ast::Ruleset>(body->statements[i]))
          return nullptr;
        const auto* const r =
            static_cast<const scss::ast::Ruleset*>(body->statements[i]);
        if (r->selectors == nullptr || r->selectors->selectors.size() != 1 ||
            r->selectors->selectors[0]->sequences.size() != 1)
          return nullptr;
        return r->selectors->selectors[0]->sequences[0];
      };
      const auto* const on = sequence(0);
      if (on == nullptr || on->type == nullptr ||
          !on->type->hasParentRefPrefix ||
          !is<scss::ast::IdentValue>(on->type->name) ||
          static_cast<const scss::ast::IdentValue*>(on->type->name)->name !=
              "--on")
        failures += fail(label, "&--on is not a parent with suffix --on");
      const auto* const dashed = sequence(1);
      if (dashed == nullptr || dashed->type == nullptr ||
          dashed->items.size() != 3 ||
          !is<scss::ast::Interpolation>(dashed->items[0]) ||
          !is<scss::ast::IdentValue>(dashed->items[1]) ||
          !is<scss::ast::Interpolation>(dashed->items[2]))
        failures += fail(label, "&-#{$a}-#{$b} is not one compound selector");
    }
  }

  // An unknown at-rule keeps its value as text, with or without a block;
  // `@at-root` and `@debug` have their own shapes; the last statement owes
  // no `;`.
  {
    const std::string text =
        "@page :first { margin: 1px; }\n"
        "@layer a, b;\n"
        ".x { @at-root .a { color: red; } @debug $a; }\n"
        "@import \"a\" screen;\n"
        "$v: \"x\"";
    const char* const label = "<at-rules>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      if (sheet.statements.size() != 5) {
        failures += fail(label, "wrong statement count");
      } else {
        const auto* const page =
            is<scss::ast::AtStatementDeclaration>(sheet.statements[0])
                ? static_cast<const scss::ast::AtStatementDeclaration*>(
                      sheet.statements[0])
                : nullptr;
        if (page == nullptr || page->name != "page" ||
            page->value.size() != 1 ||
            !is<scss::ast::RawText>(page->value[0]) ||
            static_cast<const scss::ast::RawText*>(page->value[0])->text !=
                ":first" ||
            page->body == nullptr)
          failures += fail(label, "@page :first is not text and a block");
        const auto* const layer =
            is<scss::ast::AtStatementDeclaration>(sheet.statements[1])
                ? static_cast<const scss::ast::AtStatementDeclaration*>(
                      sheet.statements[1])
                : nullptr;
        if (layer == nullptr || layer->body != nullptr ||
            layer->value.size() != 1)
          failures += fail(label, "@layer a, b; is not a bodiless at-rule");
        const auto* const outer =
            is<scss::ast::Ruleset>(sheet.statements[2])
                ? static_cast<const scss::ast::Ruleset*>(sheet.statements[2])
                : nullptr;
        if (outer == nullptr || outer->body == nullptr ||
            outer->body->statements.size() != 2 ||
            !is<scss::ast::AtRootDeclaration>(outer->body->statements[0]) ||
            static_cast<const scss::ast::AtRootDeclaration*>(
                outer->body->statements[0])
                    ->rule == nullptr ||
            !is<scss::ast::DebugDeclaration>(outer->body->statements[1]))
          failures += fail(label, "@at-root .a {} or @debug lost its shape");
        const auto* const import =
            is<scss::ast::ImportDeclaration>(sheet.statements[3])
                ? static_cast<const scss::ast::ImportDeclaration*>(
                      sheet.statements[3])
                : nullptr;
        if (import == nullptr || import->media.size() != 1)
          failures += fail(label, "@import \"a\" screen lost its media");
        if (!is<scss::ast::VariableDeclaration>(sheet.statements[4]))
          failures += fail(label, "$v: \"x\" at the end is not a variable");
      }
    }
  }

  // Any identifier glued to a number is its unit, an exponent is not; an
  // `@import`'s functions come before its media.
  {
    const std::string text =
        "$a: 1in;\n"
        "$b: 1e3;\n"
        "@import url(a.css) layer(base) screen;\n";
    const char* const label = "<units>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto value = [&](unsigned i) -> const cursiv::AstNode* {
        return sheet.statements.size() == 3 &&
                       is<scss::ast::VariableDeclaration>(sheet.statements[i])
                   ? static_cast<const scss::ast::VariableDeclaration*>(
                         sheet.statements[i])
                         ->value
                   : nullptr;
      };
      if (!is<scss::ast::DimensionLit>(value(0)) ||
          static_cast<const scss::ast::DimensionLit*>(value(0))->text !=
              "1in")
        failures += fail(label, "1in is not one dimension");
      if (!is<scss::ast::NumberLit>(value(1)) ||
          static_cast<const scss::ast::NumberLit*>(value(1))->magnitude !=
              1000)
        failures += fail(label, "1e3 is not the number 1000");
      const auto* const import =
          sheet.statements.size() == 3 &&
                  is<scss::ast::ImportDeclaration>(sheet.statements[2])
              ? static_cast<const scss::ast::ImportDeclaration*>(
                    sheet.statements[2])
              : nullptr;
      if (import == nullptr || import->modifiers.size() != 1 ||
          import->media.size() != 1)
        failures += fail(label, "layer(base) screen is not modifier then media");
    }
  }

  // `not` flags a media query and `and` joins its features; a class in a
  // pseudo-class argument keeps its glued interpolation.
  {
    const std::string text =
        "@media not all and (monochrome) {}\n"
        ":not(.a-#{$b}) {}\n";
    const char* const label = "<media query>";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(label, "does not parse clean");
    } else {
      const auto& sheet =
          *static_cast<const scss::ast::Stylesheet*>(result.value);
      const auto* const media =
          sheet.statements.size() == 2 &&
                  is<scss::ast::MediaDeclaration>(sheet.statements[0])
              ? static_cast<const scss::ast::MediaDeclaration*>(
                    sheet.statements[0])
              : nullptr;
      const scss::ast::MediaQuery* const query =
          media != nullptr && media->queries.size() == 1 ? media->queries[0]
                                                         : nullptr;
      if (query == nullptr || !query->isNot ||
          !is<scss::ast::IdentValue>(query->typeOrValue) ||
          static_cast<const scss::ast::IdentValue*>(query->typeOrValue)
                  ->name != "all" ||
          query->expressions.size() != 1 ||
          !is<scss::ast::MediaExpression>(query->expressions[0]) ||
          static_cast<const scss::ast::MediaExpression*>(
              query->expressions[0])
                  ->feature != "monochrome")
        failures += fail(label, "not all and (monochrome) lost its shape");

      const auto* const rule =
          sheet.statements.size() == 2 &&
                  is<scss::ast::Ruleset>(sheet.statements[1])
              ? static_cast<const scss::ast::Ruleset*>(sheet.statements[1])
              : nullptr;
      const scss::ast::SimpleSelectorSequence* const seq =
          rule != nullptr && rule->selectors != nullptr &&
                  rule->selectors->selectors.size() == 1 &&
                  rule->selectors->selectors[0]->sequences.size() == 1
              ? rule->selectors->selectors[0]->sequences[0]
              : nullptr;
      const auto* const pseudo =
          seq != nullptr && seq->items.size() == 1 &&
                  is<scss::ast::PseudoSelector>(seq->items[0])
              ? static_cast<const scss::ast::PseudoSelector*>(seq->items[0])
              : nullptr;
      if (pseudo == nullptr || pseudo->args.size() != 1 ||
          !is<scss::ast::ClassSelector>(pseudo->args[0]))
        failures += fail(label, ":not(.a-#{$b}) is not one class argument");
    }
  }

  if (failures > 0) return 1;

  std::printf("scss: %d example documents checked\n", count);
  return 0;
}
