// The r language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "r/core/ast.hpp"
#include "r/core/LiteralText.hpp"

namespace r {

struct R : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // A letter as R reads one in a UTF-8 locale.
  static constexpr auto LETTER = uc::Alphabetic;
  static constexpr auto word_boundary = LETTER | uc::Nd | cr<"_.">;
  static constexpr auto reserved = kw<"if"> | kw<"else"> | kw<"repeat"> |
      kw<"while"> | kw<"function"> | kw<"for"> | kw<"in"> | kw<"next"> |
      kw<"break"> | kw<"TRUE"> | kw<"FALSE"> | kw<"NULL"> | kw<"Inf"> |
      kw<"NaN"> | kw<"NA"> | kw<"NA_integer_"> | kw<"NA_real_"> |
      kw<"NA_complex_"> | kw<"NA_character_">;
  struct ID : terminal<!reserved + (LETTER | (lit<"."> + !d)) +
                       many(LETTER | uc::Nd | cr<"_.">)> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto EXP = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto DECIMAL =
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) + option(EXP);
  static constexpr auto HEXADECIMAL =
      lit<"0"> + cr<"xX"> +
      ((some(HEXDIGIT) + option(lit<"."> + many(HEXDIGIT))) |
       (lit<"."> + some(HEXDIGIT))) +
      option(cr<"pP"> + option(cr<"+-">) + some(d));
  struct HEX_LIT : terminal<HEXADECIMAL> {};
  struct INT_LIT : terminal<(HEXADECIMAL | DECIMAL) + lit<"L">> {};
  struct FLOAT_LIT : terminal<DECIMAL> {};
  struct COMPLEX_LIT : terminal<(HEXADECIMAL | DECIMAL) + lit<"i">> {};

  static constexpr auto ESC = lit<"\\"> + any;
  struct BACKTICK_ID
      : terminal<as<unquote>(lit<"`"> + some(ESC | (!cr<R"(\\`)"> + any)) +
                             lit<"`">)> {};
  struct STRING_LIT
      : terminal<as<unquote>(
            (lit<"\""> + many(ESC | (!cr<R"(\\")"> + any)) + lit<"\"">) |
            (lit<"'"> + many(ESC | (!cr<R"(\\')"> + any)) + lit<"'">))> {};

  struct RawDashes {};
  template <auto Quote, auto Open, auto Close>
  static constexpr auto RawString =
      through(Quote + mark<RawDashes>(many(cr<"-">)) + Open, Close + same<RawDashes> + Quote);
  inline static constexpr auto rawBody = [](std::string_view t) {
    std::size_t i = 2;
    while (i < t.size() && t[i] == '-') ++i;
    const std::size_t fence = i + 1;
    return t.substr(fence, t.size() - 2 * fence + 1);
  };
  struct RAW_STRING
      : terminal<as<rawBody>(cr<"rR"> +
                             (RawString<lit<"\"">, lit<"(">, lit<")">> |
                              RawString<lit<"\"">, lit<"[">, lit<"]">> |
                              RawString<lit<"\"">, lit<"{">, lit<"}">> |
                              RawString<lit<"\"">, lit<"|">, lit<"|">> |
                              RawString<lit<"'">, lit<"(">, lit<")">> |
                              RawString<lit<"'">, lit<"[">, lit<"]">> |
                              RawString<lit<"'">, lit<"{">, lit<"}">> |
                              RawString<lit<"'">, lit<"|">, lit<"|">>))> {};

  struct USER_OP : terminal<lit<"%"> + many(cr<R"(^%\r\n)">) + lit<"%">> {};

  // -- Operators, by precedence tier (tight to loose); each `op` slot keeps
  // its matched text, so a consumer reads the actual spelling.
  static constexpr auto NamespaceOp = lit<":::"> | lit<"::">;
  static constexpr auto ComponentOp = lit<"@"> | lit<"$">;
  static constexpr auto ExponentOp = lit<"^"> | lit<"**">;
  static constexpr auto SignOp = lit<"+"> | lit<"-">;
  static constexpr auto RangeOp = lit<":">;
  static constexpr auto MulDivOp = lit<"*"> | lit<"/">;
  static constexpr auto AddSubOp = lit<"+"> | lit<"-">;
  static constexpr auto ComparisonOp =
      lit<">="> | lit<"<="> | lit<"=="> | lit<"!="> | lit<">"> | lit<"<">;
  static constexpr auto NotOp = lit<"!">;
  static constexpr auto AndOp = lit<"&&"> | lit<"&">;
  static constexpr auto OrOp = lit<"||"> | lit<"|">;
  static constexpr auto TildeOp = lit<"~">;
  static constexpr auto RightAssignOp = lit<"->>"> | lit<"->">;
  static constexpr auto LeftAssignOp = lit<"<<-"> | lit<"<-"> | lit<":=">;
  static constexpr auto EqAssignOp = lit<"=", !lit<"=">>;
  static constexpr auto PipeOp = ref<USER_OP> | lit<"|>">;

  // A statement ends at an explicit `;`, or, if the next token cannot
  // continue it, at a line break in the gap (comments included), at a
  // closing `}` or at the end of input -- R's newline-as-separator, spelled
  // as an ASI-style implied token.
  static constexpr auto Eos = implied(lit<";">, gap::line | &lit<"}"> | eof);

  template <bool Lines>
  struct Expr;
  template <bool Lines>
  static constexpr auto Continue = [](auto token) {
    if constexpr (Lines)
      return !gap::line + token;
    else
      return token;
  };

  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expr<false>>) +
            lit<")">)> {};

  struct BlockExpr : rule<create<ast::Block>(
                         lit<"{"> + many(lit<";">) +
                         many(append<&ast::Block::body>(ref<Expr<true>>) + Eos +
                              many(lit<";">)) +
                         lit<"}">)> {};

  template <bool Lines>
  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expr<false>>) +
            lit<")"> + assign<&ast::IfExpr::thenBranch>(ref<Expr<Lines>>) +
            option(commit(kw<"else">, assign<&ast::IfExpr::elseBranch>(ref<Expr<Lines>>))))> {};

  template <bool Lines>
  struct ForExpr
      : rule<create<ast::ForExpr>(
            kw<"for"> + lit<"("> + assign<&ast::ForExpr::var>(ref<ID> | ref<BACKTICK_ID>) +
            kw<"in"> + assign<&ast::ForExpr::seq>(ref<Expr<false>>) + lit<")"> +
            assign<&ast::ForExpr::body>(ref<Expr<Lines>>))> {};

  template <bool Lines>
  struct WhileExpr
      : rule<create<ast::WhileExpr>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileExpr::cond>(ref<Expr<false>>) + lit<")"> +
            assign<&ast::WhileExpr::body>(ref<Expr<Lines>>))> {};

  template <bool Lines>
  struct RepeatExpr
      : rule<create<ast::RepeatExpr>(
            kw<"repeat"> + assign<&ast::RepeatExpr::body>(ref<Expr<Lines>>))> {
  };

  template <bool Lines>
  struct HelpExpr
      : rule<create<ast::HelpExpr>(
            lit<"?"> + assign<&ast::HelpExpr::operand>(ref<Expr<Lines>>))> {};

  struct Param
      : rule<(flag<&ast::Param::dots>(kw<"...">) |
              flag<&ast::Param::dot>(kw<".">) |
              assign<&ast::Param::name>(ref<ID> | ref<BACKTICK_ID>)) +
             option(lit<"="> + assign<&ast::Param::def>(ref<Expr<false>>))> {};

  template <bool Lines>
  struct FunctionDefExpr
      : rule<create<ast::FunctionDef>(
            (kw<"function"> | lit<"\\">)+lit<"("> +
            many(append<&ast::FunctionDef::params>(ref<Param>), lit<",">) +
            lit<")"> + assign<&ast::FunctionDef::body>(ref<Expr<Lines>>))> {};

  struct NextExpr : rule<create<ast::NextStmt>(kw<"next">)> {};
  struct BreakExpr : rule<create<ast::BreakStmt>(kw<"break">)> {};

  struct IdentExpr
      : rule<assign<&ast::Ident::name>(ref<ID>) |
             flag<&ast::Ident::quoted>(
                 assign<&ast::Ident::name>(ref<BACKTICK_ID>))> {};
  struct StringExpr
      : rule<assign<&ast::StringLit::text>(ref<STRING_LIT>) |
             flag<&ast::StringLit::raw>(
                 assign<&ast::StringLit::text>(ref<RAW_STRING>))> {};
  struct HexExpr : rule<assign<&ast::HexLit::text>(ref<HEX_LIT>)> {};
  struct IntExpr : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  struct FloatExpr : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {};
  struct ComplexExpr
      : rule<assign<&ast::ComplexLit::text>(ref<COMPLEX_LIT>)> {};

  struct NullExpr : rule<create<ast::NullLit>(kw<"NULL">)> {};
  struct NaExpr : rule<assign<&ast::NaLit::text>(
                          kw<"NA"> | kw<"NA_integer_"> | kw<"NA_real_"> |
                          kw<"NA_character_"> | kw<"NA_complex_">)> {};
  struct Placeholder : rule<create<ast::Placeholder>(lit<"_">)> {};
  struct InfExpr : rule<create<ast::InfLit>(kw<"Inf">)> {};
  struct NanExpr : rule<create<ast::NanLit>(kw<"NaN">)> {};
  struct TrueExpr : rule<create<ast::TrueLit>(kw<"TRUE">)> {};
  struct FalseExpr : rule<create<ast::FalseLit>(kw<"FALSE">)> {};

  template <bool Lines>
  struct Primary
      : rule<ref<ParenExpr> | ref<BlockExpr> | ref<IfExpr<Lines>>  //
             | ref<ForExpr<Lines>> | ref<WhileExpr<Lines>> |
             ref<RepeatExpr<Lines>>                                //
             | ref<HelpExpr<Lines>> | ref<FunctionDefExpr<Lines>>  //
             | ref<NextExpr> | ref<BreakExpr>                  //
             | ref<StringExpr> | ref<Placeholder> | ref<HexExpr> |
             ref<ComplexExpr>                     //
             | ref<FloatExpr> | ref<IntExpr>  //
             | ref<NullExpr> | ref<NaExpr> | ref<InfExpr> |
             ref<NanExpr> | ref<TrueExpr> | ref<FalseExpr>  //
             | ref<IdentExpr>> {};

  static constexpr auto ArgForms =
      (assign<&ast::Arg::name>(kw<"..."> | kw<"."> | ref<ID> |
                               ref<BACKTICK_ID> | ref<STRING_LIT> |
                               kw<"NULL">) +
       lit<"="> + option(assign<&ast::Arg::value>(ref<Expr<false>>))) |
      flag<&ast::Arg::dots>(kw<"...">) |
      flag<&ast::Arg::dot>(kw<".">);

  struct Arg
      : rule<ArgForms | option(assign<&ast::Arg::value>(ref<Expr<false>>))> {};
  struct FilledArg
      : rule<ArgForms | assign<&ast::Arg::value>(ref<Expr<false>>)> {};

  // A lone slot is never empty: `f()` holds no `Arg`, `f(,)` two.
  template <auto Member>
  static constexpr auto ArgListInto = option(
      append<Member>(ref<Arg>) + some(lit<","> + append<Member>(ref<Arg>))  //
      | append<Member>(ref<FilledArg>));

  template <bool Lines>
  struct PostfixExpr
      : rule<ref<Primary<Lines>> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                      Continue<Lines>(assign<&ast::BinaryExpr::op>(
                          NamespaceOp | ComponentOp)) +
                      assign<&ast::BinaryExpr::right>(ref<IdentExpr> |
                                                      ref<StringExpr>)) |
                  nest<ast::IndexExpr, &ast::IndexExpr::object>(
                      Continue<Lines>(
                          flag<&ast::IndexExpr::listAccess>(lit<"[[">)) +
                      ArgListInto<&ast::IndexExpr::args> + lit<"]"> +
                      lit<"]">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::object>(
                      Continue<Lines>(lit<"[">) +
                      ArgListInto<&ast::IndexExpr::args> + lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      Continue<Lines>(lit<"(">) +
                      ArgListInto<&ast::CallExpr::args> + lit<")">))> {};

  template <bool Lines>
  struct UnarySign;
  template <bool Lines>
  struct Power
      : rule<ref<PostfixExpr<Lines>> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 Continue<Lines>(assign<&ast::BinaryExpr::op>(ExponentOp)) +
                 assign<&ast::BinaryExpr::right>(ref<UnarySign<Lines>>)))> {};

  template <bool Lines, auto Op>
  struct InfixOp : rule<Continue<Lines>(Op)> {};

  template <bool Lines>
  struct UnarySign
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(SignOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnarySign<Lines>>))  //
             | ref<Power<Lines>>> {};

  template <bool Lines>
  struct ComparisonLevel
      : infix<UnarySign<Lines>, ast::BinaryExpr,
              levels(left(ref<InfixOp<Lines, RangeOp>>),
                     left(ref<InfixOp<Lines, lit<"=>">>>),
                     left(ref<InfixOp<Lines, PipeOp>>),
                     left(ref<InfixOp<Lines, MulDivOp>>),
                     left(ref<InfixOp<Lines, AddSubOp>>),
                     nonassoc(ref<InfixOp<Lines, ComparisonOp>>))> {};

  template <bool Lines>
  struct UnaryNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(NotOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryNot<Lines>>))  //
             | ref<ComparisonLevel<Lines>>> {};

  template <bool Lines>
  struct OrLevel : infix<UnaryNot<Lines>, ast::BinaryExpr,
                         levels(left(ref<InfixOp<Lines, AndOp>>),
                                left(ref<InfixOp<Lines, OrOp>>))> {};

  template <bool Lines>
  struct UnaryFormula
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(TildeOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryFormula<Lines>>))  //
             | ref<OrLevel<Lines>>> {};

  template <bool Lines>
  struct FormulaLevel : infix<UnaryFormula<Lines>, ast::BinaryExpr,
                              left(ref<InfixOp<Lines, TildeOp>>)> {};

  template <bool Lines>
  struct Assignment : infix<FormulaLevel<Lines>, ast::AssignExpr,
                            levels(left(ref<InfixOp<Lines, RightAssignOp>>),
                                   right(ref<InfixOp<Lines, LeftAssignOp>>),
                                   right(ref<InfixOp<Lines, EqAssignOp>>)),
                            &ast::AssignExpr::target, &ast::AssignExpr::op,
                            &ast::AssignExpr::value> {};

  template <bool Lines>
  struct Expr : infix<Assignment<Lines>, ast::BinaryExpr,
                      left(ref<InfixOp<Lines, lit<"?">>>)> {};

  struct Program
      : rule<create<ast::Program>(
            many(lit<";">) + many(append<&ast::Program::body>(ref<Expr<true>>) +
                                  Eos + many(lit<";">)))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace r
