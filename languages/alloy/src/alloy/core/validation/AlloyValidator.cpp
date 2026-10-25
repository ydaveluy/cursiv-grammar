#include "alloy/core/validation/AlloyValidator.hpp"

#include <array>
#include <memory>
#include <unordered_set>

#include "alloy/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace alloy {
namespace {
bool badName(std::string_view name) {
  return name.find_first_of("$/@") != std::string_view::npos;
}
} // namespace

void AlloyValidator::checkModule(const ast::Module &module,
                                 cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> names;
  auto declare = [&](const cursiv::AstNode &node, std::string_view name,
                     std::string_view field, std::size_t index = 0) {
    const auto span = accept.featureSpan(node, field, index)
                          .value_or(accept.spanOfName(name, node));
    if (badName(name))
      accept.error(span, "A declared name cannot contain $, / or @.",
                   "alloy.name");
    if (!names.insert(name).second)
      accept.error(span, "Duplicate signature or module parameter.",
                   "alloy.duplicate-signature");
  };
  if (module.moduleDecl)
    for (const auto *param : module.moduleDecl->names)
      if (param)
        declare(*param, param->name, "name");
  for (const auto *paragraph : module.paragraphs) {
    if (!paragraph)
      continue;
    if (const auto *sig = cursiv::ast_cast<ast::SigDecl>(paragraph))
      for (std::size_t i = 0; i < sig->names.size(); ++i)
        declare(*sig, sig->names[i], "names", i);
    if (const auto *enumeration = cursiv::ast_cast<ast::EnumDecl>(paragraph)) {
      declare(*enumeration, enumeration->name, "name");
      for (std::size_t i = 0; i < enumeration->members.size(); ++i)
        declare(*enumeration, enumeration->members[i], "members", i);
    }
    if (const auto *pred = cursiv::ast_cast<ast::PredDecl>(paragraph))
      if (badName(pred->name))
        accept
            .error(*pred, "A declared name cannot contain $, / or @.",
                   "alloy.name")
            .property<&ast::PredDecl::name>();
    if (const auto *fun = cursiv::ast_cast<ast::FunDecl>(paragraph))
      if (badName(fun->name))
        accept
            .error(*fun, "A declared name cannot contain $, / or @.",
                   "alloy.name")
            .property<&ast::FunDecl::name>();
  }
  if (!module.paragraphs.empty() && module.paragraphs[0]) {
    const auto first = accept.spanOf(*module.paragraphs[0]).begin;
    for (const auto *import : module.imports)
      if (import && accept.spanOf(*import).begin > first)
        accept.error(*import, "Imports must precede the module's declarations.",
                     "alloy.import-order");
  }
}

void AlloyValidator::checkSignature(const ast::SigDecl &sig,
                                    cursiv::ValidationAcceptor &accept) const {
  std::array<bool, 5> seen{};
  unsigned multiplicities = 0;
  for (std::size_t i = 0; i < sig.qualifiers.size(); ++i) {
    const auto qualifier = sig.qualifiers[i];
    const auto index = static_cast<std::size_t>(qualifier);
    if (seen[index])
      accept.error(sig, "Duplicate signature qualifier.", "alloy.qualifier")
          .property<&ast::SigDecl::qualifiers>(i);
    seen[index] = true;
    if (qualifier == ast::SigQualifier::lone ||
        qualifier == ast::SigQualifier::one ||
        qualifier == ast::SigQualifier::some)
      ++multiplicities;
  }
  if (multiplicities > 1)
    accept
        .error(sig, "A signature has at most one multiplicity.",
               "alloy.multiplicity")
        .property<&ast::SigDecl::qualifiers>();
  if (seen[static_cast<std::size_t>(ast::SigQualifier::abstract_)] &&
      cursiv::ast_cast<ast::SigIn>(sig.ext))
    accept.error(sig, "A subset signature cannot be abstract.", "alloy.subset")
        .property<&ast::SigDecl::qualifiers>();
}

