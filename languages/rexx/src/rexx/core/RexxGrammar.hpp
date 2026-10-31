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

// The token includes adjacent comments and the opening parenthesis. Trivia
// cannot consume a comment that decides whether this is a function call.
inline constexpr auto function_head =
    [](std::string_view text, std::pmr::memory_resource* mem) {
      if (text.front() != '\'' && text.front() != '"')
        return text.substr(0, text.find_first_of("/("));
      const char quote = text.front();
      std::size_t end = 1;
      while (end < text.size()) {
        if (text[end++] != quote) continue;
        if (end < text.size() && text[end] == quote) { ++end; continue; }
        break;
      }
      return unquote_string(text.substr(0, end), mem);
    };

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
  static constexpr auto KWD_CASELESS = ikw<"CASELESS">;
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
  static constexpr auto KWD_LINEIN = ikw<"LINEIN">;
  static constexpr auto KWD_LOWER = ikw<"LOWER">;
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
  static constexpr auto NumberBody =
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<"+-">) + some(d));
  struct NUMBER : terminal<NumberBody> {};
  static constexpr auto Letter = cr<"A-Za-z_!?#@$">;
  static constexpr auto SymChar = cr<"A-Za-z_!?#@$0-9.">;
  // A nonnumeric constant symbol starts with a digit or dot. Exclude a
  // complete numeric token at its head, keeping 1e2 disjoint from NUMBER.
  struct CONST_SYMBOL
      : terminal<!(NumberBody + !SymChar) + cr<"0-9."> + many(SymChar)> {};
  struct VAR_SYMBOL : terminal<Letter + many(SymChar)> {};

  static constexpr auto BinaryBody = option(
      repeat<1, 4>(cr<"01">) + many(many(Blank) + repeat<4>(cr<"01">)));
  static constexpr auto HexBody = option(
      repeat<1, 2>(cr<"0-9A-Fa-f">) +
      many(many(Blank) + repeat<2>(cr<"0-9A-Fa-f">)));
  struct BINARY_STRING
      : terminal<((lit<"\""> + BinaryBody + lit<"\"">) |
                  (lit<"'"> + BinaryBody + lit<"'">)) + cr<"bB"> + !SymChar> {};
  struct HEX_STRING
      : terminal<((lit<"\""> + HexBody + lit<"\"">) |
                  (lit<"'"> + HexBody + lit<"'">)) + cr<"xX"> + !SymChar> {};

  // A string may not cross a line; its own quote, doubled, is the escape.
  static constexpr auto DqChar = lit<"\"\""> | cr<R"(^"\r\n)">;
  static constexpr auto SqChar = lit<"''"> | cr<R"(^'\r\n)">;
  static constexpr auto QuotedBody =
      (lit<"\""> + many(DqChar) + lit<"\"">) |
      (lit<"'"> + many(SqChar) + lit<"'">);
  struct STRING
      : terminal<as<unquote_string>(
            !(QuotedBody + cr<"xXbB"> + !SymChar) + QuotedBody)> {};

  struct CONCAT : terminal<lit<"||">> {};

  struct EQ : terminal<as<ast::Op::Eq>(lit<"=">)> {};
  struct PLUS : terminal<as<ast::Op::Add>(lit<"+">)> {};
  struct MINUS : terminal<as<ast::Op::Sub>(lit<"-">)> {};
  struct DIV : terminal<as<ast::Op::Div>(lit<"/">)> {};
  // `\`, `^`, `¬` negate; the fourth reference spelling (`/`) is unreachable
  // there (DIV is declared first at the same one-byte length) and is left
  // out.
  struct NOT_TOK : terminal<as<ast::Op::Not>(lit<"\\"> | lit<"^"> | lit<"~"> | lit<"¬">)> {
  };

  struct STOP : terminal<lit<".">> {};
  struct COMMA : terminal<lit<",">> {};
  struct COLON : terminal<lit<":">> {};
  struct SEMICOL : terminal<lit<";">> {};

  static constexpr auto Delim = ref<EOL> | ref<SEMICOL>;

  // ---- names ----
  struct VarRef
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

  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct BinaryLit
      : rule<assign<&ast::BinaryLit::text>(ref<BINARY_STRING>)> {};
  struct HexLit
      : rule<assign<&ast::HexLit::text>(ref<HEX_STRING>)> {
  };
  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            ref<BR_O> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            ref<BR_C>)> {};

  struct ArgSlot
      : rule<create<ast::Arg>(option(assign<&ast::Arg::value>(ref<Expression>)))> {
  };

  struct FilledArgSlot
      : rule<assign<&ast::Arg::value>(ref<Expression>)> {};

  // A lone slot is never empty: `f()` holds no `Arg`.
  template <auto Member>
  static constexpr auto ArgListInto =
      append<Member>(ref<ArgSlot>) +
          some(ref<COMMA> + append<Member>(ref<ArgSlot>))  //
      | append<Member>(ref<FilledArgSlot>);

  struct FUNCTION_OPEN
      : terminal<as<function_head>(FunctionNameExpr + many(CommentBody) + lit<"(">)> {};
  struct FunctionCall
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<FUNCTION_OPEN>) +
            option(ArgListInto<&ast::Call::args>) + ref<BR_C>)> {};

  struct Term
      : rule<ref<FunctionCall> | ref<ParenExpr> | ref<Symbol> |
             ref<BinaryLit> | ref<HexLit> | ref<StringLit>> {};

  struct PrefixExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(
                     ref<PLUS> | ref<MINUS> | ref<NOT_TOK>) +
                 assign<&ast::PrefixExpr::operand>(ref<PrefixExpr>)) |
             ref<Term>> {};

  struct SumLevel
      : infix<PrefixExpr, ast::BinExpr,
              levels(left(as<ast::Op::Pow>(lit<"**">)),
                     left(as<ast::Op::Mul>(lit<"*">) | ref<DIV> | as<ast::Op::IntDiv>(lit<"%">) | as<ast::Op::Mod>(lit<"//">)),
                     left(ref<PLUS> | ref<MINUS>))> {};

  // concatenation: the operator is not a token in two of its three
  // readings, so it is not spelled as an `infix<>` level.
  static constexpr auto GapEdgeBlank =
      gap::is(Blank + many(any)) | gap::is(many(any) + Blank);
  struct Concatenation
      : rule<ref<SumLevel> +
             many(nest<ast::Concat, &ast::Concat::left>(
                 (flag<&ast::Concat::explicitOp>(ref<CONCAT>) +
                  assign<&ast::Concat::right>(ref<SumLevel>)) |
                 ((flag<&ast::Concat::blank>(GapEdgeBlank) | !GapEdgeBlank) +
                  assign<&ast::Concat::right>(ref<SumLevel>))))> {};

  static constexpr auto CompareOp =
      ref<EQ> | as<ast::Op::Neq>((lit<"\\="> | lit<"^="> | lit<"~="> | lit<"¬=">)) | as<ast::Op::LtGt>(lit<"<>">) | as<ast::Op::GtLt>(lit<"><">) | as<ast::Op::GtEq>(lit<">=">) |
      as<ast::Op::LtEq>(lit<"<=">) | as<ast::Op::NGt>((lit<"\\>"> | lit<"^>"> | lit<"~>"> | lit<"¬>">)) | as<ast::Op::NLt>((lit<"\\<"> | lit<"^<"> | lit<"~<"> | lit<"¬<">)) | as<ast::Op::Gt>(lit<">">) | as<ast::Op::Lt>(lit<"<">) |
      as<ast::Op::SEq>(lit<"==">) | as<ast::Op::SNeq>((lit<"\\=="> | lit<"^=="> | lit<"~=="> | lit<"¬==">)) | as<ast::Op::SGtEq>(lit<">>=">) | as<ast::Op::SLtEq>(lit<"<<=">) |
      as<ast::Op::SNGt>((lit<"\\>>"> | lit<"^>>"> | lit<"~>>"> | lit<"¬>>">)) | as<ast::Op::SNLt>((lit<"\\<<"> | lit<"^<<"> | lit<"~<<"> | lit<"¬<<">)) | as<ast::Op::SGt>(lit<">>">) | as<ast::Op::SLt>(lit<"<<">);

  struct Expression
      : infix<Concatenation, ast::BinExpr,
              levels(left(CompareOp), left(as<ast::Op::And>(lit<"&">)),
                     left(as<ast::Op::Or>(lit<"|">) | as<ast::Op::Xor>(lit<"&&">)))> {};

  // ---- templates: ARG/PULL/PARSE's target_ and trigger_ ----
  struct TplVar
      : rule<create<ast::TplVar>(
            assign<&ast::TplVar::name>(ref<VAR_SYMBOL> | ref<SPECIAL_VAR>))> {
  };
  struct TplDrop : rule<create<ast::TplDrop>(ref<STOP>)> {};
  struct TplStringPattern
      : rule<assign<&ast::TplStringPattern::value>(ref<STRING>)> {};
  struct TplVarPattern
      : rule<create<ast::TplVarPattern>(
            ref<BR_O> +
            assign<&ast::TplVarPattern::name>(ref<VAR_SYMBOL> |
                                               ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};
  struct TplAbsPos
      : rule<create<ast::TplAbsPos>(
            option(ref<EQ>) + assign<&ast::TplAbsPos::number>(ref<NUMBER>))> {
  };
  struct TplAbsPosVar
      : rule<create<ast::TplAbsPosVar>(
            ref<EQ> + ref<BR_O> +
            assign<&ast::TplAbsPosVar::name>(ref<VAR_SYMBOL> |
                                              ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};
  struct TplRelPos
      : rule<create<ast::TplRelPos>(
            (flag<&ast::TplRelPos::minus>(ref<MINUS>) | ref<PLUS>) +
            assign<&ast::TplRelPos::number>(ref<NUMBER>))> {};
  struct TplRelPosVar
      : rule<create<ast::TplRelPosVar>(
            (flag<&ast::TplRelPosVar::minus>(ref<MINUS>) | ref<PLUS>) +
            ref<BR_O> +
            assign<&ast::TplRelPosVar::name>(ref<VAR_SYMBOL> |
                                              ref<SPECIAL_VAR>) +
            ref<BR_C>)> {};

  static constexpr auto TplItemExpr =
      ref<TplStringPattern> | ref<TplVarPattern> |
      ref<TplAbsPosVar> | ref<TplAbsPos> | ref<TplRelPosVar> |
      ref<TplRelPos> | ref<TplVar> | ref<TplDrop>;

  // template_list: COMMA* template_ (COMMA+ template_)* -- a slot may be
  // empty (the source it would read is skipped) between commas.
  struct TemplateSlot
      : rule<create<ast::Template>(many(append<&ast::Template::items>(
            TplItemExpr)))> {};

  // ---- statements ----
  struct Instruction;

  // An assignment never ends before a quote: the quote opens or continues
  // its value, a reading that always wins.
  struct Assignment
      : rule<assign<&ast::Assignment::target>(
                 ref<VAR_SYMBOL> | ref<SPECIAL_VAR> | ref<CONST_SYMBOL>) +
             ref<EQ> +
             option(assign<&ast::Assignment::value>(ref<Expression>)) +
             !cr<R"("')">> {};
  struct CommandStmt
      : rule<assign<&ast::CommandStmt::value>(ref<Expression>)> {};
  struct Label
      : rule<assign<&ast::Label::name>(ref<VAR_SYMBOL> | ref<CONST_SYMBOL> |
                                        ref<NUMBER> | ref<STRING>) +
             ref<COLON>> {};
  struct Include
      : rule<assign<&ast::IncludeStmt::text>(ref<STMT_INCLUDE>)> {};

  // A block: instructions, labels, includes and separators, freely mixed --
  // used for the top-level program and every do_/select_ body alike.
  struct BlockBody;

  // ---- do_ / if_ / select_ ----
  struct DoClause
      : rule<create<ast::DoClause>(
            assign<&ast::DoClause::keyword>(KWD_TO | KWD_BY | KWD_FOR) +
            assign<&ast::DoClause::value>(ref<Expression>))> {};
  static constexpr auto DoCnt =
      many(append<&ast::DoStmt::clauses>(ref<DoClause>));
  static constexpr auto DoRep =
      (assign<&ast::DoStmt::counter>(ref<Assignment>) + DoCnt) |
      flag<&ast::DoStmt::forever>(KWD_FOREVER) |
      assign<&ast::DoStmt::repeatExpr>(ref<Expression>);
  static constexpr auto DoCond =
      (flag<&ast::DoStmt::isWhile>(KWD_WHILE) +
       assign<&ast::DoStmt::condExpr>(ref<Expression>)) |
      (flag<&ast::DoStmt::isUntil>(KWD_UNTIL) +
       assign<&ast::DoStmt::condExpr>(ref<Expression>));

  // A header needs at least one delimiter before its first instruction:
  // `DO` glued straight to what follows is refused.
  struct DoStmt
      : rule<create<ast::DoStmt>(
            KWD_DO + option(DoRep) + option(DoCond) + some(Delim) +
            assign<&ast::DoStmt::body>(ref<BlockBody>) + KWD_END +
            option(assign<&ast::DoStmt::endName>(ref<VAR_SYMBOL> |
                                                  ref<SPECIAL_VAR>)))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            KWD_IF + assign<&ast::IfStmt::cond>(ref<Expression>) +
            many(Delim) + KWD_THEN + many(Delim) +
            assign<&ast::IfStmt::thenBranch>(ref<Instruction>) +
            option(many(Delim) + KWD_ELSE + many(Delim) +
                   assign<&ast::IfStmt::elseBranch>(ref<Instruction>)))> {};

  struct When
      : rule<create<ast::WhenClause>(
            KWD_WHEN + assign<&ast::WhenClause::cond>(ref<Expression>) +
            many(Delim) + KWD_THEN + many(Delim) +
            assign<&ast::WhenClause::body>(ref<Instruction>))> {};
  // A bare `ref<Instruction>` (then_/else_/when_'s single action) does not
  // sit inside a block loop, so nothing else consumes the delimiter that
  // follows it: each step here does, the way BlockBody's own loop does.
  struct SelectStmt
      : rule<create<ast::SelectStmt>(
            KWD_SELECT + some(Delim) +
            some(append<&ast::SelectStmt::whens>(ref<When>) +
                 many(Delim)) +
            option(flag<&ast::SelectStmt::hasOtherwise>(KWD_OTHERWISE) +
                   many(Delim) +
                   assign<&ast::SelectStmt::otherwiseBody>(ref<BlockBody>)) +
            KWD_END)> {};

  // ---- keyword_instruction ----
  struct AddressStmt
      : rule<create<ast::AddressStmt>(
            KWD_ADDRESS +
            option((assign<&ast::AddressStmt::envName>(TakenConstantExpr) +
                    option(assign<&ast::AddressStmt::command>(
                        ref<Expression>))) |
                   (KWD_VALUE +
                    assign<&ast::AddressStmt::envValue>(ref<Expression>))))> {
  };

  struct ArgStmt
      : rule<create<ast::ArgStmt>(
            KWD_ARG +
            option(append<&ast::ArgStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> +
                        append<&ast::ArgStmt::templates>(ref<TemplateSlot>))))> {
  };

  static constexpr auto CallArgs = ArgListInto<&ast::CallStmt::args>;
  struct CallStmt
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

  struct DropStmt
      : rule<create<ast::DropStmt>(KWD_DROP +
            some(append<&ast::DropStmt::vars>(ref<VarRef>)))> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt>(
            KWD_EXIT + option(assign<&ast::ExitStmt::value>(ref<Expression>)))> {
  };
  struct InterpretStmt
      : rule<KWD_INTERPRET +
             assign<&ast::InterpretStmt::value>(ref<Expression>)> {};
  struct IterateStmt
      : rule<create<ast::IterateStmt>(
            KWD_ITERATE +
            option(assign<&ast::IterateStmt::label>(ref<VAR_SYMBOL> |
                                                     ref<SPECIAL_VAR>)))> {};
  struct LeaveStmt
      : rule<create<ast::LeaveStmt>(
            KWD_LEAVE +
            option(assign<&ast::LeaveStmt::label>(ref<VAR_SYMBOL> |
                                                   ref<SPECIAL_VAR>)))> {};
  struct NopStmt : rule<create<ast::NopStmt> + KWD_NOP> {};

  struct NumericStmt
      : rule<create<ast::NumericStmt>(
            KWD_NUMERIC +
            ((assign<&ast::NumericStmt::kind>(as<ast::NumericKind::Digits>(KWD_DIGITS)) +
              option(assign<&ast::NumericStmt::amount>(ref<Expression>))) |
             (assign<&ast::NumericStmt::kind>(as<ast::NumericKind::Fuzz>(KWD_FUZZ)) +
              option(assign<&ast::NumericStmt::amount>(ref<Expression>))) |
             (assign<&ast::NumericStmt::kind>(as<ast::NumericKind::Form>(KWD_FORM)) +
              option(flag<&ast::NumericStmt::engineering>(
                         KWD_ENGINEERING) |
                     flag<&ast::NumericStmt::scientific>(
                         KWD_SCIENTIFIC) |
                     (KWD_VALUE + assign<&ast::NumericStmt::formValue>(
                                      ref<Expression>)) |
                     assign<&ast::NumericStmt::formValue>(
                         ref<Expression>)))))> {};

  struct OptionsStmt
      : rule<create<ast::OptionsStmt>(KWD_OPTIONS + option(assign<&ast::OptionsStmt::value>(ref<Expression>)))> {
  };

  static constexpr auto ParseCase =
      flag<&ast::ParseStmt::upper>(KWD_UPPER) | flag<&ast::ParseStmt::lower>(KWD_LOWER);
  static constexpr auto ParseCaseless = flag<&ast::ParseStmt::caseless>(KWD_CASELESS);
  static constexpr auto ParseFlags =
      (ParseCase + option(ParseCaseless)) | (ParseCaseless + option(ParseCase));

  struct ParseStmt
      : rule<create<ast::ParseStmt>(
            KWD_PARSE +
            option(ParseFlags) +
            ((assign<&ast::ParseStmt::source>(
                 as<ast::ParseSource::Arg>(KWD_ARG) | as<ast::ParseSource::External>(KWD_EXTERNAL) |
                 as<ast::ParseSource::Numeric>(KWD_NUMERIC) | as<ast::ParseSource::Pull>(KWD_PULL) |
                 as<ast::ParseSource::Source>(KWD_SOURCE) | as<ast::ParseSource::Version>(KWD_VERSION) |
                 as<ast::ParseSource::Linein>(KWD_LINEIN))) |
             (assign<&ast::ParseStmt::source>(as<ast::ParseSource::Value>(KWD_VALUE)) +
              option(assign<&ast::ParseStmt::valueExpr>(ref<Expression>)) +
              KWD_WITH) |
             (assign<&ast::ParseStmt::source>(as<ast::ParseSource::Var>(KWD_VAR)) +
              assign<&ast::ParseStmt::varName>(ref<VAR_SYMBOL> |
                                                ref<SPECIAL_VAR>))) +
            option(append<&ast::ParseStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> + append<&ast::ParseStmt::templates>(
                                         ref<TemplateSlot>))))> {};

  struct ProcedureStmt
      : rule<create<ast::ProcedureStmt>(
            KWD_PROCEDURE +
            option(flag<&ast::ProcedureStmt::expose>(KWD_EXPOSE) +
                   some(append<&ast::ProcedureStmt::exposed>(
                       ref<VarRef>))))> {};

  struct PullStmt
      : rule<create<ast::PullStmt>(
            KWD_PULL +
            option(append<&ast::PullStmt::templates>(ref<TemplateSlot>) +
                   many(ref<COMMA> + append<&ast::PullStmt::templates>(
                                         ref<TemplateSlot>))))> {};

  struct PushStmt
      : rule<create<ast::PushStmt>(
            KWD_PUSH + option(assign<&ast::PushStmt::value>(ref<Expression>)))> {
  };
  struct QueueStmt
      : rule<create<ast::QueueStmt>(
            KWD_QUEUE +
            option(assign<&ast::QueueStmt::value>(ref<Expression>)))> {};
  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            KWD_RETURN +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)))> {};
  struct SayStmt
      : rule<create<ast::SayStmt>(
            KWD_SAY + option(assign<&ast::SayStmt::value>(ref<Expression>)))> {
  };

  struct SignalStmt
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

  struct TraceStmt
      : rule<create<ast::TraceStmt>(
            KWD_TRACE +
            option(assign<&ast::TraceStmt::constant>(TakenConstantExpr) |
                   (KWD_VALUE +
                    assign<&ast::TraceStmt::value>(ref<Expression>)) |
                   assign<&ast::TraceStmt::value>(ref<Expression>) |
                   assign<&ast::TraceStmt::constant>(KWD_ERROR) |
                   assign<&ast::TraceStmt::constant>(KWD_FAILURE) |
                   assign<&ast::TraceStmt::constant>(KWD_OFF)))> {};

  struct UpperStmt
      : rule<create<ast::UpperStmt>(
            KWD_UPPER + some(append<&ast::UpperStmt::vars>(ref<VarRef>)))> {
  };

  struct KeywordInstruction
      : rule<ref<AddressStmt> | ref<ArgStmt> | ref<CallStmt> |
             ref<DropStmt> | ref<ExitStmt> | ref<InterpretStmt> |
             ref<IterateStmt> | ref<LeaveStmt> | ref<NopStmt> |
             ref<NumericStmt> | ref<OptionsStmt> | ref<ParseStmt> |
             ref<ProcedureStmt> | ref<PullStmt> | ref<PushStmt> |
             ref<QueueStmt> | ref<ReturnStmt> | ref<SayStmt> |
             ref<SignalStmt> | ref<TraceStmt> | ref<UpperStmt>> {
  };

  struct Instruction
      : rule<ref<DoStmt> | ref<IfStmt> | ref<SelectStmt> |
             ref<Assignment> | ref<KeywordInstruction> |
             ref<CommandStmt>> {};

  struct BlockBody
      : rule<create<ast::Block>(
            many(Delim | append<&ast::Block::body>(ref<Label>) |
                 append<&ast::Block::body>(ref<Include>) |
                 append<&ast::Block::body>(ref<Instruction>)))> {};

  using entry = BlockBody;
  using ignored = list<WS, COMMENT, CONTINUATION>;
};

}  // namespace rexx
