#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "informix/core/ast.hpp"

namespace informix_native_followup {
namespace a = informix::ast;
inline bool number(const a::Expr* node, double value) {
  const auto* n = cursiv::ast_cast<a::NumberLit>(node);
  return n && !n->negative && n->value == value;
}
inline bool name(const a::Expr* node, std::string_view value) {
  const auto* n = cursiv::ast_cast<a::NameRef>(node);
  return n && !n->base && n->name == value;
}
template<class Language> bool run(const Language& language) {
  bool ok = true;
  auto check = [&](bool value, const char* what) {
    if (!value) {std::fprintf(stderr, "informix native: %s\n", what);ok = false;}
  };
  auto expression = [&](std::string_view text, auto inspect) {
    const std::string source = "MAIN LET x = " + std::string(text) + " END MAIN";
    const auto result = language.language.parser->parse(source);
    const auto* root = result.template value_as<a::Program>();
    const auto* let = root && root->main && root->main->body.size() == 1
        ? cursiv::ast_cast<a::LetStmt>(root->main->body[0]) : nullptr;
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
                       let && let->values.size() == 1;
    check(clean, "clean expression and one owned LET value");
    if (!clean) {
      std::fprintf(stderr, "informix expression: %.*s\n", int(text.size()), text.data());
      for (const auto& d : result.diagnostics)
        std::fprintf(stderr, "kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
    }
    if (let && let->values.size() == 1) {
      check(let->values[0]->parent == let, "LET expression parent");
      inspect(let->values[0]);
    }
  };
  expression("2 ** 3 ** 2", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::BinaryExpr>(node);
    const auto* left = p ? cursiv::ast_cast<a::BinaryExpr>(p->left) : nullptr;
    check(p && p->op == a::Op::power && number(p->right, 2) && left &&
          left->op == a::Op::power && number(left->left, 2) && number(left->right, 3),
          "power is left associative in the native table");
  });
  expression("8 * 5 MOD 3", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::BinaryExpr>(node);
    const auto* right = p ? cursiv::ast_cast<a::BinaryExpr>(p->right) : nullptr;
    check(p && p->op == a::Op::multiply && number(p->left, 8) && right &&
          right->op == a::Op::mod && number(right->left, 5), "MOD binds before multiply");
  });
  expression("NOT a = b AND c", [&](const auto* node) {
    const auto* and_ = cursiv::ast_cast<a::BinaryExpr>(node);
    const auto* not_ = and_ ? cursiv::ast_cast<a::UnaryExpr>(and_->left) : nullptr;
    const auto* equal = not_ ? cursiv::ast_cast<a::BinaryExpr>(not_->operand) : nullptr;
    check(and_ && and_->op == a::Op::logical_and && name(and_->right, "c") &&
          not_ && not_->op == "NOT" && equal && equal->op == a::Op::eq,
          "NOT binds after comparisons and before AND");
  });
  expression("a == b", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::BinaryExpr>(node);
    check(p && p->op == a::Op::eq && name(p->left, "a") && name(p->right, "b"), "native double equality");
  });
  expression("a NOT IN (1, 2)", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::InExpr>(node);
    check(p && p->negated && name(p->operand, "a") && p->values.size() == 2 &&
          number(p->values[0], 1) && number(p->values[1], 2) && !p->subquery,
          "IN list and negation retained");
  });
  expression("a IN (SELECT id FROM t)", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::InExpr>(node);
    check(p && !p->negated && p->values.empty() && p->subquery &&
          p->subquery->parent == p, "IN subquery retained");
  });
  expression("a > SOME (SELECT id FROM t)", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::QuantifiedExpr>(node);
    check(p && p->some && !p->all && !p->any && p->op == a::CompareOp::gt &&
          p->subquery, "SOME quantifier retained");
  });
  expression(R"4gl(a LIKE "!_%" ESCAPE "!")4gl", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::LikeExpr>(node);
    check(p && !p->matches && p->escape && p->escape->value == "!" &&
          p->escape->parent == p, "LIKE ESCAPE literal retained");
  });
  expression("a LIKE b = c", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::BinaryExpr>(node);
    check(p && p->op == a::Op::eq && cursiv::ast_cast<a::LikeExpr>(p->left) &&
          name(p->right, "c"), "LIKE binds before comparison");
  });
  expression("a IN (1) BETWEEN 0 AND 1 IS NULL", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::IsNullExpr>(node);
    const auto* between = p ? cursiv::ast_cast<a::BetweenExpr>(p->operand) : nullptr;
    check(between && cursiv::ast_cast<a::InExpr>(between->operand),
          "IN, BETWEEN and IS NULL have distinct native priorities");
  });
  expression("a || b CLIPPED", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::FormatExpr>(node);
    const auto* concat = p ? cursiv::ast_cast<a::BinaryExpr>(p->operand) : nullptr;
    check(p && p->op == a::FormatOperator::clipped && concat &&
          concat->op == a::Op::concat, "CLIPPED binds after concatenation");
  });
  expression(R"4gl(a USING "####")4gl", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::FormatExpr>(node);
    const auto* format = p ? cursiv::ast_cast<a::StringLit>(p->format) : nullptr;
    check(p && p->op == a::FormatOperator::using_ && name(p->operand, "a") &&
          format && format->value == "####", "USING format string retained");
  });
  expression("ASCII 65", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::FormatExpr>(node);
    check(p && p->op == a::FormatOperator::ascii && number(p->operand, 65), "ASCII prefix operator");
  });
  expression("ASCII(65)", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::FuncCall>(node);
    check(p && p->name == "ASCII" && p->args.size() == 1 && number(p->args[0], 65),
          "existing parenthesized built-in call retains its AST");
  });
  expression("CURRENT YEAR TO SECOND", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::TemporalExpr>(node);
    check(p && p->kind == a::TemporalKind::current && p->payload.empty() &&
          p->from == a::TimeUnit::year && p->to == a::TimeUnit::second, "CURRENT qualifier retained");
  });
  expression("DATETIME(2024-02-29 12:34:56) YEAR TO SECOND", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::TemporalExpr>(node);
    check(p && p->kind == a::TemporalKind::datetime_literal &&
          p->payload == "(2024-02-29 12:34:56)" && p->from == a::TimeUnit::year &&
          p->to == a::TimeUnit::second, "DATETIME literal source and qualifier retained");
  });
  expression("INTERVAL(2 06:00) DAY(2) TO MINUTE", [&](const auto* node) {
    const auto* p = cursiv::ast_cast<a::TemporalExpr>(node);
    check(p && p->kind == a::TemporalKind::interval_literal && p->payload == "(2 06:00)" &&
          p->from == a::TimeUnit::day && p->fromPrecision == 2 && p->to == a::TimeUnit::minute,
          "INTERVAL literal and precision retained");
  });

  constexpr std::string_view source = R"4gl(
