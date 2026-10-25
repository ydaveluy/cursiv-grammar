#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "elixir/core/ast.hpp"

namespace elixir {

inline constexpr auto dropFirst = [](std::string_view t) { return t.substr(1); };
using DropFirst = cursiv::text_conv<dropFirst>;

struct Elixir : cursiv::GrammarSpec {
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct NL : terminal<lit<"\r\n"> | lit<"\n"> | lit<"\r">> {};
  struct INTERP_OPEN : terminal<lit<"#{"> + many(s | ref<COMMENT>)> {};
  struct SPACES : terminal<some(cr<R"( \t)"> | (lit<"\\"> + (lit<"\r\n"> | lit<"\n">)))> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_?!"> | uc::L | uc::Nl | uc::Mn | uc::Mc | uc::Nd | uc::Pc;

  struct TRUE : terminal<kw<"true">> {};
  struct FALSE : terminal<kw<"false">> {};
  struct NIL : terminal<kw<"nil">> {};
  struct WHEN : terminal<kw<"when">> {};
  struct AND : terminal<kw<"and">> {};
  struct OR : terminal<kw<"or">> {};
  struct NOT : terminal<kw<"not">> {};
  struct IN : terminal<kw<"in">> {};
  struct FN : terminal<kw<"fn">> {};
  struct DO : terminal<kw<"do">> {};
  struct END : terminal<kw<"end">> {};
  struct CATCH : terminal<kw<"catch">> {};
  struct RESCUE : terminal<kw<"rescue">> {};
  struct AFTER : terminal<kw<"after">> {};
  struct ELSE : terminal<kw<"else">> {};
  static constexpr auto EscapeSeq = lit<"\\"> + any;

  static constexpr auto AtomOperatorTok =
      lit<"**"> | lit<"^^^"> | lit<"..."> | lit<"@"> | lit<"."> | lit<"!"> | lit<"^"> | lit<"---"> | lit<"*"> |
      lit<"/"> | lit<"+"> | lit<"-"> | lit<"++"> | lit<"--"> | lit<"+++"> |
      lit<".."> | lit<"<>"> | lit<"|>"> | lit<"<<<"> | lit<">>>"> |
      lit<"<<~"> | lit<"~>>"> | lit<"<~"> | lit<"~>"> | lit<"<~>"> |
      lit<"<|>"> | lit<"<"> | lit<">"> | lit<"<="> | lit<">="> | lit<"=="> |
      lit<"!="> | lit<"=~"> | lit<"==="> | lit<"!=="> | lit<"&&"> |
      lit<"&&&"> | lit<"||"> | lit<"|||"> | lit<"="> | lit<"&"> | lit<"|"> |
      lit<"::"> | lit<"<-"> | lit<"->"> | lit<"\\\\">;

  struct ATOM
      : terminal<lit<":"> + (((uc::L | cr<"_">) + many(uc::L | uc::Nl | uc::Mn | uc::Mc | uc::Nd | uc::Pc | cr<"_0-9@">) +
                                  option(cr<"!?">)) |
                              AtomOperatorTok),
                 DropFirst> {};

  struct CODEPOINT : terminal<lit<"?"> +
      ((lit<"\\"> + any) | (!lit<"\\"> + any)), DropFirst> {};

  struct HEXADECIMAL
      : terminal<lit<"0x"> + some(cr<"0-9a-fA-F">) +
                 many(lit<"_"> + some(cr<"0-9a-fA-F">))> {};
  struct OCTAL : terminal<lit<"0o"> + some(cr<"0-7">) +
                          many(lit<"_"> + some(cr<"0-7">))> {};
  struct BINARY : terminal<lit<"0b"> + some(cr<"01">) +
                           many(lit<"_"> + some(cr<"01">))> {};
  struct INTEGER : terminal<some(d) + many(lit<"_"> + some(d))> {};
  static constexpr auto DecimalDigits = some(d) + many(lit<"_"> + some(d));
  struct FLOAT : terminal<DecimalDigits + lit<"."> + DecimalDigits +
      option(cr<"eE"> + option(cr<"+-">) + DecimalDigits)> {};

