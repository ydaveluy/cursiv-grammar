// The moo language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "moo/core/ast.hpp"

namespace moo {

inline constexpr auto strip_quotes = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<strip_quotes>;

struct Moo : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // Disabled client commands are semicolon lines outside @program. Inside
  // a program, semicolons terminate statements and never hide later code.
  struct COMMENT : terminal<lit<";"> + some(cr<R"(^\r\n)">)> {};

  struct WORD : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9!_*">)> {};
  struct SYSNAME
      : terminal<lit<"$"> + option(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9!_*">))> {};
  struct STRINGLITERAL
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};
  struct INTEGER : terminal<some(d)> {};
  struct REAL : terminal<some(d) + lit<"."> + some(d)> {};
  struct OBJREF : terminal<lit<"#"> + some(d)> {};
  struct PERMISSIONS : terminal<some(cr<"rcxd">)> {};

  struct EQ : terminal<lit<"==">, value<ast::Operator::equal>> {};
  struct NEQ : terminal<lit<"!=">, value<ast::Operator::not_equal>> {};
  struct GTE : terminal<lit<">=">, value<ast::Operator::greater_equal>> {};
  struct LTE : terminal<lit<"<=">, value<ast::Operator::less_equal>> {};
  struct GT : terminal<lit<">">, value<ast::Operator::greater>> {};
  struct LT : terminal<lit<"<">, value<ast::Operator::less>> {};
  struct AND : terminal<lit<"&&">, value<ast::Operator::logical_and>> {};
  struct OR : terminal<lit<"||">, value<ast::Operator::logical_or>> {};

  struct POW : terminal<lit<"^">, value<ast::Operator::power>> {};
  struct TIMES : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MOD : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct PLUSOP : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUSOP : terminal<lit<"-">, value<ast::Operator::subtract>> {};

  static constexpr auto Name = ref<WORD> | ref<SYSNAME>;
  static constexpr auto NameOrString = Name | ref<STRINGLITERAL>;
  static constexpr auto CompareOps = ref<EQ> | ref<NEQ> | ref<GTE> |
                                     ref<LTE> | ref<GT> | ref<LT>;

  struct Expression;
  struct SignedAtom;
  struct Atom;
  struct PropertyChain;
  struct Statement;

