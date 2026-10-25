#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "blueprint/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
namespace a = blueprint::ast;
int main() {
  const auto lang =
      cursiv::create_language<blueprint::BlueprintCoreServices,
                              blueprint::BlueprintSharedCoreServices,
                              blueprint::install_core_module>();
  bool ok = true;
  auto check = [&](bool yes, const char *id, const char *what) {
    if (!yes) {
      std::fprintf(stderr, "blueprint %s: %s\n", id, what);
      ok = false;
    }
  };
  for (const auto &c : cases) {
    const std::string_view id = c.name, text = c.source;
    auto r = lang.language.parser->parse(text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), c.name,
          "clean parse with AST preserved");
    check(r.cst().text() == text, c.name, "exact source");
    auto *d = cursiv::ast_cast<a::Document>(r.value);
    check(d, c.name, "Document root");
    if (!d)
      continue;
    if (c.expected)
      check(audit::view(*d).toJsonString({.pretty = false}) == c.expected,
            c.name, "full native value tree");
    check(d->bom == (id == "bom"), c.name, "BOM marker");
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *s = cursiv::ast_cast<a::StringLit>(&n))
        check(s->raw.size() >= 2 && text.find(s->raw) != text.npos, c.name,
              "whole quoted source");
      lang.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    check(owned && seen.size() == r.nodes.size(), c.name, "complete ownership");
    a::Assignment *assign = nullptr;
    a::Module *module = nullptr;
    if (!d->definitions.empty()) {
      if (auto *x = std::get_if<a::Assignment *>(&d->definitions[0]))
        assign = *x;
      if (auto *x = std::get_if<a::Module *>(&d->definitions[0]))
        module = *x;
    }
    if (id.starts_with("int-") && assign && assign->value &&
        assign->value->terms.size() == 1) {
      auto *n = std::get_if<a::IntegerLit *>(&assign->value->terms[0]);
      check(n && *n &&
                (*n)->raw ==
                    (id == "int-spaced-minus" ? "-123" : text.substr(2)),
            c.name, "raw integer spelling");
    }
    if ((id.starts_with("string-") || id == "raw-string" ||
         id == "raw-string-lines") &&
        assign && assign->value && assign->value->terms.size() == 1) {
      auto *n = std::get_if<a::StringLit *>(&assign->value->terms[0]);
      check(n && *n && (*n)->raw == text.substr(2), c.name,
            "raw string spelling");
      if (n && *n && id == "string-nul")
        check(blueprint::string_value(**n).text == std::string(3, '\0'), c.name,
              "all three NUL bytes retained");
      if (n && *n && id == "raw-string-lines")
        check(blueprint::string_value(**n).text == "a\nbc", c.name,
              "raw CR removal");
    }
    if (id == "module-legacy" || id == "module-legacy-bad-separator")
      check(module && module->parenthesized && !module->properties.empty() &&
                module->properties[0]->equals == (id == "module-legacy"),
            c.name, "module delimiters and property separator");
    if (id == "module-brace-bad-separator")
      check(module && !module->parenthesized &&
                module->properties.size() == 1 && module->properties[0]->equals,
            c.name, "invalid separator preserved");
    if (id == "append-spaced") {
      auto *n = d->definitions.size() == 2
                    ? std::get_if<a::Assignment *>(&d->definitions[1])
                    : nullptr;
      check(n && *n && (*n)->append, c.name, "spaced append operator");
    }
    if (id == "select-tuple" && assign && assign->value) {
      auto *n = std::get_if<a::Select *>(&assign->value->terms[0]);
      check(n && *n && (*n)->tuple && (*n)->conditions.size() == 2 &&
                (*n)->cases.size() == 2 && (*n)->cases[0]->tuple &&
                (*n)->cases[0]->patterns.size() == 2 && (*n)->cases[1]->comma,
            c.name, "condition and pattern tuples/comma");
    }
    auto v = cursiv::test::validationHelper(lang.language, std::string(text));
    std::vector<std::string> codes;
    for (const auto &x : v.diagnostics) {
      check(x.code && x.code->isString() && x.range.begin < x.range.end &&
                x.range.end <= text.size(),
            c.name, "bounded runtime diagnostic");
      if (x.code && x.code->isString())
        codes.push_back(x.code->string());
    }
    std::sort(codes.begin(), codes.end());
    std::string actual;
    for (auto &x : codes) {
      if (!actual.empty())
        actual += ",";
      actual += x;
    }
    check(actual == c.errors, c.name, "exact runtime error codes/count");
  }
  return ok ? 0 : 1;
}
