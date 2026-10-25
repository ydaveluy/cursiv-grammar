#include "audit_regressions.hpp"
#include "native_regressions.hpp"
// The smoke test of the language: every file of example/ parses clean, and
// features.ttl is read back field by field so a converter that silently
// yields the wrong value (or refuses, visible only in `diagnostics`) is
// caught even though the structural checks alone would pass.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "turtle/core/CoreServices.hpp"
#include "turtle/core/ast.hpp"

namespace {

int fail(const char *what) {
  std::fprintf(stderr, "turtle: %s\n", what);
  return 1;
}

std::string read(const std::string &path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

// features.ttl lives beside this test, not in the corpus: it is written here
// for the constructs the two upstream documents never reach.
const char *const kExamples[] = {CURSIV_EXAMPLE_DIR "/iriref_test.ttl",
                                 CURSIV_EXAMPLE_DIR "/Vehicle.ttl",
                                 CURSIV_TEST_DIR "/features.ttl"};

template <class T> const T *as(const cursiv::AstNode *node) {
  return (node != nullptr && node->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T *>(node)
             : nullptr;
}

} // namespace

int main() {
  if (!audit_regressions::run() || !turtle_native_regressions::run())
    return 1;
  const auto language =
      cursiv::create_language<turtle::TurtleCoreServices,
                              turtle::TurtleSharedCoreServices,
                              turtle::install_core_module>();

  bool saw_features = false;

  for (const char *const name : kExamples) {
    const std::string text = read(name);
    if (text.empty())
      return fail("an example is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("an example does not parse");
    if (!result.diagnostics.empty())
      return fail("an example carries a diagnostic");

    const turtle::ast::Document *doc = as<turtle::ast::Document>(result.value);
    if (doc == nullptr)
      return fail("an example built no document");

    if (std::string_view(name).ends_with("features.ttl")) {
      // The parse result and its arena die with `result`: copy nothing,
      // read now.
      saw_features = true;

      if (doc->statements.size() != 13)
        return fail("features.ttl: unexpected statement count");

      const auto *prefix = as<turtle::ast::PrefixDecl>(doc->statements[0]);
      if (prefix == nullptr || prefix->prefix != "ex" ||
          prefix->iri != "http://example.org/")
        return fail("features.ttl: @prefix ex: did not read back");

      const auto *sparqlPrefix =
          as<turtle::ast::SparqlPrefixDecl>(doc->statements[3]);
      if (sparqlPrefix == nullptr || sparqlPrefix->prefix != "foaf")
        return fail("features.ttl: PREFIX foaf: did not read back");

      // `ex:Alice a foaf:Person ; ex:age 42 ; ex:balance -3.5 ; ...`
      const auto *alice = as<turtle::ast::Triples>(doc->statements[7]);
      if (alice == nullptr || alice->properties == nullptr ||
          alice->properties->items.size() != 9)
        return fail("features.ttl: ex:Alice's properties did not read back");

      const auto *isA =
          as<turtle::ast::RdfType>(alice->properties->items[0]->predicate);
      if (isA == nullptr)
        return fail("features.ttl: the `a` shorthand did not read back");

      const auto *age = alice->properties->items[1];
      const auto *ageValue = as<turtle::ast::NumberLit>(age->objects[0]);
      if (ageValue == nullptr || ageValue->text != "42")
        return fail("features.ttl: ex:age 42 did not read back");

      const auto *balance = alice->properties->items[2];
      const auto *balanceValue =
          as<turtle::ast::NumberLit>(balance->objects[0]);
      if (balanceValue == nullptr || balanceValue->text != "-3.5")
        return fail("features.ttl: ex:balance -3.5 did not read back");

      const auto *name = alice->properties->items[5];
      const auto *nameValue = as<turtle::ast::RdfLiteral>(name->objects[0]);
      if (nameValue == nullptr || nameValue->value != "Alice" ||
          nameValue->lang != "en")
        return fail("features.ttl: ex:name \"Alice\"@en did not read back");

      const auto *id = alice->properties->items[6];
      const auto *idValue = as<turtle::ast::RdfLiteral>(id->objects[0]);
      const auto *idType =
          idValue != nullptr ? as<turtle::ast::PrefixedTerm>(idValue->datatype)
                             : nullptr;
      if (idValue == nullptr || idValue->value != "007" || idType == nullptr ||
          idType->value != "ex:Code")
        return fail("features.ttl: ex:id \"007\"^^ex:Code did not read back");

      const auto *favorites = alice->properties->items[7];
      const auto *collection =
          as<turtle::ast::Collection>(favorites->objects[0]);
      if (collection == nullptr || collection->items.size() != 3)
        return fail("features.ttl: ex:favorites (...) did not read back");
    }
  }

  if (!saw_features)
    return fail("features.ttl was not exercised");

  std::puts("turtle: the corpus parses");
  return 0;
}
