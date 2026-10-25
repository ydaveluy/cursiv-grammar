// The rexx language: its grammar.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rexx/core/ast.hpp"

namespace rexx {

// A quoted string's own delimiter, doubled, is its one escape; the other
// quote character needs none.
inline constexpr auto unquote_string =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      const char q = t.front();
      const std::string_view body = t.substr(1, t.size() - 2);
      if (body.find(q) == std::string_view::npos) return body;
      const std::span<char> room = cursiv::text_room(mem, body.size());
      std::size_t n = 0;
      for (std::size_t i = 0; i < body.size(); ++i) {
        room[n++] = body[i];
        if (body[i] == q) ++i;
      }
      return std::string_view(room.data(), n);
    };
using Unquoted = cursiv::text_conv<unquote_string>;

struct Rexx : cursiv::GrammarSpec {
  // Identifier bytes past the default `w`: `!?#@$` and `.` (compound
  // variables), so a keyword's boundary matches what a symbol may hold.
  static constexpr auto word_boundary = cr<"A-Za-z0-9_!?#@$.">;

  // ---- trivia ----
  struct COMMENT
      : terminal<lit<"/*"> +
                 many(ref<COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  static constexpr auto CommentBody = ref<COMMENT>;
  static constexpr auto Blank = cr<R"( \t\f\v)">;
  struct WS : terminal<some(Blank)> {};
  static constexpr auto EolBody =
      lit<"\n\r"> | lit<"\r\n"> | lit<"\n"> | lit<"\r">;
  struct EOL : terminal<EolBody> {};
  // A comma continues a clause across a line break, comments and blanks
  // included between the comma and the break.
  struct CONTINUATION
      : terminal<lit<","> + many(CommentBody | Blank) + EolBody> {};

  // `/* %include name... */`: a directive shaped like a comment.
  struct STMT_INCLUDE
      : terminal<lit<"/*"> + many(Blank) + lit<"%"> + cr<"iI"> + cr<"nN"> +
                     cr<"cC"> + cr<"lL"> + cr<"uU"> + cr<"dD"> + cr<"eE"> +
                     some(Blank) +
                     some(cr<"A-Za-z_!?#@$"> + many(cr<"A-Za-z_!?#@$0-9.">)) +
                     many(Blank) + lit<"*/">> {};

  // ---- keywords: case-insensitive, spelled once. The seven also usable as
  // a bare name (ADDRESS(), ARG() ...) share their terminal with
  // FunctionNameExpr below.
  static constexpr auto KWD_ADDRESS = ikw<"ADDRESS">;
  static constexpr auto KWD_ARG = ikw<"ARG">;
  static constexpr auto KWD_BY = ikw<"BY">;
  static constexpr auto KWD_CALL = ikw<"CALL">;
  static constexpr auto KWD_DIGITS = ikw<"DIGITS">;
  static constexpr auto KWD_DO = ikw<"DO">;
  static constexpr auto KWD_DROP = ikw<"DROP">;
  static constexpr auto KWD_ELSE = ikw<"ELSE">;
  static constexpr auto KWD_END = ikw<"END">;
  static constexpr auto KWD_ENGINEERING = ikw<"ENGINEERING">;
  static constexpr auto KWD_ERROR = ikw<"ERROR">;
  static constexpr auto KWD_EXIT = ikw<"EXIT">;
  static constexpr auto KWD_EXPOSE = ikw<"EXPOSE">;
  static constexpr auto KWD_EXTERNAL = ikw<"EXTERNAL">;
  static constexpr auto KWD_FAILURE = ikw<"FAILURE">;
  static constexpr auto KWD_FOR = ikw<"FOR">;
  static constexpr auto KWD_FOREVER = ikw<"FOREVER">;
  static constexpr auto KWD_FORM = ikw<"FORM">;
  static constexpr auto KWD_FUZZ = ikw<"FUZZ">;
  static constexpr auto KWD_HALT = ikw<"HALT">;
  static constexpr auto KWD_IF = ikw<"IF">;
  static constexpr auto KWD_INTERPRET = ikw<"INTERPRET">;
  static constexpr auto KWD_ITERATE = ikw<"ITERATE">;
  static constexpr auto KWD_LEAVE = ikw<"LEAVE">;
  static constexpr auto KWD_NAME = ikw<"NAME">;
  static constexpr auto KWD_NOP = ikw<"NOP">;
  static constexpr auto KWD_NOVALUE = ikw<"NOVALUE">;
  static constexpr auto KWD_NUMERIC = ikw<"NUMERIC">;
  static constexpr auto KWD_OFF = ikw<"OFF">;
  static constexpr auto KWD_ON = ikw<"ON">;
  static constexpr auto KWD_OPTIONS = ikw<"OPTIONS">;
  static constexpr auto KWD_OTHERWISE = ikw<"OTHERWISE">;
  static constexpr auto KWD_PARSE = ikw<"PARSE">;
  static constexpr auto KWD_PROCEDURE = ikw<"PROCEDURE">;
  static constexpr auto KWD_PULL = ikw<"PULL">;
  static constexpr auto KWD_PUSH = ikw<"PUSH">;
  static constexpr auto KWD_QUEUE = ikw<"QUEUE">;
  static constexpr auto KWD_RETURN = ikw<"RETURN">;
  static constexpr auto KWD_SAY = ikw<"SAY">;
  static constexpr auto KWD_SCIENTIFIC = ikw<"SCIENTIFIC">;
  static constexpr auto KWD_SELECT = ikw<"SELECT">;
  static constexpr auto KWD_SIGNAL = ikw<"SIGNAL">;
  static constexpr auto KWD_SOURCE = ikw<"SOURCE">;
  static constexpr auto KWD_SYNTAX = ikw<"SYNTAX">;
  static constexpr auto KWD_THEN = ikw<"THEN">;
  static constexpr auto KWD_TO = ikw<"TO">;
  static constexpr auto KWD_TRACE = ikw<"TRACE">;
  static constexpr auto KWD_UNTIL = ikw<"UNTIL">;
  static constexpr auto KWD_UPPER = ikw<"UPPER">;
  static constexpr auto KWD_VALUE = ikw<"VALUE">;
  static constexpr auto KWD_VAR = ikw<"VAR">;
  static constexpr auto KWD_VERSION = ikw<"VERSION">;
  static constexpr auto KWD_WHEN = ikw<"WHEN">;
  static constexpr auto KWD_WHILE = ikw<"WHILE">;
  static constexpr auto KWD_WITH = ikw<"WITH">;

  struct BR_O : terminal<lit<"(">> {};
  struct BR_C : terminal<lit<")">> {};

  struct SPECIAL_VAR : terminal<ikw<"RESULT"> | ikw<"RC"> | ikw<"SIGL">> {};

  // NUMBER, then CONST_SYMBOL, then VAR_SYMBOL: their alphabets overlap, so
  // the longer match (a symbol swallowing digits the shorter NUMBER stopped
  // at) must win, in this declaration order.
  struct NUMBER
      : terminal<((some(d) + option(lit<"."> + many(d))) |
                  (lit<"."> + some(d))) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  static constexpr auto Letter = cr<"A-Za-z_!?#@$">;
  static constexpr auto SymChar = cr<"A-Za-z_!?#@$0-9.">;
  // A digit/dot run alone is a NUMBER, never a CONST_SYMBOL: the two tie in
  // length on every plain literal, so the terminals must be disjoint by
  // construction (a letter somewhere makes it a symbol; a bare dot stays
  // one, since NUMBER's own dot form demands a digit after it).
  struct CONST_SYMBOL
      : terminal<(cr<"0-9."> + many(cr<"0-9.">) + Letter + many(SymChar)) |
                 lit<".">> {};
  struct VAR_SYMBOL : terminal<Letter + many(SymChar)> {};

  // Binary_string_value_: 1-4 digits, then groups of exactly 4; a space run
  // between groups is free.
  struct BINARY_STRING
      : terminal<(lit<"\""> | lit<"'">) + repeat<0, 3>(cr<"01">) + cr<"01"> +
                     many(many(lit<" ">) + repeat<4, 4>(cr<"01">)) +
                     (lit<"\""> | lit<"'">) + cr<"bB">> {};
  // Hex_string_value_: 1-2 digits, then groups of exactly 2.
  struct HEX_STRING
      : terminal<(lit<"\""> | lit<"'">) + option(cr<"0-9A-Fa-f">) +
                     cr<"0-9A-Fa-f"> +
                     many(many(lit<" ">) + repeat<2, 2>(cr<"0-9A-Fa-f">)) +
                     (lit<"\""> | lit<"'">) + cr<"xX">> {};

  // A string may not cross a line; its own quote, doubled, is the escape.
  static constexpr auto DqChar = lit<"\"\""> | cr<R"(^"\r\n)">;
  static constexpr auto SqChar = lit<"''"> | cr<R"(^'\r\n)">;
  struct STRING
      : terminal<(lit<"\""> + many(DqChar) + lit<"\"">) |
                     (lit<"'"> + many(SqChar) + lit<"'">),
                 Unquoted> {};

