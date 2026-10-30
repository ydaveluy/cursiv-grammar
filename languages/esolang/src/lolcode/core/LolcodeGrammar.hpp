// The lolcode language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lolcode/core/ast.hpp"

namespace lolcode {

inline constexpr auto win_or_fail = [](std::string_view t) {
  return t == "WIN";
};

inline constexpr auto unquote = [](std::string_view t) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.size() < 2 || t.front() != '"' || t.back() != '"')
    return Result::err("A string literal needs opening and closing quotes.");
  return Result::ok(t.substr(1, t.size() - 2));
};

struct Lolcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct SEPARATOR : terminal<eol | lit<",">> {};
  struct CONTINUATION : terminal<(lit<"..."> | lit<"…">) + many(cr<R"( \t)">) + eol> {};

  // An identifier must start with a letter or underscore, so a pure-digit
  // spelling stays disjoint from NUMBER.
  struct LABEL : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};

  static constexpr auto number_body = option(lit<"-">) +
      ((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d)));
  static constexpr auto bool_body = kw<"WIN"> | kw<"FAIL">;

  struct NUMBER : terminal<as<double>(number_body)> {};

  struct BOOL : terminal<as<win_or_fail>(bool_body)> {};
  struct NOOBK : terminal<kw<"NOOB">> {};

  // LOLCODE uses colon escapes. Keep their source spelling for later
  // interpolation; a backslash itself is an ordinary string character.
  static constexpr auto string_body = lit<"\""> +
      many((lit<":"> + any) | cr<R"(^":)">) + lit<"\"">;
  struct STRING : terminal<as<unquote>(string_body)> {};

  struct LINE_COMMENT : terminal<kw<"BTW"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(kw<"OBTW">, lit<"TLDR">)> {};

  // Binary operators have two operands and an optional AN separator.
  struct DIFFRINT : terminal<as<ast::Op::ne>(kw<"DIFFRINT">)> {};

  static constexpr auto binOp =
      as<ast::Op::eq>((kw<"BOTH"> + kw<"SAEM">)) | ref<DIFFRINT> | as<ast::Op::both>((kw<"BOTH"> + kw<"OF">)) | as<ast::Op::either>((kw<"EITHER"> + kw<"OF">)) | as<ast::Op::xor_>((kw<"WON"> + kw<"OF">)) |
      as<ast::Op::greater>((kw<"BIGGR"> + kw<"OF">)) | as<ast::Op::less>((kw<"SMALLR"> + kw<"OF">)) | as<ast::Op::add>((kw<"SUM"> + kw<"OF">)) | as<ast::Op::sub>((kw<"DIFF"> + kw<"OF">)) |
      as<ast::Op::mul>((kw<"PRODUKT"> + kw<"OF">)) | as<ast::Op::div>((kw<"QUOSHUNT"> + kw<"OF">)) | as<ast::Op::mod>((kw<"MOD"> + kw<"OF">));

  struct BoolLit : rule<assign<&ast::BoolLit::value>(ref<BOOL>)> {};
  struct NoobLit : rule<create<ast::NoobLit> + ref<NOOBK>> {};
  struct NumLit : rule<assign<&ast::NumLit::value>(ref<NUMBER>)> {};
  struct StrLit : rule<assign<&ast::StrLit::text>(ref<STRING>)> {};

  static constexpr auto atom =
      ref<BoolLit> | ref<NoobLit> | ref<NumLit> | ref<StrLit>;

  struct BinOp;
  struct Cast;
  struct AllOf;
  struct AnyOf;
  struct Not;
  struct FuncCall;
  struct ExplicitCall;
  struct VarRef;
  struct Concat;

  // A call's own argument list has no separator between names: were it
  // reachable from a general operand position, a run of names anywhere in
  // an expression could extend into a call ending at any later MKAY?,
  // ambiguous with the shorter reading that stops at one name.
  // FuncCall is therefore reachable only from `statement`, never from
  // `expression`; a call cannot be an operand or another call's argument.
  static constexpr auto expression =
      ref<BinOp> | ref<Cast> | ref<AllOf> | ref<AnyOf> |
      ref<Not> | ref<Concat> | ref<ExplicitCall> | ref<VarRef> | atom;

  struct BinOp : rule<assign<&ast::BinOp::op>(binOp) +
                          assign<&ast::BinOp::left>(expression) + option(kw<"AN">) +
                          assign<&ast::BinOp::right>(expression)> {};

  struct Cast
      : rule<kw<"MAEK"> + assign<&ast::CastExpr::value>(expression) + option(kw<"A">) +
             assign<&ast::CastExpr::targetType>(ref<LABEL>)> {};

  struct AllOf
      : rule<(kw<"ALL"> + kw<"OF">) +
             append<&ast::AllOf::operands>(expression) +
             many(option(kw<"AN">) + append<&ast::AllOf::operands>(expression)) +
             (kw<"MKAY"> | kw<"MKAY?"> | &ref<SEPARATOR> | &ref<LINE_COMMENT>)> {};

  struct AnyOf
      : rule<(kw<"ANY"> + kw<"OF">) +
             append<&ast::AnyOf::operands>(expression) +
             many(option(kw<"AN">) + append<&ast::AnyOf::operands>(expression)) +
             (kw<"MKAY"> | kw<"MKAY?"> | &ref<SEPARATOR> | &ref<LINE_COMMENT>)> {};

  struct Concat
      : rule<kw<"SMOOSH"> + append<&ast::ConcatExpr::operands>(expression) +
             many(option(kw<"AN">) + append<&ast::ConcatExpr::operands>(expression)) +
             (kw<"MKAY"> | &ref<SEPARATOR> | &ref<LINE_COMMENT>)> {};

  struct Not
      : rule<kw<"NOT"> + assign<&ast::NotExpr::operand>(expression)> {};

  struct ExplicitCall
      : rule<create<ast::FuncCall>(
            (kw<"I"> + kw<"IZ">) + assign<&ast::FuncCall::callee>(ref<LABEL>) +
            option(
                kw<"YR"> + append<&ast::FuncCall::args>(expression) +
                many((kw<"AN"> + kw<"YR">) + append<&ast::FuncCall::args>(expression))) +
            (kw<"MKAY"> | kw<"MKAY?">))> {};

  struct FuncCall
      : rule<assign<&ast::FuncCall::callee>(ref<LABEL>) +
             some(append<&ast::FuncCall::args>(expression)) + kw<"MKAY?">> {};

  struct VarRef : rule<assign<&ast::VarRef::name>(ref<LABEL>)> {};

  struct Comment : rule<assign<&ast::Comment::text>(ref<LINE_COMMENT> |
                                                        ref<BLOCK_COMMENT>)> {};

  struct Statement;
  struct StatementLine;
  static constexpr auto statement = ref<StatementLine>;
  static constexpr auto separator = option(ref<LINE_COMMENT>) + some(ref<SEPARATOR>);

  // The one label token stays raw: its quotes and numeric spelling matter
  // to literal identity, independently of the ordinary expression converters.
  struct CaseLiteral
      : terminal<bool_body | number_body | string_body> {};

  template <auto Body>
  static constexpr auto case_body = many(
      append<Body>(!(kw<"OMG"> | kw<"OMGWTF"> | kw<"OIC">) + statement) |
      ref<SEPARATOR>);

  struct CaseClause
      : rule<kw<"OMG"> + assign<&ast::CaseClause::literal>(ref<CaseLiteral>) +
             separator + case_body<&ast::CaseClause::body>> {};

  struct Switch
      : rule<kw<"WTF?"> + separator +
             some(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) +
             option(flag<&ast::SwitchStmt::hasDefault>(kw<"OMGWTF">) + separator +
                    case_body<&ast::SwitchStmt::defaultBody>) +
             kw<"OIC">> {};

  struct Loop
      : rule<(kw<"IM"> + kw<"IN"> + kw<"YR">) + assign<&ast::LoopStmt::label>(ref<LABEL>) +
             option(assign<&ast::LoopStmt::operation>(kw<"UPPIN"> | kw<"NERFIN"> | ref<LABEL>) +
                    kw<"YR"> + assign<&ast::LoopStmt::iterator>(ref<LABEL>)) +
             option((flag<&ast::LoopStmt::until>(kw<"TIL">) | kw<"WILE">) +
                    assign<&ast::LoopStmt::condition>(expression)) + separator +
             many(append<&ast::LoopStmt::body>(statement) | ref<SEPARATOR>) + (kw<"IM"> + kw<"OUTTA"> + kw<"YR">) +
             assign<&ast::LoopStmt::endLabel>(ref<LABEL>)> {};

  struct Decl
      : rule<(kw<"I"> + kw<"HAS"> + kw<"A">) + assign<&ast::Declaration::name>(ref<LABEL>) +
             option(kw<"ITZ"> + assign<&ast::Declaration::init>(expression))> {
  };

  struct PrintBlock
      : rule<kw<"VISIBLE"> + many(append<&ast::PrintBlock::items>(expression)) +
             option(flag<&ast::PrintBlock::suppressNewline>(lit<"!">)) +
             option(kw<"MKAY?">)> {};

  struct MebbeClause;
  struct NoWaiClause;
  static constexpr auto elseClause =
      ref<MebbeClause> | ref<NoWaiClause>;

  struct MebbeClause
      : rule<kw<"MEBBE"> + assign<&ast::MebbeClause::cond>(expression) + separator +
             many(append<&ast::MebbeClause::body>(statement) | ref<SEPARATOR>) +
             option(assign<&ast::MebbeClause::next>(elseClause))> {};

  struct NoWaiClause
      : rule<(kw<"NO"> + kw<"WAI">) + separator + many(append<&ast::NoWaiClause::body>(statement) | ref<SEPARATOR>)> {
  };

  struct IfBlock
      : rule<(kw<"O"> + kw<"RLY?">) + separator + (kw<"YA"> + kw<"RLY">) + separator +
             many(append<&ast::IfBlock::body>(statement) | ref<SEPARATOR>) +
             option(assign<&ast::IfBlock::elseChain>(elseClause)) + kw<"OIC">> {
  };

  struct InputBlock
      : rule<kw<"GIMMEH"> + assign<&ast::InputBlock::name>(ref<LABEL>)> {};

  struct FuncDecl
      : rule<((kw<"HOW"> + kw<"IZ"> + kw<"I">) | (kw<"HOW"> + kw<"DUZ"> + kw<"I">)) + assign<&ast::FuncDecl::name>(ref<LABEL>) +
             option(kw<"YR"> + append<&ast::FuncDecl::params>(ref<LABEL>) +
                    many((kw<"AN"> + kw<"YR">) +
                         append<&ast::FuncDecl::params>(ref<LABEL>))) +
             separator + many(append<&ast::FuncDecl::body>(statement) | ref<SEPARATOR>) +
             (kw<"IF"> + kw<"U"> + kw<"SAY"> + kw<"SO">)> {};

  struct Assign
      : rule<assign<&ast::Assignment::name>(ref<LABEL>) + kw<"R"> +
             assign<&ast::Assignment::value>(expression)> {};

  struct Retype
      : rule<assign<&ast::RetypeStmt::name>(ref<LABEL>) + (kw<"IS"> + kw<"NOW"> + kw<"A">) +
             assign<&ast::RetypeStmt::targetType>(ref<LABEL>)> {};

  struct Return
      : rule<(kw<"FOUND"> + kw<"YR">) + assign<&ast::ReturnStmt::value>(expression)> {};

  struct Break : rule<create<ast::BreakStmt>(kw<"GTFO">)> {};

  struct Statement
      : rule<ref<Return> | ref<Break> | ref<Retype> | ref<Loop> | ref<Decl> | ref<Comment> |
             ref<PrintBlock> | ref<IfBlock> | ref<Switch> | ref<InputBlock> |
             ref<FuncDecl> | ref<Assign> | ref<FuncCall> |
             expression> {};

  struct StatementLine : rule<ref<Statement> + (separator | eof)> {};

  struct Program
      : rule<create<ast::Program> + many(ref<SEPARATOR>) + kw<"HAI"> +
             option(assign<&ast::Program::version>(ref<NumLit>)) + separator +
             many(append<&ast::Program::statements>(statement) | ref<SEPARATOR>) +
             flag<&ast::Program::hasKthxbye>(kw<"KTHXBYE">) + many(ref<SEPARATOR>)> {};

  using entry = Program;
  using ignored = list<WS, CONTINUATION>;
};

} // namespace lolcode