  struct ALIAS : terminal<cr<"A-Z"> + many(uc::L | uc::Nl | uc::Mn | uc::Mc | uc::Nd | uc::Pc | cr<"_0-9">)> {};
  struct VARIABLE
      : terminal<(uc::Ll | uc::Lm | uc::Lo | uc::Nl | cr<"_">) + many(uc::L | uc::Nl | uc::Mn | uc::Mc | uc::Nd | uc::Pc | cr<"_0-9">) +
                 option(cr<"!?">)> {};

  static constexpr auto NLs = many(ref<NL>);
  static constexpr auto Comma = lit<","> + NLs;
  static constexpr auto Eoe = some(lit<";"> | ref<NL>);

  struct Expression;
  struct NoDoExpression;
  struct BlockRule;
  struct ClauseBlockRule;
  struct InfixHigh;
  struct Membership;
  struct SectionRule;
  struct ClauseBody;
  struct AmpWrapExpr;
  struct InfixLow1;
  struct UnaryOpExpr;
  struct PostfixExpr;
  struct NamedPrimary;
  static constexpr auto Reserved = ref<TRUE> | ref<FALSE> | ref<NIL> | ref<WHEN> |
      ref<AND> | ref<OR> | ref<NOT> | ref<IN> | ref<FN> | ref<DO> | ref<END> |
      ref<CATCH> | ref<RESCUE> | ref<AFTER> | ref<ELSE>;
  struct VariableName : rule<!Reserved + ref<VARIABLE>, text_conv<[](std::string_view s) { return s; }>> {};
  static constexpr auto KeywordNameTok = ref<VARIABLE> | Reserved;
  struct VarRefRule : rule<assign<&ast::VarRef::name>(ref<VariableName>)> {};
  static constexpr auto AnyOperatorTok =
      lit<"**"> | lit<"^^^"> | lit<"..."> | lit<"+"> | lit<"-"> | lit<"!"> | lit<"^"> | ref<NOT> | lit<"~~~"> |
      lit<"*"> | lit<"/"> | lit<"++"> | lit<"--"> | lit<"+++"> | lit<"---"> |
      lit<".."> | lit<"<>"> | ref<IN> | lit<"<"> | lit<">"> | lit<"<="> |
      lit<">="> | lit<"=="> | lit<"!="> | lit<"=~"> | lit<"==="> |
      lit<"!=="> | lit<"&&"> | lit<"&&&"> | ref<AND> | lit<"||"> |
      lit<"|||"> | ref<OR> | lit<"<<<"> | lit<">>>"> | lit<"<<~"> |
      lit<"~>>"> | lit<"<~"> | lit<"~>"> | lit<"<~>"> | lit<"<|>">;
  struct OperatorLitRule
      : rule<assign<&ast::OperatorLit::text>(AnyOperatorTok)> {};

