#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "xsdregex/core/CoreServices.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language =
      cursiv::create_language<xsdregex::XsdRegexCoreServices,
                              xsdregex::XsdRegexSharedCoreServices,
                              xsdregex::install_core_module>();
  unsigned roundtrips = 0, controls = 0;
  for (const auto &c : nativeCases) {
    auto fail = [&](const char *w) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), w);
      return 1;
    };
    std::ifstream in(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                         std::string(c.file),
                     std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(in)), {});
    const auto r = language.language.parser->parse(source);
    const auto *d = cursiv::ast_cast<xa::RegExp>(r.value);
    const bool clean =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (!d || r.cst().text() != source || clean != c.clean)
      return fail("parse/source");
    bool raw = true, owned = !d->parent;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto inside = [&](std::string_view v) {
      if (v.empty())
        return true;
      auto lo = reinterpret_cast<std::uintptr_t>(source.data()),
           p = reinterpret_cast<std::uintptr_t>(v.data());
      return p >= lo && p - lo <= source.size() &&
             v.size() <= source.size() - (p - lo);
    };
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *p = cursiv::ast_cast<xa::CharProp>(&n))
        raw &= inside(p->name);
      if (auto *q = cursiv::ast_cast<xa::Quantifier>(&n)) {
        raw &= inside(q->min);
        if (q->max)
          raw &= inside(*q->max);
      }
      if (auto *v = cursiv::ast_cast<xa::CharRange>(&n))
        raw &= inside(v->from) && inside(v->to);
      if (auto *p = cursiv::ast_cast<xa::Piece>(&n))
        if (auto *s = std::get_if<std::string_view>(&p->atom))
          raw &= inside(*s);
      if (auto *p = cursiv::ast_cast<xa::CharClassExpr>(&n))
        for (auto &v : p->items)
          if (auto *s = std::get_if<std::string_view>(&v))
            raw &= inside(*s);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    if (!raw || !owned || seen.size() != r.nodes.size())
      return fail("ownership/raw fields");
    if (!clean) {
      if (r.diagnostics.empty())
        return fail("negative syntax diagnosis");
      continue;
    }
    ++roundtrips;
    if (pattern(*d) != source)
      return fail("full AST roundtrip to native source");
    const auto v = cursiv::test::validationHelper(language.language, source);
    if (v.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty()) {
      ++controls;
      const auto &x = v.diagnostics[0];
      if (x.code != c.error || x.range.begin >= x.range.end ||
          x.range.end > source.size())
        return fail("runtime code/range");
    }
    const auto first = [&]() { return d->branches[0]->pieces[0]; };
    if (c.name == "empty") {
      if (d->branches.size() != 1 || !d->branches[0]->pieces.empty())
        return fail("empty branch");
    }
    if (c.name == "empty-alternatives") {
      if (d->branches.size() != 3 || !d->branches[0]->pieces.empty() ||
          d->branches[1]->pieces.size() != 1 || !d->branches[2]->pieces.empty())
        return fail("empty alternation operands");
    }
    if (c.name == "surrogate-property" || c.name == "complement-property") {
      auto *p = std::get<xa::CharProp *>(first()->atom);
      if (p->name != "Cs" || p->negated != (c.name == "complement-property"))
        return fail("Unicode category identity");
    }
    if (c.name == "original-example-chargroup-sub1") {
      auto *p = std::get<xa::CharClassExpr *>(first()->atom);
      if (p->items.size() != 2 ||
          std::get<std::string_view>(p->items[0]) != "a" ||
          std::get<std::string_view>(p->items[1]) != "-" || !p->subtract ||
          p->subtract->items.size() != 1)
        return fail("literal dash before subtraction");
    }
    if (c.name == "original-example-chargroup-sub2") {
      auto *p = std::get<xa::CharClassExpr *>(first()->atom);
      if (p->items.size() != 1 ||
          std::get<std::string_view>(p->items[0]) != "-" || !p->subtract)
        return fail("dash-only subtraction base");
    }
    if (c.name == "dash-internal") {
      auto *p = std::get<xa::CharClassExpr *>(first()->atom);
      if (p->items.size() != 3 ||
          !std::holds_alternative<xa::CharRange *>(p->items[0]) ||
          std::get<std::string_view>(p->items[1]) != "-" ||
          std::get<std::string_view>(p->items[2]) != "c")
        return fail("range and literal middle dash");
    }
    if (c.name == "range-escaped" || c.name == "range-escaped-reversed") {
      auto *p = std::get<xa::CharClassExpr *>(first()->atom);
      if (p->items.size() != 1)
        return fail("escaped range atom count");
      auto *v = std::get<xa::CharRange *>(p->items[0]);
      if (v->from != (c.name == "range-escaped" ? "\\t" : "\\n") ||
          v->to != (c.name == "range-escaped" ? "\\n" : "\\t"))
        return fail("single-escape range endpoints");
    }
    if (c.name == "quant-leading-zero") {
      auto *q = first()->quant;
      if (!q || q->kind != xa::QuantifierKind::counted || q->min != "0002" ||
          q->max != "0003" || !q->comma)
        return fail("raw counted bounds");
    }
    if (c.name == "quant-uint32-overflow") {
      auto *q = first()->quant;
      if (!q || q->min != "4294967296" || q->comma || q->max)
        return fail("large count must not become zero");
    }
    if (c.name == "quant-unbounded-text") {
      auto *q = first()->quant;
      if (!q || q->min != "1234567890123456789012345678901234567890" ||
          !q->comma || q->max)
        return fail("unbounded raw minimum");
    }
    if (c.name == "original-non-ascii-chars") {
      const std::string_view expected[] = {"c", "a", "f", "é", "中", "\n"};
      if (d->branches[0]->pieces.size() != std::size(expected))
        return fail("Unicode scalar count");
      for (std::size_t i = 0; i < std::size(expected); ++i)
        if (std::get<std::string_view>(d->branches[0]->pieces[i]->atom) !=
            expected[i])
          return fail("Unicode scalar values");
    }
  }
  std::printf(
      "XSD regex: %zu cases, %u AST source roundtrips, %u runtime controls\n",
      std::size(nativeCases), roundtrips, controls);
  return roundtrips == 47 && controls == 13 ? 0 : 1;
}
