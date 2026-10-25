#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace turtle_native_regressions {
namespace ast = turtle::ast;
struct Case {
  std::string_view name, source;
  bool unicodeError, formFeed;
  std::size_t begin, length;
};
// Fixed RDFLib 7.6.0/Raptor controls; native executables are not needed on CI.
inline constexpr Case cases[] = {
    {"iri-unicode", "<urn:\\u03BB> <urn:p> <urn:\\U0001F600> .", false, false,
     0, 0},
    {"replacement-literal", "<urn:s> <urn:p> \"\\uFFFD\" .", false, false, 0,
     0},
    {"replacement-iri", "<urn:\\uFFFD> <urn:p> <urn:o> .", false, false, 0, 0},
    {"maximal-scalar", "<urn:s> <urn:p> \"\\U0010FFFF\" .", false, false, 0, 0},
    {"nul-literal", "<urn:s> <urn:p> \"\\u0000\" .", false, false, 0, 0},
    {"literal-backslash-escape", "<urn:s> <urn:p> \"\\\\U00110000\" .", false,
     false, 0, 0},
    {"literal-backslash-surrogate", "<urn:s> <urn:p> \"\\\\uD800\" .", false,
     false, 0, 0},
    {"unicode-all-quotes",
     "<urn:s> <urn:p> '\\u03BB'; <urn:q> \"\\u03BB\"; <urn:r> '''\\u03BB'''; "
     "<urn:t> \"\"\"\\u03BB\"\"\" .",
     false, false, 0, 0},
    {"local-escape",
     "@prefix ex: <https://example.test/> . ex:a\\~b ex:p ex:one\\!two .",
     false, false, 0, 0},
    {"percent-name",
     "@prefix ex: <https://example.test/> . ex:a%2Fb ex:p ex:a%2fb .", false,
     false, 0, 0},
    {"numeric-lexemes",
     "<urn:s> <urn:p> +0001, -00, 000.50, 1E+03, .5, -.25, +2e-3 .", false,
     false, 0, 0},
    {"huge-integer",
     "<urn:s> <urn:p> "
     "9999999999999999999999999999999999999999999999999999999999999999999999999"
     "9999999999999999999999999999999999999999999999999999999999999999999999999"
     "9999999999999999999999999999999999999999999999999999999999999999999999999"
     "9999999999999999999999999999999999999999999999999999999999999999999999999"
     "9999999999999999999999999999999999999999999999999999999999999999999999999"
     "99999999999999999999999999999999999 .",
     false, false, 0, 0},
    {"typed-raw-value",
     "<urn:s> <urn:p> \"000123\"^^<http://www.w3.org/2001/XMLSchema#integer> .",
     false, false, 0, 0},
    {"ill-typed-value",
     "<urn:s> <urn:p> "
     "\"not-number\"^^<http://www.w3.org/2001/XMLSchema#integer> .",
     false, false, 0, 0},
    {"language-case", "<urn:s> <urn:p> \"text\"@i-KLINGON .", false, false, 0,
     0},
    {"empty-strings", "<urn:s> <urn:p> \"\", '', \"\"\"\"\"\", '''''' .", false,
     false, 0, 0},
    {"long-multiline", "<urn:s> <urn:p> \"\"\"line one\nline two\"\"\" .",
     false, false, 0, 0},
    {"long-inner-quotes",
     "<urn:s> <urn:p> \"\"\"one \"quote\" and \"\"two\"\" quotes\"\"\" .",
     false, false, 0, 0},
    {"collection-order", "<urn:s> <urn:p> (<urn:a> <urn:b> <urn:a>) .", false,
     false, 0, 0},
    {"collection-nesting", "<urn:s> <urn:p> (() (1 2) [<urn:q> 3]) .", false,
     false, 0, 0},
    {"blank-label-reuse", "_:same <urn:p> _:same . _:same <urn:q> _:other .",
     false, false, 0, 0},
    {"anonymous-distinct", "[] <urn:p> [] . [] <urn:p> [] .", false, false, 0,
     0},
    {"property-list-nesting",
     "[<urn:p> [<urn:q> 1]] <urn:r> [<urn:s> 2; <urn:t> 3,4] .", false, false,
     0, 0},
    {"duplicate-objects", "<urn:s> <urn:p> <urn:a>, <urn:b>, <urn:a> .", false,
     false, 0, 0},
    {"repeated-properties", "<urn:s> <urn:p> 1; <urn:q> 2; <urn:p> 1 .", false,
     false, 0, 0},
    {"base-prefix-order",
     "@base <https://example.test/a/> . @prefix ex: <one#> . @base <../b/> . "
     "ex:s ex:p <o> .",
     false, false, 0, 0},
    {"prefix-rebinding",
     "@prefix ex: <urn:one:> . ex:s ex:p ex:o . @prefix ex: <urn:two:> . ex:s "
     "ex:p ex:o .",
     false, false, 0, 0},
    {"sparql-case",
     "pReFiX ex: <urn:ex:> BaSe <https://example.test/> ex:s ex:p <o> .", false,
     false, 0, 0},
    {"empty-graph", "# comment only\n", false, false, 0, 0},
    {"standalone-blank", "[] .", false, false, 0, 0},
    {"invalid-unicode-string", "<urn:s> <urn:p> \"\\U00110000\" .", true, false,
     17, 10},
    {"invalid-unicode-iri", "<urn:\\U00110000> <urn:p> <urn:o> .", true, false,
     5, 10},
    {"invalid-unicode-prefix",
     "@prefix ex: <urn:\\U00110000> . ex:s ex:p ex:o .", true, false, 17, 10},
    {"invalid-unicode-base",
     "@base <https://example.test/\\U00110000> . <s> <p> <o> .", true, false,
     28, 10},
    {"invalid-unicode-sparql-prefix",
     "PREFIX ex: <urn:\\U00110000> ex:s ex:p ex:o .", true, false, 16, 10},
    {"invalid-unicode-sparql-base",
     "BASE <https://example.test/\\U00110000> <s> <p> <o> .", true, false, 27,
     10},
    {"invalid-unicode-datatype", "<urn:s> <urn:p> \"x\"^^<urn:\\U00110000> .",
     true, false, 26, 10},
    {"invalid-unicode-nested", "<urn:s> <urn:p> ([<urn:q> \"\\U00110000\"]) .",
     true, false, 27, 10},
    {"invalid-unicode-long", "<urn:s> <urn:p> \"\"\"\\U00110000\"\"\" .", true,
     false, 19, 10},
    {"invalid-unicode-long-single", "<urn:s> <urn:p> '''\\U00110000''' .", true,
     false, 19, 10},
    {"invalid-unicode-maxword", "<urn:s> <urn:p> \"\\UFFFFFFFF\" .", true,
     false, 17, 10},
    {"invalid-unicode-after-backslash", "<urn:s> <urn:p> \"\\\\\\U00110000\" .",
     true, false, 19, 10},
    {"surrogate-literal", "<urn:s> <urn:p> \"\\uD800\" .", true, false, 17, 6},
    {"surrogate-iri", "<urn:\\uDFFF> <urn:p> <urn:o> .", true, false, 5, 6},
    {"form-feed-gap", "<urn:s>\f<urn:p> <urn:o> .", false, true, 0, 0},
    {"form-feed-string", "<urn:s> <urn:p> \"a\fb\" .", false, false, 0, 0},
};
template <class T> const T *as(const cursiv::AstNode *n) {
  return cursiv::ast_cast<T>(n);
}
inline const ast::Triples *triple(const ast::Document &doc, std::size_t i = 0) {
  return i < doc.statements.size() ? as<ast::Triples>(doc.statements[i])
                                   : nullptr;
}
inline const ast::Term *object(const ast::Triples *t, std::size_t p = 0,
                               std::size_t i = 0) {
  return t && t->properties && p < t->properties->items.size() &&
                 i < t->properties->items[p]->objects.size()
             ? t->properties->items[p]->objects[i]
             : nullptr;
}
inline bool iri_is(const ast::Term *t, std::string_view value) {
  const auto *n = as<ast::IriRef>(t);
  return n && n->value == value;
}
inline bool typed(const Case &c, const ast::Document &doc) {
  const auto *t = triple(doc);
  const auto *literal = as<ast::RdfLiteral>(object(t));
  if (c.name == "replacement-literal")
    return literal && literal->value == "�";
  if (c.name == "replacement-iri")
    return t && iri_is(t->subject, "urn:�");
  if (c.name == "maximal-scalar")
    return literal && literal->value == std::string_view("\xF4\x8F\xBF\xBF", 4);
  if (c.name == "nul-literal")
    return literal && literal->value.size() == 1 && literal->value[0] == 0;
  if (c.name == "literal-backslash-escape")
    return literal && literal->value == "\\U00110000";
  if (c.name == "literal-backslash-surrogate")
    return literal && literal->value == "\\uD800";
  if (c.name == "unicode-all-quotes") {
    if (!t || !t->properties || t->properties->items.size() != 4)
      return false;
    for (std::size_t i = 0; i < 4; ++i) {
      const auto *n = as<ast::RdfLiteral>(object(t, i));
      if (!n || n->value != "λ")
        return false;
    }
    return true;
  }
  if (c.name == "numeric-lexemes") {
    const std::string_view expected[] = {"+0001", "-00",  "000.50", "1E+03",
                                         ".5",    "-.25", "+2e-3"};
    if (!t || !t->properties || t->properties->items.size() != 1 ||
        t->properties->items[0]->objects.size() != 7)
      return false;
    for (std::size_t i = 0; i < 7; ++i) {
      const auto *n = as<ast::NumberLit>(object(t, 0, i));
      if (!n || n->text != expected[i])
        return false;
    }
    return true;
  }
  if (c.name == "huge-integer") {
    const auto *n = as<ast::NumberLit>(object(t));
    return n && n->text.size() == 400 &&
           n->text.find_first_not_of('9') == std::string_view::npos;
  }
  if (c.name == "typed-raw-value" || c.name == "ill-typed-value")
    return literal &&
           literal->value ==
               (c.name == "typed-raw-value" ? "000123" : "not-number") &&
           iri_is(literal->datatype,
                  "http://www.w3.org/2001/XMLSchema#integer");
  if (c.name == "language-case")
    return literal && literal->value == "text" && literal->lang == "i-KLINGON";
  if (c.name == "empty-strings") {
    if (!t || !t->properties || t->properties->items[0]->objects.size() != 4)
      return false;
    for (std::size_t i = 0; i < 4; ++i) {
      const auto *n = as<ast::RdfLiteral>(object(t, 0, i));
      if (!n || !n->value.empty())
        return false;
    }
    return true;
  }
  if (c.name == "collection-order") {
    const auto *n = as<ast::Collection>(object(t));
    return n && n->items.size() == 3 && iri_is(n->items[0], "urn:a") &&
           iri_is(n->items[1], "urn:b") && iri_is(n->items[2], "urn:a");
  }
  if (c.name == "duplicate-objects")
    return t && t->properties && t->properties->items[0]->objects.size() == 3 &&
           iri_is(object(t, 0, 0), "urn:a") &&
           iri_is(object(t, 0, 1), "urn:b") && iri_is(object(t, 0, 2), "urn:a");
  if (c.name == "repeated-properties")
    return t && t->properties && t->properties->items.size() == 3 &&
           iri_is(t->properties->items[0]->predicate, "urn:p") &&
           iri_is(t->properties->items[1]->predicate, "urn:q") &&
           iri_is(t->properties->items[2]->predicate, "urn:p");
  if (c.name == "blank-label-reuse") {
    const auto *s = t ? as<ast::BlankNode>(t->subject) : nullptr;
    const auto *o = as<ast::BlankNode>(object(t));
    const auto *t2 = triple(doc, 1);
    const auto *s2 = t2 ? as<ast::BlankNode>(t2->subject) : nullptr;
    return doc.statements.size() == 2 && s && o && s2 && s->label == "same" &&
           o->label == "same" && s2->label == "same";
  }
  if (c.name == "anonymous-distinct") {
    const auto *t2 = triple(doc, 1);
    const auto *s = t ? as<ast::BlankNode>(t->subject) : nullptr;
    const auto *o = as<ast::BlankNode>(object(t));
    const auto *s2 = t2 ? as<ast::BlankNode>(t2->subject) : nullptr;
    const auto *o2 = as<ast::BlankNode>(object(t2));
    return doc.statements.size() == 2 && s && o && s2 && o2 &&
           s->label.empty() && o->label.empty() && s2->label.empty() &&
           o2->label.empty() && s != o && s != s2 && s != o2 && o != s2 &&
           o != o2 && s2 != o2;
  }
  if (c.name == "form-feed-string")
    return literal && literal->value == "a\fb";
  return true;
}
inline bool run() {
  const auto language =
      cursiv::create_language<turtle::TurtleCoreServices,
                              turtle::TurtleSharedCoreServices,
                              turtle::install_core_module>();
  const auto factory = turtle::generated::Turtle::create_parser();
  for (const auto &c : cases) {
    const std::string source(c.source);
    const auto check = [&](const cursiv::ParseResult &r) {
      const auto *doc = r.value_as<ast::Document>();
      if (r.cst().text() != source ||
          (!c.formFeed && (!r.accepted || r.cst().recovered() ||
                           !r.diagnostics.empty() || !doc)))
        return false;
      if (!doc)
        return c.formFeed;
      bool owned = !doc->parent;
      std::unordered_set<const cursiv::AstNode *> seen;
      const auto captured = [&](std::string_view text) {
        if (text.empty())
          return true;
        const auto at = reinterpret_cast<std::uintptr_t>(text.data()),
                   base = reinterpret_cast<std::uintptr_t>(source.data());
        return at >= base && at - base <= source.size() &&
               text.size() <= source.size() - (at - base);
      };
      const auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
        if (!seen.insert(&n).second || n.cstNodeId >= r.cst().size()) {
          owned = false;
          return;
        }
        const auto &span = r.cst()[n.cstNodeId];
        owned &= span.begin <= span.end && span.end <= source.size();
        if (const auto *x = as<ast::NumberLit>(&n))
          owned &= captured(x->text);
        if (const auto *x = as<ast::PrefixedTerm>(&n))
          owned &= captured(x->value);
        if (const auto *x = as<ast::BlankNode>(&n))
          owned &= captured(x->label);
        if (const auto *x = as<ast::RdfLiteral>(&n))
          owned &= captured(x->lang);
        language.language.astReflection->visitMembers(
            n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
              owned &= child.parent == &n &&
                       r.cst().contains(n.cstNodeId, child.cstNodeId);
              self(self, child);
            });
      };
      walk(walk, *doc);
      return owned && seen.size() == r.nodes.size() && typed(c, *doc);
    };
    const auto a = language.language.parser->parse(source);
    const auto b = factory->parse(source);
    if (!check(a) || !check(b)) {
      std::fprintf(stderr, "turtle native AST/source: %.*s\n",
                   int(c.name.size()), c.name.data());
      return false;
    }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    for (const auto &d : validation.diagnostics)
      if (d.range.begin > d.range.end || d.range.end > source.size())
        return false;
    if (c.unicodeError) {
      if (validation.diagnostics.size() != 1)
        return false;
      const auto &d = validation.diagnostics[0];
      if (!d.code || !d.code->isString() ||
          d.code->string() != "turtle.unicode-scalar" ||
          d.range.begin != c.begin || d.range.end != c.begin + c.length)
        return false;
    } else if (c.formFeed) {
      if (validation.diagnostics.empty())
        return false;
    } else if (!validation.diagnostics.empty())
      return false;
  }
  return true;
}
} // namespace turtle_native_regressions
