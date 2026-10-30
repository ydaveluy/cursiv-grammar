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


  // The course tokenizer distinguishes a standalone prefix operator from
  // a minus sign attached to a numeric word.
  static constexpr auto operandGap = &cr<R"( \t\r\n)">;

  struct PrefixExp;

  struct Param : rule<assign<&ast::Param::name>(ref<ID>)> {};

  struct FnDef
      : rule<kw<"fun"> + assign<&ast::FnDef::name>(ref<ID>) +
             assign<&ast::FnDef::param>(ref<Param>) + lit<"="> +
             assign<&ast::FnDef::body>(ref<PrefixExp>)> {};

  struct VarAssign
      : rule<assign<&ast::VarAssign::name>(ref<ID>) + lit<"="> +
             assign<&ast::VarAssign::value>(ref<PrefixExp>)> {};

  struct PrefixExp
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::add>(lit<"+", operandGap>) | as<ast::Operator::subtract>(lit<"-", operandGap>) |
                                               as<ast::Operator::multiply>(lit<"*", operandGap>) | as<ast::Operator::divide>(lit<"/", operandGap>)) +
                 assign<&ast::BinaryExpr::left>(ref<PrefixExp>) +
                 assign<&ast::BinaryExpr::right>(ref<PrefixExp>))  //
             | create<ast::Call>(
                   lit<"("> + assign<&ast::Call::func>(ref<ID>) +
                   assign<&ast::Call::arg>(ref<PrefixExp>) + lit<")">)  //
             | create<ast::NameExpr>(assign<&ast::NameExpr::target>(ref<ID>))  //
             | create<ast::NumberLiteral>(
                   option(flag<&ast::NumberLiteral::negative>(lit<"-">) + gap::none) +
                   assign<&ast::NumberLiteral::value>(ref<DIGITS>))> {};

  // The trailing expression is the program's result, so it is its own
  // member rather than the statement list's last entry.
  struct Model
      : rule<create<ast::Model> +
             many(append<&ast::Model::statements>(ref<FnDef>) |
                  append<&ast::Model::statements>(ref<VarAssign>)) +
             assign<&ast::Model::result>(ref<PrefixExp>)> {};

  using entry = Model;
  using ignored = WS;
};

}  // namespace scotty
