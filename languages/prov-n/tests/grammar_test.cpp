// The smoke test of the language: every corpus document parses clean, and
// one is read back field by field to catch a converter that silently
// mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "provn/core/CoreServices.hpp"
#include "provn/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "provn: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "example1.provn",
    "example2.provn",
    "example3.provn",
    "example4.provn",
};

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      provn::ProvNCoreServices, provn::ProvNSharedCoreServices,
      provn::install_core_module>();

  {
    const auto result = language.language.parser->parse(R"prov(document
prefix ex <http://example.org/\u00E9>
entity(𐀀, [ex:big=1234567890123456789012345678901234567890,
            ex:text="line\n\u00E9\U0001F600",
            ex:long="""long\ttext"""])
activity(ex:a, 2026-10-28T12:00:00+14:00, -)
endDocument)prov");
    const auto *doc = result.value_as<provn::ast::Document>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc || doc->prefixes.size() != 1 ||
        doc->prefixes[0]->iri != "http://example.org/é" ||
        doc->expressions.size() != 2)
      return fail("native Unicode/unbounded integer document rejected");
    const auto *e = cursiv::ast_cast<provn::ast::EntityExpr>(doc->expressions[0]);
    if (!e || e->entity != "𐀀" || e->attributes.size() != 3) return 1;
    const auto *integer = cursiv::ast_cast<provn::ast::IntLiteral>(e->attributes[0]->value);
    const auto *text = cursiv::ast_cast<provn::ast::StringLiteral>(e->attributes[1]->value);
    const auto *longText = cursiv::ast_cast<provn::ast::StringLiteral>(e->attributes[2]->value);
    if (!integer || integer->value != "1234567890123456789012345678901234567890" ||
        !text || text->text != "line\né😀" || !longText || longText->text != "long\ttext")
      return fail("native lexical values lost in AST");
  }
  for (const auto *source : {
           "document activity(a,2026-10-28T12:00:00+19:00,-) endDocument",
           "document activity(a,2026-10-28T12:00:00+14:01,-) endDocument",
           "document entity(\ue000) endDocument",
           "document activity(a,2026-02-29T12:00:00,-) endDocument",
           "document activity(a,1900-02-29T12:00:00,-) endDocument"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-invalid timezone or qualified-name character accepted");
  }
  for (const auto *source : {
           "document activity(a,2000-02-29T24:00:00,-) endDocument",
           "document activity(a,0000-02-29T00:00:00,-) endDocument",
           "document activity(a,-0004-02-29T00:00:00,-) endDocument"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("valid XSD1.1 leap-year/dateTime spelling rejected");
  }

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<provn::ast::Document>)
      return fail((std::string(name) + ": built no Document").c_str());

    const auto& doc = *static_cast<const provn::ast::Document*>(root);
    if (doc.expressions.empty())
      return fail((std::string(name) + ": Document holds no expression").c_str());
  }

  // features.provn covers constructs the corpus never uses: wasStartedBy,
  // wasEndedBy, wasInvalidatedBy, wasInformedBy, wasInfluencedBy, hadMember,
  // a typed literal, an extensibility relation with every argument shape,
  // and a bundle.
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty()) return fail("features.provn: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("features.provn: does not parse clean");
    if (!result.diagnostics.empty())
      return fail("features.provn: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<provn::ast::Document>)
      return fail("features.provn: built no Document");
    const auto& doc = *static_cast<const provn::ast::Document*>(root);
    if (doc.bundles.size() != 1)
      return fail("features.provn: expected one bundle");
    if (doc.bundles[0]->expressions.size() != 2)
      return fail("features.provn: bundle holds the wrong expression count");
  }

  // example4.provn, read back field by field: a default namespace, a
  // bare identifier and a prefixed one both reaching EntityExpr.
  const std::string ex4Text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example4.provn");
  const cursiv::ParseResult ex4Result = language.language.parser->parse(ex4Text);
  const auto& ex4 = *static_cast<const provn::ast::Document*>(ex4Result.value);

  if (!ex4.defaultNamespace.has_value() ||
      *ex4.defaultNamespace != "http://anotherexample.org/")
    return fail("example4.provn: wrong default namespace");
  if (ex4.prefixes.size() != 1 || ex4.prefixes[0]->prefix != "ex")
    return fail("example4.provn: wrong prefix");
  if (ex4.expressions.size() != 4)
    return fail("example4.provn: expected four expressions");

  if (ex4.expressions[0]->type != &cursiv::ast_type_tag<provn::ast::EntityExpr>)
    return fail("example4.provn: first expression is not an EntityExpr");
  const auto& entity0 =
      *static_cast<const provn::ast::EntityExpr*>(ex4.expressions[0]);
  if (entity0.entity != "e2")
    return fail("example4.provn: first entity should read 'e2'");

  if (ex4.expressions[2]->type != &cursiv::ast_type_tag<provn::ast::EntityExpr>)
    return fail("example4.provn: third expression is not an EntityExpr");
  const auto& entity2 =
      *static_cast<const provn::ast::EntityExpr*>(ex4.expressions[2]);
  if (entity2.entity != "ex:e2")
    return fail("example4.provn: third entity should read 'ex:e2'");

  // example1.provn: the typed attribute value on the second entity.
  const std::string ex1Text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example1.provn");
  const cursiv::ParseResult ex1Result = language.language.parser->parse(ex1Text);
  const auto& ex1 = *static_cast<const provn::ast::Document*>(ex1Result.value);
  if (ex1.expressions.size() != 5)
    return fail("example1.provn: expected five expressions");
  const auto& entityWithAttrs =
      *static_cast<const provn::ast::EntityExpr*>(ex1.expressions[3]);
  if (entityWithAttrs.attributes.size() != 2)
    return fail("example1.provn: expected two attribute-value pairs");
  if (entityWithAttrs.attributes[0]->value->type !=
      &cursiv::ast_type_tag<provn::ast::StringLiteral>)
    return fail("example1.provn: attribute value should be a StringLiteral");
  const auto& labelValue = *static_cast<const provn::ast::StringLiteral*>(
      entityWithAttrs.attributes[0]->value);
  if (labelValue.text != "employment-stats-2011")
    return fail("example1.provn: wrong label text");

  std::puts("provn: the corpus and the feature sample parse");
  return 0;
}
