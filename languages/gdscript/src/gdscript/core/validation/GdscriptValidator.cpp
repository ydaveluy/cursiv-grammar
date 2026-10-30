#include "gdscript/core/validation/GdscriptValidator.hpp"

#include <functional>
#include <memory>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "gdscript/core/CoreServices.hpp"

namespace gdscript {
namespace {
bool valid_string(std::string_view text) {
  const auto quote = text.starts_with("\"\"\"") ? 3U : 1U;
  if (text.size() < 2 * quote) return false;
  text = text.substr(quote, text.size() - 2 * quote);
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\0') return false;
    if (text[i] != '\\') continue;
    if (++i == text.size()) return false;
    if (text[i] == 'u') {
      for (unsigned j = 0; j < 4; ++j) {
        if (++i == text.size()) return false;
        const char c = text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
              (c >= 'a' && c <= 'f')))
          return false;
      }
    } else if (std::string_view("abfnrtv'\"\\/").find(text[i]) ==
               std::string_view::npos)
      return false;
  }
  return true;
}
bool in_loop(const cursiv::AstNode& node, bool is_continue) {
  for (const auto* p = node.parent; p; p = p->parent) {
    if (cursiv::ast_cast<ast::WhileStmt>(p) ||
        cursiv::ast_cast<ast::ForStmt>(p))
      return true;
    if (is_continue && cursiv::ast_cast<ast::MatchCase>(p)) return true;
    if (cursiv::ast_cast<ast::MethodDecl>(p)) break;
  }
  return false;
}
}  // namespace

void GdscriptValidator::checkProgram(const ast::Program& node,
                                     cursiv::ValidationAcceptor& accept) const {
  std::function<void(const cursiv::vector<ast::TopLevelDecl*>&, bool, bool)>
      check = [&](const auto& body, bool inner, bool has_header_inheritance) {
        bool has_inheritance = has_header_inheritance, has_classname = false,
             has_tool = false, blocks_inheritance = false;
        std::unordered_set<std::string_view> symbols, methods, signals;
        for (const auto* n : body) {
          if (const auto* inherit = cursiv::ast_cast<ast::Inheritance>(n)) {
            if (has_inheritance)
              accept.error(*inherit, "extends can only appear once per class")
                  .code("gdscript.duplicate-extends");
            if (blocks_inheritance)
              accept.error(*inherit, "extends must precede member declarations")
                  .code("gdscript.extends-order");
            has_inheritance = true;
          } else if (const auto* name =
                         cursiv::ast_cast<ast::ClassNameDecl>(n)) {
            if (inner)
              accept.error(*name, "class_name is only valid in the main class")
                  .code("gdscript.class-name-scope");
            if (has_classname)
              accept.error(*name, "class_name can only appear once")
                  .code("gdscript.duplicate-class-name");
            has_classname = true;
          } else if (cursiv::ast_cast<ast::ToolDecl>(n)) {
            if (has_tool)
              accept.error(*n, "tool can only appear once per class")
                  .code("gdscript.duplicate-tool");
            has_tool = true;
          } else if (const auto* method =
                         cursiv::ast_cast<ast::MethodDecl>(n)) {
            if (!methods.insert(method->name).second)
              accept.error(*method, "duplicate method name")
                  .template property<&ast::MethodDecl::name>()
                  .code("gdscript.duplicate-method");
            blocks_inheritance |= !method->isStatic;
          } else if (const auto* signal =
                         cursiv::ast_cast<ast::SignalDecl>(n)) {
            if (!signals.insert(signal->name).second)
              accept.error(*signal, "duplicate signal name")
                  .template property<&ast::SignalDecl::name>()
                  .code("gdscript.duplicate-signal");
            std::unordered_set<std::string_view> arguments;
            for (const auto p : signal->params)
              if (!arguments.insert(p).second)
                accept.error(*signal, "duplicate signal parameter")
                    .code("gdscript.duplicate-parameter");
          } else {
            auto add_symbol = [&](std::string_view name) {
              if (!symbols.insert(name).second)
                accept.error(*n, "duplicate class member name")
                    .code("gdscript.duplicate-member");
            };
            if (const auto* variable = cursiv::ast_cast<ast::ClassVarDecl>(n)) {
              add_symbol(variable->name);
              blocks_inheritance = true;
            }
            if (const auto* constant = cursiv::ast_cast<ast::ConstDecl>(n)) {
              add_symbol(constant->name);
              blocks_inheritance = true;
            }
            if (const auto* enumeration = cursiv::ast_cast<ast::EnumDecl>(n)) {
              if (!enumeration->name.empty()) add_symbol(enumeration->name);
              std::unordered_set<std::string_view> items;
              for (const auto* item : enumeration->items) {
                if (!items.insert(item->name).second)
                  accept.error(*item, "duplicate enum item")
                      .code("gdscript.duplicate-enum-item");
                if (enumeration->name.empty()) add_symbol(item->name);
              }
              blocks_inheritance |=
                  !enumeration->name.empty() || !enumeration->items.empty();
            }
            if (const auto* nested = cursiv::ast_cast<ast::InnerClassDecl>(n)) {
              add_symbol(nested->name);
              blocks_inheritance = true;
              check(nested->body, true, nested->headerInherits != nullptr);
            }
          }
        }
      };
  check(node.body, false, false);
}

