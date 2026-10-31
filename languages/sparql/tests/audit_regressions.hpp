#pragma once

#include <cstdio>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "sparql/core/CoreServices.hpp"
#include "sparql/core/ast.hpp"
#include "sparql/generated/Sparql/Module.hpp"

namespace audit_regressions {
namespace ast = sparql::ast;
template <class T> const T *as(const cursiv::AstNode *p) {
  return cursiv::ast_cast<T>(p);
}
inline const ast::SelectQuery *select(const cursiv::ParseResult &result) {
  const auto *query = result.value_as<ast::Query>();
  const auto *form =
      query ? std::get_if<ast::SelectQuery *>(&query->form) : nullptr;
  return form ? *form : nullptr;
}
inline const ast::Triple *first_triple(const ast::GroupGraphPattern *pattern) {
  if (!pattern)
    return nullptr;
  for (const auto &element : pattern->elements)
    if (const auto *triple = std::get_if<ast::Triple *>(&element))
      return *triple;
  return nullptr;
}
inline bool run() {
  const auto services =
      cursiv::create_language<sparql::SparqlCoreServices,
                              sparql::SparqlSharedCoreServices,
                              sparql::install_core_module>();
  const auto& parser = services.language.parser;
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  {
    const std::string source = "#\nSELECT * WHERE {}";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "comment and empty graph");
  }
  {
    const std::string source =
        R"SRC(PREFIX ex: <urn:ex:> SELECT * WHERE { <urn:\u03BB> ex:p "A\n\u0042\U0001F600"; ex:q ex: })SRC";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(),
          "escaped RDF terms parse");
    const auto *form = select(result);
    const auto *triple = form ? first_triple(form->where) : nullptr;
    const auto *iri = triple ? as<ast::IriRef>(triple->subject) : nullptr;
    check(iri && iri->value == "urn:λ", "IRI Unicode value");
    const auto *properties = triple ? triple->properties : nullptr;
    check(properties && properties->items.size() == 2, "two predicate groups");
    if (properties && properties->items.size() == 2) {
      const auto *first = properties->items[0];
      const auto *literal = first && first->objects.size() == 1
                                ? as<ast::StringLit>(first->objects[0])
                                : nullptr;
      check(literal && literal->value == "A\nB😀", "decoded literal value");
      const auto *second = properties->items[1];
      const auto *name = second && second->objects.size() == 1
                             ? as<ast::PrefixedTerm>(second->objects[0])
                             : nullptr;
      check(name && name->value == "ex:", "namespace term retains colon");
    }
    const auto *query = result.value_as<ast::Query>();
    check(query && query->prologue && query->prologue->prefixes.size() == 1 &&
              query->prologue->prefixes[0]->prefix == "ex",
          "declaration retains prefix without colon");
  }
  {
    const std::string source =
        R"SRC(SELECT ?\u0061 WHERE { ?\u0061 <urn:p> <urn:o> })SRC";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(),
          "Unicode variable spelling");
    const auto *form = select(result);
    const auto *var =
        form && form->vars.size() == 1 ? as<ast::Var>(form->vars[0]) : nullptr;
    check(var && var->name == "a", "decoded variable name");
  }
  {
    const std::string source =
        "SELECT ?v (-?v AS ?minus) (TRUE AS ?truth) WHERE { ?v <urn:p> 1 }";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(),
          "expression projections");
    const auto *form = select(result);
    check(form && form->vars.size() == 3, "projection count");
    if (form && form->vars.size() == 3) {
      const auto *minus = as<ast::Projection>(form->vars[1]);
      const auto *unary =
          minus ? as<ast::UnaryExpr>(minus->expression) : nullptr;
      check(minus && minus->variable && minus->variable->name == "minus" &&
                unary && unary->op == "-" && as<ast::Var>(unary->operand),
            "projection expression and alias");
      const auto *truth = as<ast::Projection>(form->vars[2]);
      const auto *boolean =
          truth ? as<ast::BoolLit>(truth->expression) : nullptr;
      check(truth && truth->variable && truth->variable->name == "truth" &&
                boolean && boolean->value,
            "boolean projection value");
    }
  }
  {
    const std::string source =
        "ASK WHERE { [# comment\n] <urn:p> (# comment\n) }";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(),
          "comments in anonymous data");
    const auto *query = result.value_as<ast::Query>();
    const auto *ask =
        query ? std::get_if<ast::AskQuery *>(&query->form) : nullptr;
    const auto *triple = ask && *ask ? first_triple((*ask)->where) : nullptr;
    const auto *blank = triple ? as<ast::BlankNode>(triple->subject) : nullptr;
    check(blank && blank->label.empty(), "anonymous blank node");
    const auto *property =
        triple && triple->properties && triple->properties->items.size() == 1
            ? triple->properties->items[0]
            : nullptr;
    check(property && property->objects.size() == 1 &&
              as<ast::NilTerm>(property->objects[0]),
          "empty RDF collection");
  }
  {
    const std::string source = "SELECT * WHERE {} LIMIT 18446744073709551616 "
                               "OFFSET 999999999999999999999";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "unbounded modifier lexemes");
    const auto *form = select(result);
    check(form && form->modifier &&
              form->modifier->limit == "18446744073709551616" &&
              form->modifier->offset == "999999999999999999999",
          "exact LIMIT/OFFSET");
  }
  {
    const std::string source = "ASK WHERE { FILTER(1+2*3=7 && 2-1=1) }";
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered(), "arithmetic syntax");
    const auto *query = result.value_as<ast::Query>();
    const auto *ask =
        query ? std::get_if<ast::AskQuery *>(&query->form) : nullptr;
    const auto *filter =
        ask && *ask && (*ask)->where && (*ask)->where->elements.size() == 1
            ? std::get_if<ast::Filter *>(&(*ask)->where->elements[0])
            : nullptr;
    const auto *logical = filter && *filter
                              ? as<ast::BinaryExpr>((*filter)->constraint)
                              : nullptr;
    const auto *equal = logical ? as<ast::BinaryExpr>(logical->left) : nullptr;
    const auto *add = equal ? as<ast::BinaryExpr>(equal->left) : nullptr;
    const auto *multiply = add ? as<ast::BinaryExpr>(add->right) : nullptr;
    check(logical && logical->op == ast::Operator::logical_and && equal &&
              equal->op == ast::Operator::equal && add &&
              add->op == ast::Operator::add && multiply &&
              multiply->op == ast::Operator::multiply,
          "native arithmetic precedence");
  }
  {
    const auto language =
        cursiv::create_language<sparql::SparqlCoreServices,
                                sparql::SparqlSharedCoreServices,
                                sparql::install_core_module>();
    for (const std::string source :
         {"ASK { FILTER(REGEX(\"a\")) }", "ASK { FILTER(BOUND(1)) }",
          "ASK { FILTER(STR(1, 2)) }"}) {
      cursiv::Document document(
          cursiv::TextDocument::create("memory:builtin", "sparql", 0, source));
      document.parseResult = parser->parse(source);
      check(document.parseResult.accepted &&
                !document.parseResult.cst().recovered(),
            "builtin restrictions stay outside parser");
      int errors = 0;
      auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
      cursiv::ValidationAcceptor accept(sink, document, source);
      const auto *query = document.parseResult.value_as<ast::Query>();
      const auto *ask =
          query ? std::get_if<ast::AskQuery *>(&query->form) : nullptr;
      const ast::BuiltInCall *call = nullptr;
      if (ask && *ask && (*ask)->where && (*ask)->where->elements.size() == 1) {
        const auto *filter =
            std::get_if<ast::Filter *>(&(*ask)->where->elements[0]);
        if (filter && *filter)
          call = as<ast::BuiltInCall>((*filter)->constraint);
      }
      check(call != nullptr, "builtin AST retains invalid operands");
      if (call)
        language.language.validator->checkBuiltInCall(*call, accept);
      check(errors == 1, "precise builtin runtime error");
    }
    const std::string source = "SELECT * WHERE { :s :p :o }";
    cursiv::Document document(
        cursiv::TextDocument::create("memory:prefix", "sparql", 0, source));
    document.parseResult = parser->parse(source);
    check(document.parseResult.accepted &&
              !document.parseResult.cst().recovered(),
          "undeclared prefix remains parseable");
    int errors = 0;
    auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, document, source);
    if (const auto *query = document.parseResult.value_as<ast::Query>())
      language.language.validator->checkPrefixesDeclared(*query, accept);
    check(errors == 3, "default prefix must be declared");
  }
  for (const auto source : {"ASK { <urn:bad iri> <urn:p> <urn:o> }",
                            "ASK { <urn:s> <urn:p> \"a\"@é }"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "SPARQL 1.0 excludes ASCII controls/spaces in IRIs and non-ASCII language tags");
  }
  {
    const auto result = parser->parse("ASK { <urn:s> <urn:p> \"a\"@en-US9 }");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "SPARQL ASCII language subtags include digits after the first group");
  }
  {
    const auto language = cursiv::create_language<sparql::SparqlCoreServices,
        sparql::SparqlSharedCoreServices, sparql::install_core_module>();
    const std::string source = "PREFIX ex: <urn:a:> PREFIX ex: <urn:b:> ASK {}";
    cursiv::Document document(cursiv::TextDocument::create("memory:duplicate-prefix", "sparql", 0, source));
    document.parseResult = parser->parse(source);
    check(document.parseResult.accepted && !document.parseResult.cst().recovered(),
          "prefix redeclarations remain represented for validation");
    int duplicates = 0;
    auto sink = [&](cursiv::Diagnostic&& diagnostic) {
      if (diagnostic.code && *diagnostic.code == "sparql.prefix.duplicate") ++duplicates;
    };
    cursiv::ValidationAcceptor accept(sink, document, source);
    if (const auto* query = document.parseResult.value_as<ast::Query>())
      language.language.validator->checkPrefixesDeclared(*query, accept);
    check(duplicates == 1, "SPARQL 1.0 A.5 forbids redeclaring a prefix");
  }
  return ok;
}
} // namespace audit_regressions
