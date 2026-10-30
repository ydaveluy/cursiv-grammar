#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "teal/core/ast.hpp"
#include "teal/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "teal/generated/Teal/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = teal::generated::Teal::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = teal::ast;
  auto r = parser->parse(
      "local a,b,c: number,string,boolean = 1,'x',true\nlocal function f(): "
      "number,string,boolean return 1,'x',true end");
  check(r.accepted && !r.cst().recovered(), "three annotated and return types");
  auto *b = as<ast::Block>(r.value);
  auto *d = b && !b->statements.empty()
                ? as<ast::LocalAttrAssignStat>(b->statements[0])
                : nullptr;
  check(d && d->types.size() == 3, "all annotated types retained");
  for (const char* newline : {"\n", "\r", "\r\n", "\n\r"}) {
    const auto long_result = parser->parse(
        std::string("local x = [==[") + newline + "a" + newline + "b]==]");
    const auto* root = long_result.value_as<ast::Block>();
    const auto* local = root && root->statements.size() == 1
        ? cursiv::ast_cast<ast::LocalAttrAssignStat>(root->statements[0]) : nullptr;
    const auto* value = local && local->values.size() == 1
        ? cursiv::ast_cast<ast::StringExpr>(local->values[0]) : nullptr;
    check(long_result.accepted && !long_result.cst().recovered() &&
              long_result.diagnostics.empty() && value && value->value == "a\nb",
          "Lua long strings drop the first newline and normalize all newline forms");
  }
  {
    const auto modern = parser->parse(
        "global a,b,c: number,string,boolean\n"
        "local record Point x:number y:number end\n"
        "local enum Color 'red' 'blue' end\n"
        "global record R field:number end\n"
        "local type Str = string\n"
        "local type F = function(number):number");
    const auto *block = modern.value_as<ast::Block>();
    check(modern.accepted && !modern.cst().recovered() && modern.diagnostics.empty() &&
          block && block->statements.size() == 6, "native named declarations");
    const auto *global = block && block->statements.size() == 6
        ? cursiv::ast_cast<ast::GlobalAttrStat>(block->statements[0]) : nullptr;
    const auto *local = block && block->statements.size() == 6
        ? cursiv::ast_cast<ast::LocalNewTypeStat>(block->statements[1]) : nullptr;
    const auto *record = local ? cursiv::ast_cast<ast::RecordNewType>(local->value) : nullptr;
    const auto *alias = block && block->statements.size() == 6
        ? cursiv::ast_cast<ast::LocalNewTypeStat>(block->statements[4]) : nullptr;
    check(global && global->types.size() == 3 && record && record->fields.size() == 2 &&
          !record->legacy && local->name == "Point" && record->parent == local &&
          alias && alias->name == "Str" && cursiv::ast_cast<ast::AliasNewType>(alias->value),
          "all global types and named/alias bodies retained");
  }
  for (const char *name : {"number", "string", "boolean", "as", "is", "enum",
                            "global", "record", "functiontype", "type"}) {
    const auto source = std::string("local ") + name + "=1";
    const auto r = parser->parse(source);
    const auto *block = r.value_as<ast::Block>();
    const auto *decl = block && block->statements.size() == 1
        ? cursiv::ast_cast<ast::LocalAttrAssignStat>(block->statements[0]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
          decl && decl->vars.size() == 1 && decl->vars[0]->name == name,
          "contextual type names remain ordinary variable names");
  }
  for (const char *source : {"--[=unfinished\nlocal x=1", "local x=\"a\\\r\nb\""}) {
    const auto r = parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "native partial comment opener and escaped CRLF");
  }
  for (const char *source : {R"(local x="a\q")", R"(local x="\256")",
                             R"(local x="\xG0")", R"(local x="\u{}")"}) {
    const auto r = parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.size() == 1 &&
          r.diagnostics[0].kind == cursiv::DiagnosticKind::conversion_error,
          "malformed native short-string escape diagnostic");
  }
  {
    const auto language = cursiv::create_language<teal::TealCoreServices,
        teal::TealSharedCoreServices, teal::install_core_module>();
    const auto result = cursiv::test::validationHelper(language.language,
        "local Point = record x:number end");
    check(result.diagnostics.size() == 1 && result.diagnostics[0].code &&
          result.diagnostics[0].code->isString() &&
          result.diagnostics[0].code->string() == "teal.legacy-type",
          "native deprecated type syntax retains AST and has an error");
  }
  return ok;
}
} // namespace audit_regressions
