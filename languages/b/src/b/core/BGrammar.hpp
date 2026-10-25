// The b language: its grammar.
#pragma once

#include "b/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace b {

inline constexpr auto stringValue = [](std::string_view text,
                                       std::pmr::memory_resource *mem) {
  text = text.substr(1, text.size() - 2);
  if (text.find('*') == text.npos)
    return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '*' && i + 1 < text.size()) {
      c = text[++i];
      if (c == '0' || c == 'e')
        c = '\0';
      else if (c == 't')
        c = '\t';
      else if (c == 'r')
        c = '\r';
      else if (c == 'n')
        c = '\n';
    }
    room[n++] = c;
  }
  return std::string_view(room.data(), n);
};
using Unquoted = cursiv::text_conv<stringValue>;

struct B : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct BLOCKCOMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_."> + many(cr<"a-zA-Z0-9_.">)> {};
  struct INT : terminal<some(d)> {};
  static constexpr auto escape = lit<"*"> + any;
  struct STRING1
      : terminal<lit<"\""> + many(escape | cr<R"(^"*\n)">) + lit<"\"">,
                 Unquoted> {};
  struct STRING2 : terminal<lit<"'"> + many(escape | cr<R"(^'*\n)">) + lit<"'">,
                            Unquoted> {};

  // `*` joins the unary operators here (Users' Reference to B §4.2.1): a
  // prefix, right-to-left indirection, one tier looser than a primary
  // expression and tighter than the multiplicative level.
  static constexpr auto UnaryOp = lit<"-"> | lit<"!"> | lit<"*">;
  static constexpr auto IncDecOp = lit<"++"> | lit<"--">;
  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto AddOp = lit<"+"> | lit<"-">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>">;
  static constexpr auto RelOp = lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto EqOp = lit<"=="> | lit<"!=">;
  static constexpr auto AndOp = lit<"&">;
  static constexpr auto OrOp = lit<"|">;

  struct Rvalue;
  struct Statement;
  struct Ternary;

  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};

  struct IntegerLit : rule<assign<&ast::IntLit::value>(ref<INT>)> {};
  struct StringLitRule : rule<assign<&ast::StringLit::text>(ref<STRING1>)> {};
  struct CharLitRule : rule<assign<&ast::CharLit::text>(ref<STRING2>)> {};

  struct Constant
      : rule<ref<IntegerLit> | ref<StringLitRule> | ref<CharLitRule>> {};

  struct Ival : rule<ref<Constant> | ref<Identifier>> {};

  struct Assignment;
  struct PrimaryBase : rule<(lit<"("> + ref<Rvalue> + lit<")">) |
                            ref<Constant> | ref<Identifier>> {};

  static constexpr auto IndexStep =
      nest<ast::IndexExpr, &ast::IndexExpr::array>(
          lit<"["> + assign<&ast::IndexExpr::index>(ref<Rvalue>) + lit<"]">);
  static constexpr auto CallStep = nest<ast::CallExpr, &ast::CallExpr::callee>(
      lit<"("> + many(append<&ast::CallExpr::args>(ref<Assignment>), lit<",">) +
      lit<")">);
  struct Primary : rule<ref<PrimaryBase> + many(IndexStep | CallStep)> {};

  struct Postfix : rule<ref<Primary> +
                        many(nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                            assign<&ast::PostfixExpr::op>(IncDecOp)))> {};

  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(UnaryOp | IncDecOp) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             create<ast::AddrExpr>(
                 lit<"&"> + assign<&ast::AddrExpr::target>(ref<UnaryExpr>)) |
             ref<Postfix>> {};

  struct BinaryExpr
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(MulOp) >> left(AddOp) >> left(ShiftOp) >> left(RelOp) >>
               left(EqOp) >> left(AndOp) >> left(OrOp))> {};

  struct Ternary
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryExpr>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Rvalue>) + lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Ternary>)) //
             | ref<BinaryExpr>> {};

  static constexpr auto CompoundOp = lit<"*"> | lit<"%"> | lit<"/", !lit<"*">> |
                                     AddOp | ShiftOp | RelOp | EqOp | AndOp |
                                     OrOp;
  struct AssignOperator
      : terminal<
            lit<"="> + option(CompoundOp),
            text_conv<[](std::string_view text) { return text.substr(1); }>> {};
  struct Assignment
      : rule<(assign<&ast::AssignExpr::target>(ref<Ternary>) +
                 assign<&ast::AssignExpr::op>(ref<AssignOperator>) +
                 assign<&ast::AssignExpr::value>(ref<Assignment>)) |
             ref<Ternary>> {};

  struct Rvalue
      : infix<Assignment, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, right(lit<",">)> {};

  struct NullStmt : rule<create<ast::NullStmt>(lit<";">)> {};

  struct ExprStmt
      : rule<assign<&ast::ExprStmt::value>(ref<Rvalue>) + lit<";">> {};

  struct Block
      : rule<lit<"{"> + many(append<&ast::Block::stmts>(ref<Statement>)) +
             lit<"}">> {};

  struct ReturnStmt
      : rule<kw<"return"> +
             option(lit<"("> + assign<&ast::ReturnStmt::value>(ref<Rvalue>) +
                    lit<")">) +
             lit<";">> {};

  struct GotoStmt
      : rule<kw<"goto"> + assign<&ast::GotoStmt::target>(ref<Rvalue>) +
             lit<";">> {};

  // No parens around the value, unlike `if`/`while`.
  struct SwitchStmt
      : rule<kw<"switch"> + assign<&ast::SwitchStmt::value>(ref<Rvalue>) +
             assign<&ast::SwitchStmt::body>(ref<Statement>)> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Rvalue>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Rvalue>) +
             lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct CaseStmt
      : rule<kw<"case"> + assign<&ast::CaseLabel::value>(ref<Constant>) +
             lit<":"> + assign<&ast::CaseLabel::body>(ref<Statement>)> {};

  struct LabeledStmt
      : rule<assign<&ast::LabeledStmt::name>(ref<Identifier>) + lit<":"> +
             assign<&ast::LabeledStmt::body>(ref<Statement>)> {};

  struct ExternStmt
      : rule<kw<"extrn"> +
             some(append<&ast::ExternStmt::names>(ref<Identifier>), lit<",">) +
             lit<";">> {};

  struct AutoVarRule
      : rule<create<ast::AutoVar>(
            assign<&ast::AutoVar::name>(ref<Identifier>) +
            option((flag<&ast::AutoVar::bracketed>(lit<"[">) +
                    option(assign<&ast::AutoVar::size>(ref<Constant>)) +
                    lit<"]">) |
                   assign<&ast::AutoVar::size>(ref<Constant>)))> {};

  struct AutoStmt
      : rule<kw<"auto"> +
             some(append<&ast::AutoStmt::vars>(ref<AutoVarRule>), lit<",">) +
             lit<";">> {};

  struct Statement : rule<ref<ExternStmt>    //
                          | ref<AutoStmt>    //
                          | ref<LabeledStmt> //
                          | ref<CaseStmt>    //
                          | ref<Block>       //
                          | ref<IfStmt>      //
                          | ref<WhileStmt>   //
                          | ref<SwitchStmt>  //
                          | ref<GotoStmt>    //
                          | ref<ReturnStmt>  //
                          | ref<ExprStmt>    //
                          | ref<NullStmt>> {};

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            assign<&ast::FunctionDef::name>(ref<Identifier>) + lit<"("> +
            many(append<&ast::FunctionDef::params>(ref<Identifier>), lit<",">) +
            lit<")"> + assign<&ast::FunctionDef::body>(ref<Statement>))> {};

  struct VarDef
      : rule<create<ast::VarDef>(
            assign<&ast::VarDef::name>(ref<Identifier>) +
            option(flag<&ast::VarDef::bracketed>(lit<"[">) +
                   option(assign<&ast::VarDef::size>(ref<Constant>)) +
                   lit<"]">) +
            many(append<&ast::VarDef::inits>(ref<Ival>) + option(lit<",">)) +
            lit<";">)> {};

  struct Definition : rule<ref<FunctionDef> | ref<VarDef>> {};

  struct ProgramRule
      : rule<many(append<&ast::Program::items>(ref<Definition>) | lit<";">)> {};

  static constexpr auto word_boundary = w | cr<".">;
  using entry = ProgramRule;
  using ignore = list<WS, BLOCKCOMMENT>;
};

} // namespace b
