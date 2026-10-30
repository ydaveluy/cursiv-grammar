#include "turtledoc/core/validation/TurtleDocValidator.hpp"

#include <charconv>
#include <cstdint>
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

// Conversions retain a replacement value for malformed Unicode so an editor
// still gets an AST. Diagnose the original escape through its CST anchor.
void check_unicode(const cursiv::AstNode &node, std::string_view member,
                   std::string_view source,
                   cursiv::ValidationAcceptor &accept) {
  const auto span =
      accept.featureSpan(node, member, 0).value_or(accept.spanOf(node));
  if (span.begin > span.end || span.end > source.size())
    return;
  const auto text = source.substr(span.begin, span.end - span.begin);
  for (std::size_t i = 0; i + 1 < text.size(); ++i) {
    if (text[i] != '\\')
      continue;
    const char kind = text[++i];
    if (kind != 'u' && kind != 'U')
      continue;
    const std::size_t digits = kind == 'u' ? 4 : 8;
    if (i + 1 + digits > text.size())
      continue;
    std::uint32_t scalar = 0;
    const auto parsed = std::from_chars(
        text.data() + i + 1, text.data() + i + 1 + digits, scalar, 16);
    if (parsed.ec == std::errc{} &&
        parsed.ptr == text.data() + i + 1 + digits &&
        (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))) {
      const auto begin = span.begin + static_cast<std::uint32_t>(i - 1);
      accept
          .error(node, "Unicode escape is not a Unicode scalar value",
                 "turtledoc.unicode-scalar")
          .range({begin, begin + static_cast<std::uint32_t>(digits + 2)});
    }
    i += digits;
  }
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
  if (term->type == &cursiv::ast_type_tag<ast::IriRef>) {
    check_unicode(*term, "value", source, accept);
  } else if (term->type == &cursiv::ast_type_tag<ast::PrefixedTerm>) {
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
    check_unicode(*term, "value", source, accept);
    check_term(static_cast<const ast::RdfLiteral *>(term)->datatype, prefixes,
               source, accept);
  }
}

} // namespace

void TurtleDocValidator::checkDocument(const ast::Document &document,
                                    cursiv::ValidationAcceptor &accept) const {
  const auto *owner = document.getDocument();
  const std::string_view source =
      owner ? owner->textDocument().getText() : std::string_view{};
  Prefixes prefixes;
  for (const ast::Statement *const statement : document.statements) {
    if (statement == nullptr)
      continue;
    if (statement->type == &cursiv::ast_type_tag<ast::PrefixDecl>) {
      check_unicode(*statement, "iri", source, accept);
      prefixes.insert(static_cast<const ast::PrefixDecl *>(statement)->prefix);
      const auto text = spelling(*statement, source, accept);
      if (!text.empty() && !text.starts_with("@prefix"))
        accept.error(*statement, "use lowercase @prefix");
    } else if (statement->type == &cursiv::ast_type_tag<ast::BaseDecl>) {
      check_unicode(*statement, "iri", source, accept);
      const auto text = spelling(*statement, source, accept);
      if (!text.empty() && !text.starts_with("@base"))
        accept.error(*statement, "use lowercase @base");
    } else if (statement->type ==
               &cursiv::ast_type_tag<ast::SparqlPrefixDecl>) {
      check_unicode(*statement, "iri", source, accept);
      prefixes.insert(
          static_cast<const ast::SparqlPrefixDecl *>(statement)->prefix);
    } else if (statement->type == &cursiv::ast_type_tag<ast::SparqlBaseDecl>) {
      check_unicode(*statement, "iri", source, accept);
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

} // namespace turtledocdoc