DATABASE stores;
{ Native brace comment spans
  lines and hides LET discarded = 999 }
MAIN
DEFINE a, b, n INTEGER;
DEFINE r REAL;
DEFINE rec RECORD LIKE owner.customer.*;
DEFINE col LIKE owner.customer.id;
DEFINE text CHAR(40);
LET text = "a\"b\\c;{#--}";
LET text = 'it\'s';
--@ LET a = 17;
--# LET a = 999;
LABEL again: LET b = 1;
SET LOCK MODE TO WAIT 5;
OPEN WINDOW w AT 1,2 WITH 10 ROWS,40 COLUMNS
  ATTRIBUTE (PROMPT LINE LAST-2, FORM LINE FIRST, COMMENT LINE OFF, MESSAGE LINE n);
MENU "choices"
  BEFORE MENU LET a = 3;
  COMMAND "Done" "Exit menu" EXIT MENU
  COMMAND KEY("q") EXIT MENU
END MENU;
SELECT id FROM stores@remote:owner.customer;
SELECT id FROM "owner".customer;
CREATE TABLE blobs (payload BYTE IN TABLE);
END MAIN;
REPORT report_name(p)
DEFINE p CHAR(40);
FORMAT
 FIRST PAGE HEADER
  PRINT "one";
 PAGE HEADER
  PRINT p # A semicolon in a comment ; must not suppress newline.
 ON EVERY ROW
  PRINT 3 SPACES, p WORDWRAP RIGHT MARGIN 15;