  struct BoolRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ref<TRUE>) | ref<FALSE>)> {};
  struct NilRule : rule<create<ast::NilLit> + ref<NIL>> {};
  struct AliasRule : rule<assign<&ast::AliasLit::name>(ref<ALIAS>)> {};

  struct IntegerLitRule
      : rule<assign<&ast::IntegerLit::text>(ref<INTEGER>)> {};
  struct HexLitRule : rule<assign<&ast::HexLit::text>(ref<HEXADECIMAL>)> {};
  struct OctalLitRule : rule<assign<&ast::OctalLit::text>(ref<OCTAL>)> {};
  struct BinLitRule : rule<assign<&ast::BinLit::text>(ref<BINARY>)> {};
  struct FloatLitRule : rule<assign<&ast::FloatLit::text>(ref<FLOAT>)> {};

  struct CAPTURE_INDEX : terminal<lit<"&", &d>> {};
  struct CAPTURE_OP : terminal<lit<"&", !d>> {};
  struct CaptureIntRule
      : rule<create<ast::AmpExpr>(ref<CAPTURE_INDEX> +
                 assign<&ast::AmpExpr::operand>(ref<IntegerLitRule> |
                     ref<HexLitRule> | ref<OctalLitRule> | ref<BinLitRule>))> {};

  // The string is raw; only an interpolation restores normal trivia.
  template<auto Texts, auto Holes, class Fragment, cursiv::str Delimiter>
  static constexpr auto LiteralBody = tight(skip(ignored<SPACES>, hide<COMMENT>, lit<Delimiter>) + append<Texts>(ref<Fragment>) +
      many(ref<INTERP_OPEN> + skip(ignored<SPACES>, hide<COMMENT>,
                append<Holes>(ref<BlockRule>) + lit<"}">) +
           append<Texts>(ref<Fragment>)) + lit<Delimiter>);
  struct StrText : terminal<some((lit<"\\"> + any) | lit<"#", !lit<"{">> | cr<R"(^"\\#)">)> {};
  struct StrFragment : rule<option(ref<StrText>)> {};
  struct StringLitRule : rule<create<ast::StringLit>(LiteralBody<&ast::StringLit::texts, &ast::StringLit::holes, StrFragment, "\"">)> {};
  using MultiStrText = StrText;
  struct MultiStrQuotes : terminal<lit<"\"\""> | lit<"\"">> {};
  struct MultiStrFragment : rule<many(ref<MultiStrText> | ref<MultiStrQuotes>)> {};
  struct MultiStringLitRule : rule<create<ast::MultiStringLit>(LiteralBody<&ast::MultiStringLit::texts, &ast::MultiStringLit::holes, MultiStrFragment, "\"\"\"">)> {};
  struct CharsText : terminal<some((lit<"\\"> + any) | lit<"#", !lit<"{">> | cr<R"(^'\\#)">)> {};
  struct CharsFragment : rule<option(ref<CharsText>)> {};
  struct CharlistLitRule : rule<create<ast::CharlistLit>(LiteralBody<&ast::CharlistLit::texts, &ast::CharlistLit::holes, CharsFragment, "'">)> {};
  using MultiCharsText = CharsText;
  struct MultiCharsQuotes : terminal<lit<"''"> | lit<"'">> {};
  struct MultiCharsFragment : rule<many(ref<MultiCharsText> | ref<MultiCharsQuotes>)> {};
  struct MultiCharlistLitRule : rule<create<ast::MultiCharlistLit>(LiteralBody<&ast::MultiCharlistLit::texts, &ast::MultiCharlistLit::holes, MultiCharsFragment, "'''">)> {};
  struct QuotedAtomRule : rule<create<ast::QuotedAtomLit>(lit<":"> + tight((LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, StrFragment, "\""> |
       LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, CharsFragment, "'">)))> {};
  struct AtomLitRule : rule<assign<&ast::AtomLit::text>(ref<ATOM>)> {};
  struct CodepointLitRule
      : rule<assign<&ast::CodepointLit::text>(ref<CODEPOINT>)> {};
  template<cursiv::str Close> struct SigilText : terminal<some(
      (lit<"\\"> + any) | lit<"#", !lit<"{">> | ~(cr<Close> | cr<R"(\\#)">))> {};
  template<cursiv::str Close> struct SigilFragment : rule<option(ref<SigilText<Close>>)> {};
  template<cursiv::str Close> struct RawSigilText : terminal<some(
      (lit<"\\"> + any) | ~(cr<Close> | cr<R"(\\)">))> {};
  template<cursiv::str Close> struct RawSigilFragment : rule<option(ref<RawSigilText<Close>>)> {};
  struct RawMultiStrText : terminal<some((lit<"\\"> + any) | cr<R"(^"\\)">)> {};
  struct RawMultiCharsText : terminal<some((lit<"\\"> + any) | cr<R"(^'\\)">)> {};
  struct RawMultiStrFragment : rule<many(ref<RawMultiStrText> | ref<MultiStrQuotes>)> {};
  struct RawMultiCharsFragment : rule<many(ref<RawMultiCharsText> | ref<MultiCharsQuotes>)> {};
  template<cursiv::str Close> using LowerSigilFragment = std::conditional_t<Close.size() == 3,
      std::conditional_t<Close[0] == '"', MultiStrFragment, MultiCharsFragment>, SigilFragment<Close>>;
  template<cursiv::str Close> using UpperSigilFragment = std::conditional_t<Close.size() == 3,
      std::conditional_t<Close[0] == '"', RawMultiStrFragment, RawMultiCharsFragment>, RawSigilFragment<Close>>;
  template<cursiv::str Close> struct SigilEnd : terminal<lit<Close> + many(cr<"a-zA-Z0-9">),
      text_conv<[](std::string_view s) { return s.substr(Close.size()); }>> {};
  template<cursiv::str Open, cursiv::str Close> static constexpr auto SigilBody =
      assign<&ast::SigilLit::delimiter>(lit<Open>) +
      append<&ast::SigilLit::texts>(ref<LowerSigilFragment<Close>>) +
      many(ref<INTERP_OPEN> + skip(ignored<SPACES>, hide<COMMENT>,
          append<&ast::SigilLit::holes>(ref<BlockRule>) + lit<"}">) +
          append<&ast::SigilLit::texts>(ref<LowerSigilFragment<Close>>)) +
      assign<&ast::SigilLit::modifiers>(ref<SigilEnd<Close>>);
  template<cursiv::str Open, cursiv::str Close> static constexpr auto RawSigilBody =
      assign<&ast::SigilLit::delimiter>(lit<Open>) +
      append<&ast::SigilLit::texts>(ref<UpperSigilFragment<Close>>) +
      assign<&ast::SigilLit::modifiers>(ref<SigilEnd<Close>>);
  struct SigilNameLower : terminal<cr<"a-z">> {};
  struct SigilNameUpper : terminal<cr<"A-Z"> + many(cr<"A-Z0-9">)> {};
  struct SigilLitRule : rule<create<ast::SigilLit>(lit<"~"> + tight(((assign<&ast::SigilLit::name>(ref<SigilNameLower>) +
        (SigilBody<"/", "/"> | SigilBody<"|", "|"> | SigilBody<"(", ")"> |
         SigilBody<"[", "]"> | SigilBody<"{", "}"> | SigilBody<"<", ">"> |
         SigilBody<"\"", "\""> | SigilBody<"'", "'"> |
         SigilBody<"\"\"\"", "\"\"\""> | SigilBody<"'''", "'''">)) |
       (assign<&ast::SigilLit::name>(ref<SigilNameUpper>) +
        (RawSigilBody<"/", "/"> | RawSigilBody<"|", "|"> | RawSigilBody<"(", ")"> |
         RawSigilBody<"[", "]"> | RawSigilBody<"{", "}"> | RawSigilBody<"<", ">"> |
         RawSigilBody<"\"", "\""> | RawSigilBody<"'", "'"> |
         RawSigilBody<"\"\"\"", "\"\"\""> | RawSigilBody<"'''", "'''">)))))> {};

  struct NullaryOperatorRule : rule<assign<&ast::OperatorLit::text>(lit<"..."> | lit<"..">)> {};
  struct QuotedKeyRule : rule<create<ast::QuotedAtomLit>(
      LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, StrFragment, "\""> |
      LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, CharsFragment, "'">)> {};
  template<class Value> struct KeywordOption : rule<create<ast::OptionEntry>(
      (assign<&ast::OptionEntry::key>(KeywordNameTok | AtomOperatorTok) |
       assign<&ast::OptionEntry::quotedKey>(ref<QuotedKeyRule>)) + gap::none +
      lit<":"> + NLs + assign<&ast::OptionEntry::value>(ref<Value>))> {};
  using KeywordOptionRule = KeywordOption<Expression>;
  using NoDoKeywordOptionRule = KeywordOption<NoDoExpression>;
  static constexpr auto ListItem = append<&ast::ListExpr::keywords>(ref<KeywordOptionRule>) |
                                  append<&ast::ListExpr::items>(ref<AmpWrapExpr>);
  struct ListRule : rule<create<ast::ListExpr>(lit<"["> + NLs +
      option(some(ListItem, Comma) + option(lit<"|"> + NLs + assign<&ast::ListExpr::tail>(ref<AmpWrapExpr>)) + option(Comma) + NLs) + lit<"]">)> {};
  struct TupleRule : rule<create<ast::TupleExpr>(lit<"{"> + NLs +
      option(some(append<&ast::TupleExpr::items>(ref<Expression>), Comma) + option(Comma) + NLs) + lit<"}">)> {};
  struct MapEntryRule : rule<assign<&ast::MapEntry::key>(ref<Expression>) + lit<"=>"> + NLs +
      assign<&ast::MapEntry::value>(ref<Expression>)> {};
  static constexpr auto MapItem = append<&ast::MapExpr::keywords>(ref<KeywordOptionRule>) |
                                 append<&ast::MapExpr::entries>(ref<MapEntryRule>);
  struct MapRule : rule<create<ast::MapExpr>(lit<"%"> +
      option(!lit<"{"> + assign<&ast::MapExpr::structType>(ref<UnaryOpExpr>)) + lit<"{"> + NLs +
      option(assign<&ast::MapExpr::base>(ref<AmpWrapExpr>) + lit<"|"> + NLs) +
      option(some(MapItem, Comma) + option(lit<",">) + NLs) + lit<"}">)> {};
  struct BitstringRule : rule<create<ast::BitstringExpr>(lit<"<<"> + NLs +
      option(some(append<&ast::BitstringExpr::items>(ref<Expression>), Comma) + option(Comma) + NLs) + lit<">>">)> {};
  struct IndexArgRule : rule<create<ast::ListExpr>(some(append<&ast::ListExpr::keywords>(ref<KeywordOptionRule>), Comma) + option(Comma))> {};
  struct NestedExprRule : rule<lit<"("> + NLs + assign<&ast::NestedExpr::inner>(ref<Expression> | ref<ClauseBlockRule>) + NLs + lit<")">> {};
  struct AtExprRule : rule<create<ast::AtExpr>(lit<"@"> + NLs + assign<&ast::AtExpr::operand>(ref<UnaryOpExpr>))> {};
  struct DoBlockRule : rule<create<ast::DoBlock>(ref<DO> +
      assign<&ast::DoBlock::body>(ref<ClauseBlockRule>) +
      many(append<&ast::DoBlock::sections>(ref<SectionRule>)) + ref<END>)> {};
  struct SectionRule : rule<create<ast::BlockSection>(
      assign<&ast::BlockSection::kind>(ref<AFTER> | ref<RESCUE> | ref<CATCH> | ref<ELSE>) +
      assign<&ast::BlockSection::body>(ref<ClauseBlockRule>))> {};
  template<class T> static constexpr auto Params =
      some(append<&T::params>(ref<AmpWrapExpr>), Comma);
  template<class T> struct Clause : rule<create<T>(
      ((lit<"("> + NLs + option(Params<T> + NLs) + lit<")">) | option(Params<T>)) +
      option(ref<WHEN> + NLs + assign<&T::guard>(ref<InfixLow1>)) + lit<"->"> + NLs +
      assign<&T::body>(ref<ClauseBody>))> {};
  struct ClauseBody : rule<create<ast::Block>(
      option(some(append<&ast::Block::body>(ref<Expression>) + !(lit<"->"> | lit<",">), Eoe)))> {};
  static constexpr auto ExpressionBlock = option(Eoe) +
      option(some(append<&ast::Block::body>(ref<Expression>), Eoe) + option(Eoe));
  struct BlockRule : rule<create<ast::Block>(ExpressionBlock)> {};
  struct ClauseBlockRule : rule<create<ast::Block>(option(Eoe) +
      option(some(append<&ast::Block::body>(ref<Clause<ast::ClauseExpr>> | ref<Expression>), Eoe) + option(Eoe)))> {};
  struct AnonFnRule : rule<ref<FN> + NLs +
      some(append<&ast::AnonFn::clauses>(ref<Clause<ast::AnonFnClause>>), Eoe) + option(Eoe) + ref<END>> {};
  static constexpr auto DotNameTok = KeywordNameTok;
  struct DotNameRule : rule<assign<&ast::VarRef::name>(DotNameTok)> {};
  struct DotMember : rule<ref<DotNameRule> | ref<AliasRule> | ref<OperatorLitRule> |
      ref<AtomLitRule> | ref<StringLitRule> | ref<TupleRule>> {};
  struct Primary : rule<ref<AnonFnRule> | ref<NestedExprRule> | ref<AtExprRule> |
      ref<ListRule> | ref<TupleRule> | ref<MapRule> | ref<BoolRule> | ref<BitstringRule> |
      ref<NamedPrimary> | ref<QuotedAtomRule> | ref<AtomLitRule> | ref<CaptureIntRule> | ref<IntegerLitRule> | ref<HexLitRule> |
      ref<OctalLitRule> | ref<BinLitRule> | ref<FloatLitRule> | ref<SigilLitRule> |
      ref<StringLitRule> | ref<MultiStringLitRule> | ref<CharlistLitRule> | ref<MultiCharlistLitRule> |
      ref<AliasRule> | ref<CodepointLitRule> | ref<NilRule> | ref<NullaryOperatorRule>> {};
  static constexpr auto CallArgItem = append<&ast::CallExpr::options>(ref<KeywordOptionRule>) |
                                     append<&ast::CallExpr::args>(ref<Expression>);
  static constexpr auto NoDoArgItem = append<&ast::CallExpr::options>(ref<NoDoKeywordOptionRule>) |
                                     append<&ast::CallExpr::args>(ref<NoDoExpression>);
  static constexpr auto ParenArgs = NLs + option(some(CallArgItem, Comma) + option(Comma) + NLs);
  static constexpr auto NoParenArgs = !gap::none + !lit<"("> + !lit<".."> +
      ((!(lit<"+"> | lit<"-">)) | &(lit<"+", !s> | lit<"-", !s>)) +
      flag<&ast::CallExpr::noParens>(some(NoDoArgItem, Comma));
  struct NamedPrimary : rule<ref<VarRefRule> + option(
      nest<ast::CallExpr, &ast::CallExpr::callee>(NoParenArgs))> {};
  struct PostfixExpr : rule<ref<Primary> + many(
      nest<ast::IndexExpr, &ast::IndexExpr::target>(gap::none + lit<"["> + NLs +
          assign<&ast::IndexExpr::index>(ref<IndexArgRule> | ref<Expression>) + option(Comma) + NLs + lit<"]">) |
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          option(flag<&ast::CallExpr::dotCall>(lit<".">)) + gap::none + lit<"("> + ParenArgs + lit<")">) |
      (nest<ast::DotExpr, &ast::DotExpr::target>(lit<"."> + NLs + assign<&ast::DotExpr::member>(ref<DotMember>)) +
       option(nest<ast::CallExpr, &ast::CallExpr::callee>(NoParenArgs))))> {};
  struct UnaryOpExpr : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"!"> | lit<"^"> | ref<NOT> | lit<"~~~">) +
      NLs + assign<&ast::UnaryExpr::operand>(ref<UnaryOpExpr>)) | ref<PostfixExpr>> {};
  struct MUL_OP : rule<lit<"*"> + NLs, value<ast::Operator::multiply>> {};
  struct DIV_OP : rule<lit<"/"> + NLs, value<ast::Operator::divide>> {};
  struct ADD_OP : rule<lit<"+"> + NLs, value<ast::Operator::add>> {};
  struct SUB_OP : rule<lit<"-"> + NLs, value<ast::Operator::subtract>> {};
  struct CONCAT_OP : rule<lit<"++"> + NLs, value<ast::Operator::concat>> {};
  struct LISTSUB_OP : rule<lit<"--"> + NLs, value<ast::Operator::list_sub>> {};
  struct CONCAT3_OP : rule<lit<"+++"> + NLs, value<ast::Operator::concat3>> {};
  struct LISTSUB3_OP
      : rule<lit<"---"> + NLs, value<ast::Operator::list_sub3>> {};
  struct RANGE_OP : rule<lit<".."> + NLs, value<ast::Operator::range>> {};
  struct DIAMOND_OP : rule<lit<"<>"> + NLs, value<ast::Operator::diamond>> {};
  struct IN_OP : rule<ref<IN> + NLs, value<ast::Operator::in_>> {};
  struct NOTIN_OP : rule<ref<NOT> + ref<IN> + NLs, value<ast::Operator::not_in>> {};
  struct PIPEOP_OP : rule<lit<"|>"> + NLs, value<ast::Operator::pipe_op>> {};
  struct LTLTTILDE_OP
      : rule<lit<"<<~"> + NLs, value<ast::Operator::ltlt_tilde>> {};
  struct TILDEGTGT_OP
      : rule<lit<"~>>"> + NLs, value<ast::Operator::tilde_gtgt>> {};
  struct LTTILDE_OP : rule<lit<"<~"> + NLs, value<ast::Operator::lt_tilde>> {};
  struct TILDEGT_OP : rule<lit<"~>"> + NLs, value<ast::Operator::tilde_gt>> {};
  struct LTTILDEGT_OP
      : rule<lit<"<~>"> + NLs, value<ast::Operator::lt_tilde_gt>> {};
  struct LTPIPEGT_OP
      : rule<lit<"<|>"> + NLs, value<ast::Operator::lt_pipe_gt>> {};
  struct LT3_OP : rule<lit<"<<<"> + NLs, value<ast::Operator::lt3>> {};
  struct GT3_OP : rule<lit<">>>"> + NLs, value<ast::Operator::gt3>> {};
  struct LESSEQ_OP : rule<lit<"<="> + NLs, value<ast::Operator::less_eq>> {};
  struct GREATEREQ_OP
      : rule<lit<">="> + NLs, value<ast::Operator::greater_eq>> {};
  struct LESS_OP : rule<lit<"<"> + NLs, value<ast::Operator::less>> {};
  struct GREATER_OP : rule<lit<">"> + NLs, value<ast::Operator::greater>> {};
  struct STRICTEQ_OP
      : rule<lit<"==="> + NLs, value<ast::Operator::strict_equal>> {};
  struct STRICTNEQ_OP
      : rule<lit<"!=="> + NLs, value<ast::Operator::strict_not_equal>> {};
  struct EQUAL_OP : rule<lit<"=="> + NLs, value<ast::Operator::equal>> {};
  struct NOTEQUAL_OP : rule<lit<"!="> + NLs, value<ast::Operator::not_equal>> {};
  struct MATCH_OP : rule<lit<"=~"> + NLs, value<ast::Operator::match_op>> {};
  struct ANDANDAND_OP
      : rule<lit<"&&&"> + NLs, value<ast::Operator::and_and_and>> {};
  struct ANDAND_OP : rule<lit<"&&"> + NLs, value<ast::Operator::and_and>> {};
  struct ANDKW_OP : rule<ref<AND> + NLs, value<ast::Operator::and_kw>> {};
  struct ORORPOR_OP : rule<lit<"|||"> + NLs, value<ast::Operator::or_or_or>> {};
  struct OROR_OP : rule<lit<"||"> + NLs, value<ast::Operator::or_or>> {};
  struct ORKW_OP : rule<ref<OR> + NLs, value<ast::Operator::or_kw>> {};
  struct PATTERNMATCH_OP
      : rule<lit<"="> + NLs, value<ast::Operator::pattern_match>> {};

  struct POWER_OP : rule<lit<"**"> + NLs, value<ast::Operator::power>> {};
  struct STEP_OP : rule<lit<"//"> + NLs, value<ast::Operator::range_step>> {};
  struct XOR_OP : rule<lit<"^^^"> + NLs, value<ast::Operator::xor_>> {};
  struct InfixHigh
      : infix<UnaryOpExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<POWER_OP>) >> left(ref<MUL_OP> | ref<DIV_OP>) >>
               left(ref<ADD_OP> | ref<SUB_OP>) >>
               right(ref<CONCAT3_OP> | ref<LISTSUB3_OP> | ref<CONCAT_OP> |
                     ref<LISTSUB_OP> | ref<RANGE_OP> | ref<DIAMOND_OP>) >>
               right(ref<STEP_OP>) >> left(ref<XOR_OP>))> {};
  struct MembershipTail : rule<ref<UnaryOpExpr> + some(
      nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(ref<IN_OP>) +
          assign<&ast::BinaryExpr::right>(ref<InfixHigh>))) + !ref<IN>> {};
  struct NegatedMembership : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(ref<NOT> | lit<"!">) + NLs +
      assign<&ast::UnaryExpr::operand>(ref<MembershipTail>))> {};
  struct MembershipBase : rule<ref<NegatedMembership> | ref<InfixHigh>> {};
  struct Membership : infix<MembershipBase, &ast::BinaryExpr::left,
      &ast::BinaryExpr::op, &ast::BinaryExpr::right, left(ref<NOTIN_OP> | ref<IN_OP>)> {};
  struct InfixMid : infix<Membership, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
      &ast::BinaryExpr::right, (
               left(ref<PIPEOP_OP> | ref<LT3_OP> | ref<GT3_OP> |
                    ref<LTLTTILDE_OP> | ref<TILDEGTGT_OP> | ref<LTTILDEGT_OP> |
                    ref<LTTILDE_OP> | ref<TILDEGT_OP> | ref<LTPIPEGT_OP>) >>
               left(ref<LESSEQ_OP> | ref<GREATEREQ_OP> | ref<LESS_OP> |
                    ref<GREATER_OP>) >>
               left(ref<STRICTEQ_OP> | ref<STRICTNEQ_OP> | ref<EQUAL_OP> |
                    ref<NOTEQUAL_OP> | ref<MATCH_OP>) >>
               left(ref<ANDANDAND_OP> | ref<ANDAND_OP> | ref<ANDKW_OP>) >>
               left(ref<ORORPOR_OP> | ref<OROR_OP> | ref<ORKW_OP>))> {};

  struct MatchExpr
      : rule<ref<InfixMid> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<PATTERNMATCH_OP>) +
                 assign<&ast::BinaryExpr::right>(ref<AmpWrapExpr>)))> {};

  struct CapturedOperatorArity : rule<create<ast::BinaryExpr>(
      assign<&ast::BinaryExpr::left>(ref<OperatorLitRule>) +
      assign<&ast::BinaryExpr::op>(ref<DIV_OP>) +
      assign<&ast::BinaryExpr::right>(ref<IntegerLitRule>))> {};

  struct AmpWrapExpr
      : rule<create<ast::AmpExpr>(
                 ref<CAPTURE_OP> +
                 assign<&ast::AmpExpr::operand>(ref<CapturedOperatorArity> | ref<AmpWrapExpr>))
             | ref<MatchExpr>> {};

  struct PREPEND_OP : rule<lit<"|"> + NLs, value<ast::Operator::prepend>> {};
  struct TYPE_OP : rule<lit<"::"> + NLs, value<ast::Operator::type_op>> {};
  struct WHEN_OP : rule<ref<WHEN> + NLs, value<ast::Operator::when_op>> {};
  struct DEFAULTVALUE_OP
      : rule<lit<"\\\\"> + NLs, value<ast::Operator::default_value>> {};
  struct LARROW_OP : rule<lit<"<-"> + NLs, value<ast::Operator::larrow>> {};

  struct InfixLow1
      : infix<AmpWrapExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (right(ref<PREPEND_OP>) >> right(ref<TYPE_OP>) >>
               right(ref<WHEN_OP>))> {};

  struct NoDoExpression
      : infix<InfixLow1, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<LARROW_OP> | ref<DEFAULTVALUE_OP>)> {};

  struct Expression : rule<ref<NoDoExpression> + option(
      nest<ast::BlockCallExpr, &ast::BlockCallExpr::callee>(assign<&ast::BlockCallExpr::block>(ref<DoBlockRule>)))> {};
  using entry = BlockRule;
  using ignore = SPACES;
  using hidden = COMMENT;
};
}  // namespace elixir
