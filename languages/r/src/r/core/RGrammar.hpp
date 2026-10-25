// The r language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "r/core/ast.hpp"

namespace r {

struct R : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto LETTER = uc::L;
  static constexpr auto word_boundary = LETTER | d | cr<"_.">;
  struct ID : terminal<(LETTER | (lit<"."> + !d)) +
                       many(LETTER | uc::Nd | cr<"_.">)> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto EXP = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto DECIMAL =
      (some(d) + option(lit<"."> + many(d)) | lit<"."> + some(d)) + option(EXP);
  static constexpr auto HEXADECIMAL =
      lit<"0"> + cr<"xX"> +
      (some(HEXDIGIT) + option(lit<"."> + many(HEXDIGIT)) |
       lit<"."> + some(HEXDIGIT)) +
      option(cr<"pP"> + option(cr<"+-">) + some(d));
  struct HEX_LIT : terminal<HEXADECIMAL> {};
  struct INT_LIT : terminal<(HEXADECIMAL | DECIMAL) + lit<"L">> {};
  struct FLOAT_LIT : terminal<DECIMAL> {};
  struct COMPLEX_LIT : terminal<(HEXADECIMAL | DECIMAL) + lit<"i">> {};

  static constexpr auto ESC = lit<"\\"> + any;
  inline static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  using Unquoted = text_conv<unquote>;
  struct BACKTICK_ID
      : terminal<lit<"`"> + many(ESC | (!cr<R"(\\`)"> + any)) + lit<"`">,
                 Unquoted> {};
  struct STRING_LIT
      : terminal<(lit<"\""> + many(ESC | (!cr<R"(\\")"> + any)) + lit<"\"">) |
                     (lit<"'"> + many(ESC | (!cr<R"(\\')"> + any)) + lit<"'">),
                 Unquoted> {};

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
      : terminal<cr<"rR"> + (RawString<lit<"\"">, lit<"(">, lit<")">> |
                             RawString<lit<"\"">, lit<"[">, lit<"]">> |
                             RawString<lit<"\"">, lit<"{">, lit<"}">> |
                             RawString<lit<"\"">, lit<"|">, lit<"|">> |
                             RawString<lit<"'">, lit<"(">, lit<")">> |
                             RawString<lit<"'">, lit<"[">, lit<"]">> |
                             RawString<lit<"'">, lit<"{">, lit<"}">> |
                             RawString<lit<"'">, lit<"|">, lit<"|">>),
                 text_conv<rawBody>> {};

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
  static constexpr auto EqAssignOp = lit<"=">;
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

  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expr<false>>) +
            lit<")">)> {};

  struct BlockExpr : rule<create<ast::Block>(
                         lit<"{"> + many(lit<";">) +
                         many(append<&ast::Block::body>(ref<Expr<true>>) + Eos +
                              many(lit<";">)) +
                         lit<"}">)> {};

  template <bool Lines>
  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expr<false>>) +
            lit<")"> + assign<&ast::IfExpr::thenBranch>(ref<Expr<Lines>>) +
            option(commit(kw<"else">, assign<&ast::IfExpr::elseBranch>(ref<Expr<Lines>>))))> {};

  template <bool Lines>
  struct ForExprRule
      : rule<create<ast::ForExpr>(
            kw<"for"> + lit<"("> + assign<&ast::ForExpr::var>(ref<ID>) +
            kw<"in"> + assign<&ast::ForExpr::seq>(ref<Expr<false>>) + lit<")"> +
            assign<&ast::ForExpr::body>(ref<Expr<Lines>>))> {};

  template <bool Lines>
  struct WhileExprRule
      : rule<create<ast::WhileExpr>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileExpr::cond>(ref<Expr<false>>) + lit<")"> +
            assign<&ast::WhileExpr::body>(ref<Expr<Lines>>))> {};

  template <bool Lines>
  struct RepeatExprRule
      : rule<create<ast::RepeatExpr>(
            kw<"repeat"> + assign<&ast::RepeatExpr::body>(ref<Expr<Lines>>))> {
  };

  template <bool Lines>
  struct HelpExprRule
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
            option(
                some(append<&ast::FunctionDef::params>(ref<Param>), lit<",">)) +
            lit<")"> + assign<&ast::FunctionDef::body>(ref<Expr<Lines>>))> {};

  struct NextExprRule : rule<create<ast::NextStmt>(kw<"next">)> {};
  struct BreakExprRule : rule<create<ast::BreakStmt>(kw<"break">)> {};

  struct IdentExprRule
      : rule<assign<&ast::Ident::name>(ref<ID>) |
             flag<&ast::Ident::quoted>(
                 assign<&ast::Ident::name>(ref<BACKTICK_ID>))> {};
  struct StringExprRule
      : rule<assign<&ast::StringLit::text>(ref<STRING_LIT>) |
             flag<&ast::StringLit::raw>(
                 assign<&ast::StringLit::text>(ref<RAW_STRING>))> {};
  struct HexExprRule : rule<assign<&ast::HexLit::text>(ref<HEX_LIT>)> {};
  struct IntExprRule : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  struct FloatExprRule : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {};
  struct ComplexExprRule
      : rule<assign<&ast::ComplexLit::text>(ref<COMPLEX_LIT>)> {};

  struct NullExprRule : rule<create<ast::NullLit>(kw<"NULL">)> {};
  struct NaExprRule : rule<assign<&ast::NaLit::text>(
                          kw<"NA"> | kw<"NA_integer_"> | kw<"NA_real_"> |
                          kw<"NA_character_"> | kw<"NA_complex_">)> {};
  struct PlaceholderRule : rule<create<ast::Placeholder>(lit<"_">)> {};
  struct InfExprRule : rule<create<ast::InfLit>(kw<"Inf">)> {};
  struct NanExprRule : rule<create<ast::NanLit>(kw<"NaN">)> {};
  struct TrueExprRule : rule<create<ast::TrueLit>(kw<"TRUE">)> {};
  struct FalseExprRule : rule<create<ast::FalseLit>(kw<"FALSE">)> {};

  template <bool Lines>
  struct Primary
      : rule<ref<ParenExprRule> | ref<BlockExpr> | ref<IfExprRule<Lines>>  //
             | ref<ForExprRule<Lines>> | ref<WhileExprRule<Lines>> |
             ref<RepeatExprRule<Lines>>                                //
             | ref<HelpExprRule<Lines>> | ref<FunctionDefExpr<Lines>>  //
             | ref<NextExprRule> | ref<BreakExprRule>                  //
             | ref<StringExprRule> | ref<PlaceholderRule> | ref<HexExprRule> |
             ref<ComplexExprRule>                     //
             | ref<FloatExprRule> | ref<IntExprRule>  //
             | ref<NullExprRule> | ref<NaExprRule> | ref<InfExprRule> |
             ref<NanExprRule> | ref<TrueExprRule> | ref<FalseExprRule>  //
             | ref<IdentExprRule>> {};

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
                      assign<&ast::BinaryExpr::right>(ref<IdentExprRule> |
                                                      ref<StringExprRule>)) |
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
  struct ComparisonLevel : infix<UnarySign<Lines>, &ast::BinaryExpr::left,
                                 &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                                 (left(ref<InfixOp<Lines, RangeOp>>) >>
                                  left(ref<InfixOp<Lines, lit<"=>">>>) >>
                                  left(ref<InfixOp<Lines, PipeOp>>) >>
                                  left(ref<InfixOp<Lines, MulDivOp>>) >>
                                  left(ref<InfixOp<Lines, AddSubOp>>) >>
                                  left(ref<InfixOp<Lines, ComparisonOp>>))> {};

  template <bool Lines>
  struct UnaryNot
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(NotOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryNot<Lines>>))  //
             | ref<ComparisonLevel<Lines>>> {};

  template <bool Lines>
  struct OrLevel : infix<UnaryNot<Lines>, &ast::BinaryExpr::left,
                         &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                         (left(ref<InfixOp<Lines, AndOp>>) >>
                          left(ref<InfixOp<Lines, OrOp>>))> {};

  template <bool Lines>
  struct UnaryFormula
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(TildeOp) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryFormula<Lines>>))  //
             | ref<OrLevel<Lines>>> {};

  template <bool Lines>
  struct FormulaLevel
      : infix<UnaryFormula<Lines>, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<InfixOp<Lines, TildeOp>>)> {};

  template <bool Lines>
  struct Assignment : infix<FormulaLevel<Lines>, &ast::AssignExpr::target,
                            &ast::AssignExpr::op, &ast::AssignExpr::value,
                            (left(ref<InfixOp<Lines, RightAssignOp>>) >>
                             right(ref<InfixOp<Lines, LeftAssignOp>>) >>
                             right(ref<InfixOp<Lines, EqAssignOp>>))> {};

  template <bool Lines>
  struct Expr
      : infix<Assignment<Lines>, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<InfixOp<Lines, lit<"?">>>)> {};

  struct Program
      : rule<create<ast::Program>(
            many(lit<";">) + many(append<&ast::Program::body>(ref<Expr<true>>) +
                                  Eos + many(lit<";">)))> {};

  using entry = Program;
  using ignore = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace r
