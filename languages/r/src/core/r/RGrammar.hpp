// The r language: its grammar.
#pragma once

#include "cursiv/grammar/GrammarSpec.hpp"
#include "r/ast.hpp"
#include "r/LiteralText.hpp"

namespace r {

struct R : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  // A line end is the token `NEWLINE` where a rule reads it, a blank elsewhere.
  struct NEWLINE : terminal<eol> {};
  struct NL_TRIVIA : terminal<eol> {};
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

  // A statement ends at `;` or at a line end.
  static constexpr auto Sep = ref<NEWLINE> | lit<";">;
  static constexpr auto NLs = many(ref<NEWLINE>);

  // Inside `( )` and `[ ]` line ends are blanks; inside `{ }` they count again.
  // `[[` is two openers, as R reads it, and `]]` two closers.
  static constexpr auto indent_open = lit<"("> | lit<"[">;
  static constexpr auto indent_block = lit<"{">;
  static constexpr auto indent_close = lit<")"> | lit<"]"> | lit<"}">;
  using indent_newline = NEWLINE;

  struct Expr;
  struct Assignment;

  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expr>) +
            lit<")">)> {};

  struct BlockExpr : rule<create<ast::Block>(
                         lit<"{"> + many(Sep) +
                         many(append<&ast::Block::body>(ref<Expr>) +
                              (some(Sep) | &lit<"}">)) +
                         lit<"}">)> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expr>) +
            lit<")"> + assign<&ast::IfExpr::thenBranch>(ref<Assignment>) +
            option(commit(NLs + kw<"else">, assign<&ast::IfExpr::elseBranch>(ref<Assignment>))))> {};

  struct ForExpr
      : rule<create<ast::ForExpr>(
            kw<"for"> + lit<"("> + assign<&ast::ForExpr::var>(ref<ID> | ref<BACKTICK_ID>) +
            kw<"in"> + assign<&ast::ForExpr::seq>(ref<Expr>) + lit<")"> +
            assign<&ast::ForExpr::body>(ref<Assignment>))> {};

  struct WhileExpr
      : rule<create<ast::WhileExpr>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileExpr::cond>(ref<Expr>) + lit<")"> +
            assign<&ast::WhileExpr::body>(ref<Assignment>))> {};

  struct RepeatExpr
      : rule<create<ast::RepeatExpr>(
            kw<"repeat"> + assign<&ast::RepeatExpr::body>(ref<Assignment>))> {
  };

  struct HelpExpr
      : rule<create<ast::HelpExpr>(
            lit<"?"> + assign<&ast::HelpExpr::operand>(ref<Assignment>))> {};

  struct Param
      : rule<(flag<&ast::Param::dots>(kw<"...">) |
              flag<&ast::Param::dot>(kw<".">) |
              assign<&ast::Param::name>(ref<ID> | ref<BACKTICK_ID>)) +
             option(lit<"="> + assign<&ast::Param::def>(ref<Expr>))> {};

  struct FunctionDefExpr
      : rule<create<ast::FunctionDef>(
            (kw<"function"> | lit<"\\">)+lit<"("> +
            many(append<&ast::FunctionDef::params>(ref<Param>), lit<",">) +
            lit<")"> + assign<&ast::FunctionDef::body>(ref<Assignment>))> {};

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

  // Constructs that end in an expression read as far as they can: a postfix
  // operator after one belongs to that expression.
  struct OpenPrimary
      : rule<ref<IfExpr> | ref<ForExpr> | ref<WhileExpr> | ref<RepeatExpr> |
             ref<HelpExpr> | ref<FunctionDefExpr>> {};

  struct Primary
      : rule<ref<ParenExpr> | ref<BlockExpr>  //
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
       lit<"="> + option(assign<&ast::Arg::value>(ref<Expr>))) |
      flag<&ast::Arg::dots>(kw<"...">) |
      flag<&ast::Arg::dot>(kw<".">);

  struct Arg
      : rule<ArgForms | option(assign<&ast::Arg::value>(ref<Expr>))> {};
  struct FilledArg
      : rule<ArgForms | assign<&ast::Arg::value>(ref<Expr>)> {};

  // A lone slot is never empty: `f()` holds no `Arg`, `f(,)` two.
  template <auto Member>
  static constexpr auto ArgListInto = option(
      append<Member>(ref<Arg>) + some(lit<","> + append<Member>(ref<Arg>))  //
      | append<Member>(ref<FilledArg>));

  struct PostfixExpr
      : rule<ref<OpenPrimary> |
             (ref<Primary> +
              many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                       assign<&ast::BinaryExpr::op>(NamespaceOp | ComponentOp) +
                       assign<&ast::BinaryExpr::right>(ref<IdentExpr> |
                                                       ref<StringExpr>)) |
                   nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> + gap::none +
                       flag<&ast::IndexExpr::listAccess>(lit<"[">) +
                       ArgListInto<&ast::IndexExpr::args> + lit<"]"> +
                       lit<"]">) |
                   nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> + ArgListInto<&ast::IndexExpr::args> + lit<"]">) |
                   nest<ast::CallExpr, &ast::CallExpr::callee>(
                       lit<"("> + ArgListInto<&ast::CallExpr::args> + lit<")">)))> {};

  template <auto Op>
  struct InfixOp : rule<Op> {};

  // One table, tightest first: `^` (right), the sign prefix, `:`, `=>`,
  // `%op%` and `|>`, `*` `/`, `+` `-`, the comparisons, `!`, `&` `&&`,
  // `|` `||`, the formula prefix `~`, the binary `~`.
  struct FormulaLevel
      : infix<PostfixExpr, ast::BinaryExpr,
              levels(right(ExponentOp),
                     prefix<ast::UnaryExpr>(SignOp),
                     left(ref<InfixOp<RangeOp>>),
                     left(ref<InfixOp<lit<"=>">>>),
                     left(ref<InfixOp<PipeOp>>),
                     left(ref<InfixOp<MulDivOp>>),
                     left(ref<InfixOp<AddSubOp>>),
                     nonassoc(ref<InfixOp<ComparisonOp>>),
                     prefix<ast::UnaryExpr>(NotOp),
                     left(ref<InfixOp<AndOp>>),
                     left(ref<InfixOp<OrOp>>),
                     prefix<ast::UnaryExpr>(TildeOp),
                     left(ref<InfixOp<TildeOp>>))> {};

  struct Assignment : infix<FormulaLevel, ast::AssignExpr,
                            levels(left(ref<InfixOp<RightAssignOp>>),
                                   right(ref<InfixOp<LeftAssignOp>>),
                                   right(ref<InfixOp<EqAssignOp>>)),
                            &ast::AssignExpr::target, &ast::AssignExpr::op,
                            &ast::AssignExpr::value> {};

  struct Expr : infix<Assignment, ast::BinaryExpr,
                      left(ref<InfixOp<lit<"?">>>)> {};

  struct Program
      : rule<create<ast::Program>(
            many(Sep) + many(append<&ast::Program::body>(ref<Expr>) +
                             (some(Sep) | eof)))> {};

  using entry = Program;
  using ignored = list<WS, NL_TRIVIA>;
  using hidden = list<SL_COMMENT>;
};

}  // namespace r
