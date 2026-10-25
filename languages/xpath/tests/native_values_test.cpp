#include "NativeCases.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "xpath/core/CoreServices.hpp"
#include "xpath/core/ast.hpp"
#include <cstdio>
#include <string_view>
#include <unordered_set>
namespace a = xpath::ast;
int main() {
  const auto lang = cursiv::create_language<xpath::XpathCoreServices,
                                            xpath::XpathSharedCoreServices,
                                            xpath::install_core_module>();
  bool ok = true;
  auto check = [&](bool condition, const char *id, const char *detail) {
    if (!condition) {
      std::fprintf(stderr, "xpath %s: %s\n", id, detail);
      ok = false;
    }
  };
  auto unwrap = [](a::Expr *p) -> a::Expr * {
    auto *s = cursiv::ast_cast<a::SequenceExpr>(p);
    return s && s->items.size() == 1 ? s->items[0] : p;
  };
  for (const auto &c : cases) {
    const std::string_view id = c.name, text = c.source;
    auto r = lang.language.parser->parse(text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), c.name,
          "clean parse");
    check(r.cst().text() == text, c.name, "source");
    auto *d = cursiv::ast_cast<a::Document>(r.value);
    check(d && !d->expressions.empty(), c.name, "document");
    if (!d || d->expressions.empty())
      continue;
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      lang.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    check(owned && seen.size() == r.nodes.size(), c.name, "complete ownership");
    a::Expr *e = unwrap(d->expressions[0]);
    if (id.starts_with("integer")) {
      auto *n = cursiv::ast_cast<a::IntegerLit>(e);
      check(n && n->raw == text, c.name, "exact unbounded integer");
    }
    if (id.starts_with("decimal")) {
      auto *n = cursiv::ast_cast<a::DecimalLit>(e);
      check(n && n->raw == text, c.name, "exact decimal");
    }
    if (id.starts_with("double")) {
      auto *n = cursiv::ast_cast<a::DoubleLit>(e);
      check(n && n->raw == text, c.name, "exact exponent literal");
    }
    if (id == "arity-huge") {
      auto *n = cursiv::ast_cast<a::NamedFunctionRef>(e);
      check(n && n->name == "fn:concat" && n->arity == "18446744073709551616",
            c.name, "unbounded raw arity");
    }
    if (id == "precedence") {
      auto *n = cursiv::ast_cast<a::BinaryExpr>(e);
      auto *l = n ? cursiv::ast_cast<a::BinaryExpr>(n->left) : nullptr;
      auto *m = l ? cursiv::ast_cast<a::BinaryExpr>(l->right) : nullptr;
      auto *q = n ? cursiv::ast_cast<a::BinaryExpr>(n->right) : nullptr;
      check(n && n->op == a::BinOp::sub && l && l->op == a::BinOp::add && m &&
                m->op == a::BinOp::mul && q && q->op == a::BinOp::div_,
            c.name, "precedence tree");
    }
    if (id == "left-assoc") {
      auto *n = cursiv::ast_cast<a::BinaryExpr>(e);
      auto *l = n ? cursiv::ast_cast<a::BinaryExpr>(n->left) : nullptr;
      check(n && l && n->op == a::BinOp::sub && l->op == a::BinOp::sub, c.name,
            "left association");
    }
    if (id == "unary-arrow") {
      auto *n = cursiv::ast_cast<a::ArrowExpr>(e);
      check(n && cursiv::ast_cast<a::UnaryExpr>(n->base) &&
                n->steps.size() == 1 && n->steps[0]->name == "abs" &&
                n->steps[0]->args && n->steps[0]->args->positional.empty(),
            c.name, "unary and arrow binding");
    }
    if (id == "map-lookup") {
      auto *n = cursiv::ast_cast<a::PostfixExpr>(e);
      auto *m = n ? cursiv::ast_cast<a::MapConstructor>(n->base) : nullptr;
      check(m && m->entries.size() == 2 && n->suffixes.size() == 1, c.name,
            "map entries and suffix");
      if (m && m->entries.size() == 2) {
        auto *k = cursiv::ast_cast<a::StringLit>(m->entries[1]->key);
        auto *v = cursiv::ast_cast<a::IntegerLit>(m->entries[1]->value);
        check(k && k->value == "b" && v && v->raw == "2", c.name,
              "second map key/value");
      }
    }
    if (id == "instance-function-zero") {
      auto *n = cursiv::ast_cast<a::InstanceOfExpr>(e);
      auto *t = n && n->type ? n->type->itemType : nullptr;
      check(t && t->name == "function" && t->params.empty() && t->returnType &&
                t->returnType->itemType &&
                t->returnType->itemType->name == "xs:integer",
            c.name, "zero parameter function type and return");
    }
    auto v = cursiv::test::validationHelper(lang.language, std::string(text));
    const char *expected =
        id == "duplicate-param" ? "xpath.duplicate-parameter"
        : (id == "reserved-switch" || id == "reserved-typeswitch")
            ? "xpath.reserved-function"
            : nullptr;
    check(expected ? v.diagnostics.size() == 1 : v.diagnostics.empty(), c.name,
          "runtime count");
    if (expected && v.diagnostics.size() == 1) {
      const auto &diag = v.diagnostics[0];
      check(diag.code && diag.code->isString() &&
                diag.code->string() == expected &&
                diag.range.begin < diag.range.end && diag.range.end <= text.size(),
            c.name, "runtime code and range");
    }
  }
  return ok ? 0 : 1;
}
