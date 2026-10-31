// The moo language: its grammar.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "moo/core/ast.hpp"

namespace moo {

inline constexpr auto strip_quotes = [](std::string_view t,
                                        std::pmr::memory_resource* mem) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.size() < 2 || t.front() != '"' || t.back() != '"')
    return Result::err("A string literal needs opening and closing quotes.");
  const auto body = t.substr(1, t.size() - 2);
  if (body.find('\\') == body.npos) return Result::ok(body);
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\\' && i + 1 < body.size()) ++i;
    room[n++] = body[i];
  }
  return Result::ok(std::string_view(room.data(), n));
};

struct Moo : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // Disabled client commands are semicolon lines outside @program. Inside
  // a program, semicolons terminate statements and never hide later code.
  struct COMMENT : terminal<lit<";"> + some(cr<R"(^\r\n)">)> {};

  // Native1.8 keywords are case-insensitive and cannot name program variables.
  static constexpr auto Reserved =
      ikw<"if"> | ikw<"else"> | ikw<"elseif"> | ikw<"endif"> |
      ikw<"for"> | ikw<"in"> | ikw<"endfor"> | ikw<"fork"> |
      ikw<"endfork"> | ikw<"return"> | ikw<"while"> | ikw<"endwhile"> |
      ikw<"try"> | ikw<"except"> | ikw<"finally"> | ikw<"endtry"> |
      ikw<"ANY"> | ikw<"break"> | ikw<"continue"> | ikw<"E_NONE"> |
      ikw<"E_TYPE"> | ikw<"E_DIV"> | ikw<"E_PERM"> | ikw<"E_PROPNF"> |
      ikw<"E_VERBNF"> | ikw<"E_VARNF"> | ikw<"E_INVIND"> | ikw<"E_RECMOVE"> |
      ikw<"E_MAXREC"> | ikw<"E_RANGE"> | ikw<"E_ARGS"> | ikw<"E_NACC"> |
      ikw<"E_INVARG"> | ikw<"E_QUOTA"> | ikw<"E_FLOAT">;

  // Program identifiers must not absorb multiplication or logical-not.
  // Client command patterns retain the imported wildcard spellings separately.
  struct WORD : terminal<!Reserved + cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct CLIENT_WORD : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9!_*">)> {};
  struct CLIENT_SYSNAME
      : terminal<lit<"$"> + option(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9!_*">))> {};
  struct SYSNAME
      : terminal<lit<"$"> + option(cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">))> {};
  struct STRINGLITERAL
      : terminal<as<strip_quotes>(lit<"\""> +
                 many((lit<"\\"> + cr<R"(^\r\n)">) | cr<R"(^"\\\r\n)">) +
                 lit<"\"">)> {
  };
  struct INTEGER : terminal<some(d)> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct REAL : terminal<(((some(d) + lit<".", !lit<".">> + many(d)) |
                           (lit<"."> + some(d))) + option(exponent)) |
                          (some(d) + exponent)> {};
  struct OBJREF : terminal<lit<"#"> + option(lit<"-">) + some(d)> {};
  struct PERMISSIONS : terminal<some(cr<"rcxd">)> {};

  struct ERROR_NAME : terminal<ikw<"E_NONE"> | ikw<"E_TYPE"> | ikw<"E_DIV"> | ikw<"E_PERM"> | ikw<"E_PROPNF"> | ikw<"E_VERBNF"> | ikw<"E_VARNF"> | ikw<"E_INVIND"> | ikw<"E_RECMOVE"> | ikw<"E_MAXREC"> | ikw<"E_RANGE"> | ikw<"E_ARGS"> | ikw<"E_NACC"> | ikw<"E_INVARG"> | ikw<"E_QUOTA"> | ikw<"E_FLOAT">> {};

  struct AND : terminal<as<ast::Operator::logical_and>(lit<"&&">)> {};

  struct POW : terminal<as<ast::Operator::power>(lit<"^">)> {};

  static constexpr auto Name = ref<WORD> | ref<SYSNAME> | ikw<"do">;
  static constexpr auto ClientName = ref<CLIENT_WORD> | ref<CLIENT_SYSNAME>;
  static constexpr auto ClientNameOrString = ClientName | ref<STRINGLITERAL>;
  static constexpr auto ClientObject = ClientNameOrString | ref<OBJREF>;
  static constexpr auto CompareOps = as<ast::Operator::equal>(lit<"==">) | as<ast::Operator::not_equal>(lit<"!=">) | as<ast::Operator::greater_equal>(lit<">=">) |
                                     as<ast::Operator::less_equal>(lit<"<=">) | as<ast::Operator::greater>(lit<">">) | as<ast::Operator::less>(lit<"<">) | as<ast::Operator::in_>(ikw<"in">);

  struct Expression;
  struct SignedAtom;
  struct Atom;
  struct PropertyChain;
  struct VerbTarget;
  struct Statement;

  // Native LambdaMOO: exponentiation associates right; && and || share
  // one left-associative level. Unary operators bind tighter than power.
  struct BinaryExprRule
      : infix<SignedAtom, ast::BinaryExpr,
              levels(right(ref<POW>),                          //
                     left(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::modulo>(lit<"%">)),  //
                     left(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)),
                     left(CompareOps),
                     left(ref<AND> | as<ast::Operator::logical_or>(lit<"||">)))> {};

  // Native '?' and '|' are one non-associative level below &&/||.
  // Parenthesized expressions restart this level through Atom.
  struct Expression : rule<ref<BinaryExprRule> + option(
      nest<ast::ConditionalExpr, &ast::ConditionalExpr::condition>(
          lit<"?"> + assign<&ast::ConditionalExpr::consequent>(ref<BinaryExprRule>) +
          lit<"|"> + assign<&ast::ConditionalExpr::alternate>(ref<BinaryExprRule>)))> {};

  struct SignedAtom
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<SignedAtom>)) |
             create<ast::NotExpr>(lit<"!"> +
                 assign<&ast::NotExpr::operand>(ref<SignedAtom>)) |
             ref<Atom>> {};

  struct StringLit : rule<create<ast::StringLit> +
                              assign<&ast::StringLit::value>(
                                  ref<STRINGLITERAL>)> {};
  struct IntLit : rule<create<ast::IntLit> +
                           assign<&ast::IntLit::value>(ref<INTEGER>)> {};
  struct RealLit : rule<create<ast::RealLit> +
                            assign<&ast::RealLit::value>(ref<REAL>)> {};
  struct ErrorLit : rule<assign<&ast::ErrorLit::name>(ref<ERROR_NAME>)> {};
  struct ObjRef : rule<create<ast::ObjRef> +
                           assign<&ast::ObjRef::text>(ref<OBJREF>)> {};
  struct List
      : rule<create<ast::ListExpr> + lit<"{"> +
             many(append<&ast::ListExpr::items>(ref<Expression>), lit<",">) +
             lit<"}">> {};
  struct FuncCall
      : rule<create<ast::FuncCall> + assign<&ast::FuncCall::name>(Name) +
             lit<"("> +
             many(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
             lit<")">> {};
  struct VerbCall
      : rule<create<ast::VerbCall> +
             assign<&ast::VerbCall::target>(ref<VerbTarget>) + lit<":"> +
             assign<&ast::VerbCall::verb>(Name) +
             option(lit<"("> +
                    many(append<&ast::VerbCall::args>(ref<Expression>), lit<",">) +
                    lit<")">)> {};

  struct Atom
      : rule<ref<StringLit> | ref<ErrorLit> | ref<FuncCall> | ref<VerbCall> |
             ref<PropertyChain> | ref<IntLit> | ref<RealLit> |
             ref<List> | ref<ObjRef> |
             (lit<"("> + ref<Expression> + lit<")">)> {};

  template <auto NameShape>
  static constexpr auto PropertyStep =
      (lit<"."> + append<&ast::Property::path>(NameShape)) |
      (lit<"["> + append<&ast::Property::path>(ref<Expression>) + lit<"]">);
  template <auto NameShape, bool BareLiteral>
  static constexpr auto LiteralPropertyTail = [] {
    if constexpr (BareLiteral) return many(PropertyStep<NameShape>);
    else return some(PropertyStep<NameShape>);
  }();
  template <auto NameShape, bool BareLiteral>
  static constexpr auto PropertyBody =
      create<ast::Property> +
      ((assign<&ast::Property::base>(NameShape) + many(PropertyStep<NameShape>)) |
       (flag<&ast::Property::stringBase>(assign<&ast::Property::base>(ref<STRINGLITERAL>)) +
        LiteralPropertyTail<NameShape, BareLiteral>) |
       (flag<&ast::Property::objectBase>(assign<&ast::Property::base>(ref<OBJREF>)) +
        LiteralPropertyTail<NameShape, BareLiteral>));
  struct PropertyChain : rule<PropertyBody<Name, false>> {};
  struct VerbTarget : rule<PropertyBody<Name, true>> {};
  struct ClientPropertyChain : rule<PropertyBody<ClientName, true>> {};

  struct Condition : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct ElseIfClause
      : rule<ikw<"elseif"> + assign<&ast::ElseIfClause::cond>(ref<Condition>) +
             many(append<&ast::ElseIfClause::body>(ref<Statement>))> {};
  struct IfStmt
      : rule<create<ast::IfStmt> + ikw<"if"> +
             assign<&ast::IfStmt::cond>(ref<Condition>) +
             many(append<&ast::IfStmt::thenBody>(ref<Statement>)) +
             many(append<&ast::IfStmt::elseIfs>(ref<ElseIfClause>)) +
             option(flag<&ast::IfStmt::hasElse>(ikw<"else">) +
                    many(append<&ast::IfStmt::elseBody>(ref<Statement>))) +
             ikw<"endif"> + option(lit<";">)> {};

  struct WhileStmt : rule<create<ast::WhileStmt> + ikw<"while"> +
                          option(assign<&ast::WhileStmt::name>(ref<WORD>)) +
                          assign<&ast::WhileStmt::cond>(ref<Condition>) +
                          many(append<&ast::WhileStmt::body>(ref<Statement>)) +
                          ikw<"endwhile"> + option(lit<";">)> {};

  struct DoStmt
      : rule<create<ast::DoStmt> + ikw<"do"> +
             many(append<&ast::DoStmt::body>(ref<Statement>)) + ikw<"while"> +
             assign<&ast::DoStmt::cond>(ref<Condition>)> {};

  struct ForkStmt : rule<ikw<"fork"> +
      option(assign<&ast::ForkStmt::name>(ref<WORD>)) +
      lit<"("> + assign<&ast::ForkStmt::delay>(ref<Expression>) + lit<")"> +
      many(append<&ast::ForkStmt::body>(ref<Statement>)) + ikw<"endfork">> {};

  struct ForStmt
      : rule<create<ast::ForStmt> + ikw<"for"> +
             assign<&ast::ForStmt::var>(Name) + ikw<"in"> +
             ((lit<"["> + assign<&ast::ForStmt::from>(ref<Expression>) + lit<".."> +
               assign<&ast::ForStmt::to>(ref<Expression>) + lit<"]">) |
              assign<&ast::ForStmt::iterable>(ref<Expression>)) +
             many(append<&ast::ForStmt::body>(ref<Statement>)) + ikw<"endfor">> {
  };

  struct ExceptClause
      : rule<ikw<"except"> + option(assign<&ast::ExceptClause::variable>(ref<WORD>)) +
             lit<"("> +
             (flag<&ast::ExceptClause::any>(ikw<"ANY">) |
              some(append<&ast::ExceptClause::codes>(ref<Expression>), lit<",">)) +
             lit<")"> + many(append<&ast::ExceptClause::body>(ref<Statement>))> {};
  struct TryStmt
      : rule<create<ast::TryStmt> + ikw<"try"> +
             many(append<&ast::TryStmt::body>(ref<Statement>)) +
             (some(append<&ast::TryStmt::clauses>(ref<ExceptClause>)) |
              (flag<&ast::TryStmt::hasFinally>(ikw<"finally">) +
               many(append<&ast::TryStmt::cleanup>(ref<Statement>))) |
              // Keep the imported pedagogical unparenthesized handler form.
              (ikw<"except"> + assign<&ast::TryStmt::except>(ref<ClientPropertyChain>) +
               many(append<&ast::TryStmt::handler>(ref<Statement>)))) +
             ikw<"endtry">> {};
  struct LoopControlStmt
      : rule<(flag<&ast::LoopControlStmt::continue_>(ikw<"continue">) | ikw<"break">) +
             option(assign<&ast::LoopControlStmt::name>(ref<WORD>)) + lit<";">> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt> +
             assign<&ast::AssignStmt::target>(ref<PropertyChain>) +
             lit<"="> + assign<&ast::AssignStmt::value>(ref<Expression>) +
             lit<";">> {};

  struct ReturnCmd
      : rule<create<ast::ReturnCommand> + ikw<"return"> +
             option(assign<&ast::ReturnCommand::value>(ref<Expression>))> {};

  struct CommandStmt
      : rule<create<ast::CommandStmt> +
             assign<&ast::CommandStmt::command>(ref<VerbCall> |
                                                ref<ReturnCmd> | ref<FuncCall>) +
             lit<";">> {};

  struct Statement
      : rule<ref<IfStmt> | ref<WhileStmt> | ref<DoStmt> | ref<ForStmt> |
             ref<ForkStmt> |
             ref<AssignStmt> | ref<TryStmt> | ref<LoopControlStmt> | ref<CommandStmt>> {};

  struct ProgramDecl
      : rule<create<ast::ProgramDecl> + ikw<"@program"> +
             assign<&ast::ProgramDecl::object>(ClientObject) + lit<":"> +
             assign<&ast::ProgramDecl::verb>(ClientName) +
             many(append<&ast::ProgramDecl::body>(ref<Statement>)) + lit<".">> {
  };

  struct VerbDecl
      : rule<create<ast::VerbDecl> + ikw<"@verb"> +
             assign<&ast::VerbDecl::object>(ClientObject) + lit<":"> +
             assign<&ast::VerbDecl::verb>(ClientName) +
             some(append<&ast::VerbDecl::args>(ClientName)) +
             option(assign<&ast::VerbDecl::permissions>(ref<PERMISSIONS>))> {
  };

  struct PropertyDecl
      : rule<create<ast::PropertyDecl> +
             (ikw<"@property"> | ikw<"@prop">) +
             assign<&ast::PropertyDecl::target>(ref<ClientPropertyChain>) +
             option(lit<"=">) +
             option(assign<&ast::PropertyDecl::value>(ref<Expression>)) +
             option(assign<&ast::PropertyDecl::permissions>(
                 ref<PERMISSIONS>))> {};

  struct RmPropertyDecl
      : rule<create<ast::RmPropertyDecl> +
             (ikw<"@rmproperty"> | ikw<"@rmprop">) +
             assign<&ast::RmPropertyDecl::name>(ClientName)> {};

  struct SetPropertyDecl
      : rule<create<ast::SetPropertyDecl> + ikw<"@set"> +
             assign<&ast::SetPropertyDecl::target>(ref<ClientPropertyChain>) +
             ikw<"to"> +
             assign<&ast::SetPropertyDecl::value>(ref<Expression>)> {};

  struct DisplayPropertyDecl
      : rule<create<ast::DisplayPropertyDecl> +
             (ikw<"@display"> | ikw<"@disp">) +
             assign<&ast::DisplayPropertyDecl::target>(ref<ClientPropertyChain>)> {
  };

  struct KidsDecl : rule<create<ast::KidsDecl> + ikw<"@kids"> +
                             assign<&ast::KidsDecl::name>(ClientName)> {};

  struct ParentDecl : rule<create<ast::ParentDecl> + ikw<"@parent"> +
                               assign<&ast::ParentDecl::name>(ClientName)> {};

  struct DescribeDecl
      : rule<create<ast::DescribeDecl> + ikw<"@describe"> +
             assign<&ast::DescribeDecl::target>(ref<ClientPropertyChain>) +
             ikw<"as"> +
             assign<&ast::DescribeDecl::value>(ref<Expression>)> {};

  struct ContentsDecl
      : rule<create<ast::ContentsDecl> + ikw<"@contents"> +
             assign<&ast::ContentsDecl::name>(ClientName)> {};

  struct NoteditDecl
      : rule<create<ast::NoteditDecl> + ikw<"@notedit"> +
             assign<&ast::NoteditDecl::target>(ref<ClientPropertyChain>)> {};

  struct CreateDecl
      : rule<create<ast::CreateDecl> + ikw<"@create"> +
             assign<&ast::CreateDecl::parent>(ref<CLIENT_SYSNAME>) + ikw<"called"> +
             some(append<&ast::CreateDecl::args>(ref<Expression>),
                  lit<",">)> {};

  struct EditDecl : rule<create<ast::EditDecl> + ikw<"@edit"> +
                             assign<&ast::EditDecl::target>(
                                 ref<ClientPropertyChain>)> {};

  struct AddAliasDecl
      : rule<create<ast::AddAliasDecl> + ikw<"@addalias"> +
             some(append<&ast::AddAliasDecl::names>(ClientName), lit<",">) +
             ikw<"to"> +
             assign<&ast::AddAliasDecl::target>(ref<Expression>)> {};

  struct Declaration
      : rule<ref<ProgramDecl> | ref<VerbDecl> |
             ref<PropertyDecl> | ref<RmPropertyDecl> |
             ref<SetPropertyDecl> | ref<DisplayPropertyDecl> |
             ref<KidsDecl> | ref<ParentDecl> | ref<DescribeDecl> |
             ref<ContentsDecl> | ref<NoteditDecl> |
             ref<CreateDecl> | ref<EditDecl> |
             ref<AddAliasDecl>> {};

  struct Prog
      : rule<create<ast::Prog> + many(ref<COMMENT>) +
             some(append<&ast::Prog::declarations>(ref<Declaration>) +
                  many(ref<COMMENT>))> {};

  using entry = Prog;
  using ignored = WS;
};

}  // namespace moo
