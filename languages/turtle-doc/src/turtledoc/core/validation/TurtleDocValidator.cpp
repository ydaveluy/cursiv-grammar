#include "turtledoc/core/validation/TurtleDocValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "turtledoc/core/CoreServices.hpp"

namespace turtledoc {

namespace {

using Prefixes = std::unordered_set<std::string_view>;

std::string_view spelling(const cursiv::AstNode &node, std::string_view source,
                          cursiv::ValidationAcceptor &accept) {
  const auto span = accept.spanOf(node);
  return span.begin <= span.end && span.end <= source.size()
             ? source.substr(span.begin, span.end - span.begin)
             : std::string_view{};
}

std::string_view prefix_of(std::string_view name) {
  const std::size_t colon = name.find(':');
  return colon == std::string_view::npos ? name : name.substr(0, colon);
}

void check_term(const ast::Term *term, const Prefixes &prefixes,
                std::string_view source, cursiv::ValidationAcceptor &accept);

void check_properties(const ast::PredicateObjectList *properties,
                      const Prefixes &prefixes, std::string_view source,
                      cursiv::ValidationAcceptor &accept) {
  if (properties == nullptr)
    return;
  for (const ast::PredicateObject *const po : properties->items) {
    if (po == nullptr)
      continue;
    check_term(po->predicate, prefixes, source, accept);
    for (const ast::Term *const object : po->objects)
      check_term(object, prefixes, source, accept);
  }
}

void check_term(const ast::Term *term, const Prefixes &prefixes,
                std::string_view source, cursiv::ValidationAcceptor &accept) {
  if (term == nullptr)
    return;
  if (term->type == &cursiv::ast_type_tag<ast::PrefixedTerm>) {
    const auto &prefixed = *static_cast<const ast::PrefixedTerm *>(term);
    const std::string_view prefix = prefix_of(prefixed.value);
    if (!prefixes.contains(prefix))
      accept.error(prefixed, "prefix is not declared")
          .property<&ast::PrefixedTerm::value>();
  } else if (term->type == &cursiv::ast_type_tag<ast::BoolLit>) {
    const auto text = spelling(*term, source, accept);
    if (!text.empty() && text != "true" && text != "false")
      accept.error(*term, "Turtle booleans must be lowercase");
  } else if (term->type == &cursiv::ast_type_tag<ast::Collection>) {
    for (const ast::Term *const item :
         static_cast<const ast::Collection *>(term)->items)
      check_term(item, prefixes, source, accept);
  } else if (term->type == &cursiv::ast_type_tag<ast::BlankNodePropertyList>) {
    check_properties(
        static_cast<const ast::BlankNodePropertyList *>(term)->properties,
        prefixes, source, accept);
  } else if (term->type == &cursiv::ast_type_tag<ast::RdfLiteral>) {
    check_term(static_cast<const ast::RdfLiteral *>(term)->datatype, prefixes,
               source, accept);
  }
}

} // namespace

void TurtleDocValidator::checkDocument(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  const auto *owner = document.getDocument();
  const std::string_view source =
      owner ? owner->textDocument().getText() : std::string_view{};
  Prefixes prefixes;
  for (const ast::Statement *const statement : document.statements) {
    if (statement == nullptr)
      continue;
    if (statement->type == &cursiv::ast_type_tag<ast::PrefixDecl>) {
      prefixes.insert(static_cast<const ast::PrefixDecl *>(statement)->prefix);
      const auto text = spelling(*statement, source, accept);
      if (!text.empty() && !text.starts_with("@prefix"))
        accept.error(*statement, "use lowercase @prefix");
    } else if (statement->type == &cursiv::ast_type_tag<ast::BaseDecl>) {
      const auto text = spelling(*statement, source, accept);
      if (!text.empty() && !text.starts_with("@base"))
        accept.error(*statement, "use lowercase @base");
    } else if (statement->type ==
               &cursiv::ast_type_tag<ast::SparqlPrefixDecl>) {
      prefixes.insert(
          static_cast<const ast::SparqlPrefixDecl *>(statement)->prefix);
    } else if (statement->type == &cursiv::ast_type_tag<ast::Triples>) {
      const auto &triples = *static_cast<const ast::Triples *>(statement);
      check_term(triples.subject, prefixes, source, accept);
      check_properties(triples.properties, prefixes, source, accept);
      const auto *blank = cursiv::ast_cast<ast::BlankNode>(triples.subject);
      if (!triples.properties && !(blank && blank->label.empty()) &&
          !cursiv::ast_cast<ast::BlankNodePropertyList>(triples.subject))
        accept.error(triples,
                     "expected a predicate and object after the subject");
    }
  }
}

void register_validation_checks(TurtleDocCoreServices &services) {
  using V = TurtleDocValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(
          validator)});
}

} // namespace turtledoc
