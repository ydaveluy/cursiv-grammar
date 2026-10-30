#include "stringtemplate/core/validation/StringtemplateValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "stringtemplate/core/CoreServices.hpp"

namespace stringtemplate {

void StringtemplateValidator::checkArguments(
    const ast::ArgList &args, cursiv::ValidationAcceptor &accept) const {
  bool named = false, positional = false, forwarded = false;
  for (std::size_t i = 0; i < args.items.size(); ++i) {
    const auto &arg = *args.items[i];
    if (arg.ellipsis) {
      forwarded = true;
      if (i + 1 != args.items.size())
        accept.error(arg, "The argument forwarding marker must be last.",
                     "stringtemplate.argument-forwarding");
    } else if (arg.name.empty())
      positional = true;
    else
      named = true;
  }
  if (positional && (named || forwarded))
    accept.error(args, "Use either named or positional arguments.",
                 "stringtemplate.argument-kinds");
}

void StringtemplateValidator::checkMapping(
    const ast::MapExpr &map, cursiv::ValidationAcceptor &accept) const {
  if (map.sources.size() > 1 &&
      (map.stages.empty() || map.stages[0]->targets.size() != 1))
    accept.error(map, "A zipped mapping needs one initial template.",
                 "stringtemplate.zip-target");
  for (std::size_t i = 0; i < map.stages.size(); ++i) {
    const std::size_t expected = i == 0 ? map.sources.size() : 1;
    for (const auto* target : map.stages[i]->targets)
      if (const auto* anonymous = cursiv::ast_cast<ast::SubTemplateExpr>(target);
          anonymous && anonymous->params.size() != expected)
        accept.error(*anonymous,
                     "An anonymous mapping template needs one parameter per input.",
                     "stringtemplate.map-arity");
  }
}

void StringtemplateValidator::checkStage(
    const ast::MapStage &stage, cursiv::ValidationAcceptor &accept) const {
  for (const auto *target : stage.targets)
    if (!cursiv::ast_cast<ast::CallExpr>(target) &&
        !cursiv::ast_cast<ast::DynamicCallExpr>(target) &&
        !cursiv::ast_cast<ast::SubTemplateExpr>(target))
      accept.error(*target, "A mapping target must invoke a template.",
                   "stringtemplate.map-target");
}

void StringtemplateValidator::checkOption(
    const ast::Option &option, cursiv::ValidationAcceptor &accept) const {
  const auto name = option.name;
  if (name != "anchor" && name != "format" && name != "null" &&
      name != "separator" && name != "wrap")
    accept.error(option, "Unknown expression option.", "stringtemplate.option");
  else if (!option.value && name != "anchor" && name != "wrap")
    accept.error(option, "This option needs a value.",
                 "stringtemplate.option-value");
}

void register_validation_checks(StringtemplateCoreServices &services) {
  services.validator =
      std::make_unique<const StringtemplateValidator>(services);
  using V = StringtemplateValidator;
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkArguments>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMapping>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkStage>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOption>(
           validator)});
}

} // namespace stringtemplate
