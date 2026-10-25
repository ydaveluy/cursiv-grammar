#include "smtlibv2/core/validation/Smtlibv2Validator.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <memory>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "smtlibv2/core/CoreServices.hpp"

namespace smtlibv2 {
namespace {
template <class Range>
void unique_names(const Range& xs, cursiv::ValidationAcceptor& a) {
  std::unordered_set<std::string_view> names;
  for (const auto* x : xs)
    if (!names.insert(x->name).second)
      a.error(*x, "a binding name must be unique in this list");
}
const ast::SpecConstant* constant(const ast::Value& v) {
  const auto* p = std::get_if<ast::SpecConstant*>(&v);
  return p ? *p : nullptr;
}
}  // namespace
void Smtlibv2Validator::checkOption(const ast::SetOptionCommand& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (!n.option) return;
  const auto& o = *n.option;
  if (std::holds_alternative<std::monostate>(o.value)) {
    a.error(o, "an option requires a value");
    return;
  }
  constexpr std::array boolean_options = {
      ":global-declarations", ":interactive-mode",
      ":print-success",       ":produce-assertions",
      ":produce-assignments", ":produce-models",
      ":produce-proofs",      ":produce-unsat-assumptions",
      ":produce-unsat-cores"};
  constexpr std::array numeral_options = {
      ":random-seed", ":reproducible-resource-limit", ":verbosity"};
  const auto* c = constant(o.value);
  if (std::find(boolean_options.begin(), boolean_options.end(), o.keyword) !=
      boolean_options.end()) {
    const auto* v = std::get_if<std::string_view>(&o.value);
    if (!v || (*v != "true" && *v != "false"))
      a.error(o, "this option requires true or false")
          .property<&ast::Attribute::value>();
  } else if (std::find(numeral_options.begin(), numeral_options.end(),
                       o.keyword) != numeral_options.end()) {
    if (!c || c->isString || c->text.empty() ||
        c->text.find_first_not_of("0123456789") != std::string_view::npos)
      a.error(o, "this option requires a numeral")
          .property<&ast::Attribute::value>();
  } else if (o.keyword == ":diagnostic-output-channel" ||
             o.keyword == ":regular-output-channel") {
    if (!c || !c->isString)
      a.error(o, "this option requires a string")
          .property<&ast::Attribute::value>();
  }
}
void Smtlibv2Validator::checkApplication(const ast::AppTerm& n,
                                         cursiv::ValidationAcceptor& a) const {
  if (n.args.empty())
    a.error(n,
            "an application requires arguments; a constant is written without "
            "parentheses");
}
void Smtlibv2Validator::checkLet(const ast::LetTerm& n,
                                 cursiv::ValidationAcceptor& a) const {
  unique_names(n.bindings, a);
}
void Smtlibv2Validator::checkQuantifier(const ast::QuantTerm& n,
                                        cursiv::ValidationAcceptor& a) const {
  if (n.vars.empty()) a.error(n, "a quantifier requires a bound variable");
  unique_names(n.vars, a);
}
void Smtlibv2Validator::checkMatch(const ast::MatchTerm& n,
                                   cursiv::ValidationAcceptor& a) const {
  if (n.cases.empty()) a.error(n, "match requires a case");
}
void Smtlibv2Validator::checkAnnotation(const ast::AnnotTerm& n,
                                        cursiv::ValidationAcceptor& a) const {
  if (n.attrs.empty()) a.error(n, "an annotated term requires an attribute");
}
void Smtlibv2Validator::checkRecursive(const ast::DefineFunsRecCommand& n,
                                       cursiv::ValidationAcceptor& a) const {
  if (n.decls.empty())
    a.error(n, "recursive definitions require a function declaration");
  if (n.decls.size() != n.bodies.size())
    a.error(n, "each recursive function declaration requires one body");
  unique_names(n.decls, a);
}
void Smtlibv2Validator::checkDatatypes(const ast::DeclareDatatypesCommand& n,
                                       cursiv::ValidationAcceptor& a) const {
  if (n.sorts.empty()) a.error(n, "datatype declarations require a sort");
  if (n.sorts.size() != n.decls.size()) {
    a.error(n, "each declared datatype sort requires one datatype body");
    return;
  }
  unique_names(n.sorts, a);
  for (std::size_t i = 0; i < n.sorts.size(); ++i) {
    const auto& text = n.sorts[i]->arity;
    std::uint64_t arity = 0;
    const auto [end, ec] =
        std::from_chars(text.data(), text.data() + text.size(), arity);
    if (ec != std::errc{} || end != text.data() + text.size() ||
        arity != n.decls[i]->params.size())
      a.error(*n.sorts[i],
              "the datatype arity must equal its number of type parameters");
  }
}
void Smtlibv2Validator::checkDatatype(const ast::DatatypeDec& n,
                                      cursiv::ValidationAcceptor& a) const {
  if (n.constructors.empty()) a.error(n, "a datatype requires a constructor");
  unique_names(n.constructors, a);
}
void Smtlibv2Validator::checkFunction(const ast::FunctionDef& n,
                                      cursiv::ValidationAcceptor& a) const {
  unique_names(n.params, a);
}
void register_validation_checks(Smtlibv2CoreServices& services) {
  using V = Smtlibv2Validator;
  services.validator = std::make_unique<const V>(services);
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkOption>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkApplication>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLet>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkQuantifier>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMatch>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAnnotation>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRecursive>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDatatypes>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDatatype>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(v)});
}
}  // namespace smtlibv2