  struct CONCAT : terminal<lit<"||">> {};

  struct EQ : terminal<lit<"=">, value<ast::Op::Eq>> {};
  struct PLUS : terminal<lit<"+">, value<ast::Op::Add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Op::Sub>> {};
  struct MUL : terminal<lit<"*">, value<ast::Op::Mul>> {};
  struct DIV : terminal<lit<"/">, value<ast::Op::Div>> {};
  struct QUOTIENT : terminal<lit<"%">, value<ast::Op::IntDiv>> {};
  struct REMAINDER : terminal<lit<"//">, value<ast::Op::Mod>> {};
  struct POW : terminal<lit<"**">, value<ast::Op::Pow>> {};
  // `\`, `^`, `¬` negate; the fourth reference spelling (`/`) is unreachable
  // there (DIV is declared first at the same one-byte length) and is left
  // out.
  struct NOT_TOK
      : terminal<lit<"\\"> | lit<"^"> | lit<"¬">, value<ast::Op::Not>> {};
  struct OR_TOK : terminal<lit<"|">, value<ast::Op::Or>> {};
  struct XOR_TOK : terminal<lit<"&&">, value<ast::Op::Xor>> {};
  struct AND_TOK : terminal<lit<"&">, value<ast::Op::And>> {};

  struct CMPS_Eq : terminal<lit<"==">, value<ast::Op::SEq>> {};
  struct CMPS_Neq : terminal<lit<"\\==">, value<ast::Op::SNeq>> {};
  struct CMPS_MEq : terminal<lit<">>=">, value<ast::Op::SGtEq>> {};
  struct CMPS_LEq : terminal<lit<"<<=">, value<ast::Op::SLtEq>> {};
  struct CMPS_NM : terminal<lit<"\\>>">, value<ast::Op::SNGt>> {};
  struct CMPS_NL : terminal<lit<"\\<<">, value<ast::Op::SNLt>> {};
  struct CMPS_M : terminal<lit<">>">, value<ast::Op::SGt>> {};
  struct CMPS_L : terminal<lit<"<<">, value<ast::Op::SLt>> {};

