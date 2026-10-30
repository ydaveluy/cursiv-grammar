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

inline constexpr auto drop_rem = [](std::string_view t,
                                    std::pmr::memory_resource*) {
  return t.substr(3);
};

struct Basic : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct REM : terminal<ikw<"REM">> {};
  // A comment's introducer and its text are one token, not a keyword
  // followed by a run: `ikw<"REM">` would refuse text glued right against
  // the word (no space). At least one trailing byte, so an empty comment
  // ties with REM on length, and REM -- declared as its own terminal --
  // wins that tie.
  static constexpr auto RemWord = cr<"Rr"> + cr<"Ee"> + cr<"Mm">;
  struct COMMENT : terminal<as<drop_rem>(RemWord + some(cr<R"(^\r\n)">))> {};

  struct STRINGLITERAL
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">)> {};

  struct IDENT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};

  struct LINENUMBER : terminal<as<std::int64_t>(some(d))> {};
  struct NUMBER : terminal<as<double>(some(d) + many(cr<"Ee"> + some(d)))> {};
  struct FLOAT : terminal<as<double>(many(d) + lit<"."> + some(d) +
                                     many(cr<"Ee"> + some(d)))> {};

  static constexpr auto Suffix = as<ast::VarSuffix::dollar>(lit<"$">) | as<ast::VarSuffix::percent>(lit<"%">);

  struct LPAREN : terminal<lit<"(">> {};
  struct RPAREN : terminal<lit<")">> {};
  struct COMMA : terminal<lit<",">> {};
  struct COLONTOK : terminal<lit<":">> {};
  struct SEMICOLON : terminal<lit<";">> {};
  struct EQTOK : terminal<lit<"=">> {};
  struct GTTOK : terminal<lit<">">> {};
  struct LTTOK : terminal<lit<"<">> {};


  // GE/LE/NE combine two adjacent single-char tokens, or a vestigial
  // three-byte spelling (`>: `, `<: `, a literal space before the digits)
  // some BASIC dialects still accept.
  struct GE : rule<as<ast::Op::ge>((ref<GTTOK> + ref<EQTOK>) |
                                   (ref<EQTOK> + ref<GTTOK>) | lit<">: ">)> {};
  struct LE : rule<as<ast::Op::le>((ref<LTTOK> + ref<EQTOK>) |
                                   (ref<EQTOK> + ref<LTTOK>) | lit<"<: ">)> {};
  struct NE : rule<as<ast::Op::ne>(ref<LTTOK> + ref<GTTOK>)> {};

  static constexpr auto OpPow = as<ast::Op::pow>(lit<"^">);
  static constexpr auto OpMulDiv = as<ast::Op::mul>(lit<"*">) | as<ast::Op::div_>(lit<"/">);
  static constexpr auto OpAddSub = as<ast::Op::add>(lit<"+">) | as<ast::Op::sub>(lit<"-">);
  static constexpr auto OpCompare =
      ref<GE> | ref<LE> | ref<NE> | as<ast::Op::eq>(ref<EQTOK>) | as<ast::Op::gt>(ref<GTTOK>) | as<ast::Op::lt>(ref<LTTOK>);
  static constexpr auto OpAndOr = as<ast::Op::and_>(ikw<"AND">) | as<ast::Op::or_>(ikw<"OR">);

  // Builtin functions: one keyword, one to three parenthesised arguments,
  // and no structure beyond that -- unified rather than one struct each.
  struct CHR : terminal<as<ast::BuiltinFunc::chr>(ikw<"CHR$">)> {};
  struct SQR : terminal<as<ast::BuiltinFunc::sqr>(ikw<"SQR">)> {};
  struct LEN : terminal<as<ast::BuiltinFunc::len>(ikw<"LEN">)> {};
  struct STR : terminal<as<ast::BuiltinFunc::str>(ikw<"STR$">)> {};
  struct ASC : terminal<as<ast::BuiltinFunc::asc>(ikw<"ASC">)> {};
  struct SCRN : terminal<as<ast::BuiltinFunc::scrn>(ikw<"SCRN">)> {};
  struct MID : terminal<as<ast::BuiltinFunc::mid>(ikw<"MID$">)> {};
  struct PDL : terminal<as<ast::BuiltinFunc::pdl>(ikw<"PDL">)> {};
  struct PEEK : terminal<as<ast::BuiltinFunc::peek>(ikw<"PEEK">)> {};
  struct SPC : terminal<as<ast::BuiltinFunc::spc>(ikw<"SPC">)> {};
  struct FRE : terminal<as<ast::BuiltinFunc::fre>(ikw<"FRE">)> {};
  struct POS : terminal<as<ast::BuiltinFunc::pos>(ikw<"POS">)> {};
  struct USR : terminal<as<ast::BuiltinFunc::usr>(ikw<"USR">)> {};
  struct LEFT : terminal<as<ast::BuiltinFunc::left>(ikw<"LEFT$">)> {};
  struct VAL : terminal<as<ast::BuiltinFunc::val>(ikw<"VAL">)> {};
  struct RIGHT : terminal<as<ast::BuiltinFunc::right>(ikw<"RIGHT$">)> {};
  struct SIN : terminal<as<ast::BuiltinFunc::sin>(ikw<"SIN">)> {};
  struct COS : terminal<as<ast::BuiltinFunc::cos>(ikw<"COS">)> {};
  struct TAN : terminal<as<ast::BuiltinFunc::tan>(ikw<"TAN">)> {};
  struct ATN : terminal<as<ast::BuiltinFunc::atn>(ikw<"ATN">)> {};
  struct RND : terminal<as<ast::BuiltinFunc::rnd>(ikw<"RND">)> {};
  struct SGN : terminal<as<ast::BuiltinFunc::sgn>(ikw<"SGN">)> {};
  struct LOG : terminal<as<ast::BuiltinFunc::log>(ikw<"LOG">)> {};
  struct ABS : terminal<as<ast::BuiltinFunc::abs>(ikw<"ABS">)> {};

  static constexpr auto Func1 =
      ref<SQR> | ref<LEN> | ref<ASC> | ref<PDL> | ref<PEEK> | as<ast::BuiltinFunc::intf>(ikw<"INT">) |
      ref<SPC> | ref<FRE> | ref<POS> | ref<USR> | ref<VAL> | ref<SIN> |
      ref<COS> | ref<TAN> | ref<ATN> | ref<RND> | ref<SGN> | as<ast::BuiltinFunc::exp>(ikw<"EXP">) |
      ref<LOG> | ref<ABS> | as<ast::BuiltinFunc::tabf>(ikw<"TAB">) | ref<CHR> | ref<STR>;
  static constexpr auto Func2 = ref<LEFT> | ref<RIGHT> | ref<SCRN>;

  struct Expression;
  struct Statement;

  struct Subscript
      : rule<create<ast::Subscript>(
            ref<LPAREN> +
            some(append<&ast::Subscript::args>(ref<Expression>), ref<COMMA>) +
            ref<RPAREN>)> {};

  struct VarRef
      : rule<create<ast::VarRef>(
            assign<&ast::VarRef::name>(ref<IDENT>) +
            option(assign<&ast::VarRef::suffix>(Suffix)) +
            many(append<&ast::VarRef::subscripts>(ref<Subscript>)))> {};

  struct NumberLit
      : rule<create<ast::NumberLit>(
            option(flag<&ast::NumberLit::negative>(lit<"-">) |
                   lit<"+">) +
            assign<&ast::NumberLit::value>(ref<NUMBER> | ref<FLOAT>))> {};

  struct StringLit
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRINGLITERAL>))> {};

  struct FuncCall
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
  struct FnCall
      : rule<create<ast::FnCall>(
            ref<FN> + assign<&ast::FnCall::name>(ref<IDENT>) +
            option(assign<&ast::FnCall::suffix>(Suffix)) + ref<LPAREN> +
            assign<&ast::FnCall::arg>(ref<Expression>) + ref<RPAREN>)> {};

  struct Prim
      : rule<ref<StringLit> | ref<NumberLit> | ref<FnCall> |
             ref<FuncCall> | ref<VarRef> |
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
      : infix<SignedPrim, ast::BinaryExpr,
              levels(left(OpPow),      //
                     left(OpMulDiv),   //
                     left(OpAddSub),   //
                     nonassoc(OpCompare),  //
                     left(OpAndOr))> {};

  struct Datum : rule<ref<NumberLit> | ref<StringLit>> {};

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

  static constexpr auto SimpleKw =
      as<ast::SimpleKind::cls>(ikw<"CLS">) | as<ast::SimpleKind::load>(ikw<"LOAD">) | as<ast::SimpleKind::save>(ikw<"SAVE">) | as<ast::SimpleKind::trace>(ikw<"TRACE">) |
      as<ast::SimpleKind::notrace>(ikw<"NOTRACE">) | as<ast::SimpleKind::flash>(ikw<"FLASH">) | as<ast::SimpleKind::inverse>(ikw<"INVERSE">) | as<ast::SimpleKind::gr>(ikw<"GR">) |
      as<ast::SimpleKind::normal>(ikw<"NORMAL">) | as<ast::SimpleKind::shload>(ikw<"SHLOAD">) | as<ast::SimpleKind::clear>(ikw<"CLEAR">) | as<ast::SimpleKind::run>(ikw<"RUN">) |
      as<ast::SimpleKind::stop>(ikw<"STOP">) | as<ast::SimpleKind::text>(ikw<"TEXT">) | as<ast::SimpleKind::home>(ikw<"HOME">) | as<ast::SimpleKind::hgr2>(ikw<"HGR2">) | as<ast::SimpleKind::hgr>(ikw<"HGR">) |
      as<ast::SimpleKind::end_>(ikw<"END">) | as<ast::SimpleKind::return_>(ikw<"RETURN">) | as<ast::SimpleKind::restore>(ikw<"RESTORE">);

  struct SimpleStmt
      : rule<create<ast::SimpleStmt>(assign<&ast::SimpleStmt::kind>(SimpleKw))> {
  };

  // Statements that are one keyword and a small run of expressions.
  // `PR#` / `IN#` end on `#`, not a word-class byte, so `ikw<>` states no
  // boundary and reads straight into the digit that follows (`PR#1`).

  static constexpr auto Arg1 =
      as<ast::ArgKind::vtab>(ikw<"VTAB">) | as<ast::ArgKind::htab>(ikw<"HTAB">) | as<ast::ArgKind::include_>(ikw<"INCLUDE">);

  struct ArgStmt
      : rule<
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::plot>(ikw<"PLOT">) | as<ast::ArgKind::poke>(ikw<"POKE">)) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(Arg1) +
                                 append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::himem>(ikw<"HIMEM">) | as<ast::ArgKind::lomem>(ikw<"LOMEM">)) +
                ref<COLONTOK> + append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(as<ast::ArgKind::tab_>(ikw<"TAB">)) +
                                 ref<LPAREN> +
                                 append<&ast::ArgStmt::args>(ref<Expression>) +
                                 ref<RPAREN>) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(
                    as<ast::ArgKind::speed>(ikw<"SPEED">) | as<ast::ArgKind::rot>(ikw<"ROT">) | as<ast::ArgKind::scale>(ikw<"SCALE">) | as<ast::ArgKind::color>(ikw<"COLOR">) |
                    as<ast::ArgKind::hcolor>(ikw<"HCOLOR">)) +
                ref<EQTOK> + append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::hlin>(ikw<"HLIN">) | as<ast::ArgKind::vlin>(ikw<"VLIN">)) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<ATKW> +
                append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::prnumber>(ikw<"PR#">) | as<ast::ArgKind::innumber>(ikw<"IN#">)) +
                append<&ast::ArgStmt::args>(ref<NumberLit>)) |
            create<ast::ArgStmt>(assign<&ast::ArgStmt::kind>(as<ast::ArgKind::ampersand>(lit<"&">)) +
                                 append<&ast::ArgStmt::args>(ref<Expression>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::get>(ikw<"GET">) | as<ast::ArgKind::call>(ikw<"CALL">)) +
                some(append<&ast::ArgStmt::args>(ref<Expression>), ref<COMMA>)) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::wait>(ikw<"WAIT">)) +
                append<&ast::ArgStmt::args>(ref<Expression>) + ref<COMMA> +
                append<&ast::ArgStmt::args>(ref<Expression>) +
                option(ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::xdraw>(ikw<"XDRAW">) | as<ast::ArgKind::draw>(ikw<"DRAW">)) +
                append<&ast::ArgStmt::args>(ref<Expression>) +
                option(ref<ATKW> +
                       append<&ast::ArgStmt::args>(ref<Expression>) +
                       ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::list_>(ikw<"LIST">)) +
                option(append<&ast::ArgStmt::args>(ref<Expression>))) |
            create<ast::ArgStmt>(
                assign<&ast::ArgStmt::kind>(as<ast::ArgKind::pop_>(ikw<"POP">)) +
                option(append<&ast::ArgStmt::args>(ref<Expression>) +
                       ref<COMMA> +
                       append<&ast::ArgStmt::args>(ref<Expression>)))> {};

  struct PlotPath
      : rule<create<ast::PlotPath>(
            (flag<&ast::PlotPath::vertical>(ref<VPLOTKW>) |
             ref<HPLOTKW>) +
            option(append<&ast::PlotPath::coords>(ref<Expression>) +
                   ref<COMMA> +
                   append<&ast::PlotPath::coords>(ref<Expression>)) +
            many(ref<TOKW> + append<&ast::PlotPath::coords>(ref<Expression>) +
                 ref<COMMA> +
                 append<&ast::PlotPath::coords>(ref<Expression>)))> {};

  struct CommentStmt
      : rule<create<ast::Comment>(
                 assign<&ast::Comment::text>(ref<COMMENT>)) |
             create<ast::Comment>(ref<REM>)> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            option(flag<&ast::AssignStmt::explicitLet>(ref<LETKW>)) +
            assign<&ast::AssignStmt::target>(ref<VarRef>) + ref<EQTOK> +
            some(append<&ast::AssignStmt::values>(ref<Expression>),
                 ref<COMMA>))> {};

  struct PrintStmt
      : rule<create<ast::PrintStmt>(
            (ref<PRINTKW> | ref<QUESTIONKW>) +append<&ast::PrintStmt::items>(
                create<ast::PrintItem>(
                    option(assign<&ast::PrintItem::value>(ref<Expression>)))) +
            many(append<&ast::PrintStmt::items>(create<ast::PrintItem>(
                assign<&ast::PrintItem::separator>(ref<COMMA> | ref<SEMICOLON>) +
                        option(assign<&ast::PrintItem::value>(
                            ref<Expression>))))))> {};

  struct InputStmt
      : rule<create<ast::InputStmt>(
            ref<INPUTKW> +
            option(assign<&ast::InputStmt::prompt>(ref<STRINGLITERAL>) +
                   (ref<COMMA> | ref<SEMICOLON>)) +
            some(append<&ast::InputStmt::vars>(ref<VarRef>), ref<COMMA>))> {
  };

  struct ReadStmt
      : rule<create<ast::ReadStmt>(
            ref<READKW> +
            some(append<&ast::ReadStmt::vars>(ref<VarRef>), ref<COMMA>))> {
  };

  struct DimStmt
      : rule<create<ast::DimStmt>(
            ref<DIMKW> +
            some(append<&ast::DimStmt::vars>(ref<VarRef>), ref<COMMA>))> {};

  struct StoreStmt
      : rule<create<ast::StoreStmt>(
            ref<STOREKW> + assign<&ast::StoreStmt::var>(ref<VarRef>))> {};

  struct RecallStmt
      : rule<create<ast::RecallStmt>(
            ref<RECALLKW> +
            assign<&ast::RecallStmt::var>(ref<VarRef>))> {};

  struct DataStmt
      : rule<create<ast::DataStmt>(
            ref<DATAKW> + append<&ast::DataStmt::values>(ref<Datum>) +
            many(ref<COMMA> +
                 option(append<&ast::DataStmt::values>(ref<Datum>))))> {};

  struct DefStmt
      : rule<create<ast::DefStmt>(
            ref<DEFKW> +
            option(flag<&ast::DefStmt::explicitFn>(ref<FN>)) +
            assign<&ast::DefStmt::name>(ref<IDENT>) +
            option(assign<&ast::DefStmt::nameSuffix>(Suffix)) + ref<LPAREN> +
            assign<&ast::DefStmt::param>(ref<IDENT>) +
            option(assign<&ast::DefStmt::paramSuffix>(Suffix)) + ref<RPAREN> +
            ref<EQTOK> + assign<&ast::DefStmt::body>(ref<Expression>))> {};

  struct OnErrStmt
      : rule<create<ast::OnErrStmt>(
            ref<ONERRKW> + ref<GOTOKW> +
            assign<&ast::OnErrStmt::target>(ref<LINENUMBER>))> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            ref<GOTOKW> + assign<&ast::GotoStmt::target>(ref<LINENUMBER>))> {};

  struct GosubStmt
      : rule<create<ast::GosubStmt>(
            ref<GOSUBKW> +
            assign<&ast::GosubStmt::target>(ref<Expression>))> {};

  struct OnGotoStmt
      : rule<create<ast::OnGotoStmt>(
            ref<ONKW> + assign<&ast::OnGotoStmt::selector>(ref<Expression>) +
            ref<GOTOKW> +
            some(append<&ast::OnGotoStmt::targets>(ref<LINENUMBER>),
                 ref<COMMA>))> {};

  struct OnGosubStmt
      : rule<create<ast::OnGosubStmt>(
            ref<ONKW> + assign<&ast::OnGosubStmt::selector>(ref<Expression>) +
            ref<GOSUBKW> +
            some(append<&ast::OnGosubStmt::targets>(ref<LINENUMBER>),
                 ref<COMMA>))> {};

  // The optional trailing `statement NEXT var?` covers every document a
  // bare `FOR ... TO ...` header alone would: the shorter form is dropped
  // as redundant, never a shorter match.
  struct ForStmt
      : rule<create<ast::ForStmt>(
            ref<FORKW> + assign<&ast::ForStmt::var>(ref<VarRef>) +
            ref<EQTOK> + assign<&ast::ForStmt::from>(ref<Expression>) +
            ref<TOKW> + assign<&ast::ForStmt::to>(ref<Expression>) +
            option(ref<STEPKW> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            option(assign<&ast::ForStmt::body>(ref<Statement>) +
                   ref<NEXTKW> +
                   option(assign<&ast::ForStmt::nextVar>(ref<VarRef>))))> {
  };

  struct NextStmt
      : rule<create<ast::NextStmt>(
            ref<NEXTKW> +
            option(some(append<&ast::NextStmt::vars>(ref<VarRef>),
                        ref<COMMA>)))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            ref<IFKW> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            option(ref<THENKW>) +
            (assign<&ast::IfStmt::thenStmt>(ref<Statement>) |
             assign<&ast::IfStmt::thenLine>(ref<LINENUMBER>)))> {};

  struct Statement
      : rule<ref<PrintStmt> | ref<AssignStmt> | ref<IfStmt> |
             ref<ForStmt> | ref<NextStmt> | ref<GotoStmt> |
             ref<GosubStmt> | ref<OnGotoStmt> | ref<OnGosubStmt> |
             ref<OnErrStmt> | ref<InputStmt> | ref<ReadStmt> |
             ref<DimStmt> | ref<StoreStmt> | ref<RecallStmt> |
             ref<DataStmt> | ref<DefStmt> | ref<PlotPath> |
             ref<ArgStmt> | ref<SimpleStmt>> {};

  struct CommentItem
      : rule<create<ast::StmtItem>(
            assign<&ast::StmtItem::stmt>(ref<CommentStmt>))> {};

  struct AmprStmt
      : rule<create<ast::StmtItem>(
                 option(flag<&ast::StmtItem::ampersandPrefix>(
                     ref<AMPERSAND>)) +
                 assign<&ast::StmtItem::stmt>(ref<Statement>)) |
             ref<CommentItem>> {};

  struct Line
      : rule<create<ast::Line>(
            assign<&ast::Line::number>(ref<LINENUMBER>) +
            ((append<&ast::Line::items>(ref<AmprStmt>) +
              many(ref<COLONTOK> +
                   option(append<&ast::Line::items>(ref<AmprStmt>)))) |
             append<&ast::Line::items>(ref<CommentItem>)))> {};

  struct Program
      : rule<create<ast::Program>(
            some(append<&ast::Program::lines>(ref<Line>)))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<>;
};

}  // namespace basic
