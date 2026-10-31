#include "lookup_cases.hpp"
// The smoke test of the language: every corpus document parses clean,
// features.less covers what the corpus never exercises (including a
// precedence probe read back at AST level), and example1.less is read back
// field by field to catch a converter that silently mismaps its value.

#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "less/core/CoreServices.hpp"
#include "less/core/ast.hpp"
#include "selector_order_cases.hpp"
#include "resume_regressions.hpp"
#include "detached_argument_cases.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "less: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// A CommandStatement carrying exactly one bare atom: no `+`/`-`, no
// `*`/`/`/`%`. Returns it, or null if the shape does not hold.
const less::ast::Expr *bare_atom(const less::ast::CommandStatement &cmd) {
  if (cmd.terms.size() != 1 || cmd.terms[0]->op.has_value())
    return nullptr;
  const less::ast::MulExpr *mul = cmd.terms[0]->operand;
  if (mul == nullptr || mul->terms.size() != 1 ||
      mul->terms[0]->op.has_value() || mul->terms[0]->items.size() != 1)
    return nullptr;
  return mul->terms[0]->items[0];
}

const char *const kExamples[] = {"example1.less"};

bool adjacency_candidates(const cursiv::Parser& parser) {
  struct Case {
    const char* source;
    unsigned calls;
    unsigned atRules;
  };
  const Case cases[] = {{"@x ();", 0, 1}, {"@x( );", 0, 1},
                        {"@x();", 1, 0}, {"@x(); @x ();", 1, 1}};
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    unsigned calls = 0, atRules = 0;
    for (const auto* node : result.nodes) {
      calls += cursiv::ast_cast<less::ast::DetachedCall>(node) != nullptr;
      atRules += cursiv::ast_cast<less::ast::AtRule>(node) != nullptr;
    }
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        calls == test.calls && atRules == test.atRules)
      continue;
    ok = false;
    std::fprintf(stderr,
                 "less candidate [%s]: accepted=%d recovered=%d diagnostics=%zu calls=%u expected=%u atRules=%u expected=%u\n",
                 test.source, result.accepted, result.cst().recovered(),
                 result.diagnostics.size(), calls, test.calls, atRules, test.atRules);
    for (const auto& diagnostic : result.diagnostics)
      std::fprintf(stderr,
                   "less candidate diagnostic: kind=%u element=%u bytes=%u..%u\n",
                   static_cast<unsigned>(diagnostic.kind),
                   static_cast<unsigned>(diagnostic.element),
                   static_cast<unsigned>(diagnostic.begin),
                   static_cast<unsigned>(diagnostic.end));
  }
  return ok;
}

