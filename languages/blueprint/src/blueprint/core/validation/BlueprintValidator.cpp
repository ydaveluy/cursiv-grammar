#include "blueprint/core/validation/BlueprintValidator.hpp"
#include "blueprint/core/CoreServices.hpp"
#include "blueprint/core/Value.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>
namespace blueprint {
namespace {
template <auto Member, class T>
void checkName(const T &n, cursiv::ValidationAcceptor &a) {
  if ((n.*Member).find('-') != (n.*Member).npos)
    a.error(n, "An Android Blueprint identifier cannot contain a hyphen.")
        .template property<Member>()
        .code("blueprint.identifier");
}
void part(std::string &key, std::string_view value) {
  key += std::to_string(value.size()) + ":";
  key += value;
}
std::optional<std::string> fingerprint(const ast::Pattern &pattern) {
  return std::visit(
      [](auto p) -> std::optional<std::string> {
        using T = decltype(p);
        if constexpr (std::is_same_v<T, bool>)
          return p ? "bool:true" : "bool:false";
        else {
          if (!p)
            return {};
          if constexpr (std::is_same_v<T, ast::AnyPattern *>)
            return "any";
          else if constexpr (std::is_same_v<T, ast::DefaultPattern *>)
            return "default";
          else if constexpr (std::is_same_v<T, ast::IntegerLit *>) {
            auto v = integer_value(*p);
            return v ? std::optional<std::string>("int:" + std::to_string(*v))
                     : std::nullopt;
          } else {
            auto v = string_value(*p);
            return v.error.empty()
                       ? std::optional<std::string>("string:" + v.text)
                       : std::nullopt;
          }
        }
      },
      pattern);
}
} // namespace
void BlueprintValidator::checkInteger(const ast::IntegerLit &n,
                                      cursiv::ValidationAcceptor &a) const {
  if (!integer_value(n))
    a.error(n, "Expected a decimal signed 64-bit integer accepted by the "
               "Blueprint scanner; its source text is retained.")
        .property<&ast::IntegerLit::raw>()
        .code("blueprint.integer");
}
void BlueprintValidator::checkString(const ast::StringLit &n,
                                     cursiv::ValidationAcceptor &a) const {
  const auto value = string_value(n);
  if (!value.error.empty())
    a.error(n, value.error)
        .property<&ast::StringLit::raw>()
        .code("blueprint.string");
}
void BlueprintValidator::checkProperty(const ast::Property &n,
                                       cursiv::ValidationAcceptor &a) const {
  checkName<&ast::Property::name>(n, a);
  const auto *module = cursiv::ast_cast<ast::Module>(n.parent);
  if (module || cursiv::ast_cast<ast::MapValue>(n.parent)) {
    const bool equals = module && module->parenthesized;
    if (n.equals != equals)
      a.error(n, equals ? "A property in a parenthesized module uses '='."
                        : "A property in a map or braced module uses ':'.")
          .code("blueprint.property-separator");
  }
}
void BlueprintValidator::checkModule(const ast::Module &n,
                                     cursiv::ValidationAcceptor &a) const {
  checkName<&ast::Module::type>(n, a);
  if (n.properties.empty() && n.trailingComma)
    a.error(n, "An empty module cannot contain a comma.")
        .code("blueprint.empty-comma");
}
void BlueprintValidator::checkList(const ast::ListValue &n,
                                   cursiv::ValidationAcceptor &a) const {
  if (n.items.empty() && n.trailingComma)
    a.error(n, "An empty list cannot contain a comma.")
        .code("blueprint.empty-comma");
}
void BlueprintValidator::checkMap(const ast::MapValue &n,
                                  cursiv::ValidationAcceptor &a) const {
  if (n.properties.empty() && n.trailingComma)
    a.error(n, "An empty map cannot contain a comma.")
        .code("blueprint.empty-comma");
}
void BlueprintValidator::checkCondition(const ast::Condition &n,
                                        cursiv::ValidationAcceptor &a) const {
  checkName<&ast::Condition::function>(n, a);
  if (n.args.empty() && n.trailingComma)
    a.error(n, "An empty condition argument list cannot contain a comma.")
        .code("blueprint.empty-comma");
}
void BlueprintValidator::checkVariable(const ast::Variable &n,
                                       cursiv::ValidationAcceptor &a) const {
  checkName<&ast::Variable::name>(n, a);
}
void BlueprintValidator::checkBinding(const ast::AnyPattern &n,
                                      cursiv::ValidationAcceptor &a) const {
  checkName<&ast::AnyPattern::binding>(n, a);
}
void BlueprintValidator::checkAssignment(const ast::Assignment &n,
                                         cursiv::ValidationAcceptor &a) const {
  checkName<&ast::Assignment::name>(n, a);
  if (n.name == "default" || n.name == "unset")
    a.error(n, "'default' and 'unset' are reserved assignment names.")
        .property<&ast::Assignment::name>()
        .code("blueprint.reserved-assignment");
}
void BlueprintValidator::checkSelect(const ast::Select &n,
                                     cursiv::ValidationAcceptor &a) const {
  if (n.tuple && n.conditions.size() < 2)
    a.error(n,
            "A parenthesized condition tuple requires at least two conditions.")
        .code("blueprint.conditions-tuple");
  std::unordered_set<std::string_view> conditions;
  for (const auto *c : n.conditions)
    if (c->args.empty() && !conditions.insert(c->function).second)
      a.error(*c, "This select condition is duplicated.")
          .code("blueprint.duplicate-condition");
  bool nonUnset = false;
  std::unordered_set<std::string> patterns;
  for (std::size_t i = 0; i < n.cases.size(); ++i) {
    const auto *c = n.cases[i];
    nonUnset |= c->value != nullptr;
    if (c->tuple != n.tuple)
      a.error(*c,
              n.tuple
                  ? "Each branch needs a pattern tuple for the condition tuple."
                  : "A single condition uses a single pattern without "
                    "parentheses.")
          .code("blueprint.pattern-shape");
    if (c->patterns.size() != n.conditions.size())
      a.error(
           *c,
           "The number of patterns must match the number of select conditions.")
          .code("blueprint.pattern-arity");
    if (i + 1 < n.cases.size() && !c->comma)
      a.error(*c, "Select branches must be separated by commas.")
          .code("blueprint.case-comma");
    bool allDefault = !c->patterns.empty(), valid = true;
    std::string key;
    std::unordered_set<std::string_view> bindings;
    for (const auto &p : c->patterns) {
      allDefault &= std::holds_alternative<ast::DefaultPattern *>(p);
      auto f = fingerprint(p);
      if (f)
        part(key, *f);
      else
        valid = false;
      std::visit(
          [&](auto value) {
            using T = decltype(value);
            if constexpr (std::is_same_v<T, ast::AnyPattern *>) {
              if (value && !value->binding.empty() &&
                  !bindings.insert(value->binding).second)
                a.error(*value, "This branch binds the same name twice.")
                    .code("blueprint.duplicate-binding");
            } else if constexpr (std::is_same_v<T, ast::StringLit *>) {
              if (!value)
                return;
              if (value->raw.starts_with('`'))
                a.error(*value, "A select string pattern uses double quotes, "
                                "not backticks.")
                    .code("blueprint.pattern-raw");
              const auto decoded = string_value(*value);
              if (decoded.error.empty() && decoded.text.starts_with("__soong"))
                a.error(*value,
                        "Select patterns starting with '__soong' are reserved.")
                    .code("blueprint.pattern-reserved");
            } else if constexpr (std::is_same_v<T, ast::IntegerLit *>) {
              if (value && value->raw.starts_with('-'))
                a.error(
                     *value,
                     "A select integer pattern cannot start with a minus sign.")
                    .code("blueprint.pattern-negative");
            }
          },
          p);
    }
    if (valid && !patterns.insert(key).second)
      a.error(*c, "This select pattern tuple is duplicated.")
          .code("blueprint.duplicate-pattern");
    if (allDefault && i + 1 < n.cases.size())
      a.error(*c, "The all-default branch must be last.")
          .code("blueprint.default-order");
  }
  if (!nonUnset)
    a.error(n,
            "A select requires at least one branch whose value is not 'unset'.")
        .code("blueprint.select-empty");
}
void register_validation_checks(BlueprintCoreServices &services) {
  using V = BlueprintValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
#define CHECK(name)                                                            \
  cursiv::ValidationRegistry::makeValidationCheck<&V::name>(*services.validator)
      CHECK(checkInteger), CHECK(checkString), CHECK(checkProperty),
      CHECK(checkModule), CHECK(checkList), CHECK(checkMap),
      CHECK(checkCondition), CHECK(checkVariable), CHECK(checkBinding),
      CHECK(checkAssignment), CHECK(checkSelect)
#undef CHECK
  });
}
} // namespace blueprint