void GdscriptValidator::checkMethod(const ast::MethodDecl& node,
                                    cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> names;
  bool defaulted = false;
  for (const auto* param : node.params) {
    if (!names.insert(param->name).second)
      accept.error(*param, "duplicate parameter name")
          .template property<&ast::Param::name>()
          .code("gdscript.duplicate-parameter");
    if (defaulted && !param->defaultValue)
      accept
          .error(*param,
                 "a required parameter cannot follow a default parameter")
          .code("gdscript.default-parameter-order");
    defaulted |= param->defaultValue != nullptr;
    if (param->isInferred && !param->defaultValue)
      accept.error(*param, "inferred parameter types require a default value")
          .code("gdscript.inferred-parameter");
  }
  if (node.hasSuperCall && node.name != "_init")
    accept.error(node, "only _init can call the parent constructor")
        .code("gdscript.parent-constructor");
  if (node.name == "_init" && node.isStatic)
    accept.error(node, "a constructor cannot be static")
        .code("gdscript.static-constructor");
  if (node.isStatic && !node.rpcMode.empty())
    accept.error(node, "RPC methods cannot be static")
        .code("gdscript.static-rpc");
}
void GdscriptValidator::checkString(const ast::StringLit& node,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!valid_string(node.text))
    accept.error(node, "invalid GDScript string escape")
        .template property<&ast::StringLit::text>()
        .code("gdscript.string-escape");
}
void GdscriptValidator::checkPath(const ast::NodePathLit& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!valid_string(node.text))
    accept.error(node, "invalid NodePath string escape")
        .template property<&ast::NodePathLit::text>()
        .code("gdscript.string-escape");
}
void GdscriptValidator::checkNumber(const ast::FloatLit& node,
                                    cursiv::ValidationAcceptor& accept) const {
  const auto final = node.text.find_last_not_of('_');
  if (final == node.text.npos || node.text[final] < '0' || node.text[final] > '9' ||
      node.text.find('E') != node.text.npos)
    accept
        .error(node,
               "GDScript 3 floating point literals need a final digit and "
               "lowercase e")
        .template property<&ast::FloatLit::text>()
        .code("gdscript.number");
}
void GdscriptValidator::checkInteger(const ast::IntLit& node,
                                     cursiv::ValidationAcceptor& accept) const {
  if (node.text.starts_with("0X") || node.text.starts_with("0B") ||
      ((node.text.starts_with("0x") || node.text.starts_with("0b")) &&
       node.text.find_first_not_of('_', 2) == node.text.npos))
    accept.error(node, "GDScript 3 base literals need lowercase x/b and a digit")
        .template property<&ast::IntLit::text>()
        .code("gdscript.number");
}
void GdscriptValidator::checkCast(const ast::CastExpr& node,
                                 cursiv::ValidationAcceptor& accept) const {
  if (cursiv::ast_cast<ast::CastExpr>(node.value))
    accept.error(node, "A cast result cannot be cast again directly")
        .code("gdscript.chained-cast");
  if (const auto* assignment = cursiv::ast_cast<ast::AssignStmt>(node.parent);
      assignment && assignment->target == &node)
    accept.error(node, "A cast expression is not an assignment target")
        .code("gdscript.cast-assignment");
}
void GdscriptValidator::checkCall(const ast::CallExpr& node,
                                  cursiv::ValidationAcceptor& accept) const {
  const auto* name = cursiv::ast_cast<ast::Name>(node.func);
  if (!name) return;
  if (name->id == "yield" && !node.args.empty() && node.args.size() != 2)
    accept.error(node, "yield expects zero or two arguments")
        .code("gdscript.yield-arity");
  if (name->id == "preload" && node.args.size() != 1)
    accept.error(node, "preload expects one constant path argument")
        .code("gdscript.preload-arity");
}
void GdscriptValidator::checkBreak(const ast::BreakStmt& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (!in_loop(node, false))
    accept.error(node, "break is only valid in a loop")
        .code("gdscript.break-scope");
}
void GdscriptValidator::checkContinue(
    const ast::ContinueStmt& node, cursiv::ValidationAcceptor& accept) const {
  if (!in_loop(node, true))
    accept.error(node, "continue is only valid in a loop or match branch")
        .code("gdscript.continue-scope");
}
void register_validation_checks(GdscriptCoreServices& services) {
  using V = GdscriptValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkProgram>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMethod>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPath>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInteger>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCast>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBreak>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkContinue>(v)});
}
}  // namespace gdscript