bool guarded_rulesets(const cursiv::Parser& parser) {
  bool ok = true;
  const auto check = [&](bool value, const char* source, const char* message) {
    if (value) return;
    std::fprintf(stderr, "less guard [%s]: %s\n", source, message);
    ok = false;
  };
  struct Case {
    const char* source;
    bool negated;
    bool comparison;
    less::ast::CompareOp op;
  };
  for (const auto& test : {
           Case{"a when (1=1){color:red;}", false, true, less::ast::CompareOp::eq},
           Case{".panel when not (@enabled=false){width:10px;}", true, true, less::ast::CompareOp::eq},
           Case{"& when (isnumber(1)){x:1;}", false, false, less::ast::CompareOp::eq},
            Case{"a when (1=>0){x:1;}", false, true, less::ast::CompareOp::ge},
            Case{"a when (1=<2){x:1;}", false, true, less::ast::CompareOp::le},
           Case{"outer{inner when (@size>0){x:1;}}", false, true, less::ast::CompareOp::gt}}) {
    const auto result = parser.parse(test.source);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          test.source, "clean guarded ruleset expected");
    unsigned guards = 0;
    for (const auto* node : result.nodes) {
      const auto* selector = cursiv::ast_cast<less::ast::Selector>(node);
      if (!selector || !selector->guard) continue;
      ++guards;
      const auto* guard = selector->guard;
      const auto* selectors = cursiv::ast_cast<less::ast::Selectors>(selector->parent);
      const auto* ruleset = selectors ? cursiv::ast_cast<less::ast::Ruleset>(selectors->parent) : nullptr;
      check(guard->parent == selector && guard->test && guard->test->parent == guard &&
                guard->test->left && guard->test->left->parent == guard->test &&
                (test.comparison ? guard->test->right && guard->test->right->parent == guard->test
                                 : !guard->test->right) &&
                selectors && selectors->items.size() == 1 && selectors->items[0] == selector &&
                ruleset && ruleset->selectors == selectors && ruleset->body &&
                ruleset->body->parent == ruleset,
            test.source, "selector/condition/block ownership lost");
      check(guard->negated == test.negated && guard->test &&
                guard->test->op.has_value() == test.comparison &&
                (!test.comparison || *guard->test->op == test.op),
            test.source, "guard negation/comparison capture lost");
    }
    check(guards == 1, test.source, "exactly one selector guard expected");
  }
  {
    const auto result = parser.parse("a,b{color:red;}");
    const auto* sheet = cursiv::ast_cast<less::ast::Stylesheet>(result.value);
    const auto* ruleset = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<less::ast::Ruleset>(sheet->statements[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
              ruleset && ruleset->selectors && ruleset->selectors->items.size() == 2 &&
              !ruleset->selectors->items[0]->guard && !ruleset->selectors->items[1]->guard,
          "a,b{color:red;}", "ordinary selector list changed");
  }
  {
    const auto result = parser.parse(".m(@x) when (@x>0){x:@x;}");
    const less::ast::MixinDef* mixin = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<less::ast::MixinDef>(node)) mixin = value;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
              mixin && mixin->guard.size() == 1 && mixin->guard[0]->parent == mixin,
          "mixin guard", "existing mixin guard changed");
  }
  for (const auto* source : {"a,b when (1=1){x:1;}",
                             "a when (1=1),b{x:1;}",
                             "a when (1=1),b when (1=1){x:1;}",
                             "a when (){x:1;}",
                              "a when (1=>){x:1;}",
                              "a when (1=<){x:1;}",
                             "a when (1=1) b{x:1;}"}) {
    const auto result = parser.parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          source, "invalid guard selector list or condition accepted");
  }
  return ok;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<less::LessCoreServices,
                                less::LessSharedCoreServices,
                                less::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "@base : 10px; @media screen and (min-width: 10px) { a { width: (1 + "
        "1); } } @custom \"a;b\";");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool group = false, media = false, variable = false, quoted = false;
    for (const auto *node : audit_result.nodes) {
      if (const auto *e = cursiv::ast_cast<less::ast::ParenExpr>(node))
        group = e->value;
      if (const auto *a = cursiv::ast_cast<less::ast::AtRule>(node))
        media = media || (a->name == "media" && a->body &&
                          a->prelude.find("screen") != std::string_view::npos);
    }
    for (const auto *node : audit_result.nodes) {
      if (const auto *v = cursiv::ast_cast<less::ast::VariableDecl>(node))
        variable = variable || (v->name && v->name->name == "base");
      if (const auto *a = cursiv::ast_cast<less::ast::AtRule>(node))
        quoted = quoted || (a->name == "custom" &&
                            a->prelude.find("a;b") != std::string_view::npos);
    }
    if (!group || !media || !variable || !quoted)
      return 1;
  }

  const auto language = cursiv::create_language<less::LessCoreServices,
                                                less::LessSharedCoreServices,
                                                less::install_core_module>();

  const bool candidatesOk = adjacency_candidates(*language.language.parser);
  const bool guardsOk = guarded_rulesets(*language.language.parser);

  if (!less_resume::run(language.language)) return fail("native regressions");
  if (!less_selector_order_cases::run(language.language)) return fail("selector order/combinators");
  if (!less_lookup_cases::run(language.language)) return 1;
  if (!less_detached_arguments::run(language.language)) return fail("detached argument regressions");
  for (const auto source : {
           "@dr:{color:red;};a{@dr();}", "@dr:{color:red;}a{@dr()}",
           ".m(){a{color:red;}}.m();", ".m(@a;@b){x:@a;y:@b;}a{.m(1;2)}",
           ".m(red){color:red;}a{.m(red);}", ".a:extend(.b){}", ".a:extend(all .b){}",
           "a{background:darken(@base,10%);color: lighten(@base,20%);}",
           "a{&:extend(.b);}", "a:nth-child(2n+1){color:red;}", ":is(.a,.b){color:red;}",
           ":not([disabled]){color:red;}", "[a^=b][a$=c][a*=d]{color:red;}",
           ".a[attr].b{color:red;}", "[a=b i]{color:red;}",
           "@import (optional) \"x.less\";", "@import \"x.css\" screen and (min-width:600px);"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail(std::string("native form rejected: ") + source);
  }
  {
    const auto result = language.language.parser->parse(
        "@dr:{color:red;};a{@dr()} .x:extend(.b all){} "
        "a:nth-child(2n+1)[attr].b{color:red;} @import (optional) \"x.css\" screen and (min-width:600px);");
    bool detached = false, call = false, extension = false, pseudo = false, attribute = false, media = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = cursiv::ast_cast<less::ast::DetachedRulesetDecl>(node))
        detached |= value->name && value->name->name == "dr" && value->body;
      if (const auto* value = cursiv::ast_cast<less::ast::DetachedCall>(node))
        call |= value->target && value->target->name == "dr";
      if (const auto* value = cursiv::ast_cast<less::ast::ExtendElement>(node))
        extension |= value->targets.size() == 1 && value->targets[0]->option == "all" &&
                     value->targets[0]->selector && value->targets[0]->selector->elements.size() == 1;
      if (const auto* value = cursiv::ast_cast<less::ast::Pseudo>(node))
        pseudo |= value->name == "nth-child" && value->hasArguments && value->argumentValues && value->argumentValues->items.size() == 1;
      if (const auto* value = cursiv::ast_cast<less::ast::Selector>(node))
        attribute |= value->elements.size() == 4 &&
            cursiv::ast_cast<less::ast::NamedElement>(value->elements[0]) &&
            cursiv::ast_cast<less::ast::Pseudo>(value->elements[1]) &&
            cursiv::ast_cast<less::ast::Attrib>(value->elements[2]) &&
            cursiv::ast_cast<less::ast::Attrib>(value->elements[2])->name == "attr" &&
            cursiv::ast_cast<less::ast::NamedElement>(value->elements[3]);
      if (const auto* value = cursiv::ast_cast<less::ast::Import>(node))
        media |= value->options.size() == 1 && value->options[0] == less::ast::ImportOption::optional_ &&
                 value->mediaQuery == "screen and (min-width:600px)";
    }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !detached || !call || !extension || !pseudo || !attribute || !media) {
      std::fprintf(stderr, "native AST flags: detached=%d call=%d extend=%d pseudo=%d attribute=%d media=%d\n", detached, call, extension, pseudo, attribute, media);
      for (const auto* n : result.nodes) {
        if (const auto* v = cursiv::ast_cast<less::ast::Import>(n))
          std::fprintf(stderr, "mediaQuery=[%.*s]\n", static_cast<int>(v->mediaQuery.size()), v->mediaQuery.data());
      }
      return fail("native detached/extend/pseudo/attribute/import AST was lost");
    }
  }
  {
    const auto result = language.language.parser->parse("@x(); @x (); @x( ); @1-x:1; a{x:@1-x;}");
    unsigned calls = 0, atRules = 0;
    for (const auto* node : result.nodes) {
      calls += cursiv::ast_cast<less::ast::DetachedCall>(node) != nullptr;
      atRules += cursiv::ast_cast<less::ast::AtRule>(node) != nullptr;
    }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || calls != 1 || atRules != 2) {
      std::fprintf(stderr,
                   "less adjacency: accepted=%d recovered=%d diagnostics=%zu calls=%u atRules=%u\n",
                   result.accepted, result.cst().recovered(),
                   result.diagnostics.size(), calls, atRules);
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr,
                     "less diagnostic: kind=%u element=%u bytes=%u..%u\n",
                     static_cast<unsigned>(diagnostic.kind),
                     static_cast<unsigned>(diagnostic.element),
                     static_cast<unsigned>(diagnostic.begin),
                     static_cast<unsigned>(diagnostic.end));
      for (const auto* node : result.nodes) {
        if (const auto* call = cursiv::ast_cast<less::ast::DetachedCall>(node))
          std::fprintf(stderr, "less call target=[%.*s]\n",
                       call->target ? static_cast<int>(call->target->name.size()) : 0,
                       call->target && !call->target->name.empty() ? call->target->name.data() : "");
        if (const auto* atRule = cursiv::ast_cast<less::ast::AtRule>(node))
          std::fprintf(stderr, "less at-rule name=[%.*s] prelude=[%.*s]\n",
                       static_cast<int>(atRule->name.size()),
                       atRule->name.empty() ? "" : atRule->name.data(),
                       static_cast<int>(atRule->prelude.size()),
                       atRule->prelude.empty() ? "" : atRule->prelude.data());
      }
      return fail("native variable-call adjacency or digit-start variable AST was lost");
    }
  }
  for (const auto source : {"@ x:1;", "@@x:1;", "@@x();", "a{x:@@@x;}", ".@{ x }{}", "[a i]{}"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("non-native variable spelling or attribute flag accepted");
  }
  {
    const auto result = language.language.parser->parse("a{.m(1,2;3);}");
    bool groups = false;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<less::ast::MixinRef>(node))
        groups |= value->args.empty() && value->argumentGroups.size() == 2 &&
                  value->argumentGroups[0]->items.size() == 2 && value->argumentGroups[1]->items.size() == 1;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !groups)
      return fail("native semicolon argument grouping lost");
  }
  {
    const auto result = language.language.parser->parse(".a:extend(all){}");
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("extend option without a target selector accepted");
  }

  for (const char *const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<less::ast::Stylesheet>)
      return fail(std::string(name) + ": built no Stylesheet");
  }

  // features.less: every construct not in the corpus.
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty())
      return fail("features.less: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      for (const auto& diagnostic : result.diagnostics) {
        const auto start = diagnostic.begin > 25 ? diagnostic.begin - 25 : 0;
        const auto context = text.substr(start, 100);
        std::fprintf(stderr, "features diagnostic kind=%u begin=%u end=%u near=[%s]\n", static_cast<unsigned>(diagnostic.kind), diagnostic.begin, diagnostic.end, context.c_str());
      }
      return fail("features.less: does not parse clean");
    }
    if (!result.diagnostics.empty())
      return fail("features.less: has diagnostics");
    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<less::ast::Stylesheet>)
      return fail("features.less: built no Stylesheet");
    const auto &sheet = *static_cast<const less::ast::Stylesheet *>(root);
    if (sheet.statements.size() < 5)
      return fail("features.less: too few top-level statements");

    // .precedence: 2 + 3*4, 10 - 2*3, 2-3-4 -- confirms `*` binds tighter
    // than `+`/`-`, and `+`/`-` chain left-associatively.
    const less::ast::Ruleset *precedence = nullptr;
    for (const auto *const stmt : sheet.statements) {
      if (stmt->type != &cursiv::ast_type_tag<less::ast::Ruleset>)
        continue;
      const auto &rs = *static_cast<const less::ast::Ruleset *>(stmt);
      if (rs.selectors == nullptr || rs.selectors->items.size() != 1 ||
          rs.selectors->items[0]->elements.size() != 1 ||
          rs.selectors->items[0]->elements[0]->type !=
              &cursiv::ast_type_tag<less::ast::NamedElement>)
        continue;
      const auto &elem = *static_cast<const less::ast::NamedElement *>(
          rs.selectors->items[0]->elements[0]);
      if (elem.name != nullptr && elem.name->parts.size() == 1 &&
          std::holds_alternative<std::string_view>(elem.name->parts[0]) &&
          std::get<std::string_view>(elem.name->parts[0]) == ".precedence") {
        precedence = &rs;
        break;
      }
    }
    if (precedence == nullptr)
      return fail("features.less: no .precedence ruleset found");
    if (precedence->body == nullptr || precedence->body->items.size() != 3)
      return fail("features.less: .precedence should hold three properties");

    auto property_value = [&](int i) -> const less::ast::CommandStatement * {
      const auto *const p =
          std::get_if<less::ast::Property *>(&precedence->body->items[i]);
      if (p == nullptr || *p == nullptr || (*p)->value == nullptr ||
          (*p)->value->items.size() != 1)
        return nullptr;
      return (*p)->value->items[0];
    };

    // a: 2 + 3 * 4 -- one "+" term, whose operand is a two-term MulExpr
    // (3, *4): `*` nests inside `+`, not a flat sibling of it.
    const less::ast::CommandStatement *const a = property_value(0);
    if (a == nullptr || a->terms.size() != 2 || a->terms[0]->op.has_value() ||
        a->terms[1]->op != less::ast::MathOp::plus ||
        a->terms[0]->operand == nullptr ||
        a->terms[0]->operand->terms.size() != 1 ||
        a->terms[1]->operand == nullptr ||
        a->terms[1]->operand->terms.size() != 2 ||
        a->terms[1]->operand->terms[1]->op != less::ast::MathOp::times)
      return fail("features.less: 2 + 3 * 4 does not bind * tighter");

    // b: 10 - 2 * 3 -- symmetric, "-" instead of "+".
    const less::ast::CommandStatement *const b = property_value(1);
    if (b == nullptr || b->terms.size() != 2 ||
        b->terms[1]->op != less::ast::MathOp::minus ||
        b->terms[1]->operand == nullptr ||
        b->terms[1]->operand->terms.size() != 2 ||
        b->terms[1]->operand->terms[1]->op != less::ast::MathOp::times)
      return fail("features.less: 10 - 2 * 3 does not bind * tighter");

    // c: 2 - 3 - 4 -- three flat, left-associative "-" terms: nesting on
    // the right would instead read 2 - (3 - 4), a different value.
    const less::ast::CommandStatement *const c = property_value(2);
    if (c == nullptr || c->terms.size() != 3 || c->terms[0]->op.has_value() ||
        c->terms[1]->op != less::ast::MathOp::minus ||
        c->terms[2]->op != less::ast::MathOp::minus)
      return fail(
          "features.less: 2 - 3 - 4 is not a flat, left-associative chain");
  }

  // example1.less, read back field by field: two variable declarations,
  // then two rulesets whose property values reference them.
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example1.less");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto &sheet = *static_cast<const less::ast::Stylesheet *>(result.value);
  if (sheet.statements.size() != 4)
    return fail("example1.less: expected four statements");

  if (sheet.statements[0]->type !=
      &cursiv::ast_type_tag<less::ast::VariableDecl>)
    return fail("example1.less: statement 0 is not a VariableDecl");
  const auto &decl0 =
      *static_cast<const less::ast::VariableDecl *>(sheet.statements[0]);
  if (decl0.name == nullptr || decl0.name->name != "primarycolor")
    return fail("example1.less: wrong variable name");
  if (decl0.value == nullptr || decl0.value->items.size() != 1)
    return fail("example1.less: expected one value in the list");
  const less::ast::CommandStatement &cmd0 = *decl0.value->items[0];
  const less::ast::Expr *const atom0 = bare_atom(cmd0);
  if (atom0 == nullptr ||
      atom0->type != &cursiv::ast_type_tag<less::ast::ColorLit>)
    return fail("example1.less: variable value is not a color");
  const auto &color0 = *static_cast<const less::ast::ColorLit *>(atom0);
  if (color0.value != "#FF7F50")
    return fail("example1.less: wrong color");

  if (sheet.statements[2]->type != &cursiv::ast_type_tag<less::ast::Ruleset>)
    return fail("example1.less: statement 2 is not a Ruleset");
  const auto &h1 =
      *static_cast<const less::ast::Ruleset *>(sheet.statements[2]);
  if (h1.selectors == nullptr || h1.selectors->items.size() != 1 ||
      h1.selectors->items[0]->elements.size() != 1)
    return fail("example1.less: h1 selector shape is wrong");
  const auto &h1Elem = *static_cast<const less::ast::NamedElement *>(
      h1.selectors->items[0]->elements[0]);
  if (h1Elem.name == nullptr || h1Elem.name->parts.size() != 1 ||
      std::get<std::string_view>(h1Elem.name->parts[0]) != "h1")
    return fail("example1.less: wrong selector name");

  if (h1.body == nullptr || h1.body->items.size() != 1)
    return fail("example1.less: h1 block should hold one property");
  const auto *const propPtr =
      std::get_if<less::ast::Property *>(&h1.body->items[0]);
  if (propPtr == nullptr || *propPtr == nullptr)
    return fail("example1.less: h1 block item is not a property");
  const less::ast::Property &color = **propPtr;
  if (color.value == nullptr || color.value->items.size() != 1)
    return fail("example1.less: color property value shape is wrong");
  const less::ast::Expr *const valueNode = bare_atom(*color.value->items[0]);
  if (valueNode == nullptr ||
      valueNode->type != &cursiv::ast_type_tag<less::ast::VarValue>)
    return fail("example1.less: color value is not a variable reference");
  const auto &varRef = *static_cast<const less::ast::VarValue *>(valueNode);
  if (varRef.name == nullptr || varRef.name->name != "primarycolor")
    return fail("example1.less: color references the wrong variable");

  std::puts("less: the corpus parses");
  return candidatesOk && guardsOk ? 0 : fail("adjacency/guard syntax or AST checks failed");
}
