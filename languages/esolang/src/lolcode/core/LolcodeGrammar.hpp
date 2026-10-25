// The lolcode language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lolcode/core/ast.hpp"

namespace lolcode {

inline constexpr auto win_or_fail = [](std::string_view t) {
  return t == "WIN";
};
using WinFail = cursiv::text_conv<win_or_fail>;

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Lolcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n)">)> {};

  // An identifier must start with a letter or underscore, so a pure-digit
  // spelling stays disjoint from NUMBER.
  struct LABEL : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};

  struct NUMBER : terminal<some(cr<"0-9">) |
                               (many(cr<"0-9">) + lit<"."> + many(cr<"0-9">)),
                           double> {};

  struct BOOL : terminal<kw<"WIN"> | kw<"FAIL">, WinFail> {};
  struct NOOBK : terminal<kw<"NOOB">> {};

  struct STRING
      : terminal<lit<"\""> + many(lit<"\\\""> | cr<R"(^"\\)">) + lit<"\"">,
                 Unquoted> {};

  struct LINE_COMMENT : terminal<kw<"BTW"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(kw<"OBTW">, lit<"TLDR">)> {};

  // The eleven prefix binary operators share one shape: OP left 'AN' right.
  struct BOTH_SAEM : terminal<kw<"BOTH SAEM">, value<ast::Op::eq>> {};
  struct DIFFRINT : terminal<kw<"DIFFRINT">, value<ast::Op::ne>> {};
  struct BOTH_OF : terminal<kw<"BOTH OF">, value<ast::Op::both>> {};
  struct EITHER_OF : terminal<kw<"EITHER OF">, value<ast::Op::either>> {};
  struct BIGGR_OF : terminal<kw<"BIGGR OF">, value<ast::Op::greater>> {};
  struct SMALLR_OF : terminal<kw<"SMALLR OF">, value<ast::Op::less>> {};
  struct SUM_OF : terminal<kw<"SUM OF">, value<ast::Op::add>> {};
  struct DIFF_OF : terminal<kw<"DIFF OF">, value<ast::Op::sub>> {};
  struct PRODUKT_OF : terminal<kw<"PRODUKT OF">, value<ast::Op::mul>> {};
  struct QUOSHUNT_OF : terminal<kw<"QUOSHUNT OF">, value<ast::Op::div>> {};
  struct MOD_OF : terminal<kw<"MOD OF">, value<ast::Op::mod>> {};

  static constexpr auto binOp =
      ref<BOTH_SAEM> | ref<DIFFRINT> | ref<BOTH_OF> | ref<EITHER_OF> |
      ref<BIGGR_OF> | ref<SMALLR_OF> | ref<SUM_OF> | ref<DIFF_OF> |
      ref<PRODUKT_OF> | ref<QUOSHUNT_OF> | ref<MOD_OF>;

  struct BoolLitRule : rule<assign<&ast::BoolLit::value>(ref<BOOL>)> {};
  struct NoobLitRule : rule<create<ast::NoobLit> + ref<NOOBK>> {};
  struct NumLitRule : rule<assign<&ast::NumLit::value>(ref<NUMBER>)> {};
  struct StrLitRule : rule<assign<&ast::StrLit::text>(ref<STRING>)> {};

  static constexpr auto atom =
      ref<BoolLitRule> | ref<NoobLitRule> | ref<NumLitRule> | ref<StrLitRule>;

  struct BinOpRule;
  struct CastRule;
  struct AllOfRule;
  struct AnyOfRule;
  struct NotRule;
  struct FuncCallRule;
  struct ExplicitCallRule;
  struct VarRefRule;

  // A call's own argument list has no separator between names: were it
  // reachable from a general operand position, a run of names anywhere in
  // an expression could extend into a call ending at any later MKAY?,
  // ambiguous with the shorter reading that stops at one name.
  // FuncCallRule is therefore reachable only from `statement`, never from
  // `expression`; a call cannot be an operand or another call's argument.
  static constexpr auto expression =
      ref<BinOpRule> | ref<CastRule> | ref<AllOfRule> | ref<AnyOfRule> |
      ref<NotRule> | ref<ExplicitCallRule> | ref<VarRefRule> | atom;

  struct BinOpRule : rule<assign<&ast::BinOp::op>(binOp) +
                          assign<&ast::BinOp::left>(expression) + kw<"AN"> +
                          assign<&ast::BinOp::right>(expression)> {};

  struct CastRule
      : rule<kw<"MAEK"> + assign<&ast::CastExpr::value>(expression) + kw<"A"> +
             assign<&ast::CastExpr::targetType>(ref<LABEL>)> {};

  struct AllOfRule
      : rule<kw<"ALL OF"> +
             some(append<&ast::AllOf::operands>(expression), kw<"AN">) +
             kw<"MKAY?">> {};

  struct AnyOfRule
      : rule<kw<"ANY OF"> +
             some(append<&ast::AnyOf::operands>(expression), kw<"AN">) +
             kw<"MKAY?">> {};

  struct NotRule
      : rule<kw<"NOT"> + assign<&ast::NotExpr::operand>(expression)> {};

  struct ExplicitCallRule
      : rule<create<ast::FuncCall>(
            kw<"I IZ"> + assign<&ast::FuncCall::callee>(ref<LABEL>) +
            option(
                kw<"YR"> + append<&ast::FuncCall::args>(expression) +
                many(kw<"AN YR"> + append<&ast::FuncCall::args>(expression))) +
            (kw<"MKAY"> | kw<"MKAY?">))> {};

  struct FuncCallRule
      : rule<assign<&ast::FuncCall::callee>(ref<LABEL>) +
             some(append<&ast::FuncCall::args>(expression)) + kw<"MKAY?">> {};

  struct VarRefRule : rule<assign<&ast::VarRef::name>(ref<LABEL>)> {};

  struct CommentRule : rule<assign<&ast::Comment::text>(ref<LINE_COMMENT> |
                                                        ref<BLOCK_COMMENT>)> {};

  struct StatementRule;
  static constexpr auto statement = ref<StatementRule>;

  struct LoopRule
      : rule<kw<"IM IN YR"> + assign<&ast::LoopStmt::label>(ref<LABEL>) +
             kw<"WILE"> + assign<&ast::LoopStmt::condition>(expression) +
             some(append<&ast::LoopStmt::body>(statement)) + kw<"IM OUTTA YR"> +
             assign<&ast::LoopStmt::endLabel>(ref<LABEL>)> {};

  struct DeclRule
      : rule<kw<"I HAS A"> + assign<&ast::Declaration::name>(ref<LABEL>) +
             option(kw<"ITZ"> + assign<&ast::Declaration::init>(expression))> {
  };

  struct PrintBlockRule
      : rule<kw<"VISIBLE"> + many(append<&ast::PrintBlock::items>(expression)) +
             option(kw<"MKAY?">)> {};

  struct MebbeClauseRule;
  struct NoWaiClauseRule;
  static constexpr auto elseClause =
      ref<MebbeClauseRule> | ref<NoWaiClauseRule>;

  struct MebbeClauseRule
      : rule<kw<"MEBBE"> + assign<&ast::MebbeClause::cond>(expression) +
             some(append<&ast::MebbeClause::body>(statement)) +
             option(assign<&ast::MebbeClause::next>(elseClause))> {};

  struct NoWaiClauseRule
      : rule<kw<"NO WAI"> + some(append<&ast::NoWaiClause::body>(statement))> {
  };

  struct IfBlockRule
      : rule<kw<"O RLY?"> + kw<"YA RLY"> +
             some(append<&ast::IfBlock::body>(statement)) +
             option(assign<&ast::IfBlock::elseChain>(elseClause)) + kw<"OIC">> {
  };

  struct InputBlockRule
      : rule<kw<"GIMMEH"> + assign<&ast::InputBlock::name>(ref<LABEL>)> {};

  struct FuncDeclRule
      : rule<kw<"HOW DUZ I"> + assign<&ast::FuncDecl::name>(ref<LABEL>) +
             option(kw<"YR"> + append<&ast::FuncDecl::params>(ref<LABEL>) +
                    many(kw<"AN YR"> +
                         append<&ast::FuncDecl::params>(ref<LABEL>))) +
             some(append<&ast::FuncDecl::body>(statement)) +
             kw<"IF U SAY SO">> {};

  struct AssignRule
      : rule<assign<&ast::Assignment::name>(ref<LABEL>) + kw<"R"> +
             assign<&ast::Assignment::value>(expression)> {};

  struct StatementRule
      : rule<ref<LoopRule> | ref<DeclRule> | ref<CommentRule> |
             ref<PrintBlockRule> | ref<IfBlockRule> | ref<InputBlockRule> |
             ref<FuncDeclRule> | ref<AssignRule> | ref<FuncCallRule> |
             expression> {};

  // The version after HAI (e.g. "HAI 1.3") is read as an optional atom.
  struct ProgramRule
      : rule<kw<"HAI"> + option(assign<&ast::Program::version>(atom)) +
             some(append<&ast::Program::statements>(statement)) +
             option(flag<&ast::Program::hasKthxbye>(kw<"KTHXBYE">))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

} // namespace lolcode