END REPORT;
)4gl";
  const auto result = language.language.parser->parse(source);
  const auto* root = result.template value_as<a::Program>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        root && root->main && root->definitions.size() == 1, "native declaration/statement source parses clean");
  if (result.cst().recovered() || !result.diagnostics.empty())
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "informix declaration: kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  bool escaped = false, single = false, classic = false, discarded = false, real = false;
  bool record = false, like = false, label = false, wait = false, relative = false, first = false;
  bool off = false, variable = false, menu = false, visible = false, hidden = false;
  bool qualified = false, quoted = false, table = false;
  bool firstPage = false, normalPage = false, spaces = false, wrap = false, suppressed = false, newline = false;
  for (const auto* node : result.nodes) {
    if (const auto* p = cursiv::ast_cast<a::StringLit>(node)) {
      escaped |= p->value == "a\"b\\c;{#--}";single |= p->value == "it's";
    }
    if (const auto* p = cursiv::ast_cast<a::LetStmt>(node); p && p->values.size() == 1) {
      classic |= name(p->target, "a") && number(p->values[0], 17);
      discarded |= number(p->values[0], 999);
    }
    if (const auto* p = cursiv::ast_cast<a::ScalarType>(node)) real |= p->keyword == "REAL";
    if (const auto* p = cursiv::ast_cast<a::RecordType>(node)) record |= p->likeTable == "owner.customer";
    if (const auto* p = cursiv::ast_cast<a::LikeType>(node)) like |= p->table == "owner.customer" && p->column == "id";
    if (const auto* p = cursiv::ast_cast<a::LabeledStmt>(node)) label |= p->label == "again" && p->inner;
    if (const auto* p = cursiv::ast_cast<a::SetLockModeStmt>(node)) wait |= p->wait && number(p->seconds, 5);
    if (const auto* p = cursiv::ast_cast<a::WindowAttrClause>(node)) {
      relative |= p->position == a::WindowLinePosition::last && number(p->offset, 2);
      first |= p->position == a::WindowLinePosition::first && !p->line && !p->offset;
      off |= p->position == a::WindowLinePosition::off;
      variable |= p->position == a::WindowLinePosition::absolute && name(p->line, "n");
    }
    if (const auto* p = cursiv::ast_cast<a::MenuEvent>(node)) {
      menu |= p->beforeMenu && p->body.size() == 1;
      const auto* option = cursiv::ast_cast<a::StringLit>(p->optionName);
      const auto* description = cursiv::ast_cast<a::StringLit>(p->description);
      visible |= option && option->value == "Done" && description && description->value == "Exit menu";
      hidden |= !p->beforeMenu && !p->optionName && !p->description && p->keys.size() == 1 && p->body.size() == 1;
    }
    if (const auto* p = cursiv::ast_cast<a::TableRef>(node)) {
      qualified |= p->qualifier == "stores" && p->server == "remote" && p->owner == "owner" && p->name == "customer";
      quoted |= p->qualifier.empty() && p->server.empty() && p->owner == "owner" && p->name == "customer";
    }
    if (const auto* p = cursiv::ast_cast<a::ColumnDef>(node)) table |= p->isByte && p->inTable && p->storeIn.empty();
    if (const auto* p = cursiv::ast_cast<a::FormatSection>(node)) {
      firstPage |= p->kind == a::FormatSectionKind::pageHeader && p->firstPage;
      normalPage |= p->kind == a::FormatSectionKind::pageHeader && !p->firstPage;
    }
    if (const auto* p = cursiv::ast_cast<a::FormatExpr>(node)) {
      spaces |= p->op == a::FormatOperator::spaces && number(p->operand, 3);
      wrap |= p->op == a::FormatOperator::wordwrap && name(p->operand, "p") && number(p->rightMargin, 15);
    }
    if (const auto* p = cursiv::ast_cast<a::PrintStmt>(node)) {
      suppressed |= p->suppressNewline;
      newline |= !p->suppressNewline && p->items.size() == 1 && name(p->items[0]->value, "p");
    }
  }
  check(escaped && single && classic && !discarded && real && record && like && label && wait,
        "native lexical/module/declaration fields retained");
  check(relative && first && off && variable && menu && visible && hidden && qualified && quoted && table,
        "native window/menu/table fields retained");
  check(firstPage && normalPage && spaces && wrap && suppressed && newline,
        "native report fields and PRINT semicolon behavior retained");
  if (root && !result.cst().recovered() && result.diagnostics.empty()) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false;return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *root);
    check(owned && seen.size() == result.nodes.size(), "every selected AST node is uniquely owned and contained in its parent CST");
  }
  const auto module = language.language.parser->parse("FUNCTION helper(x) DEFINE x INTEGER RETURN x END FUNCTION;");
  const auto* mod = module.template value_as<a::Program>();
  check(module.accepted && !module.cst().recovered() && module.diagnostics.empty() && mod &&
        !mod->main && mod->definitions.size() == 1, "function-only source module does not require MAIN");
  const auto emptyUnique = language.language.parser->parse("MAIN CREATE TABLE t (UNIQUE ()) END MAIN");
  check(!emptyUnique.accepted || emptyUnique.cst().recovered() || !emptyUnique.diagnostics.empty(),
        "UNIQUE requires at least one column");
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{R"4gl(MAIN LET x = a LIKE "a" ESCAPE "xy" END MAIN)4gl", "informix.escape"},
      Invalid{R"4gl(MAIN LET x = a LIKE "a" ESCAPE "" END MAIN)4gl", "informix.escape"},
      Invalid{R"4gl(MAIN MENU "m" COMMAND KEY("a","b","c","d","e") "visible" EXIT MENU END MENU END MAIN)4gl", "informix.menu-keys"},
      Invalid{R"4gl(MAIN MENU "m" COMMAND "visible" END MENU END MAIN)4gl", "informix.menu-body"},
      Invalid{R"4gl(MAIN MENU "m" COMMAND KEY("q") EXIT MENU END MENU END MAIN)4gl", "informix.menu-options"}}) {
    const auto validation = cursiv::test::validationHelper(language.language, c.source);
    check(validation.document->parseResult.accepted && !validation.document->parseResult.cst().recovered() &&
          validation.diagnostics.size() == 1 && validation.diagnostics[0].code == c.code,
          c.code);
  }
  const auto validEscape = cursiv::test::validationHelper(language.language,
      R"4gl(MAIN LET x = a LIKE "x" ESCAPE "é" END MAIN)4gl");
  check(validEscape.diagnostics.empty(), "one UTF-8 scalar is a valid escape character");
  return ok;
}
} // namespace informix_native_followup
