// The scotty language: its grammar. Scotty programs are fully prefixed --
// every operator names both its operands, so no precedence table is needed.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "scotty/core/ast.hpp"

namespace scotty {

struct Scotty : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct ID : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9">)> {};

  // A dot may appear anywhere in the run, never the trailing character.
  struct DIGITS : terminal<many(cr<"0-9.">) + cr<"0-9">> {};

  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};

  struct PrefixExp;

  struct ParamRule : rule<assign<&ast::Param::name>(ref<ID>)> {};

  struct FnDefRule
      : rule<kw<"fun"> + assign<&ast::FnDef::name>(ref<ID>) +
             assign<&ast::FnDef::param>(ref<ParamRule>) + lit<"="> +
             assign<&ast::FnDef::body>(ref<PrefixExp>)> {};

  struct VarAssignRule
      : rule<assign<&ast::VarAssign::name>(ref<ID>) + lit<"="> +
             assign<&ast::VarAssign::value>(ref<PrefixExp>)> {};

  struct PrefixExp
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::op>(ref<ADD> | ref<MINUS> |
                                               ref<MULTIPLY> | ref<DIVIDE>) +
                 assign<&ast::BinaryExpr::left>(ref<PrefixExp>) +
                 assign<&ast::BinaryExpr::right>(ref<PrefixExp>))  //
             | create<ast::Call>(
                   lit<"("> + assign<&ast::Call::func>(ref<ID>) +
                   assign<&ast::Call::arg>(ref<PrefixExp>) + lit<")">)  //
             | create<ast::NameExpr>(assign<&ast::NameExpr::target>(ref<ID>))  //
             | create<ast::NumberLiteral>(
                   option(flag<&ast::NumberLiteral::negative>(lit<"-">)) +
                   assign<&ast::NumberLiteral::value>(ref<DIGITS>))> {};

  // The trailing expression is the program's result, so it is its own
  // member rather than the statement list's last entry.
  struct ModelRule
      : rule<create<ast::Model> +
             many(append<&ast::Model::statements>(ref<FnDefRule>) |
                  append<&ast::Model::statements>(ref<VarAssignRule>)) +
             assign<&ast::Model::result>(ref<PrefixExp>)> {};

  using entry = ModelRule;
  using ignore = WS;
};

}  // namespace scotty
