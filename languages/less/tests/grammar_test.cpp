// The smoke test of the language: every corpus document parses clean,
// features.less covers what the corpus never exercises (including a
// precedence probe read back at AST level), and example1.less is read back
// field by field to catch a converter that silently mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "less/core/CoreServices.hpp"
#include "less/core/ast.hpp"
#include "resume_regressions.hpp"

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

  if (!less_resume::run(language.language)) return fail("native regressions");

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
    if (result.cst().recovered())
      return fail("features.less: does not parse clean");
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
  return 0;
}
