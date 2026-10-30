#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/references/ScopeComputation.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "xsmp/core/CoreServices.hpp"
#include "xsmp/core/ast.hpp"

int main() {
  namespace ast = xsmp::ast;
  const auto world = cursiv::create_language<xsmp::XsmpCoreServices,
                                             xsmp::XsmpSharedCoreServices,
                                             xsmp::install_core_module>();
  const auto& services = world.language;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "xsmp native: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [](const auto& r) {
    return r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
           r.value;
  };
  const auto owns = [&](const auto& r) {
    bool valid = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) {
        valid = false;
        return;
      }
      services.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node;
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    return valid && seen.size() == r.nodes.size();
  };
  struct Case {
    const char* id;
    const char* source;
    bool syntaxClean;
    std::string_view text;
    std::string_view character;
    bool hasCharacter;
    std::initializer_list<std::string_view> strings;
    std::string_view fieldTypeRef;
  };
  // Expected values come from a fresh official @xsmp/core2.0.3 parser
  // execution.
  const Case cases[] = {
      {"unicode-nbsp", "catalogue\u00A0c", true, "", "", false, {}, ""},
      {"unicode-ls", "catalogue\u2028c", true, "", "", false, {}, ""},
      {"bom", "\uFEFFcatalogue c", true, "", "", false, {}, ""},
      {"qualified-data",
       "catalogue c namespace N { struct S { field Smp /*comment*/ . Float64 f "
       "} }",
       true,
       "",
       "",
       false,
       {},
       "Smp.Float64"},
      {"soft-keyword-name", "catalogue catalogue", true, "", "", false, {}, ""},
      {"reserved-name", "catalogue true", false, "", "", false, {}, ""},
      {"reserved-visibility-name",
       "catalogue public",
       false,
       "",
       "",
       false,
       {},
       ""},
      {"reserved-float-name", "catalogue float", false, "", "", false, {}, ""},
      {"attribute-empty-parens",
       "catalogue c namespace N { @() primitive T }",
       false,
       "",
       "",
       false,
       {},
       ""},
      {"attribute-bare-public",
       "catalogue c namespace N { @ public primitive T }",
       true,
       "",
       "",
       false,
       {},
       ""},
      {"empty-string",
       "catalogue c namespace N { struct S { field T f = \"\" } }",
       true,
       "",
       "",
       false,
       {""},
       "T"},
      {"empty-character",
       "catalogue c namespace N { struct S { field T f = '' } }",
       true,
       "",
       "",
       true,
       {},
       "T"},
      {"nul-string",
       "catalogue c namespace N { struct S { field T f = \"\\x00\" } }",
       true,
       "",
       "",
       false,
       {std::string_view("\000", 1)},
       "T"},
      {"strings",
       "catalogue c namespace N { struct S { field T f = "
       "\"a\\n\\x41\\u03B1\\U0001F600\" \"\\123\\q\" } }",
       true,
       "",
       "",
       false,
       {"a\nAα😀", "Sq"},
       "T"},
      {"character",
       "catalogue c namespace N { struct S { field T f = '\\x41' } }",
       true,
       "",
       "A",
       true,
       {},
       "T"},
      {"surrogate-pair",
       "catalogue c namespace N { struct S { field T f = \"\\uD83D\\uDE00\" } "
       "}",
       true,
       "",
       "",
       false,
       {"😀"},
       "T"},
      {"time-seconds",
       "catalogue c namespace N { struct S { field T f = 2s } }",
       true,
       "2'000'000'000L",
       "",
       false,
       {},
       "T"},
      {"time-milliseconds",
       "catalogue c namespace N { struct S { field T f = 1'234ms } }",
       true,
       "1'234'000'000L",
       "",
       false,
       {},
       "T"},
      {"time-leading-zero",
       "catalogue c namespace N { struct S { field T f = 010s } }",
       true,
       "10'000'000'000L",
       "",
       false,
       {},
       "T"},
      {"hex-ending-d",
       "catalogue c namespace N { struct S { field T f = 0x1d } }",
       true,
       "0x1d",
       "",
       false,
       {},
       "T"},
      {"builtin-keyword",
       "catalogue c namespace N { struct S { field T f = $class } }",
       false,
       "",
       "",
       false,
       {},
       "T"},
      {"literal-integer",
       "catalogue c namespace N { struct S { field T f = 0xA'FUl } }",
       true,
       "0xA'FUl",
       "",
       false,
       {},
       "T"},
      {"huge-duration",
       "catalogue c namespace N { struct S { field T f = "
       "123456789012345678901234567890d } }",
       true,
       "10'666'666'570'666'666'657'066'666'665'696'000'000'000'000L",
       "",
       false,
       {},
       "T"},
      {"integer-no-value",
       "catalogue c namespace N { struct S { constant T x } }",
       true,
       "",
       "",
       false,
       {},
       ""},
  };
  for (const auto& c : cases) {
    const auto r = services.parser->parse(c.source);
    check(clean(r) == c.syntaxClean, c.id);
    if (!c.syntaxClean || !clean(r)) continue;
    check(owns(r) && r.cst().text() == c.source,
          "complete ownership and unchanged source");
    const auto* cat = cursiv::ast_cast<ast::Catalogue>(r.value);
    const auto* s =
        cat && cat->elements.size() == 1 &&
                cat->elements[0]->elements.size() == 1
            ? cursiv::ast_cast<ast::Structure>(cat->elements[0]->elements[0])
            : nullptr;
    const auto* field = s && s->elements.size() == 1
                            ? cursiv::ast_cast<ast::Field>(s->elements[0])
                            : nullptr;
    if (!c.fieldTypeRef.empty())
      check(field && field->type.refText == c.fieldTypeRef,
            "native qualified reference text");
    if (!c.text.empty()) {
      const auto* value =
          field ? cursiv::ast_cast<ast::IntegerLiteral>(field->default_)
                : nullptr;
      check(value && value->text == c.text, c.id);
    }
    if (c.hasCharacter) {
      const auto* value =
          field ? cursiv::ast_cast<ast::CharacterLiteral>(field->default_)
                : nullptr;
      check(value && value->value == c.character, c.id);
    }
    if (c.strings.size()) {
      const auto* value =
          field ? cursiv::ast_cast<ast::StringLiteral>(field->default_)
                : nullptr;
      bool same = value && value->value.size() == c.strings.size();
      std::size_t i = 0;
      for (const auto expected : c.strings)
        same &=
            value && i < value->value.size() && value->value[i++] == expected;
      check(same, c.id);
    }
  }
  // UTF-8 scalar value profile: native JS can retain lone UTF-16 surrogates
  // and partially decode malformed hex; these differences are explicit.
  for (const char* value : {R"("\uD800")", R"("\u0zzz")", R"("\U00110000")"}) {
    const auto r = services.parser->parse(
        std::string("catalogue c namespace N { struct S { field T f = ") +
        value + " } }");
    check(r.accepted && !r.cst().recovered() && !r.diagnostics.empty(),
          "invalid scalar/hex produces a conversion diagnostic");
  }
  {
    const std::string source =
        "catalogue c namespace N { primitive T class C { constant T hidden = 1 "
        "public constant T visible = 2 } struct S { constant T exposed = 3 "
        "field T copy = C.hidden } }";
    const auto v = cursiv::test::validationHelper(services, source);
    const auto exported =
        services.scopeComputation->collectExportedSymbols(*v.document);
    bool hidden = false, visible = false, exposed = false;
    for (const auto& symbol : exported) {
      hidden |= symbol.name == "N.C.hidden";
      visible |= symbol.name == "N.C.visible";
      exposed |= symbol.name == "N.S.exposed";
    }
    check(
        v.diagnostics.empty() && exported.size() == 7 && !hidden && visible &&
            exposed,
        "native class default-private versus structure default-public exports");
  }
  for (const auto& item : {std::pair{"catalogue c namespace N { primitive T "
                                     "struct S { constant T missing } }",
                                     "xsmp.constant-value"},
                           std::pair{"catalogue c namespace N { primitive T "
                                     "attribute T A @A primitive U }",
                                     "xsmp.attribute-value"}}) {
    const auto v =
        cursiv::test::validationHelper(services, std::string(item.first));
    bool found = false;
    for (const auto& d : v.diagnostics) found |= d.code == item.second;
    check(found, item.second);
  }
  {
    const auto v = cursiv::test::validationHelper(
        services, "catalogue c namespace N { @ public primitive T }");
    bool found = false;
    for (const auto& d : v.diagnostics)
      found |= d.code == "xsmp.attribute-type";
    check(
        found,
        "native NamedElement check diagnoses a bare attribute's missing type");
  }
  for (const char* source :
       {"catalogue c namespace N { primitive T model M implements T {} }",
        "catalogue c namespace N { primitive T struct S { field T f = T } }"}) {
    const auto v = cursiv::test::validationHelper(services, source);
    check(!v.diagnostics.empty(),
          "native target category excludes primitive as interface or constant");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services,
        "catalogue c namespace N { primitive T enum E { V = 1 } struct S { "
        "constant T C = 2 field T f = C field T g = E.V } }");
    const auto* cat =
        cursiv::ast_cast<ast::Catalogue>(v.document->parseResult.value);
    const auto* s =
        cat && cat->elements.size() == 1 &&
                cat->elements[0]->elements.size() == 3
            ? cursiv::ast_cast<ast::Structure>(cat->elements[0]->elements[2])
            : nullptr;
    const auto* f = s && s->elements.size() == 3
                        ? cursiv::ast_cast<ast::Field>(s->elements[1])
                        : nullptr;
    const auto* g = s && s->elements.size() == 3
                        ? cursiv::ast_cast<ast::Field>(s->elements[2])
                        : nullptr;
    const auto* constant =
        f ? cursiv::ast_cast<ast::NamedElementReference>(f->default_) : nullptr;
    const auto* literal =
        g ? cursiv::ast_cast<ast::NamedElementReference>(g->default_) : nullptr;
    check(v.diagnostics.empty() && constant &&
              constant->value.get<ast::Constant>() && literal &&
              literal->value.get<ast::EnumerationLiteral>(),
          "native reference target union resolves constants and enumeration "
          "literals");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services,
        "catalogue c namespace N { primitive T struct Inner { field T x } "
        "struct Outer { field Inner v = { .x = 1 } } class Base { constant T K "
        "= 7 } class Derived extends Base { field T x = K } interface ParentI "
        "{ constant T KI = 8 } interface I extends ParentI {} model MB { field "
        "T f } model MD extends MB implements I { field T ifconst = KI "
        "entrypoint E { in f } } }");
    const auto* cat =
        cursiv::ast_cast<ast::Catalogue>(v.document->parseResult.value);
    const auto* outer =
        cat && cat->elements.size() == 1 &&
                cat->elements[0]->elements.size() == 9
            ? cursiv::ast_cast<ast::Structure>(cat->elements[0]->elements[2])
            : nullptr;
    const auto* field = outer && outer->elements.size() == 1
                            ? cursiv::ast_cast<ast::Field>(outer->elements[0])
                            : nullptr;
    const auto* collection =
        field ? cursiv::ast_cast<ast::CollectionLiteral>(field->default_)
              : nullptr;
    const auto* init = collection && collection->elements.size() == 1
                           ? cursiv::ast_cast<ast::DesignatedInitializer>(
                                 collection->elements[0])
                           : nullptr;
    check(v.diagnostics.empty() && init && init->field.get() &&
              init->field.get()->name == "x",
          "native designated initializer resolves its value type's field; "
          "inherited constant also links");
    const auto* model =
        cat && cat->elements.size() == 1 &&
                cat->elements[0]->elements.size() == 9
            ? cursiv::ast_cast<ast::Model>(cat->elements[0]->elements[8])
            : nullptr;
    const auto* value = model && model->elements.size() == 2
                            ? cursiv::ast_cast<ast::Field>(model->elements[0])
                            : nullptr;
    const auto* name =
        value ? cursiv::ast_cast<ast::NamedElementReference>(value->default_)
              : nullptr;
    const auto* entry =
        model && model->elements.size() == 2
            ? cursiv::ast_cast<ast::EntryPoint>(model->elements[1])
            : nullptr;
    check(name && name->value.get<ast::Constant>() &&
              name->value.get<ast::Constant>()->name == "KI" && entry &&
              entry->input.size() == 1 && entry->input[0].get() &&
              entry->input[0].get()->name == "f",
          "native component-base field and transitive interface constant "
          "resolution");
  }
  {
    const auto v = cursiv::test::validationHelper(
        services,
        "catalogue c namespace N { primitive T struct S { field T x field T f "
        "= { .x = 1 } } }");
    check(!v.diagnostics.empty(),
          "a primitive initializer cannot capture an unrelated surrounding "
          "field");
  }
  for (const char* source : {"catalogue c namespace N { primitive _bad }",
                             "catalogue c namespace N { primitive class }"}) {
    const auto v = cursiv::test::validationHelper(services, source);
    bool found = false;
    for (const auto& d : v.diagnostics) found |= d.code == "xsmp.name";
    check(found, "native declaration name constraints");
  }
  return ok ? 0 : 1;
}
