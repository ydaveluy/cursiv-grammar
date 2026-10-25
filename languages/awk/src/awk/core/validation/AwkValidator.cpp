#include "awk/core/validation/AwkValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "awk/core/CoreServices.hpp"

namespace awk {

void AwkValidator::checkAssignment(const ast::AssignExpr& expression,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::VarRef>(expression.target) &&
      !cursiv::ast_cast<ast::IndexExpr>(expression.target) &&
      !cursiv::ast_cast<ast::FieldExpr>(expression.target))
    accept.error(expression, "Assignment needs a variable, array element or field.",
                 "awk.assignment-target").property<&ast::AssignExpr::target>();
}

void AwkValidator::checkBuiltin(const ast::CallExpr& call,
                               cursiv::ValidationAcceptor& accept) const {
  if (call.indirect) return;
  auto name = call.name;
  const bool qualified = name.starts_with("awk::");
  if (qualified) name.remove_prefix(5);
  else if (name.find("::") != std::string_view::npos) return;
  static constexpr std::string_view standard[] = {
    "atan2", "close", "cos", "exp", "fflush", "gsub", "index", "int", "length",
    "log", "match", "rand", "sin", "split", "sprintf", "sqrt", "srand", "sub",
    "substr", "system", "tolower", "toupper",
  };
  bool isStandard = false;
  for (const auto builtin : standard) isStandard |= name == builtin;
  if (qualified && isStandard) {
    accept.error(call, "This reserved function name cannot be qualified.", "awk.qualified-builtin")
        .property<&ast::CallExpr::name>();
    return;
  }
  if (!isStandard && !qualified) {
    const cursiv::AstNode* item = &call;
    while (item && item->parent && !cursiv::ast_cast<ast::Program>(item->parent)) {
      item = item->parent;
      if (const auto* function = cursiv::ast_cast<ast::FunctionDef>(item))
        for (const auto parameter : function->params) if (parameter == name) return;
    }
    if (item) if (const auto* program = cursiv::ast_cast<ast::Program>(item->parent)) {
      std::string_view current = "\"awk\"";
      for (const auto* preceding : program->items) {
        if (preceding == item) break;
        const auto* directive = cursiv::ast_cast<ast::Directive>(preceding);
        if (directive && directive->name == "namespace") current = directive->value;
      }
      if (current != "\"awk\"") return;
    }
  }
  struct Arity { std::string_view name; unsigned minimum, maximum; };
  static constexpr Arity arities[] = {
    {"atan2",2,2}, {"close",1,2}, {"cos",1,1}, {"exp",1,1}, {"gsub",2,3},
    {"index",2,2}, {"int",1,1}, {"length",0,1}, {"log",1,1}, {"match",2,3},
    {"rand",0,0}, {"sin",1,1}, {"split",2,4}, {"sqrt",1,1}, {"srand",0,1},
    {"sub",2,3}, {"substr",2,3}, {"system",1,1}, {"tolower",1,1}, {"toupper",1,1},
    {"asort",1,3}, {"asorti",1,3}, {"bindtextdomain",1,2}, {"compl",1,1},
    {"dcgettext",1,3}, {"dcngettext",3,5}, {"fflush",0,1}, {"gensub",3,4},
    {"isarray",1,1}, {"lshift",2,2}, {"mkbool",1,1}, {"mktime",1,2},
    {"patsplit",2,4}, {"rshift",2,2}, {"strftime",0,3}, {"strtonum",1,1},
    {"systime",0,0}, {"typeof",1,2},
  };
  bool valid = true;
  if (name == "and" || name == "or" || name == "xor") valid = call.args.size() >= 2;
  else if (name == "sprintf") valid = !call.args.empty();
  else for (const auto& arity : arities) if (arity.name == name) {
    valid = call.args.size() >= arity.minimum && call.args.size() <= arity.maximum;
    break;
  }
  if (!valid)
    accept.error(call, "Invalid number of arguments for " + std::string(name) + ".",
                 "awk.builtin-arity").property<&ast::CallExpr::args>();
}

void AwkValidator::checkRule(const ast::Rule& rule,
                            cursiv::ValidationAcceptor& accept) const {
  if (!rule.action && (cursiv::ast_cast<ast::BeginPattern>(rule.pattern) ||
      cursiv::ast_cast<ast::EndPattern>(rule.pattern) ||
      cursiv::ast_cast<ast::BeginFilePattern>(rule.pattern) ||
      cursiv::ast_cast<ast::EndFilePattern>(rule.pattern)))
    accept.error(rule, "This pattern needs an action block.", "awk.action")
        .property<&ast::Rule::pattern>();
}

void AwkValidator::checkDelete(const ast::DeleteStmt& statement,
                              cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::VarRef>(statement.target) &&
      !cursiv::ast_cast<ast::IndexExpr>(statement.target))
    accept.error(statement, "Delete needs an array or an array element.", "awk.delete")
        .property<&ast::DeleteStmt::target>();
}

void AwkValidator::checkSwitch(const ast::SwitchStmt& statement,
                              cursiv::ValidationAcceptor& accept) const {
  bool seen = false;
  for (const auto* clause : statement.cases) if (clause && clause->default_) {
    if (seen) accept.error(*clause, "Duplicate default clause.", "awk.default");
    seen = true;
  }
}

void AwkValidator::checkUniqueFunctionNames(
    const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string, std::vector<const ast::FunctionDef*>> names;
  std::string current = "awk";
  for (const auto* item : program.items) {
    if (const auto* directive = cursiv::ast_cast<ast::Directive>(item)) {
      if (directive->name == "namespace" && directive->value.size() >= 2)
        current = std::string(directive->value.substr(1, directive->value.size()-2));
    }
    const auto* def = cursiv::ast_cast<ast::FunctionDef>(item);
    if (!def) continue;
    std::string name(def->name);
    bool upper = !name.empty();
    for (char c : name) upper &= (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9');
    if (name.find("::") == std::string::npos && current != "awk" && !upper)
      name = current + "::" + name;
    if (name.starts_with("awk::")) name.erase(0,5);
    names[name].push_back(def);
  }

  for (const auto& [name, defs] : names) {
    if (defs.size() <= 1) continue;
    const std::string message = "Duplicate function name: " + std::string(name);
    for (const ast::FunctionDef* const def : defs)
      accept.error(*def, message).property<&ast::FunctionDef::name>();
  }
}

void register_validation_checks(AwkCoreServices& services) {
  using V = AwkValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueFunctionNames>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBuiltin>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRule>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDelete>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSwitch>(validator)});
}

}  // namespace awk