  // POW tightest, then */%, then binary +/-, all left-associative.
  struct Expression
      : infix<SignedAtom, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<POW>) >>                            //
               left(ref<TIMES> | ref<DIV> | ref<MOD>) >>    //
               left(ref<PLUSOP> | ref<MINUSOP>))> {};

  struct SignedAtom
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<SignedAtom>)) |
             ref<Atom>> {};

  struct StringLitRule : rule<create<ast::StringLit> +
                              assign<&ast::StringLit::value>(
                                  ref<STRINGLITERAL>)> {};
  struct IntLitRule : rule<create<ast::IntLit> +
                           assign<&ast::IntLit::value>(ref<INTEGER>)> {};
  struct RealLitRule : rule<create<ast::RealLit> +
                            assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct ObjRefRule : rule<create<ast::ObjRef> +
                           assign<&ast::ObjRef::text>(ref<OBJREF>)> {};
  struct ListRule
      : rule<create<ast::ListExpr> + lit<"{"> +
             option(some(append<&ast::ListExpr::items>(ref<Expression>),
                         lit<",">)) +
             lit<"}">> {};
  struct FuncCallRule
      : rule<create<ast::FuncCall> + assign<&ast::FuncCall::name>(Name) +
             lit<"("> +
             many(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
             lit<")">> {};
  struct VerbCallRule
      : rule<create<ast::VerbCall> +
             assign<&ast::VerbCall::target>(ref<PropertyChain>) + lit<":"> +
             assign<&ast::VerbCall::verb>(Name) +
             option(lit<"("> +
                    option(some(append<&ast::VerbCall::args>(ref<Expression>),
                                lit<",">)) +
                    lit<")">)> {};

  struct Atom
      : rule<ref<StringLitRule> | ref<FuncCallRule> | ref<VerbCallRule> |
             ref<PropertyChain> | ref<IntLitRule> | ref<RealLitRule> |
             ref<ListRule> | ref<ObjRefRule> |
             (lit<"("> + ref<Expression> + lit<")">) |
             create<ast::NotExpr>(
                 lit<"!"> +
                 assign<&ast::NotExpr::operand>(ref<Expression>))> {};

  // `property_`: a base name/string, then a chain of `.name` or `[expr]`
  // steps. Each step's directive wraps only the value, per the append<>
  // pitfall on a vector<variant<...>>.
  struct PropertyChain
      : rule<create<ast::Property> + assign<&ast::Property::base>(
                                         NameOrString) +
             many((lit<"."> + append<&ast::Property::path>(Name)) |
                  (lit<"["> +
                   append<&ast::Property::path>(ref<Expression>) +
                   lit<"]">))> {};

  // Comparisons bind tighter than `&&`, `&&` tighter than `||`.
  struct Comparison
      : infix<Expression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(CompareOps) >> left(ref<AND>) >> left(ref<OR>))> {};

  struct Condition : rule<lit<"("> + ref<Comparison> + lit<")">> {};

  struct IfStmt
      : rule<create<ast::IfStmt> + kw<"if"> +
             assign<&ast::IfStmt::cond>(ref<Condition>) +
             many(append<&ast::IfStmt::thenBody>(ref<Statement>)) +
             option(kw<"elseif"> +
                    assign<&ast::IfStmt::elifCond>(ref<Condition>) +
                    many(append<&ast::IfStmt::elifBody>(ref<Statement>))) +
             option(kw<"else"> +
                    many(append<&ast::IfStmt::elseBody>(ref<Statement>))) +
             kw<"endif"> + option(lit<";">)> {};

  struct WhileStmt : rule<create<ast::WhileStmt> + kw<"while"> +
                          assign<&ast::WhileStmt::cond>(ref<Condition>) +
                          many(append<&ast::WhileStmt::body>(ref<Statement>)) +
                          kw<"endwhile"> + option(lit<";">)> {};

  struct DoStmt
      : rule<create<ast::DoStmt> + kw<"do"> +
             many(append<&ast::DoStmt::body>(ref<Statement>)) + kw<"while"> +
             assign<&ast::DoStmt::cond>(ref<Condition>)> {};

  struct ForStmt
      : rule<create<ast::ForStmt> + kw<"for"> +
             assign<&ast::ForStmt::var>(Name) + kw<"in"> +
             assign<&ast::ForStmt::iterable>(ref<Expression>) +
             many(append<&ast::ForStmt::body>(ref<Statement>)) + kw<"endfor">> {
  };

  struct TryStmt
      : rule<create<ast::TryStmt> + kw<"try"> +
             many(append<&ast::TryStmt::body>(ref<Statement>)) + kw<"except"> +
             assign<&ast::TryStmt::except>(ref<PropertyChain>) +
             many(append<&ast::TryStmt::handler>(ref<Statement>)) +
             kw<"endtry">> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt> +
             assign<&ast::AssignStmt::target>(ref<PropertyChain>) +
             lit<"="> + assign<&ast::AssignStmt::value>(ref<Expression>) +
             lit<";">> {};

  struct ReturnCmdRule
      : rule<create<ast::ReturnCommand> + kw<"return"> +
             option(assign<&ast::ReturnCommand::value>(ref<Expression>))> {};

  struct CommandStmt
      : rule<create<ast::CommandStmt> +
             assign<&ast::CommandStmt::command>(ref<VerbCallRule> |
                                                ref<ReturnCmdRule>) +
             lit<";">> {};

  struct Statement
      : rule<ref<IfStmt> | ref<WhileStmt> | ref<DoStmt> | ref<ForStmt> |
             ref<AssignStmt> | ref<TryStmt> | ref<CommandStmt>> {};

  struct ProgramDeclRule
      : rule<create<ast::ProgramDecl> + lit<"@program"> +
             assign<&ast::ProgramDecl::object>(NameOrString) + lit<":"> +
             assign<&ast::ProgramDecl::verb>(Name) +
             many(append<&ast::ProgramDecl::body>(ref<Statement>)) + lit<".">> {
  };

  struct VerbDeclRule
      : rule<create<ast::VerbDecl> + lit<"@verb"> +
             assign<&ast::VerbDecl::object>(NameOrString) + lit<":"> +
             assign<&ast::VerbDecl::verb>(Name) +
             some(append<&ast::VerbDecl::args>(Name)) +
             option(assign<&ast::VerbDecl::permissions>(ref<PERMISSIONS>))> {
  };

  struct PropertyDeclRule
      : rule<create<ast::PropertyDecl> +
             (lit<"@property"> | lit<"@prop">) +
             assign<&ast::PropertyDecl::target>(ref<PropertyChain>) +
             option(lit<"=">) +
             option(assign<&ast::PropertyDecl::value>(ref<Expression>)) +
             option(assign<&ast::PropertyDecl::permissions>(
                 ref<PERMISSIONS>))> {};

  struct RmPropertyDeclRule
      : rule<create<ast::RmPropertyDecl> +
             (lit<"@rmproperty"> | lit<"@rmprop">) +
             assign<&ast::RmPropertyDecl::name>(Name)> {};

  struct SetPropertyDeclRule
      : rule<create<ast::SetPropertyDecl> + lit<"@set"> +
             assign<&ast::SetPropertyDecl::target>(ref<PropertyChain>) +
             kw<"to"> +
             assign<&ast::SetPropertyDecl::value>(ref<Expression>)> {};

  struct DisplayPropertyDeclRule
      : rule<create<ast::DisplayPropertyDecl> +
             (lit<"@display"> | lit<"@disp">) +
             assign<&ast::DisplayPropertyDecl::target>(ref<PropertyChain>)> {
  };

  struct KidsDeclRule : rule<create<ast::KidsDecl> + lit<"@kids"> +
                             assign<&ast::KidsDecl::name>(Name)> {};

  struct ParentDeclRule : rule<create<ast::ParentDecl> + lit<"@parent"> +
                               assign<&ast::ParentDecl::name>(Name)> {};

  struct DescribeDeclRule
      : rule<create<ast::DescribeDecl> + lit<"@describe"> +
             assign<&ast::DescribeDecl::target>(ref<PropertyChain>) +
             kw<"as"> +
             assign<&ast::DescribeDecl::value>(ref<Expression>)> {};

  struct ContentsDeclRule
      : rule<create<ast::ContentsDecl> + lit<"@contents"> +
             assign<&ast::ContentsDecl::name>(Name)> {};

  struct NoteditDeclRule
      : rule<create<ast::NoteditDecl> + lit<"@notedit"> +
             assign<&ast::NoteditDecl::target>(ref<PropertyChain>)> {};

  struct CreateDeclRule
      : rule<create<ast::CreateDecl> + lit<"@create"> +
             assign<&ast::CreateDecl::parent>(ref<SYSNAME>) + kw<"called"> +
             some(append<&ast::CreateDecl::args>(ref<Expression>),
                  lit<",">)> {};

  struct EditDeclRule : rule<create<ast::EditDecl> + lit<"@edit"> +
                             assign<&ast::EditDecl::target>(
                                 ref<PropertyChain>)> {};

  struct AddAliasDeclRule
      : rule<create<ast::AddAliasDecl> + lit<"@addalias"> +
             some(append<&ast::AddAliasDecl::names>(Name), lit<",">) +
             kw<"to"> +
             assign<&ast::AddAliasDecl::target>(ref<Expression>)> {};

  struct DeclarationRule
      : rule<ref<ProgramDeclRule> | ref<VerbDeclRule> |
             ref<PropertyDeclRule> | ref<RmPropertyDeclRule> |
             ref<SetPropertyDeclRule> | ref<DisplayPropertyDeclRule> |
             ref<KidsDeclRule> | ref<ParentDeclRule> | ref<DescribeDeclRule> |
             ref<ContentsDeclRule> | ref<NoteditDeclRule> |
             ref<CreateDeclRule> | ref<EditDeclRule> |
             ref<AddAliasDeclRule>> {};

  struct ProgRule
      : rule<create<ast::Prog> + many(ref<COMMENT>) +
             some(append<&ast::Prog::declarations>(ref<DeclarationRule>) +
                  many(ref<COMMENT>))> {};

  using entry = ProgRule;
  using ignore = WS;
};

}  // namespace moo
