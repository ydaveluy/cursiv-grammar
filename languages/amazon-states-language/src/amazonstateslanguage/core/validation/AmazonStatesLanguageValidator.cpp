#include "amazonstateslanguage/core/validation/AmazonStatesLanguageValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <type_traits>

#include "amazonstateslanguage/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace amazonstateslanguage {

template <class T>
void AmazonStatesLanguageValidator::checkMembers(
    const T &object, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> seen;
  for (std::size_t i = 0; i < object.members.size(); ++i)
    if (!seen.insert(object.members[i]).second)
      accept
          .error(object,
                 "Duplicate object member: " + std::string(object.members[i]))
          .template property<&T::members>(i);
  if constexpr (requires { object.extensions; })
    for (const auto* binding : object.extensions)
      if (!seen.insert(binding->key).second)
        accept.error(*binding, "Duplicate object member: " +
                                  std::string(binding->key))
            .template property<&ast::Binding::key>();
  if constexpr (std::is_same_v<T, ast::ChoiceRule>) {
    if (const auto* rule = cursiv::ast_cast<ast::ComparisonRule>(&object)) {
      if (rule->variable.size() != 1 || rule->comparison_op.size() != 1)
        accept.error(object, "A comparison rule requires one Variable and one comparison operator.");
    } else if (const auto* rule = cursiv::ast_cast<ast::CompositeRule>(&object)) {
      if (rule->logical_op.size() != 1)
        accept.error(object, "A composite rule requires one logical operator.");
    } else if (const auto* rule =
                   cursiv::ast_cast<ast::JsonataChoiceRule>(&object)) {
      if (rule->conditions.size() != 1)
        accept.error(object, "A JSONata choice requires one Condition.");
      for (const auto& condition : rule->conditions)
        if (!std::holds_alternative<bool>(condition) &&
            !std::holds_alternative<std::string_view>(condition))
          accept.error(object, "Condition must be a boolean or a JSONata string.");
    }
  }

}

void AmazonStatesLanguageValidator::checkProgram(
    const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  if (program.start_at.size() != 1)
    accept.error(program, "A state machine requires one StartAt.",
                 "asl.start-at");
  if (program.states.empty())
    accept.error(program, "A state machine requires nonempty States.",
                 "asl.states");
  std::unordered_set<std::string_view> names;
  for (const auto* state : program.states) {
    if (!names.insert(state->name).second)
      accept.error(*state, "Duplicate state name.", "asl.state-name")
          .property<&ast::State::name>();
  }
  if (program.start_at.size() == 1 &&
      !names.contains(program.start_at[0]))
    accept.error(program, "StartAt must name a state in this machine.",
                 "asl.start-at").property<&ast::Program::start_at>();
  for (auto language : program.query_languages)
    if (language != "JSONPath" && language != "JSONata")
      accept.error(program, "QueryLanguage must be JSONPath or JSONata.",
                   "asl.query-language");
}

void AmazonStatesLanguageValidator::checkState(
    const ast::State& state, cursiv::ValidationAcceptor& accept) const {
  if (state.types.size() != 1)
    accept.error(state, "A state requires one Type.", "asl.state-type");
  if (state.query_languages.size() > 1)
    accept.error(state, "Duplicate QueryLanguage.", "asl.query-language");
  for (auto language : state.query_languages)
    if (language != "JSONPath" && language != "JSONata")
      accept.error(state, "QueryLanguage must be JSONPath or JSONata.",
                   "asl.query-language");
}

void register_validation_checks(AmazonStatesLanguageCoreServices &services) {
  using V = AmazonStatesLanguageValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkProgram>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkState>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ReaderConfig>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ItemReader>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ProcessorConfig>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::Catcher>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::ChoiceRule>>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkMembers<ast::Program>>(validator)});
}

} // namespace amazonstateslanguage
