#include "audit_regressions.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "turtledoc/core/CoreServices.hpp"
#include "turtledoc/core/ast.hpp"

namespace {

int fail(const char *what) {
  std::fprintf(stderr, "turtledoc: %s\n", what);
  return 1;
}

std::string read(const std::string &path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

template <class T> const T *as(const cursiv::AstNode *node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T *>(node)
             : nullptr;
}

} // namespace

int main() {
  if (!audit_regressions::run())
    return 1;
  const auto language =
      cursiv::create_language<turtledoc::TurtleDocCoreServices,
                              turtledoc::TurtleDocSharedCoreServices,
                              turtledoc::install_core_module>();

  const char *const kDocs[] = {"directives.ttl", "features.ttl"};
  bool saw_features = false;

  for (const char *const name : kDocs) {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
    const std::string text = read(path);
    if (text.empty())
      return fail("a document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("a document does not parse");
    if (!result.diagnostics.empty())
      return fail("a document carries a diagnostic");

    const auto *doc = as<turtledoc::ast::Document>(result.value);
    if (doc == nullptr)
      return fail("a document built no Document");

    if (std::string_view(name) == "features.ttl") {
      // The parse result and its arena die with `result`: copy nothing,
      // read now.
      saw_features = true;

      if (doc->statements.size() != 5)
        return fail("features.ttl: unexpected statement count");

      const auto *prefix = as<turtledoc::ast::PrefixDecl>(doc->statements[0]);
      if (prefix == nullptr || prefix->prefix != "ex" ||
          prefix->iri != "http://example.org/")
        return fail("features.ttl: @prefix ex: did not read back");

      const auto *sparqlPrefix =
          as<turtledoc::ast::SparqlPrefixDecl>(doc->statements[1]);
      if (sparqlPrefix == nullptr || sparqlPrefix->prefix != "foaf")
        return fail("features.ttl: PREFIX foaf: did not read back");

      const auto *alice = as<turtledoc::ast::Triples>(doc->statements[2]);
      if (alice == nullptr || alice->properties == nullptr ||
          alice->properties->items.size() != 15)
        return fail("features.ttl: ex:Alice's properties did not read back");

      const auto *isA =
          as<turtledoc::ast::RdfType>(alice->properties->items[0]->predicate);
      if (isA == nullptr)
        return fail("features.ttl: the `a` shorthand did not read back");

      const auto *age = as<turtledoc::ast::NumberLit>(
          alice->properties->items[1]->objects[0]);
      if (age == nullptr || age->text != "42")
        return fail("features.ttl: ex:age 42 did not read back");

      const auto *balance = as<turtledoc::ast::NumberLit>(
          alice->properties->items[2]->objects[0]);
      if (balance == nullptr || balance->text != "-3.5")
        return fail("features.ttl: ex:balance -3.5 did not read back");

      const auto *ratio = as<turtledoc::ast::NumberLit>(
          alice->properties->items[3]->objects[0]);
      if (ratio == nullptr || ratio->text != "1.e5")
        return fail("features.ttl: ex:ratio 1.e5 (empty fraction) did not "
                    "read back");

      const auto *tiny = as<turtledoc::ast::NumberLit>(
          alice->properties->items[4]->objects[0]);
      if (tiny == nullptr || tiny->text != ".5e3")
        return fail(
            "features.ttl: ex:tiny .5e3 (leading dot) did not read back");

      const auto *active =
          as<turtledoc::ast::BoolLit>(alice->properties->items[5]->objects[0]);
      if (active == nullptr || active->value != true)
        return fail("features.ttl: ex:active true did not "
                    "convert");

      const auto *hidden =
          as<turtledoc::ast::BoolLit>(alice->properties->items[6]->objects[0]);
      if (hidden == nullptr || hidden->value != false)
        return fail("features.ttl: ex:hidden false did not convert");

      const auto *name = as<turtledoc::ast::RdfLiteral>(
          alice->properties->items[7]->objects[0]);
      if (name == nullptr || name->value != "Alice" || name->lang != "en")
        return fail("features.ttl: ex:name \"Alice\"@en did not read back");

      const auto *id = as<turtledoc::ast::RdfLiteral>(
          alice->properties->items[8]->objects[0]);
      const auto *idType = id != nullptr
                               ? as<turtledoc::ast::PrefixedTerm>(id->datatype)
                               : nullptr;
      if (id == nullptr || id->value != "007" || idType == nullptr ||
          idType->value != "ex:Code")
        return fail("features.ttl: ex:id \"007\"^^ex:Code did not read back");

      const auto *friendNode = as<turtledoc::ast::BlankNode>(
          alice->properties->items[9]->objects[0]);
      if (friendNode == nullptr || friendNode->label != "b1")
        return fail("features.ttl: ex:friend _:b1 did not read back");

      const auto *favorites = as<turtledoc::ast::Collection>(
          alice->properties->items[10]->objects[0]);
      if (favorites == nullptr || favorites->items.size() != 3)
        return fail("features.ttl: ex:favorites (...) did not read back");

      const auto *extra = as<turtledoc::ast::BlankNodePropertyList>(
          alice->properties->items[11]->objects[0]);
      if (extra == nullptr || extra->properties == nullptr ||
          extra->properties->items.size() != 1)
        return fail("features.ttl: ex:extra [ ex:k \"v\" ] did not read "
                    "back");

      const auto *tag = as<turtledoc::ast::PrefixedTerm>(
          alice->properties->items[12]->objects[0]);
      if (tag == nullptr || tag->value != "ex:x\\~y")
        return fail("features.ttl: ex:tag ex:x\\~y (PN_LOCAL_ESC) did not read "
                    "back");

      const auto *note = as<turtledoc::ast::RdfLiteral>(
          alice->properties->items[13]->objects[0]);
      if (note == nullptr || note->value != "it''s here")
        return fail("features.ttl: the long single-quote string (two "
                    "leading apostrophes) did not read back");

      const auto *quote = as<turtledoc::ast::RdfLiteral>(
          alice->properties->items[14]->objects[0]);
      if (quote == nullptr || quote->value != "she said \"\"hi\"\" to me")
        return fail("features.ttl: the long double-quote string (two "
                    "leading quotes) did not read back");

      const auto *knowsAnon = as<turtledoc::ast::Triples>(doc->statements[3]);
      if (knowsAnon == nullptr || knowsAnon->properties == nullptr ||
          knowsAnon->properties->items.size() != 1)
        return fail("features.ttl: _:b1 ex:knows [] did not read back");
      const auto *anon = as<turtledoc::ast::BlankNode>(
          knowsAnon->properties->items[0]->objects[0]);
      if (anon == nullptr || !anon->label.empty())
        return fail("features.ttl: the anonymous `[]` blank node did not read "
                    "back");
    }
  }

  if (!saw_features)
    return fail("features.ttl was not exercised");

  // Turtle comments may follow a completed statement.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("ex:s ex:p ex:o . # nope\n");
    if (result.cst().recovered())
      return fail("a comment after a statement should parse");
  }

  // StringLiteralQuote excludes CR/LF: an unterminated short string does not
  // absorb the next line.
  {
    const cursiv::ParseResult result = language.language.parser->parse(
        "ex:s ex:p \"unterminated .\nex:s ex:p \"ok\" .\n");
    if (!result.cst().recovered())
      return fail("an unterminated short string should not parse");
  }

  std::puts("turtledoc: the corpus parses");
  return 0;
}