void AlloyValidator::checkDeclaration(
    const ast::Decl &decl, cursiv::ValidationAcceptor &accept) const {
  if (decl.defined && (decl.namesDisjoint || decl.typeDisjoint))
    accept.error(decl, "A defined field cannot be disjoint.",
                 "alloy.defined-disjoint");
  if (cursiv::ast_cast<ast::PredDecl>(decl.parent) ||
      cursiv::ast_cast<ast::FunDecl>(decl.parent)) {
    if (decl.private_)
      accept
          .error(decl, "A function parameter is already private.",
                 "alloy.parameter-private")
          .keyword("private");
    if (decl.typeDisjoint)
      accept.error(decl, "A parameter cannot have a disjoint type.",
                   "alloy.parameter-disjoint");
  }
  for (auto name : decl.names)
    if (badName(name))
      accept.error(accept.spanOfName(name, decl),
                   "A declared name cannot contain $, / or @.", "alloy.name");
}

void AlloyValidator::checkEnumeration(
    const ast::EnumDecl &value, cursiv::ValidationAcceptor &accept) const {
  if (value.members.empty())
    accept.error(value, "An enumeration requires at least one member.",
                 "alloy.empty-enum");
}

void AlloyValidator::checkNumber(const ast::NumberExpr &value,
                                 cursiv::ValidationAcceptor &accept) const {
  if (value.value > 2147483647)
    accept
        .error(value,
               "The integer literal exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::NumberExpr::value>();
}

void AlloyValidator::checkString(const ast::StringExpr &value,
                                 cursiv::ValidationAcceptor &accept) const {
  const auto raw = value.raw;
  if (raw.size() == 2)
    accept
        .error(value, "An empty string literal is not supported by Alloy 4.",
               "alloy.empty-string")
        .property<&ast::StringExpr::raw>();
  for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
    if (raw[i] == '\r' || raw[i] == '\n')
      accept
          .error(value, "A string literal cannot contain a literal line break.",
                 "alloy.string-newline")
          .property<&ast::StringExpr::raw>();
    if (raw[i] == '\\' && i + 1 < raw.size() - 1) {
      const char escaped = raw[++i];
      if (escaped != 'n' && escaped != '\'' && escaped != '"' &&
          escaped != '\\')
        accept
            .error(value, "Unsupported Alloy 4 string escape.",
                   "alloy.string-escape")
            .property<&ast::StringExpr::raw>();
    }
  }
}

void AlloyValidator::checkOverallScope(
    const ast::Scope &scope, cursiv::ValidationAcceptor &accept) const {
  if (scope.overall && *scope.overall > 2147483647)
    accept
        .error(scope, "The scope exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::Scope::overall>();
}
void AlloyValidator::checkCommand(const ast::CmdDecl &command,
                                  cursiv::ValidationAcceptor &accept) const {
  if (!command.kind)
    accept.error(command, "A command requires run or check.",
                 "alloy.command-kind");
  if (command.expects && *command.expects > 2147483647)
    accept
        .error(command,
               "The expected result exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::CmdDecl::expects>();
}

void AlloyValidator::checkScope(const ast::TypeScope &scope,
                                cursiv::ValidationAcceptor &accept) const {
  if (scope.count > 2147483647)
    accept
        .error(scope, "The scope exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::TypeScope::count>();
  if (scope.end && *scope.end > 2147483647)
    accept
        .error(scope, "The scope exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::TypeScope::end>();
  if (scope.step && *scope.step > 2147483647)
    accept
        .error(scope,
               "The scope step exceeds the Alloy 4 parser's 32-bit range.",
               "alloy.integer-range")
        .property<&ast::TypeScope::step>();
  const auto end = scope.end.value_or(scope.step ? 2147483647 : scope.count);
  if (end < scope.count)
    accept
        .error(scope, "A scope's upper bound cannot precede its lower bound.",
               "alloy.scope-range")
        .property<&ast::TypeScope::end>();
  if (scope.step && *scope.step == 0 && end != scope.count)
    accept
        .error(scope, "A growing scope's step must be positive.",
               "alloy.scope-step")
        .property<&ast::TypeScope::step>();
  if (scope.type == "none" || scope.type == "univ")
    accept
        .error(scope, "A scope cannot be specified for none or univ.",
               "alloy.scope-type")
        .property<&ast::TypeScope::type>();
  if ((scope.type == "Int" || scope.type == "int" || scope.type == "seq") &&
      (scope.exactly_ || end != scope.count))
    accept.error(scope,
                 "An integer or sequence scope cannot grow or use exactly.",
                 "alloy.scope-kind");
}

void register_validation_checks(AlloyCoreServices &services) {
  using V = AlloyValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkModule>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSignature>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDeclaration>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnumeration>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOverallScope>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCommand>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkScope>(
           validator)});
}
} // namespace alloy
