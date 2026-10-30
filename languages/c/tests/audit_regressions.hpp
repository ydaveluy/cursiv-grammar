#pragma once
#include "c/core/ast.hpp"
#include "c/generated/C/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = c::generated::C::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = c::ast;
  auto r = parser->parse(
      "double x = 0x1.fp2; unsigned long n=1'000; enum E : unsigned int { A }; "
      "typedef int T; void f(void) { T * x; T(y); }");
  check(r.accepted && !r.cst().recovered(),
        "C99 hex float and permissive C23 syntax");
  auto *tu = as<ast::TranslationUnit>(r.value);
  auto *d =
      tu && !tu->items.empty() ? as<ast::Declaration>(tu->items[0]) : nullptr;
  auto *init = d && !d->declarators.empty()
                   ? as<ast::ExprInit>(d->declarators[0]->init)
                   : nullptr;
  auto *f = tu && tu->items.size() == 5 ? as<ast::FunctionDef>(tu->items[4])
                                        : nullptr;
  check(init && as<ast::FloatLit>(init->value) &&
            as<ast::FloatLit>(init->value)->text == "0x1.fp2",
        "hex float is one literal, never member access");
  auto *enumDecl =
      tu && tu->items.size() > 2 ? as<ast::Declaration>(tu->items[2]) : nullptr;
  auto *enumType = enumDecl && enumDecl->specifiers &&
                           enumDecl->specifiers->types.size() == 1
                       ? as<ast::EnumType>(enumDecl->specifiers->types[0])
                       : nullptr;
  check(enumType && enumType->hasUnderlyingColon && enumType->underlying &&
            enumType->enumerators.size() == 1,
        "C23 enum base retained");
  auto *shape = f && f->body && f->body->items.size() == 2
                    ? as<ast::AmbiguousStmt>(f->body->items[0])
                    : nullptr;
  check(shape && shape->leftName == "T" && shape->form == "*" &&
            shape->rightName == "x",
        "typedef-sensitive shape retained");
  auto *paren = f && f->body && f->body->items.size() == 2
                    ? as<ast::AmbiguousStmt>(f->body->items[1])
                    : nullptr;
  check(paren && paren->form == "(" && paren->rightName == "y",
        "parenthesized ambiguous shape retained");
  {
    const auto result = parser->parse(
        "enum E; extern enum E fn(void); enum E fn(void) { return A; }");
    const auto *unit = as<ast::TranslationUnit>(result.value);
    const auto *declaration = unit && unit->items.size() == 3
        ? as<ast::Declaration>(unit->items[1]) : nullptr;
    const auto *function = unit && unit->items.size() == 3
        ? as<ast::FunctionDef>(unit->items[2]) : nullptr;
    const auto *tag = declaration && declaration->specifiers &&
            declaration->specifiers->types.size() == 1
        ? as<ast::EnumType>(declaration->specifiers->types[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && tag &&
              tag->name == "E" && !tag->underlying &&
              declaration->declarators.size() == 1 &&
              declaration->declarators[0]->declarator->name == "fn" &&
              function && function->declarator->name == "fn",
          "enum reference does not consume the function declarator");
  }
  {
    for (const auto name : {"\u203F", "\u00B7", "\u20AC", "\u00B2",
                            "\u0378", "int\u20AC", "int\u00B7",
                            "\U0001F600", "int\U0001F600"}) {
      const std::string source = std::string("int ") + name +
          "; int legacy(void) { return " + name + "; }";
      const auto result = parser->parse(source);
      const auto *unit = as<ast::TranslationUnit>(result.value);
      const auto *declaration = unit && unit->items.size() == 2
          ? as<ast::Declaration>(unit->items[0]) : nullptr;
      const auto *function = unit && unit->items.size() == 2
          ? as<ast::FunctionDef>(unit->items[1]) : nullptr;
      const auto *statement = function && function->body &&
              function->body->items.size() == 1
          ? as<ast::ReturnStmt>(function->body->items[0]) : nullptr;
      const auto *identifier = statement ? as<ast::Ident>(statement->value) : nullptr;
      check(result.accepted && !result.cst().recovered() && declaration &&
                declaration->declarators.size() == 1 &&
                declaration->declarators[0]->declarator->name == name &&
                identifier && identifier->name == name,
            "legacy Unicode identifier spelling and keyword boundary");
    }
  }
  {
    const auto result = parser->parse("unsigned char c = u8'a';");
    const auto *unit = as<ast::TranslationUnit>(result.value);
    const auto *declaration = unit && unit->items.size() == 1
        ? as<ast::Declaration>(unit->items[0]) : nullptr;
    const auto *initializer = declaration && declaration->declarators.size() == 1
        ? as<ast::ExprInit>(declaration->declarators[0]->init) : nullptr;
    const auto *literal = initializer ? as<ast::CharLit>(initializer->value) : nullptr;
    check(result.accepted && !result.cst().recovered() && literal &&
              literal->text == "u8'a'",
          "UTF-8 character literal is one operand");
  }
  {
    const auto result = parser->parse("unsigned _BitInt(9) value = 123uwb;");
    const auto *unit = as<ast::TranslationUnit>(result.value);
    const auto *declaration = unit && unit->items.size() == 1
        ? as<ast::Declaration>(unit->items[0]) : nullptr;
    const auto *type = declaration && declaration->specifiers &&
            declaration->specifiers->types.size() == 1
        ? as<ast::BuiltinType>(declaration->specifiers->types[0]) : nullptr;
    const auto *width = type ? as<ast::IntLit>(type->bitWidth) : nullptr;
    const auto *initializer = declaration && declaration->declarators.size() == 1
        ? as<ast::ExprInit>(declaration->declarators[0]->init) : nullptr;
    const auto *literal = initializer ? as<ast::IntLit>(initializer->value) : nullptr;
    check(result.accepted && !result.cst().recovered() && type &&
              type->keywords.size() == 2 && type->keywords[0] == "unsigned" &&
              type->keywords[1] == "_BitInt" && width && width->text == "9" &&
              literal && literal->text == "123uwb",
          "bit-precise integer width and literal suffix retained");
  }
  {
    const auto result = parser->parse("unsigned const long int value;");
    const auto *unit = as<ast::TranslationUnit>(result.value);
    const auto *declaration = unit && unit->items.size() == 1
        ? as<ast::Declaration>(unit->items[0]) : nullptr;
    const auto *specifiers = declaration ? declaration->specifiers : nullptr;
    const auto *first = specifiers && specifiers->types.size() == 2
        ? as<ast::BuiltinType>(specifiers->types[0]) : nullptr;
    const auto *last = specifiers && specifiers->types.size() == 2
        ? as<ast::BuiltinType>(specifiers->types[1]) : nullptr;
    check(result.accepted && !result.cst().recovered() && first && last &&
              specifiers->qualifiers.size() == 1 &&
              specifiers->qualifiers[0] == "const" &&
              first->keywords.size() == 1 && first->keywords[0] == "unsigned" &&
              last->keywords.size() == 2 && last->keywords[0] == "long" &&
              last->keywords[1] == "int" && declaration->declarators.size() == 1 &&
              declaration->declarators[0]->declarator->name == "value",
          "qualifiers may separate builtin type specifiers");
  }
  {
    const auto result = parser->parse(
        "int a<:3:>; void proto(int a<:*:>); "
        "int choose(int x) <% return __builtin_choose_expr(1+1,x+2,(int)0); %>");
    const auto* unit = as<ast::TranslationUnit>(result.value);
    const auto* prototype = unit && unit->items.size() == 3
                                ? as<ast::Declaration>(unit->items[1]) : nullptr;
    const auto* function = unit && unit->items.size() == 3
                               ? as<ast::FunctionDef>(unit->items[2]) : nullptr;
    const auto* suffix = prototype && prototype->declarators.size() == 1 &&
                                 prototype->declarators[0]->declarator->suffixes.size() == 1
                             ? as<ast::FunctionSuffix>(prototype->declarators[0]->declarator->suffixes[0])
                             : nullptr;
    const auto* array = suffix && suffix->params.size() == 1 &&
                               suffix->params[0]->declarator &&
                               suffix->params[0]->declarator->suffixes.size() == 1
                           ? as<ast::ArraySuffix>(suffix->params[0]->declarator->suffixes[0])
                           : nullptr;
    const auto* statement = function && function->body && function->body->items.size() == 1
                               ? as<ast::ReturnStmt>(function->body->items[0]) : nullptr;
    const auto* builtin = statement ? as<ast::BuiltinExpr>(statement->value) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
              array && array->isStar && !array->size && builtin && builtin->args.size() == 3 &&
              as<ast::BinaryExpr>(builtin->args[0]) &&
              as<ast::BinaryExpr>(builtin->args[1]) && as<ast::CastExpr>(builtin->args[2]),
          "digraphs, starred array type and GNU expression operands retain their AST");
  }
  for (const auto source : {
           "void f(void) { switch(0) { case 0: default: } }",
           "typedef int T; void f(void) { label: T x; switch(0) { case 0: T y; default: } }"}) {
    const auto result = parser->parse(source);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "C23 case/default and typedef-adjacent labels may stand before declarations or block end");
  }
  return ok;
}
} // namespace audit_regressions
