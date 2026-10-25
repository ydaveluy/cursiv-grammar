// The basic language: its grammar. Lines have no significant terminator --
// whitespace, including newlines, is hidden throughout, and a line boundary
// is simply wherever the next line number can start.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "basic/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace basic {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource*) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto drop_rem = [](std::string_view t,
                                    std::pmr::memory_resource*) {
  return t.substr(3);
};
using AfterRem = cursiv::text_conv<drop_rem>;

struct Basic : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct REM : terminal<ikw<"REM">> {};
  // A comment's introducer and its text are one token, not a keyword
  // followed by a run: `ikw<"REM">` would refuse text glued right against
  // the word (no space). At least one trailing byte, so an empty comment
  // ties with REM on length, and REM -- declared as its own terminal --
  // wins that tie.
  static constexpr auto RemWord = cr<"Rr"> + cr<"Ee"> + cr<"Mm">;
  struct COMMENT : terminal<RemWord + some(cr<R"(^\r\n)">), AfterRem> {};

  struct STRINGLITERAL
      : terminal<lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">, Unquoted> {};

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  struct LINENUMBER : terminal<some(d), std::int64_t> {};
  struct NUMBER : terminal<some(d) + many(cr<"Ee"> + some(d)), double> {};
  struct FLOAT
      : terminal<many(d) + lit<"."> + some(d) + many(cr<"Ee"> + some(d)),
                double> {};

  struct DOLLAR : terminal<lit<"$">, value<ast::VarSuffix::dollar>> {};
  struct PERCENT : terminal<lit<"%">, value<ast::VarSuffix::percent>> {};
  static constexpr auto Suffix = ref<DOLLAR> | ref<PERCENT>;

  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};
  struct COMMA : terminal<lit<",">> {};
  struct COLONTOK : terminal<lit<":">> {};
  struct SEMICOLON : terminal<lit<";">> {};
  struct EQTOK : terminal<lit<"=">> {};
  struct GTTOK : terminal<lit<">">> {};
  struct LTTOK : terminal<lit<"<">> {};

  struct POW : terminal<lit<"^">, value<ast::Op::pow>> {};
  struct MUL : terminal<lit<"*">, value<ast::Op::mul>> {};
  struct DIV_ : terminal<lit<"/">, value<ast::Op::div_>> {};
  struct ADD : terminal<lit<"+">, value<ast::Op::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::Op::sub>> {};
  struct ANDOP : terminal<ikw<"AND">, value<ast::Op::and_>> {};
  struct OROP : terminal<ikw<"OR">, value<ast::Op::or_>> {};

  // GE/LE/NE combine two adjacent single-char tokens, or a vestigial
  // three-byte spelling (`>: `, `<: `, a literal space before the digits)
  // some BASIC dialects still accept.
  struct GE
      : rule<(ref<GTTOK> + ref<EQTOK>) | (ref<EQTOK> + ref<GTTOK>) |
                 lit<">: ">,
             value<ast::Op::ge>> {};
  struct LE
      : rule<(ref<LTTOK> + ref<EQTOK>) | (ref<EQTOK> + ref<LTTOK>) |
                 lit<"<: ">,
             value<ast::Op::le>> {};
  struct NE : rule<ref<LTTOK> + ref<GTTOK>, value<ast::Op::ne>> {};
  struct EQOP : rule<ref<EQTOK>, value<ast::Op::eq>> {};
  struct GTOP : rule<ref<GTTOK>, value<ast::Op::gt>> {};
  struct LTOP : rule<ref<LTTOK>, value<ast::Op::lt>> {};

  static constexpr auto OpPow = ref<POW>;
  static constexpr auto OpMulDiv = ref<MUL> | ref<DIV_>;
  static constexpr auto OpAddSub = ref<ADD> | ref<SUB>;
  static constexpr auto OpCompare =
      ref<GE> | ref<LE> | ref<NE> | ref<EQOP> | ref<GTOP> | ref<LTOP>;
  static constexpr auto OpAndOr = ref<ANDOP> | ref<OROP>;

  // Builtin functions: one keyword, one to three parenthesised arguments,
  // and no structure beyond that -- unified rather than one struct each.
  struct CHR : terminal<ikw<"CHR$">, value<ast::BuiltinFunc::chr>> {};
  struct SQR : terminal<ikw<"SQR">, value<ast::BuiltinFunc::sqr>> {};
  struct LEN : terminal<ikw<"LEN">, value<ast::BuiltinFunc::len>> {};
  struct STR : terminal<ikw<"STR$">, value<ast::BuiltinFunc::str>> {};
  struct ASC : terminal<ikw<"ASC">, value<ast::BuiltinFunc::asc>> {};
  struct SCRN : terminal<ikw<"SCRN">, value<ast::BuiltinFunc::scrn>> {};
  struct MID : terminal<ikw<"MID$">, value<ast::BuiltinFunc::mid>> {};
  struct PDL : terminal<ikw<"PDL">, value<ast::BuiltinFunc::pdl>> {};
  struct PEEK : terminal<ikw<"PEEK">, value<ast::BuiltinFunc::peek>> {};
  struct INTF : terminal<ikw<"INT">, value<ast::BuiltinFunc::intf>> {};
  struct SPC : terminal<ikw<"SPC">, value<ast::BuiltinFunc::spc>> {};
  struct FRE : terminal<ikw<"FRE">, value<ast::BuiltinFunc::fre>> {};
  struct POS : terminal<ikw<"POS">, value<ast::BuiltinFunc::pos>> {};
  struct USR : terminal<ikw<"USR">, value<ast::BuiltinFunc::usr>> {};
  struct LEFT : terminal<ikw<"LEFT$">, value<ast::BuiltinFunc::left>> {};
  struct VAL : terminal<ikw<"VAL">, value<ast::BuiltinFunc::val>> {};
  struct RIGHT : terminal<ikw<"RIGHT$">, value<ast::BuiltinFunc::right>> {};
  struct SIN : terminal<ikw<"SIN">, value<ast::BuiltinFunc::sin>> {};
  struct COS : terminal<ikw<"COS">, value<ast::BuiltinFunc::cos>> {};
  struct TAN : terminal<ikw<"TAN">, value<ast::BuiltinFunc::tan>> {};
  struct ATN : terminal<ikw<"ATN">, value<ast::BuiltinFunc::atn>> {};
  struct RND : terminal<ikw<"RND">, value<ast::BuiltinFunc::rnd>> {};
  struct SGN : terminal<ikw<"SGN">, value<ast::BuiltinFunc::sgn>> {};
  struct EXPF : terminal<ikw<"EXP">, value<ast::BuiltinFunc::exp>> {};
  struct LOG : terminal<ikw<"LOG">, value<ast::BuiltinFunc::log>> {};
  struct ABS : terminal<ikw<"ABS">, value<ast::BuiltinFunc::abs>> {};
  struct TABF : terminal<ikw<"TAB">, value<ast::BuiltinFunc::tabf>> {};

  static constexpr auto Func1 =
      ref<SQR> | ref<LEN> | ref<ASC> | ref<PDL> | ref<PEEK> | ref<INTF> |
      ref<SPC> | ref<FRE> | ref<POS> | ref<USR> | ref<VAL> | ref<SIN> |
      ref<COS> | ref<TAN> | ref<ATN> | ref<RND> | ref<SGN> | ref<EXPF> |
      ref<LOG> | ref<ABS> | ref<TABF> | ref<CHR> | ref<STR>;
  static constexpr auto Func2 = ref<LEFT> | ref<RIGHT> | ref<SCRN>;

  struct Expression;
  struct Statement;

  struct SubscriptRule
      : rule<create<ast::Subscript>(
            ref<LPAREN> +
            some(append<&ast::Subscript::args>(ref<Expression>), ref<COMMA>) +
            ref<RPAREN>)> {};

  struct VarRefRule
      : rule<create<ast::VarRef>(
            assign<&ast::VarRef::name>(ref<IDENT>) +
            option(assign<&ast::VarRef::suffix>(Suffix)) +
            many(append<&ast::VarRef::subscripts>(ref<SubscriptRule>)))> {};

  struct NumberLitRule
      : rule<create<ast::NumberLit>(
            option(flag<&ast::NumberLit::negative>(lit<"-">) |
                   lit<"+">) +
            assign<&ast::NumberLit::value>(ref<NUMBER> | ref<FLOAT>))> {};

  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRINGLITERAL>))> {};

  struct FuncCallRule
      : rule<create<ast::FuncCall>(
                 assign<&ast::FuncCall::kind>(Func1) + ref<LPAREN> +
                 append<&ast::FuncCall::args>(ref<Expression>) +
                 ref<RPAREN>) |
             create<ast::FuncCall>(
                 assign<&ast::FuncCall::kind>(Func2) + ref<LPAREN> +
                 append<&ast::FuncCall::args>(ref<Expression>) + ref<COMMA> +
                 append<&ast::FuncCall::args>(ref<Expression>) +
                 ref<RPAREN>) |
             create<ast::FuncCall>(
                 assign<&ast::FuncCall::kind>(ref<MID>) + ref<LPAREN> +
                 append<&ast::FuncCall::args>(ref<Expression>) + ref<COMMA> +
                 append<&ast::FuncCall::args>(ref<Expression>) + ref<COMMA> +
                 append<&ast::FuncCall::args>(ref<Expression>) +
                 ref<RPAREN>)> {};

  struct FN : terminal<ikw<"FN">> {};
  struct FnCallRule
      : rule<create<ast::FnCall>(
            ref<FN> + assign<&ast::FnCall::name>(ref<IDENT>) +
            option(assign<&ast::FnCall::suffix>(Suffix)) + ref<LPAREN> +
            assign<&ast::FnCall::arg>(ref<Expression>) + ref<RPAREN>)> {};

  struct Prim
      : rule<ref<StringLitRule> | ref<NumberLitRule> | ref<FnCallRule> |
             ref<FuncCallRule> | ref<VarRefRule> |
             (ref<LPAREN> + ref<Expression> + ref<RPAREN>)> {};

  struct NOTKW : terminal<ikw<"NOT">> {};
  // Each prefix wraps a `UnaryExpr` only where it is actually written, so a
  // plain primary reads back as itself rather than through an empty wrapper.
  struct SignPrim
      : rule<create<ast::UnaryExpr>(
                 flag<&ast::UnaryExpr::negative>(lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Prim>)) |
             (lit<"+"> + ref<Prim>) | ref<Prim>> {};
  struct SignedPrim
      : rule<create<ast::UnaryExpr>(
                 flag<&ast::UnaryExpr::logicalNot>(ref<NOTKW>) +
                 assign<&ast::UnaryExpr::operand>(ref<SignPrim>)) |
             ref<SignPrim>> {};

  struct Expression
      : infix<SignedPrim, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpPow) >>      //
               left(OpMulDiv) >>   //
               left(OpAddSub) >>   //
               left(OpCompare) >>  //
               left(OpAndOr))> {};

  struct DatumRule : rule<ref<NumberLitRule> | ref<StringLitRule>> {};

  // Statement keywords with no value payload of their own.
  struct PRINTKW : terminal<ikw<"PRINT">> {};
  struct QUESTIONKW : terminal<lit<"?">> {};
  struct GOTOKW : terminal<ikw<"GOTO">> {};
  struct GOSUBKW : terminal<ikw<"GOSUB">> {};
  struct IFKW : terminal<ikw<"IF">> {};
  struct NEXTKW : terminal<ikw<"NEXT">> {};
  struct THENKW : terminal<ikw<"THEN">> {};
  struct LETKW : terminal<ikw<"LET">> {};
  struct FORKW : terminal<ikw<"FOR">> {};
  struct TOKW : terminal<ikw<"TO">> {};
  struct STEPKW : terminal<ikw<"STEP">> {};
  struct INPUTKW : terminal<ikw<"INPUT">> {};
  struct DIMKW : terminal<ikw<"DIM">> {};
  struct ONKW : terminal<ikw<"ON">> {};
  struct ONERRKW : terminal<ikw<"ONERR">> {};
  struct DATAKW : terminal<ikw<"DATA">> {};
  struct READKW : terminal<ikw<"READ">> {};
  struct ATKW : terminal<ikw<"AT">> {};
  struct DEFKW : terminal<ikw<"DEF">> {};
  struct STOREKW : terminal<ikw<"STORE">> {};
  struct RECALLKW : terminal<ikw<"RECALL">> {};
  struct HPLOTKW : terminal<ikw<"HPLOT">> {};
  struct VPLOTKW : terminal<ikw<"VPLOT">> {};
  struct AMPERSAND : terminal<lit<"&">> {};

  // Zero-argument statements: one keyword, each with its own SimpleKind.
  struct CLSKW : terminal<ikw<"CLS">, value<ast::SimpleKind::cls>> {};
  struct LOADKW : terminal<ikw<"LOAD">, value<ast::SimpleKind::load>> {};
  struct SAVEKW : terminal<ikw<"SAVE">, value<ast::SimpleKind::save>> {};
  struct TRACEKW : terminal<ikw<"TRACE">, value<ast::SimpleKind::trace>> {};
  struct NOTRACEKW
      : terminal<ikw<"NOTRACE">, value<ast::SimpleKind::notrace>> {};
  struct FLASHKW : terminal<ikw<"FLASH">, value<ast::SimpleKind::flash>> {};
  struct INVERSEKW
      : terminal<ikw<"INVERSE">, value<ast::SimpleKind::inverse>> {};
  struct GRKW : terminal<ikw<"GR">, value<ast::SimpleKind::gr>> {};
  struct NORMALKW : terminal<ikw<"NORMAL">, value<ast::SimpleKind::normal>> {};
  struct SHLOADKW : terminal<ikw<"SHLOAD">, value<ast::SimpleKind::shload>> {};
  struct CLEARKW : terminal<ikw<"CLEAR">, value<ast::SimpleKind::clear>> {};
  struct RUNKW : terminal<ikw<"RUN">, value<ast::SimpleKind::run>> {};
  struct STOPKW : terminal<ikw<"STOP">, value<ast::SimpleKind::stop>> {};
  struct TEXTKW : terminal<ikw<"TEXT">, value<ast::SimpleKind::text>> {};
  struct HOMEKW : terminal<ikw<"HOME">, value<ast::SimpleKind::home>> {};
  struct HGR2KW : terminal<ikw<"HGR2">, value<ast::SimpleKind::hgr2>> {};
  struct HGRKW : terminal<ikw<"HGR">, value<ast::SimpleKind::hgr>> {};
  struct ENDKW : terminal<ikw<"END">, value<ast::SimpleKind::end_>> {};
  struct RETURNKW
      : terminal<ikw<"RETURN">, value<ast::SimpleKind::return_>> {};
  struct RESTOREKW
      : terminal<ikw<"RESTORE">, value<ast::SimpleKind::restore>> {};

  static constexpr auto SimpleKw =
      ref<CLSKW> | ref<LOADKW> | ref<SAVEKW> | ref<TRACEKW> |
      ref<NOTRACEKW> | ref<FLASHKW> | ref<INVERSEKW> | ref<GRKW> |
      ref<NORMALKW> | ref<SHLOADKW> | ref<CLEARKW> | ref<RUNKW> |
      ref<STOPKW> | ref<TEXTKW> | ref<HOMEKW> | ref<HGR2KW> | ref<HGRKW> |
      ref<ENDKW> | ref<RETURNKW> | ref<RESTOREKW>;

  struct SimpleStmtRule
      : rule<create<ast::SimpleStmt>(assign<&ast::SimpleStmt::kind>(SimpleKw))> {
  };

  // Statements that are one keyword and a small run of expressions.
  struct PLOTKW : terminal<ikw<"PLOT">, value<ast::ArgKind::plot>> {};
  struct POKEKW : terminal<ikw<"POKE">, value<ast::ArgKind::poke>> {};
  struct VTABKW : terminal<ikw<"VTAB">, value<ast::ArgKind::vtab>> {};
  struct HTABKW : terminal<ikw<"HTAB">, value<ast::ArgKind::htab>> {};
  struct HIMEMKW : terminal<ikw<"HIMEM">, value<ast::ArgKind::himem>> {};
  struct LOMEMKW : terminal<ikw<"LOMEM">, value<ast::ArgKind::lomem>> {};
  struct TABSTMTKW : terminal<ikw<"TAB">, value<ast::ArgKind::tab_>> {};
  struct SPEEDKW : terminal<ikw<"SPEED">, value<ast::ArgKind::speed>> {};
  struct ROTKW : terminal<ikw<"ROT">, value<ast::ArgKind::rot>> {};
  struct SCALEKW : terminal<ikw<"SCALE">, value<ast::ArgKind::scale>> {};
  struct COLORKW : terminal<ikw<"COLOR">, value<ast::ArgKind::color>> {};
  struct HCOLORKW : terminal<ikw<"HCOLOR">, value<ast::ArgKind::hcolor>> {};
  struct HLINKW : terminal<ikw<"HLIN">, value<ast::ArgKind::hlin>> {};
  struct VLINKW : terminal<ikw<"VLIN">, value<ast::ArgKind::vlin>> {};
  struct INCLUDEKW
      : terminal<ikw<"INCLUDE">, value<ast::ArgKind::include_>> {};
  struct GETKW : terminal<ikw<"GET">, value<ast::ArgKind::get>> {};
  struct CALLKW : terminal<ikw<"CALL">, value<ast::ArgKind::call>> {};
  struct WAITKW : terminal<ikw<"WAIT">, value<ast::ArgKind::wait>> {};
  struct XDRAWKW : terminal<ikw<"XDRAW">, value<ast::ArgKind::xdraw>> {};
  struct DRAWKW : terminal<ikw<"DRAW">, value<ast::ArgKind::draw>> {};
  struct LISTKW : terminal<ikw<"LIST">, value<ast::ArgKind::list_>> {};
  struct POPKW : terminal<ikw<"POP">, value<ast::ArgKind::pop_>> {};
  struct AMPTKW : terminal<lit<"&">, value<ast::ArgKind::ampersand>> {};
  // `PR#` / `IN#` end on `#`, not a word-class byte, so `ikw<>` states no
  // boundary and reads straight into the digit that follows (`PR#1`).
  struct PRNUMBERKW : terminal<ikw<"PR#">, value<ast::ArgKind::prnumber>> {};
  struct INNUMBERKW : terminal<ikw<"IN#">, value<ast::ArgKind::innumber>> {};

  static constexpr auto Arg1 =
      ref<VTABKW> | ref<HTABKW> | ref<INCLUDEKW>;

  struct ArgStmtRule
      : rule<
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<PLOTKW> | ref<POKEKW>) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(Arg1) +
                                 append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<HIMEMKW> | ref<LOMEMKW>) +
                ref<COLONTOK> + append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(ref<TABSTMTKW>) +
                                 ref<LPAREN> +
                                 append<&ast::ArgStmt::args>(ref<Expression>) +
                                 ref<RPAREN>) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(
                    ref<SPEEDKW> | ref<ROTKW> | ref<SCALEKW> | ref<COLORKW> |
                    ref<HCOLORKW>) +
                ref<EQTOK> + append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<HLINKW> | ref<VLINKW>) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<ATKW> +
                append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<PRNUMBERKW> | ref<INNUMBERKW>) +
                append<&ast::ArgStmt::args>(ref<NumberLitRule>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(ref<AMPTKW>) +
                                 append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<GETKW> | ref<CALLKW>) +
                some(append<&ast::ArgStmt::args>(ref<Expression>), ref<COMMA>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<WAITKW>) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>) +
                option(ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<XDRAWKW> | ref<DRAWKW>) +
                append<&ast::ArgStmt::args>(ref<Expression>) +
                option(ref<ATKW> +
                       append<&ast::ArgStmt::args>(ref<Expression>) +
                       ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<LISTKW>) +
                option(append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(ref<POPKW>) +
                option(append<&ast::ArgStmt::args>(ref<Expression>) +
                       ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>)))> {};

  struct PlotPathRule
      : rule<create<ast::PlotPath>(
            (flag<&ast::PlotPath::vertical>(ref<VPLOTKW>) |
             ref<HPLOTKW>) +
            option(append<&ast::PlotPath::coords>(ref<Expression>) +
                   ref<COMMA> +
                   append<&ast::PlotPath::coords>(ref<Expression>)) +
            many(ref<TOKW> + append<&ast::PlotPath::coords>(ref<Expression>) +
                 ref<COMMA> +
                 append<&ast::PlotPath::coords>(ref<Expression>)))> {};

  struct CommentStmtRule
      : rule<create<ast::Comment>(
                 assign<&ast::Comment::text>(ref<COMMENT>)) |
             create<ast::Comment>(ref<REM>)> {};

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            option(flag<&ast::AssignStmt::explicitLet>(ref<LETKW>)) +
            assign<&ast::AssignStmt::target>(ref<VarRefRule>) + ref<EQTOK> +
            some(append<&ast::AssignStmt::values>(ref<Expression>),
                 ref<COMMA>))> {};

  struct PrintStmtRule
      : rule<create<ast::PrintStmt>(
            (ref<PRINTKW> | ref<QUESTIONKW>) +append<&ast::PrintStmt::items>(
                create<ast::PrintItem>(
                    option(assign<&ast::PrintItem::value>(ref<Expression>)))) +
            many(append<&ast::PrintStmt::items>(create<ast::PrintItem>(
                assign<&ast::PrintItem::separator>(ref<COMMA> | ref<SEMICOLON>) +
                        option(assign<&ast::PrintItem::value>(
                            ref<Expression>))))))> {};

  struct InputStmtRule
      : rule<create<ast::InputStmt>(
            ref<INPUTKW> +
            option(assign<&ast::InputStmt::prompt>(ref<STRINGLITERAL>) +
                   (ref<COMMA> | ref<SEMICOLON>)) +
            some(append<&ast::InputStmt::vars>(ref<VarRefRule>), ref<COMMA>))> {
  };

  struct ReadStmtRule
      : rule<create<ast::ReadStmt>(
            ref<READKW> +
            some(append<&ast::ReadStmt::vars>(ref<VarRefRule>), ref<COMMA>))> {
  };

  struct DimStmtRule
      : rule<create<ast::DimStmt>(
            ref<DIMKW> +
            some(append<&ast::DimStmt::vars>(ref<VarRefRule>), ref<COMMA>))> {};

  struct StoreStmtRule
      : rule<create<ast::StoreStmt>(
            ref<STOREKW> + assign<&ast::StoreStmt::var>(ref<VarRefRule>))> {};

  struct RecallStmtRule
      : rule<create<ast::RecallStmt>(
            ref<RECALLKW> +
            assign<&ast::RecallStmt::var>(ref<VarRefRule>))> {};

  struct DataStmtRule
      : rule<create<ast::DataStmt>(
            ref<DATAKW> + append<&ast::DataStmt::values>(ref<DatumRule>) +
            many(ref<COMMA> +
                 option(append<&ast::DataStmt::values>(ref<DatumRule>))))> {};

  struct DefStmtRule
      : rule<create<ast::DefStmt>(
            ref<DEFKW> +
            option(flag<&ast::DefStmt::explicitFn>(ref<FN>)) +
            assign<&ast::DefStmt::name>(ref<IDENT>) +
            option(assign<&ast::DefStmt::nameSuffix>(Suffix)) + ref<LPAREN> +
            assign<&ast::DefStmt::param>(ref<IDENT>) +
            option(assign<&ast::DefStmt::paramSuffix>(Suffix)) + ref<RPAREN> +
            ref<EQTOK> + assign<&ast::DefStmt::body>(ref<Expression>))> {};

  struct OnErrStmtRule
      : rule<create<ast::OnErrStmt>(
            ref<ONERRKW> + ref<GOTOKW> +
            assign<&ast::OnErrStmt::target>(ref<LINENUMBER>))> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            ref<GOTOKW> + assign<&ast::GotoStmt::target>(ref<LINENUMBER>))> {};

  struct GosubStmtRule
      : rule<create<ast::GosubStmt>(
            ref<GOSUBKW> +
            assign<&ast::GosubStmt::target>(ref<Expression>))> {};

  struct OnGotoStmtRule
      : rule<create<ast::OnGotoStmt>(
            ref<ONKW> + assign<&ast::OnGotoStmt::selector>(ref<Expression>) +
            ref<GOTOKW> +
            some(append<&ast::OnGotoStmt::targets>(ref<LINENUMBER>),
                 ref<COMMA>))> {};

  struct OnGosubStmtRule
      : rule<create<ast::OnGosubStmt>(
            ref<ONKW> + assign<&ast::OnGosubStmt::selector>(ref<Expression>) +
            ref<GOSUBKW> +
            some(append<&ast::OnGosubStmt::targets>(ref<LINENUMBER>),
                 ref<COMMA>))> {};

  // The optional trailing `statement NEXT var?` covers every document a
  // bare `FOR ... TO ...` header alone would: the shorter form is dropped
  // as redundant, never a shorter match.
  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            ref<FORKW> + assign<&ast::ForStmt::var>(ref<VarRefRule>) +
            ref<EQTOK> + assign<&ast::ForStmt::from>(ref<Expression>) +
            ref<TOKW> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(ref<STEPKW> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            option(assign<&ast::ForStmt::body>(ref<Statement>) +
                   ref<NEXTKW> +
                   option(assign<&ast::ForStmt::nextVar>(ref<VarRefRule>))))> {
  };

  struct NextStmtRule
      : rule<create<ast::NextStmt>(
            ref<NEXTKW> +
            option(some(append<&ast::NextStmt::vars>(ref<VarRefRule>),
                        ref<COMMA>)))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            ref<IFKW> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            option(ref<THENKW>) +
            (assign<&ast::IfStmt::thenStmt>(ref<Statement>) |
             assign<&ast::IfStmt::thenLine>(ref<LINENUMBER>)))> {};

  struct Statement
      : rule<ref<PrintStmtRule> | ref<AssignStmtRule> | ref<IfStmtRule> |
             ref<ForStmtRule> | ref<NextStmtRule> | ref<GotoStmtRule> |
             ref<GosubStmtRule> | ref<OnGotoStmtRule> | ref<OnGosubStmtRule> |
             ref<OnErrStmtRule> | ref<InputStmtRule> | ref<ReadStmtRule> |
             ref<DimStmtRule> | ref<StoreStmtRule> | ref<RecallStmtRule> |
             ref<DataStmtRule> | ref<DefStmtRule> | ref<PlotPathRule> |
             ref<ArgStmtRule> | ref<SimpleStmtRule>> {};

  struct CommentItem
      : rule<create<ast::StmtItem>(
            assign<&ast::StmtItem::stmt>(ref<CommentStmtRule>))> {};

  struct AmprStmt
      : rule<create<ast::StmtItem>(
                 option(flag<&ast::StmtItem::ampersandPrefix>(
                     ref<AMPERSAND>)) +
                 assign<&ast::StmtItem::stmt>(ref<Statement>)) |
             ref<CommentItem>> {};

  struct LineRule
      : rule<create<ast::Line>(
            assign<&ast::Line::number>(ref<LINENUMBER>) +
            ((append<&ast::Line::items>(ref<AmprStmt>) +
              many(ref<COLONTOK> +
                   option(append<&ast::Line::items>(ref<AmprStmt>)))) |
             append<&ast::Line::items>(ref<CommentItem>)))> {};

  struct ProgramRule
      : rule<create<ast::Program>(
            some(append<&ast::Program::lines>(ref<LineRule>)))> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<>;
};

}  // namespace basic
