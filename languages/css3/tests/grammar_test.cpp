// The smoke test of the language: every corpus document parses clean, and
// two of them are read back field by field to catch a converter that
// silently mismaps its value (a color, a media query's structure).

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "css3/core/CoreServices.hpp"
#include "css3/core/ast.hpp"
#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "css3: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "at-rule.css",
    "bootstrap.css",
    "bootstrap-theme.css",
    "bootstrap-theme.min.css",
    "example1.css",
    "example2.css",
    "jquery-ui.css",
    "jquery-ui.min.css",
    "mdn-at-charset.css",
    "mdn-at-counter-style.css",
    "mdn-at-font-face.css",
    "mdn-at-font-feature-values.css",
    "mdn-at-import.css",
    "mdn-at-keyframes.css",
    "mdn-at-media.css",
    "mdn-at-namespace.css",
    "mdn-at-page.css",
    "mdn-at-supports.css",
    "mdn-at-viewport.css",
};

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      css3::Css3CoreServices, css3::Css3SharedCoreServices,
      css3::install_core_module>();
  if (!css3_audit::run(*language.language.parser)) return 1;

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result =
        language.language.parser->parse("a { font: 16px/1.5 Arial, sans-serif; "
                                        "x: f(1,2/3); y: 1e2px; \\66 oo: 0; }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    std::size_t commas = 0, slashes = 0;
    bool escapedName = false, exponent = false;
    for (const auto *node : result.nodes) {
      if (const auto *value = cursiv::ast_cast<css3::ast::PunctValue>(node)) {
        commas += value->text == ",";
        slashes += value->text == "/";
      }
      if (const auto *decl = cursiv::ast_cast<css3::ast::Declaration>(node))
        escapedName |= decl->property == "\\66 oo";
      if (const auto *value = cursiv::ast_cast<css3::ast::DimensionLit>(node))
        exponent |= value->text == "1e2px";
    }
    if (commas != 2 || slashes != 2 || !escapedName || !exponent)
      return 1;
  }

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<css3::ast::Stylesheet>)
      return fail(std::string(name) + ": built no Stylesheet");
  }

  // example1.css: three rulesets, bare element selectors, hex colors.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example1.css");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& sheet =
        *static_cast<const css3::ast::Stylesheet*>(result.value);
    if (sheet.statements.size() != 3)
      return fail("example1.css: expected three statements");

    if (sheet.statements[0]->type != &cursiv::ast_type_tag<css3::ast::Ruleset>)
      return fail("example1.css: statement 0 is not a Ruleset");
    const auto& body =
        *static_cast<const css3::ast::Ruleset*>(sheet.statements[0]);
    if (body.selectors == nullptr || body.selectors->selectors.size() != 1 ||
        body.selectors->selectors[0]->sequences.size() != 1)
      return fail("example1.css: body selector shape is wrong");
    const auto& bodySeq = *body.selectors->selectors[0]->sequences[0];
    if (bodySeq.type == nullptr || bodySeq.type->name != "body")
      return fail("example1.css: expected a bare `body` type selector");
    if (body.declarations.size() != 1 ||
        body.declarations[0]->property != "color")
      return fail("example1.css: expected one `color` declaration");
    if (body.declarations[0]->value.size() != 1 ||
        body.declarations[0]->value[0]->type !=
            &cursiv::ast_type_tag<css3::ast::HashRef>)
      return fail("example1.css: color value is not a hex color");
    const auto& color0 = *static_cast<const css3::ast::HashRef*>(
        body.declarations[0]->value[0]);
    if (color0.text != "#000000")
      return fail("example1.css: wrong color for body");
  }

  // mdn-at-media.css: the fourth @media combines only/type/three `and`
  // expressions.
  {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/mdn-at-media.css");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& sheet =
        *static_cast<const css3::ast::Stylesheet*>(result.value);
    if (sheet.statements.size() != 4)
      return fail("mdn-at-media.css: expected four @media statements");
    if (sheet.statements[3]->type != &cursiv::ast_type_tag<css3::ast::AtMedia>)
      return fail("mdn-at-media.css: statement 3 is not an AtMedia");
    const auto& media =
        *static_cast<const css3::ast::AtMedia*>(sheet.statements[3]);
    if (media.queries == nullptr || media.queries->queries.size() != 1)
      return fail("mdn-at-media.css: expected one media query");
    const auto& query = *media.queries->queries[0];
    if (!query.isOnly || query.mediaType != "screen")
      return fail("mdn-at-media.css: expected `only screen`");
    if (query.expressions.size() != 3)
      return fail("mdn-at-media.css: expected three `and` expressions");
    if (query.expressions[0]->feature != "min-device-width")
      return fail("mdn-at-media.css: wrong first media feature");
    if (media.body.size() != 1 ||
        media.body[0]->type != &cursiv::ast_type_tag<css3::ast::Ruleset>)
      return fail("mdn-at-media.css: @media body should hold one ruleset");
  }

  // calc()'s `*`/`/` bind tighter than `+`/`-`, and operators of equal
  // precedence associate left to right (CSS Values and Units Module Level
  // 3, section 8.1): `1px + 2px * 3` folds the product into one sum term,
  // and `10px - 3px - 2px` keeps three terms at one flat sum level.
  {
    auto calc_sum = [&](const char* text) -> const css3::ast::CalcSum* {
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered() || result.value == nullptr) return nullptr;
      const auto& sheet =
          *static_cast<const css3::ast::Stylesheet*>(result.value);
      if (sheet.statements.empty() ||
          sheet.statements[0]->type != &cursiv::ast_type_tag<css3::ast::Ruleset>)
        return nullptr;
      const auto& rule =
          *static_cast<const css3::ast::Ruleset*>(sheet.statements[0]);
      if (rule.declarations.empty() || rule.declarations[0]->value.empty())
        return nullptr;
      const cursiv::AstNode* const value = rule.declarations[0]->value[0];
      if (value->type != &cursiv::ast_type_tag<css3::ast::CalcExpr>)
        return nullptr;
      return static_cast<const css3::ast::CalcExpr*>(value)->sum;
    };

    const css3::ast::CalcSum* const sum1 =
        calc_sum("a { width: calc(1px + 2px * 3); }");
    if (sum1 == nullptr) return fail("calc(1px + 2px * 3): did not build a CalcSum");
    if (sum1->terms.size() != 2 || sum1->ops.size() != 1 ||
        sum1->ops[0] != css3::ast::CalcOp::add)
      return fail("calc(1px + 2px * 3): expected two sum terms joined by +");
    if (sum1->terms[0]->factors.size() != 1)
      return fail("calc(1px + 2px * 3): first term should be the lone 1px");
    if (sum1->terms[1]->factors.size() != 2 || sum1->terms[1]->ops.size() != 1 ||
        sum1->terms[1]->ops[0] != css3::ast::CalcOp::multiply)
      return fail("calc(1px + 2px * 3): second term should be the product 2px * 3");

    const css3::ast::CalcSum* const sum2 =
        calc_sum("a { width: calc(10px - 3px - 2px); }");
    if (sum2 == nullptr) return fail("calc(10px - 3px - 2px): did not build a CalcSum");
    if (sum2->terms.size() != 3 || sum2->ops.size() != 2 ||
        sum2->ops[0] != css3::ast::CalcOp::subtract ||
        sum2->ops[1] != css3::ast::CalcOp::subtract)
      return fail("calc(10px - 3px - 2px): expected three terms, both -, left "
                  "to right");
  }

  // A prelude built only from what `simpleSelectorSequence` can never open
  // with builds `UnknownRuleset`; every real selector shape, including a
  // pseudo-class/element anywhere in it, builds `Ruleset` with a `Selector`
  // -- the two readings can both complete over one span (`a::before`, read
  // as a compound pseudo-element OR as flat ident/colon/colon/ident items),
  // so this is checked by AST type, not merely by a clean parse.
  {
    struct Case {
      const char* text;
      bool wantRuleset;
    };
    const Case cases[] = {
        {"a::before { color: red; }", true},
        {"p::first-line { color: red; }", true},
        {"li::marker { color: red; }", true},
        {".x:hover { color: red; }", true},
        {"div p:first-child { color: red; }", true},
        {"a:hover, a:focus { color: red; }", true},
        {":root { --x: 1; }", true},
        {"::selection { color: red; }", true},
        {"a:not(.b) { color: red; }", true},
        {"{ color: red; }", false},
        {"2 { color: red; }", false},
        {"\"x\" { color: red; }", false},
        {"foo() { color: red; }", false},
        {": { color: red; }", false},
    };
    for (const Case& c : cases) {
      const cursiv::ParseResult result = language.language.parser->parse(c.text);
      if (result.cst().recovered())
        return fail(std::string(c.text) + ": does not parse clean");
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<css3::ast::Stylesheet>)
        return fail(std::string(c.text) + ": built no Stylesheet");
      const auto& sheet = *static_cast<const css3::ast::Stylesheet*>(root);
      if (sheet.statements.size() != 1)
        return fail(std::string(c.text) + ": expected one statement");
      const bool isRuleset = sheet.statements[0]->type ==
                              &cursiv::ast_type_tag<css3::ast::Ruleset>;
      const bool isUnknown = sheet.statements[0]->type ==
                              &cursiv::ast_type_tag<css3::ast::UnknownRuleset>;
      if (c.wantRuleset && !isRuleset)
        return fail(std::string(c.text) + ": expected a Ruleset");
      if (!c.wantRuleset && !isUnknown)
        return fail(std::string(c.text) + ": expected an UnknownRuleset");
    }
  }

  std::puts("css3: the corpus parses");
  return 0;
}
