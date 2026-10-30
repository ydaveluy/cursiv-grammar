#include "lua/core/validation/LuaValidator.hpp"

#include <memory>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lua/core/CoreServices.hpp"

namespace lua {

void LuaValidator::checkAssignTargets(const ast::AssignStat& stat,
                                      cursiv::ValidationAcceptor& accept) const {
  for (const ast::Expr* const target : stat.targets) {
    if (target != nullptr &&
        !target->type->derivesFrom(&cursiv::ast_type_tag<ast::Var>))
      accept.error(*target, "cannot assign to this expression");
  }
}

void LuaValidator::checkCallStat(const ast::CallStat& stat,
                                 cursiv::ValidationAcceptor& accept) const {
  if (stat.call != nullptr &&
      !stat.call->type->derivesFrom(&cursiv::ast_type_tag<ast::Call>))
    accept.error(*stat.call, "syntax error: function call expected");
}

void LuaValidator::checkString(const ast::StringExpr& node,
                               cursiv::ValidationAcceptor& accept) const {
  if (node.longForm) return;
  const auto text = node.value;
  const auto hex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
  };
  const auto bad = [&] { accept.error(node, "Invalid Lua string escape.", "lua.string-escape")
      .property<&ast::StringExpr::value>(); };
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\') continue;
    if (++i >= text.size()) { bad(); return; }
    const char c = text[i];
    if (std::string_view("abfnrtv\\\"'\r\n").find(c) != std::string_view::npos) continue;
    if (c == 'z') {
      while (i + 1 < text.size() && std::string_view(" \t\r\n\f\v").find(text[i + 1]) != std::string_view::npos) ++i;
    } else if (c >= '0' && c <= '9') {
      unsigned value = c - '0';
      for (unsigned n = 1; n < 3 && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9'; ++n)
        value = value * 10 + (text[++i] - '0');
      if (value > 255) { bad(); return; }
    } else if (c == 'x') {
      if (i + 2 >= text.size() || hex(text[i + 1]) < 0 || hex(text[i + 2]) < 0) { bad(); return; }
      i += 2;
    } else if (c == 'u') {
      if (i + 1 >= text.size() || text[++i] != '{') { bad(); return; }
      unsigned value = 0, count = 0;
      while (i + 1 < text.size() && hex(text[i + 1]) >= 0) {
        if (value > (0x7fffffffU >> 4)) { bad(); return; }
        value = value * 16 + static_cast<unsigned>(hex(text[++i]));
        ++count;
      }
      if (!count || i + 1 >= text.size() || text[++i] != '}') { bad(); return; }
    } else { bad(); return; }
  }
}
void LuaValidator::checkLocal(const ast::LocalStat& node,
                              cursiv::ValidationAcceptor& accept) const {
  bool close = false;
  for (const auto* var : node.vars) {
    if (!var->attrib.empty() && var->attrib != "const" && var->attrib != "close")
      accept.error(*var, "Unknown local variable attribute.", "lua.attribute")
          .property<&ast::LocalVar::attrib>();
    if (var->attrib == "close") {
      if (close) accept.error(*var, "Only one to-be-closed variable is allowed per local declaration.", "lua.close-count");
      close = true;
    }
  }
}
void LuaValidator::checkVararg(const ast::VarargExpr& node,
                               cursiv::ValidationAcceptor& accept) const {
  for (auto* parent = node.parent; parent; parent = parent->parent)
    if (const auto* body = cursiv::ast_cast<ast::FuncBody>(parent)) {
      if (!body->vararg) accept.error(node, "Varargs require a vararg function.", "lua.vararg");
      return;
    }
  // A loaded Lua chunk is itself a vararg function.
}
void LuaValidator::checkBreak(const ast::BreakStat& node,
                              cursiv::ValidationAcceptor& accept) const {
  for (auto* parent = node.parent; parent; parent = parent->parent) {
    if (cursiv::ast_cast<ast::WhileStat>(parent) || cursiv::ast_cast<ast::RepeatStat>(parent) ||
        cursiv::ast_cast<ast::NumericForStat>(parent) || cursiv::ast_cast<ast::GenericForStat>(parent)) return;
    if (cursiv::ast_cast<ast::FuncBody>(parent)) break;
  }
  accept.error(node, "Break requires an enclosing loop in the same function.", "lua.break");
}

void register_validation_checks(LuaCoreServices& services) {
  using V = LuaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignTargets>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCallStat>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLocal>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVararg>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBreak>(validator)});
}

}  // namespace lua
