#include "sparql/core/validation/SparqlValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sparql/core/CoreServices.hpp"

namespace sparql {

namespace {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using Prefixes = std::unordered_set<std::string_view>;

std::string_view prefix_of(std::string_view name) {
  const std::size_t colon = name.find(':');
  return colon == std::string_view::npos ? name : name.substr(0, colon);
}

void check_term(const ast::Term *term, const Prefixes &prefixes,
                cursiv::ValidationAcceptor &accept) {
  if (term == nullptr)
    return;
  using cursiv::ast_type_tag;
  if (term->type == &ast_type_tag<ast::PrefixedTerm>) {
    const auto &prefixed = *static_cast<const ast::PrefixedTerm *>(term);
    const std::string_view prefix = prefix_of(prefixed.value);
    if (!prefixes.contains(prefix))
      accept.error(prefixed, "prefix is not declared")
          .property<&ast::PrefixedTerm::value>();
  } else if (term->type == &ast_type_tag<ast::StringLit>) {
    check_term(static_cast<const ast::StringLit *>(term)->datatype, prefixes,
               accept);
  } else if (term->type == &ast_type_tag<ast::Collection>) {
    for (const ast::Term *const item :
         static_cast<const ast::Collection *>(term)->items)
      check_term(item, prefixes, accept);
  } else if (term->type == &ast_type_tag<ast::BlankNodePropertyList>) {
    const auto *const props =
        static_cast<const ast::BlankNodePropertyList *>(term)->properties;
    if (props != nullptr)
      for (const ast::PredicateObject *const po : props->items) {
        if (po == nullptr)
          continue;
        check_term(po->predicate, prefixes, accept);
        for (const ast::Term *const object : po->objects)
          check_term(object, prefixes, accept);
      }
  } else if (term->type == &ast_type_tag<ast::Projection>) {
    check_term(static_cast<const ast::Projection *>(term)->expression, prefixes,
               accept);
  } else if (term->type == &ast_type_tag<ast::UnaryExpr>) {
    check_term(static_cast<const ast::UnaryExpr *>(term)->operand, prefixes,
               accept);
  } else if (term->type == &ast_type_tag<ast::BinaryExpr>) {
    const auto &bin = *static_cast<const ast::BinaryExpr *>(term);
    check_term(bin.left, prefixes, accept);
    check_term(bin.right, prefixes, accept);
  } else if (term->type == &ast_type_tag<ast::FuncCall>) {
    const auto &call = *static_cast<const ast::FuncCall *>(term);
    check_term(call.target, prefixes, accept);
    for (const ast::Term *const arg : call.args)
      check_term(arg, prefixes, accept);
  } else if (term->type == &ast_type_tag<ast::BuiltInCall>) {
    for (const ast::Term *const arg :
         static_cast<const ast::BuiltInCall *>(term)->args)
      check_term(arg, prefixes, accept);
  }
}

void check_properties(const ast::PredicateObjectList *properties,
                      const Prefixes &prefixes,
                      cursiv::ValidationAcceptor &accept) {
  if (properties == nullptr)
    return;
  for (const ast::PredicateObject *const po : properties->items) {
    if (po == nullptr)
      continue;
    check_term(po->predicate, prefixes, accept);
    for (const ast::Term *const object : po->objects)
      check_term(object, prefixes, accept);
  }
}

void check_triple(const ast::Triple *triple, const Prefixes &prefixes,
                  cursiv::ValidationAcceptor &accept) {
  if (triple == nullptr)
    return;
  check_term(triple->subject, prefixes, accept);
  check_properties(triple->properties, prefixes, accept);
}

void check_pattern(const ast::GroupGraphPattern *pattern,
                   const Prefixes &prefixes,
                   cursiv::ValidationAcceptor &accept) {
  if (pattern == nullptr)
    return;
  for (const auto &element : pattern->elements) {
    std::visit(overloaded{[&](const ast::Triple *t) {
                            check_triple(t, prefixes, accept);
                          },
                          [&](const ast::OptionalGraphPattern *o) {
                            if (o != nullptr)
                              check_pattern(o->pattern, prefixes, accept);
                          },
                          [&](const ast::GroupOrUnionGraphPattern *u) {
                            if (u == nullptr)
                              return;
                            for (const ast::GroupGraphPattern *const alt :
                                 u->alternatives)
                              check_pattern(alt, prefixes, accept);
                          },
                          [&](const ast::GraphGraphPattern *g) {
                            if (g == nullptr)
                              return;
                            check_term(g->graph, prefixes, accept);
                            check_pattern(g->pattern, prefixes, accept);
                          },
                          [&](const ast::Filter *f) {
                            if (f != nullptr)
                              check_term(f->constraint, prefixes, accept);
                          }},
               element);
  }
}

void check_datasets(const cursiv::vector<ast::DatasetClause *> &datasets,
                    const Prefixes &prefixes,
                    cursiv::ValidationAcceptor &accept) {
  for (const ast::DatasetClause *const clause : datasets)
    if (clause != nullptr)
      check_term(clause->graph, prefixes, accept);
}

void check_modifier(const ast::SolutionModifier *modifier,
                    const Prefixes &prefixes,
                    cursiv::ValidationAcceptor &accept) {
  if (modifier == nullptr)
    return;
  for (const ast::OrderCondition *const condition : modifier->orderBy)
    if (condition != nullptr)
      check_term(condition->value, prefixes, accept);
}

} // namespace

void SparqlValidator::checkPrefixesDeclared(
    const ast::Query &query, cursiv::ValidationAcceptor &accept) const {
  Prefixes prefixes;
  if (query.prologue != nullptr)
    for (const ast::PrefixDecl *const decl : query.prologue->prefixes)
      if (decl != nullptr)
        prefixes.insert(decl->prefix);

  std::visit(overloaded{[&](const ast::SelectQuery *q) {
                          if (q == nullptr)
                            return;
                          for (const ast::Term *const var : q->vars)
                            check_term(var, prefixes, accept);
                          check_datasets(q->datasets, prefixes, accept);
                          check_pattern(q->where, prefixes, accept);
                          check_modifier(q->modifier, prefixes, accept);
                        },
                        [&](const ast::ConstructQuery *q) {
                          if (q == nullptr)
                            return;
                          for (const ast::Triple *const triple :
                               q->templateTriples)
                            check_triple(triple, prefixes, accept);
                          check_datasets(q->datasets, prefixes, accept);
                          check_pattern(q->where, prefixes, accept);
                          check_modifier(q->modifier, prefixes, accept);
                        },
                        [&](const ast::DescribeQuery *q) {
                          if (q == nullptr)
                            return;
                          for (const ast::Term *const var : q->vars)
                            check_term(var, prefixes, accept);
                          check_datasets(q->datasets, prefixes, accept);
                          check_pattern(q->where, prefixes, accept);
                          check_modifier(q->modifier, prefixes, accept);
                        },
                        [&](const ast::AskQuery *q) {
                          if (q == nullptr)
                            return;
                          check_datasets(q->datasets, prefixes, accept);
                          check_pattern(q->where, prefixes, accept);
                        }},
             query.form);
}

void SparqlValidator::checkBuiltInCall(
    const ast::BuiltInCall &call, cursiv::ValidationAcceptor &accept) const {
  const bool two = call.fn == ast::BuiltIn::lang_matches ||
                   call.fn == ast::BuiltIn::same_term;
  const std::size_t minimum = two || call.fn == ast::BuiltIn::regex ? 2 : 1;
  const std::size_t maximum = call.fn == ast::BuiltIn::regex ? 3 : minimum;
  if (call.args.size() < minimum || call.args.size() > maximum)
    accept.error(
        call,
        "This builtin expects " + std::to_string(minimum) +
            (minimum != maximum ? " to " + std::to_string(maximum) : "") +
            " argument(s).",
        "sparql.builtin.arity");
  if (call.fn == ast::BuiltIn::bound && call.args.size() == 1 &&
      !cursiv::ast_cast<ast::Var>(call.args[0]))
    accept.error(call, "BOUND expects a variable.", "sparql.bound.variable");
}

void register_validation_checks(SparqlCoreServices &services) {
  using V = SparqlValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkPrefixesDeclared>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBuiltInCall>(
           validator)});
}

} // namespace sparql