  struct CMP_NEq : terminal<lit<"\\=">, value<ast::Op::Neq>> {};
  struct CMP_LM : terminal<lit<"<>">, value<ast::Op::LtGt>> {};
  struct CMP_ML : terminal<lit<"><">, value<ast::Op::GtLt>> {};
  struct CMP_MEq : terminal<lit<">=">, value<ast::Op::GtEq>> {};
  struct CMP_LEq : terminal<lit<"<=">, value<ast::Op::LtEq>> {};
  struct CMP_NM : terminal<lit<"\\>">, value<ast::Op::NGt>> {};
  struct CMP_NL : terminal<lit<"\\<">, value<ast::Op::NLt>> {};
  struct CMP_M : terminal<lit<">">, value<ast::Op::Gt>> {};
  struct CMP_L : terminal<lit<"<">, value<ast::Op::Lt>> {};

  struct STOP : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};
  struct COLON : terminal<lit<":">> {};
  struct SEMICOL : terminal<lit<";">> {};

  static constexpr auto Delim = ref<EOL> | ref<SEMICOL>;

  // ---- names ----
  struct VarRefRule
      : rule<create<ast::VarRef>(
            (flag<&ast::VarRef::indirect>(ref<BR_O>) +
             assign<&ast::VarRef::name>(ref<VAR_SYMBOL> | ref<SPECIAL_VAR>) +
             ref<BR_C>) |
            assign<&ast::VarRef::name>(ref<VAR_SYMBOL> | ref<SPECIAL_VAR>))> {
  };

  static constexpr auto TakenConstantExpr =
      ref<VAR_SYMBOL> | ref<SPECIAL_VAR> | ref<CONST_SYMBOL> | ref<NUMBER> |
      ref<STRING>;
  static constexpr auto FunctionNameExpr =
      KWD_ADDRESS | KWD_ARG | KWD_DIGITS | KWD_FORM | KWD_FUZZ | KWD_TRACE |
      KWD_VALUE | TakenConstantExpr;
  static constexpr auto CallableCondition = KWD_ERROR | KWD_FAILURE | KWD_HALT;

  // ---- expressions ----
  struct Expression;

  struct Symbol
      : rule<create<ast::NumberLit>(
                 assign<&ast::NumberLit::text>(ref<NUMBER>)) |
             create<ast::NameRef>(assign<&ast::NameRef::name>(
                 ref<VAR_SYMBOL> | ref<SPECIAL_VAR> | ref<CONST_SYMBOL>))> {};

  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING>))> {};
  struct BinaryLitRule
      : rule<create<ast::BinaryLit>(
            assign<&ast::BinaryLit::text>(ref<BINARY_STRING>))> {};
  struct HexLitRule
      : rule<create<ast::HexLit>(assign<&ast::HexLit::text>(ref<HEX_STRING>))> {
  };
  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            ref<BR_O> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            ref<BR_C>)> {};

  struct ArgSlot
      : rule<create<ast::Arg>(option(assign<&ast::Arg::value>(ref<Expression>)))> {
  };

  struct FilledArgSlot
      : rule<create<ast::Arg>(assign<&ast::Arg::value>(ref<Expression>))> {};

  // A lone slot is never empty: `f()` holds no `Arg`.
  template <auto Member>
  static constexpr auto ArgListInto =
      append<Member>(ref<ArgSlot>) +
          some(ref<COMMA> + append<Member>(ref<ArgSlot>))  //
      | append<Member>(ref<FilledArgSlot>);

  struct FunctionCallRule
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(FunctionNameExpr) + ref<BR_O> +
            option(ArgListInto<&ast::Call::args>) + ref<BR_C>)> {};

  struct Term
      : rule<ref<FunctionCallRule> | ref<ParenExprRule> | ref<Symbol> |
             ref<BinaryLitRule> | ref<HexLitRule> | ref<StringLitRule>> {};

  struct PrefixExprRule
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(
                     ref<PLUS> | ref<MINUS> | ref<NOT_TOK>) +
                 assign<&ast::PrefixExpr::operand>(ref<PrefixExprRule>)) |
             ref<Term>> {};

  struct SumLevel
      : infix<PrefixExprRule, &ast::BinExpr::left, &ast::BinExpr::op,
              &ast::BinExpr::right,
              (left(ref<POW>) >>
               left(ref<MUL> | ref<DIV> | ref<QUOTIENT> | ref<REMAINDER>) >>
               left(ref<PLUS> | ref<MINUS>))> {};

  // concatenation: the operator is not a token in two of its three
  // readings, so it is not spelled as an `infix<>` level.
  static constexpr auto GapEndsBlank = gap::is(many(any) + Blank);
  struct Concatenation
      : rule<ref<SumLevel> +
             many(nest<ast::Concat, &ast::Concat::left>(
                 (flag<&ast::Concat::explicitOp>(ref<CONCAT>) +
                  assign<&ast::Concat::right>(ref<SumLevel>)) |
                 (flag<&ast::Concat::blank>(GapEndsBlank) +
                  assign<&ast::Concat::right>(ref<SumLevel>)) |
                 (!GapEndsBlank +
                  assign<&ast::Concat::right>(ref<SumLevel>))))> {};

  static constexpr auto CompareOp =
      ref<EQ> | ref<CMP_NEq> | ref<CMP_LM> | ref<CMP_ML> | ref<CMP_MEq> |
      ref<CMP_LEq> | ref<CMP_NM> | ref<CMP_NL> | ref<CMP_M> | ref<CMP_L> |
      ref<CMPS_Eq> | ref<CMPS_Neq> | ref<CMPS_MEq> | ref<CMPS_LEq> |
      ref<CMPS_NM> | ref<CMPS_NL> | ref<CMPS_M> | ref<CMPS_L>;

  struct Expression
      : infix<Concatenation, &ast::BinExpr::left, &ast::BinExpr::op,
              &ast::BinExpr::right,
              (left(CompareOp) >> left(ref<AND_TOK>) >>
               left(ref<OR_TOK> | ref<XOR_TOK>))> {};

  // ---- templates: ARG/PULL/PARSE's target_ and trigger_ ----
  struct TplVarRule
      : rule<create<ast::TplVar>(
            assign<&ast::TplVar::name>(ref<VAR_SYMBOL> | ref<SPECIAL_VAR>))> {
  };
  struct TplDropRule : rule<create<ast::TplDrop>(ref<STOP>)> {};
  struct TplStringPatternRule
      : rule<create<ast::TplStringPattern>(
            assign<&ast::TplStringPattern::value>(ref<STRING>))> {};
  struct TplVarPatternRule
      : rule<create<ast::TplVarPattern>(
            ref<BR_O> +
            assign<&ast::TplVarPattern::name>(ref<VAR_SYMBOL> |
                                               ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};
  struct TplAbsPosRule
      : rule<create<ast::TplAbsPos>(
            option(ref<EQ>) + assign<&ast::TplAbsPos::number>(ref<NUMBER>))> {
  };
  struct TplAbsPosVarRule
      : rule<create<ast::TplAbsPosVar>(
            ref<EQ> + ref<BR_O> +
            assign<&ast::TplAbsPosVar::name>(ref<VAR_SYMBOL> |
                                              ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};
  struct TplRelPosRule
      : rule<create<ast::TplRelPos>(
            (flag<&ast::TplRelPos::minus>(ref<MINUS>) | ref<PLUS>) +
            assign<&ast::TplRelPos::number>(ref<NUMBER>))> {};
  struct TplRelPosVarRule
      : rule<create<ast::TplRelPosVar>(
            (flag<&ast::TplRelPosVar::minus>(ref<MINUS>) | ref<PLUS>) +
            ref<BR_O> +
            assign<&ast::TplRelPosVar::name>(ref<VAR_SYMBOL> |
                                              ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};

  static constexpr auto TplItemExpr =
      ref<TplStringPatternRule> | ref<TplVarPatternRule> |
      ref<TplAbsPosVarRule> | ref<TplAbsPosRule> | ref<TplRelPosVarRule> |
      ref<TplRelPosRule> | ref<TplVarRule> | ref<TplDropRule>;

  // template_list: COMMA* template_ (COMMA+ template_)* -- a slot may be
  // empty (the source it would read is skipped) between commas.
  struct TemplateSlot
      : rule<create<ast::Template>(many(append<&ast::Template::items>(
            TplItemExpr)))> {};

  // ---- statements ----
  struct Instruction;

  // An assignment never ends before a quote: the quote opens or continues
  // its value, a reading that always wins.
  struct AssignmentRule
      : rule<assign<&ast::Assignment::target>(
                 ref<VAR_SYMBOL> | ref<SPECIAL_VAR> | ref<CONST_SYMBOL>) +
             ref<EQ> +
             option(assign<&ast::Assignment::value>(ref<Expression>)) +
             !cr<R"("')">> {};
  struct CommandStmtRule
      : rule<assign<&ast::CommandStmt::value>(ref<Expression>)> {};
  struct LabelRule
      : rule<assign<&ast::Label::name>(ref<VAR_SYMBOL> | ref<CONST_SYMBOL> |
                                        ref<NUMBER>) +
             ref<COLON>> {};
  struct IncludeRule
      : rule<assign<&ast::IncludeStmt::text>(ref<STMT_INCLUDE>)> {};

  // A block: instructions, labels, includes and separators, freely mixed --
  // used for the top-level program and every do_/select_ body alike.
  struct BlockBody;

  // ---- do_ / if_ / select_ ----
  struct DoClauseRule
      : rule<create<ast::DoClause>(
            assign<&ast::DoClause::keyword>(KWD_TO | KWD_BY | KWD_FOR) +
            assign<&ast::DoClause::value>(ref<Expression>))> {};
  static constexpr auto DoCnt =
      many(append<&ast::DoStmt::clauses>(ref<DoClauseRule>));
  static constexpr auto DoRep =
      (assign<&ast::DoStmt::counter>(ref<AssignmentRule>) + DoCnt) |
      flag<&ast::DoStmt::forever>(KWD_FOREVER) |
      assign<&ast::DoStmt::repeatExpr>(ref<Expression>);
  static constexpr auto DoCond =
      (flag<&ast::DoStmt::isWhile>(KWD_WHILE) +
       assign<&ast::DoStmt::condExpr>(ref<Expression>)) |
      (flag<&ast::DoStmt::isUntil>(KWD_UNTIL) +
       assign<&ast::DoStmt::condExpr>(ref<Expression>));

  // A header needs at least one delimiter before its first instruction:
  // `DO` glued straight to what follows is refused.
  struct DoStmtRule
      : rule<create<ast::DoStmt>(
            KWD_DO + option(DoRep) + option(DoCond) + some(Delim) +
            assign<&ast::DoStmt::body>(ref<BlockBody>) + KWD_END +
            option(assign<&ast::DoStmt::endName>(ref<VAR_SYMBOL> |
                                                  ref<SPECIAL_VAR>)))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            KWD_IF + assign<&ast::IfStmt::cond>(ref<Expression>) +
            many(Delim) + KWD_THEN + many(Delim) +
            assign<&ast::IfStmt::thenBranch>(ref<Instruction>) +
            option(many(Delim) + KWD_ELSE + many(Delim) +
                   assign<&ast::IfStmt::elseBranch>(ref<Instruction>)))> {};

  struct WhenRule
      : rule<create<ast::WhenClause>(
            KWD_WHEN + assign<&ast::WhenClause::cond>(ref<Expression>) +
            many(Delim) + KWD_THEN + many(Delim) +
            assign<&ast::WhenClause::body>(ref<Instruction>))> {};
  // A bare `ref<Instruction>` (then_/else_/when_'s single action) does not
  // sit inside a block loop, so nothing else consumes the delimiter that
  // follows it: each step here does, the way BlockBody's own loop does.
  struct SelectStmtRule
      : rule<create<ast::SelectStmt>(
            KWD_SELECT + some(Delim) +
            some(append<&ast::SelectStmt::whens>(ref<WhenRule>) +
                 many(Delim)) +
            option(flag<&ast::SelectStmt::hasOtherwise>(KWD_OTHERWISE) +
                   many(Delim) +
                   assign<&ast::SelectStmt::otherwiseBody>(ref<BlockBody>)) +
            KWD_END)> {};

  // ---- keyword_instruction ----
  struct AddressStmtRule
      : rule<create<ast::AddressStmt>(
            KWD_ADDRESS +
            option((assign<&ast::AddressStmt::envName>(TakenConstantExpr) +
                    option(assign<&ast::AddressStmt::command>(
                        ref<Expression>))) |
                   (KWD_VALUE +
                    assign<&ast::AddressStmt::envValue>(ref<Expression>))))> {
  };

  struct ArgStmtRule
      : rule<create<ast::ArgStmt>(
            KWD_ARG +
            option(append<&ast::ArgStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> +
                        append<&ast::ArgStmt::templates>(ref<TemplateSlot>))))> {
  };

  static constexpr auto CallArgs = ArgListInto<&ast::CallStmt::args>;
  struct CallStmtRule
      : rule<create<ast::CallStmt>(
            KWD_CALL +
            ((flag<&ast::CallStmt::onCondition>(KWD_ON) +
              assign<&ast::CallStmt::condition>(CallableCondition) +
              option(KWD_NAME +
                     assign<&ast::CallStmt::handlerName>(FunctionNameExpr))) |
             (flag<&ast::CallStmt::off>(KWD_OFF) +
              assign<&ast::CallStmt::condition>(CallableCondition)) |
             (assign<&ast::CallStmt::target>(FunctionNameExpr) +
              option((ref<BR_O> + option(CallArgs) + ref<BR_C>) |
                     CallArgs))))> {};

  struct DropStmtRule
      : rule<create<ast::DropStmt>(
            some(append<&ast::DropStmt::vars>(ref<VarRefRule>)))> {};

  struct ExitStmtRule
      : rule<create<ast::ExitStmt>(
            KWD_EXIT + option(assign<&ast::ExitStmt::value>(ref<Expression>)))> {
  };
  struct InterpretStmtRule
      : rule<KWD_INTERPRET +
             assign<&ast::InterpretStmt::value>(ref<Expression>)> {};
  struct IterateStmtRule
      : rule<create<ast::IterateStmt>(
            KWD_ITERATE +
            option(assign<&ast::IterateStmt::label>(ref<VAR_SYMBOL> |
                                                     ref<SPECIAL_VAR>)))> {};
  struct LeaveStmtRule
      : rule<create<ast::LeaveStmt>(
            KWD_LEAVE +
            option(assign<&ast::LeaveStmt::label>(ref<VAR_SYMBOL> |
                                                   ref<SPECIAL_VAR>)))> {};
  struct NopStmtRule : rule<create<ast::NopStmt> + KWD_NOP> {};

  struct NumericDigitsMarker
      : rule<KWD_DIGITS, value<ast::NumericKind::Digits>> {};
  struct NumericFuzzMarker
      : rule<KWD_FUZZ, value<ast::NumericKind::Fuzz>> {};
  struct NumericFormMarker
      : rule<KWD_FORM, value<ast::NumericKind::Form>> {};
  struct NumericStmtRule
      : rule<create<ast::NumericStmt>(
            KWD_NUMERIC +
            ((assign<&ast::NumericStmt::kind>(ref<NumericDigitsMarker>) +
              option(assign<&ast::NumericStmt::amount>(ref<Expression>))) |
             (assign<&ast::NumericStmt::kind>(ref<NumericFuzzMarker>) +
              option(assign<&ast::NumericStmt::amount>(ref<Expression>))) |
             (assign<&ast::NumericStmt::kind>(ref<NumericFormMarker>) +
              option(flag<&ast::NumericStmt::engineering>(
                         KWD_ENGINEERING) |
                     flag<&ast::NumericStmt::scientific>(
                         KWD_SCIENTIFIC) |
                     (KWD_VALUE + assign<&ast::NumericStmt::formValue>(
                                      ref<Expression>)) |
                     assign<&ast::NumericStmt::formValue>(
                         ref<Expression>)))))> {};

  struct OptionsStmtRule
      : rule<KWD_OPTIONS + assign<&ast::OptionsStmt::value>(ref<Expression>)> {
  };

  struct ParseKeyArg : rule<KWD_ARG, value<ast::ParseSource::Arg>> {};
  struct ParseKeyExternal
      : rule<KWD_EXTERNAL, value<ast::ParseSource::External>> {};
  struct ParseKeyNumeric
      : rule<KWD_NUMERIC, value<ast::ParseSource::Numeric>> {};
  struct ParseKeyPull : rule<KWD_PULL, value<ast::ParseSource::Pull>> {};
  struct ParseKeySource : rule<KWD_SOURCE, value<ast::ParseSource::Source>> {};
  struct ParseKeyVersion
      : rule<KWD_VERSION, value<ast::ParseSource::Version>> {};
  struct ParseValueMarker : rule<KWD_VALUE, value<ast::ParseSource::Value>> {};
  struct ParseVarMarker : rule<KWD_VAR, value<ast::ParseSource::Var>> {};
  struct ParseStmtRule
      : rule<create<ast::ParseStmt>(
            KWD_PARSE +
            option(flag<&ast::ParseStmt::upper>(KWD_UPPER)) +
            ((assign<&ast::ParseStmt::source>(
                 ref<ParseKeyArg> | ref<ParseKeyExternal> |
                 ref<ParseKeyNumeric> | ref<ParseKeyPull> |
                 ref<ParseKeySource> | ref<ParseKeyVersion>)) |
             (assign<&ast::ParseStmt::source>(ref<ParseValueMarker>) +
              option(assign<&ast::ParseStmt::valueExpr>(ref<Expression>)) +
              KWD_WITH) |
             (assign<&ast::ParseStmt::source>(ref<ParseVarMarker>) +
              assign<&ast::ParseStmt::varName>(ref<VAR_SYMBOL> |
                                                ref<SPECIAL_VAR>))) +
            option(append<&ast::ParseStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> + append<&ast::ParseStmt::templates>(
                                         ref<TemplateSlot>))))> {};

  struct ProcedureStmtRule
      : rule<create<ast::ProcedureStmt>(
            KWD_PROCEDURE +
            option(flag<&ast::ProcedureStmt::expose>(KWD_EXPOSE) +
                   some(append<&ast::ProcedureStmt::exposed>(
                       ref<VarRefRule>))))> {};

  struct PullStmtRule
      : rule<create<ast::PullStmt>(
            KWD_PULL +
            option(append<&ast::PullStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> + append<&ast::PullStmt::templates>(
                                         ref<TemplateSlot>))))> {};

  struct PushStmtRule
      : rule<create<ast::PushStmt>(
            KWD_PUSH + option(assign<&ast::PushStmt::value>(ref<Expression>)))> {
  };
  struct QueueStmtRule
      : rule<create<ast::QueueStmt>(
            KWD_QUEUE +
            option(assign<&ast::QueueStmt::value>(ref<Expression>)))> {};
  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            KWD_RETURN +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};
  struct SayStmtRule
      : rule<create<ast::SayStmt>(
            KWD_SAY + option(assign<&ast::SayStmt::value>(ref<Expression>)))> {
  };

  struct SignalStmtRule
      : rule<create<ast::SignalStmt>(
            KWD_SIGNAL +
            ((flag<&ast::SignalStmt::onCondition>(KWD_ON) +
              assign<&ast::SignalStmt::condition>(
                  CallableCondition | KWD_NOVALUE | KWD_SYNTAX) +
              option(KWD_NAME + assign<&ast::SignalStmt::handlerName>(
                                     FunctionNameExpr))) |
             (flag<&ast::SignalStmt::off>(KWD_OFF) +
              assign<&ast::SignalStmt::condition>(
                  CallableCondition | KWD_NOVALUE | KWD_SYNTAX)) |
             (KWD_VALUE +
              assign<&ast::SignalStmt::targetValue>(ref<Expression>)) |
             assign<&ast::SignalStmt::targetName>(TakenConstantExpr)))> {};

  struct TraceStmtRule
      : rule<create<ast::TraceStmt>(
            KWD_TRACE +
            option(assign<&ast::TraceStmt::constant>(TakenConstantExpr) |
                   (KWD_VALUE +
                    assign<&ast::TraceStmt::value>(ref<Expression>)) |
                   assign<&ast::TraceStmt::value>(ref<Expression>) |
                   assign<&ast::TraceStmt::constant>(KWD_ERROR) |
                   assign<&ast::TraceStmt::constant>(KWD_FAILURE) |
                   assign<&ast::TraceStmt::constant>(KWD_OFF)))> {};

  struct UpperStmtRule
      : rule<create<ast::UpperStmt>(
            KWD_UPPER + some(append<&ast::UpperStmt::vars>(ref<VarRefRule>)))> {
  };

  struct KeywordInstructionRule
      : rule<ref<AddressStmtRule> | ref<ArgStmtRule> | ref<CallStmtRule> |
             ref<DropStmtRule> | ref<ExitStmtRule> | ref<InterpretStmtRule> |
             ref<IterateStmtRule> | ref<LeaveStmtRule> | ref<NopStmtRule> |
             ref<NumericStmtRule> | ref<OptionsStmtRule> | ref<ParseStmtRule> |
             ref<ProcedureStmtRule> | ref<PullStmtRule> | ref<PushStmtRule> |
             ref<QueueStmtRule> | ref<ReturnStmtRule> | ref<SayStmtRule> |
             ref<SignalStmtRule> | ref<TraceStmtRule> | ref<UpperStmtRule>> {
  };

  struct Instruction
      : rule<ref<DoStmtRule> | ref<IfStmtRule> | ref<SelectStmtRule> |
             ref<AssignmentRule> | ref<KeywordInstructionRule> |
             ref<CommandStmtRule>> {};

  struct BlockBody
      : rule<create<ast::Block>(
            many(Delim | append<&ast::Block::body>(ref<LabelRule>) |
                 append<&ast::Block::body>(ref<IncludeRule>) |
                 append<&ast::Block::body>(ref<Instruction>)))> {};

  using entry = BlockBody;
  using ignore = list<WS, COMMENT, CONTINUATION>;
};

}  // namespace rexx
