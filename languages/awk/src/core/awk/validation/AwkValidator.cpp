#include "awk/validation/AwkValidator.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursiv/syntax-tree/AstWalk.hpp"
#include "cursiv/validation/ValidationRegistry.hpp"
#include "awk/CoreServices.hpp"

namespace awk {

void AwkValidator::checkAssignment(const ast::AssignExpr& expression,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::VarRef>(expression.target) &&
      !cursiv::ast_cast<ast::IndexExpr>(expression.target) &&
      !cursiv::ast_cast<ast::FieldExpr>(expression.target))
    accept.error(expression, "Assignment needs a variable, array element or field.",
                 "awk.assignment-target").property<&ast::AssignExpr::target>();
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

namespace {

using Name = std::string_view;

struct Builtin {
  unsigned min, max;
  bool extension = false;
};

const auto& builtins() {
  static const std::unordered_map<Name, Builtin> functions = {
    {"atan2",{2,2}}, {"close",{1,2}}, {"cos",{1,1}}, {"exp",{1,1}},
    {"fflush",{0,1}}, {"gsub",{2,3}}, {"index",{2,2}}, {"int",{1,1}},
    {"length",{0,1}}, {"log",{1,1}}, {"match",{2,3}}, {"rand",{0,0}},
    {"sin",{1,1}}, {"split",{2,4}}, {"sprintf",{1,~0u}}, {"sqrt",{1,1}},
    {"srand",{0,1}}, {"sub",{2,3}}, {"substr",{2,3}}, {"system",{1,1}},
    {"tolower",{1,1}}, {"toupper",{1,1}},
    {"and",{2,~0u,true}}, {"asort",{1,3,true}}, {"asorti",{1,3,true}},
    {"bindtextdomain",{1,2,true}}, {"compl",{1,1,true}}, {"dcgettext",{1,3,true}},
    {"dcngettext",{3,5,true}}, {"gensub",{3,4,true}}, {"isarray",{1,1,true}},
    {"lshift",{2,2,true}}, {"mkbool",{1,1,true}}, {"mktime",{1,2,true}},
    {"or",{2,~0u,true}}, {"patsplit",{2,4,true}}, {"rshift",{2,2,true}},
    {"strftime",{0,3,true}}, {"strtonum",{1,1,true}}, {"systime",{0,0,true}},
    {"typeof",{1,2,true}}, {"xor",{2,~0u,true}},
  };
  return functions;
}

}  // namespace

void AwkValidator::checkNames(const ast::Program& program,
                              cursiv::ValidationAcceptor& accept) const {
  struct Symbol {
    bool variable{};
    std::vector<const ast::FunctionDef*> definitions;
  };
  std::unordered_map<Name, std::unordered_map<Name, Symbol>> symbols;
  std::unordered_set<Name> parameters;
  Name current = "awk";
  const auto& functions = builtins();
  std::vector<std::pair<const cursiv::AstNode*, bool>> pending;
  for (const auto* item : program.items) {
    if (const auto* directive = cursiv::ast_cast<ast::Directive>(item)) {
      if (directive->name == "namespace" && directive->value.size() >= 2)
        current = directive->value.substr(1, directive->value.size() - 2);
      continue;
    }
    if (!item) continue;
    const auto* def = cursiv::ast_cast<ast::FunctionDef>(item);
    parameters.clear();
    if (def) {
      for (const auto parameter : def->params) {
        if (parameter.find("::") != Name::npos)
          accept.error(*def, "A parameter name cannot be qualified.", "awk.parameter").property<&ast::FunctionDef::params>();
        parameters.insert(parameter);
      }
    }
    const auto use = [&](Name spelling, const cursiv::AstNode& node) {
      const auto* call = cursiv::ast_cast<ast::CallExpr>(&node);
      const auto* definition = cursiv::ast_cast<ast::FunctionDef>(&node);
      const auto pos = spelling.find("::");
      const bool qualified = pos != Name::npos;
      const auto name = qualified ? spelling.substr(pos + 2) : spelling;
      const auto space = qualified ? spelling.substr(0, pos)
          : (name.find_first_of("abcdefghijklmnopqrstuvwxyz") == Name::npos ? Name{"awk"} : current);
      const bool keyword = name == "switch" || name == "case" || name == "default" ||
                           name == "BEGINFILE" || name == "ENDFILE";
      if (!definition && !(qualified && keyword) &&
          parameters.contains(spelling.starts_with("awk::") ? name : spelling)) {
        if (call && !call->indirect && (!call->spaced || call->args.size() != 1))
          accept.error(*call, "A function parameter needs an indirect call.", "awk.function-variable")
              .property<&ast::CallExpr::name>();
        return;
      }
      if (keyword && (!qualified || space == "awk")) {
        accept.error(node, "This keyword cannot be used as a name here.", "awk.reserved-name");
        return;
      }
      const auto fn = functions.find(name);
      if (fn != functions.end() && (space == "awk" || !fn->second.extension)) {
        if (!call || call->indirect)
          accept.error(node, "A built-in function cannot be used as a variable or definition.", "awk.reserved-name");
        else if (qualified && !fn->second.extension)
          accept.error(*call, "This reserved function name cannot be qualified.", "awk.qualified-builtin").property<&ast::CallExpr::name>();
        else if (call->args.size() < fn->second.min || call->args.size() > fn->second.max)
          accept.error(*call, "Invalid number of arguments for " + std::string(name) + ".",
                       "awk.builtin-arity").property<&ast::CallExpr::args>();
        return;
      }
      auto& symbol = symbols[space][name];
      if (definition) {
        symbol.definitions.push_back(definition);
        if (symbol.variable)
          accept.error(*definition, "A variable already has this function name.", "awk.function-variable").property<&ast::FunctionDef::name>();
      } else if (call && !call->indirect) {
        if (symbol.variable)
          accept.error(*call, "A variable cannot be called directly.", "awk.function-variable").property<&ast::CallExpr::name>();
      } else {
        if (!symbol.definitions.empty())
          accept.error(node, "A function name cannot be used as a variable.", "awk.function-variable");
        symbol.variable = true;
      }
    };
    if (def) use(def->name, *def);
    // GNU awk resolves a call after its arguments have introduced their variables.
    pending.emplace_back(item, false);
    while (!pending.empty()) {
      const auto [node, afterArguments] = pending.back();
      pending.pop_back();
      if (const auto* call = cursiv::ast_cast<ast::CallExpr>(node)) {
        if (afterArguments) {
          use(call->name, *node);
          continue;
        }
        pending.emplace_back(node, true);
      } else if (const auto* var = cursiv::ast_cast<ast::VarRef>(node)) {
        use(var->name, *node);
      } else if (const auto* loop = cursiv::ast_cast<ast::ForInStmt>(node)) {
        use(loop->var, *node);
      }
      const auto first = pending.size();
      cursiv::visitContents(*services.astReflection, *node, [&](const cursiv::AstNode& child) {
        pending.emplace_back(&child, false);
      });
      std::reverse(pending.begin() + static_cast<std::ptrdiff_t>(first), pending.end());
    }
  }
  for (const auto& [space, names] : symbols) {
    for (const auto& [name, symbol] : names) {
      if (symbol.definitions.size() > 1) {
        std::string full;
        if (space != "awk") full = std::string(space) + "::";
        full += name;
        for (const auto* def : symbol.definitions)
          accept.error(*def, "Duplicate function name: " + full).property<&ast::FunctionDef::name>();
      }
    }
  }
}

void AwkValidator::checkRegex(const ast::RegexLit& regex,
                              cursiv::ValidationAcceptor& accept) const {
  if (!regex.typed) return;
  const auto* parent = regex.parent;
  if (const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(parent);
      binary && binary->right == &regex &&
      (binary->op == ast::BinOp::Match || binary->op == ast::BinOp::NoMatch))
    return;
  if (const auto* assignment = cursiv::ast_cast<ast::AssignExpr>(parent);
      assignment && assignment->value == &regex) return;
  if (cursiv::ast_cast<ast::CallExpr>(parent) ||
      cursiv::ast_cast<ast::ReturnStmt>(parent) ||
      cursiv::ast_cast<ast::SwitchCase>(parent)) return;
  accept.error(regex, "A typed regexp constant requires a matching, assignment, call, return or case context.",
               "awk.typed-regexp-context");
}

void register_validation_checks(AwkCoreServices& services) {
  using V = AwkValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkRegex>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkNames>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRule>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDelete>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSwitch>(validator)});
}

}  // namespace awk
