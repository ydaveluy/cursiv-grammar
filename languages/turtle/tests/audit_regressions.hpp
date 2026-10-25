#pragma once

#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "turtle/core/CoreServices.hpp"
#include "turtle/core/ast.hpp"
#include "turtle/generated/Turtle/Module.hpp"

namespace audit_regressions {
namespace ast = turtle::ast;
template <class T> const T *as(const cursiv::AstNode *node) {
  return cursiv::ast_cast<T>(node);
}
inline const ast::Triples *triple(const ast::Document *document,
                                  std::size_t index) {
  return document && index < document->statements.size()
             ? as<ast::Triples>(document->statements[index])
             : nullptr;
}
inline const ast::Term *object(const ast::Triples *triples, std::size_t index) {
  const auto *properties = triples ? triples->properties : nullptr;
  if (!properties || index >= properties->items.size())
    return nullptr;
  const auto *property = properties->items[index];
  return property && property->objects.size() == 1 ? property->objects[0]
                                                   : nullptr;
}
inline bool run() {
  const auto parser = turtle::generated::Turtle::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  {
    const std::string source = R"SRC(@prefix ex: <urn:ex:> .
<urn:\u03BB> ex:p "A\n\u0042\U0001F600"; ex:q ex:;
ex:r '''line\tvalue'''; ex:t """two\rwords""" .)SRC";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "escaped RDF terms parse");
    const auto *document = result.value_as<ast::Document>();
    const auto *statement = triple(document, 1);
    const auto *iri = statement ? as<ast::IriRef>(statement->subject) : nullptr;
    check(iri && iri->value == "urn:λ", "IRI Unicode value");
    const auto *literal = as<ast::RdfLiteral>(object(statement, 0));
    check(literal && literal->value == "A\nB😀", "decoded string value");
    const auto *name = as<ast::PrefixedTerm>(object(statement, 1));
    check(name && name->value == "ex:", "namespace term retains colon");
    const auto *single = as<ast::RdfLiteral>(object(statement, 2));
    const auto *quoted = as<ast::RdfLiteral>(object(statement, 3));
    check(single && single->value == "line\tvalue" && quoted &&
              quoted->value == "two\rwords",
          "long string values");
    const auto *prefix = document && !document->statements.empty()
                             ? as<ast::PrefixDecl>(document->statements[0])
                             : nullptr;
    check(prefix && prefix->prefix == "ex" && prefix->iri == "urn:ex:",
          "declaration prefix has no colon");
  }
  {
    const std::string source = "[# comment\n] <urn:p> (# comment\n) . [] .";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(),
          "comments in anonymous nodes and collections");
    const auto *document = result.value_as<ast::Document>();
    const auto *first = triple(document, 0);
    const auto *blank = first ? as<ast::BlankNode>(first->subject) : nullptr;
    const auto *collection = as<ast::Collection>(object(first, 0));
    check(blank && blank->label.empty() && collection &&
              collection->items.empty(),
          "empty anonymous data retains AST structure");
    const auto *empty = triple(document, 1);
    check(empty && as<ast::BlankNode>(empty->subject) && !empty->properties,
          "standalone anonymous node");
  }
  {
    const std::string source = "prefix 𐀀: <urn:ex:>\nbase <urn:base:>\n"
                               "𐀀:s 𐀀:p 𐀀:o .";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "astral names and case-insensitive SPARQL directives");
    const auto *document = result.value_as<ast::Document>();
    const auto *declaration =
        document && !document->statements.empty()
            ? as<ast::SparqlPrefixDecl>(document->statements[0])
            : nullptr;
    const auto *statement = triple(document, 2);
    const auto *subject =
        statement ? as<ast::PrefixedTerm>(statement->subject) : nullptr;
    check(declaration && declaration->prefix == "𐀀" && subject &&
              subject->value == "𐀀:s",
          "astral prefix values");
  }
  {
    const std::string source = "<urn:s> <urn:p> 18446744073709551616;"
                               "<urn:q> true; <urn:r> false .";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "unbounded numeric and boolean values");
    const auto *statement = triple(result.value_as<ast::Document>(), 0);
    const auto *number = as<ast::NumberLit>(object(statement, 0));
    const auto *yes = as<ast::BoolLit>(object(statement, 1));
    const auto *no = as<ast::BoolLit>(object(statement, 2));
    check(number && number->text == "18446744073709551616" && yes &&
              yes->value && no && !no->value,
          "exact number lexeme and boolean values");
  }
  {
    const auto language =
        cursiv::create_language<turtle::TurtleCoreServices,
                                turtle::TurtleSharedCoreServices,
                                turtle::install_core_module>();
    const struct {
      const char *text;
      int errors;
    } cases[] = {
        {":s :p :o .", 3},
        {"ex:s ex:p ex:o . @prefix ex: <urn:ex:> .", 3},
        {"@PreFix ex: <urn:ex:> . ex:s ex:p True .", 2},
        {"<urn:s> .", 1},
        {"prefix : <urn:ex:> :s :p : . [] .", 0},
    };
    for (const auto &control : cases) {
      const std::string source = control.text;
      cursiv::Document document(
          cursiv::TextDocument::create("memory:turtle", "turtle", 0, source));
      document.parseResult = parser->parse(source);
      check(document.parseResult.accepted &&
                !document.parseResult.cst().recovered(),
            "RDF restrictions stay outside parser");
      if (!document.parseResult.value)
        continue;
      document.parseResult.value->parent = &document;
      int errors = 0;
      auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
      cursiv::ValidationAcceptor accept(sink, document, source);
      language.language.validator->checkDocument(
          *document.parseResult.value_as<ast::Document>(), accept);
      check(errors == control.errors, "RDF runtime diagnostic count");
    }
  }
  return ok;
}
} // namespace audit_regressions
