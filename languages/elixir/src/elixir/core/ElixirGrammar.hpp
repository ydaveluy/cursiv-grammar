#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "elixir/core/ast.hpp"

namespace elixir {

inline constexpr auto dropFirst = [](std::string_view t) { return t.substr(1); };

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
      lit<"<<>>"> | lit<"%{}"> | lit<"%"> | lit<"{}"> | lit<"..//"> | lit<"~~~"> |
      lit<"**"> | lit<"^^^"> | lit<"..."> | lit<"@"> | lit<"."> | lit<"!"> | lit<"^"> | lit<"---"> | lit<"*"> |
      lit<"/"> | lit<"+"> | lit<"-"> | lit<"++"> | lit<"--"> | lit<"+++"> |
      lit<".."> | lit<"<>"> | lit<"|>"> | lit<"<<<"> | lit<">>>"> |
      lit<"<<~"> | lit<"~>>"> | lit<"<~"> | lit<"~>"> | lit<"<~>"> |
      lit<"<|>"> | lit<"<"> | lit<">"> | lit<"<="> | lit<">="> | lit<"=="> |
      lit<"!="> | lit<"=~"> | lit<"==="> | lit<"!=="> | lit<"&&"> |
      lit<"&&&"> | lit<"||"> | lit<"|||"> | lit<"="> | lit<"&"> | lit<"|"> |
      lit<"::"> | lit<"<-"> | lit<"->"> | lit<"\\\\">;

  struct ATOM : terminal<as<dropFirst>(
                    lit<":"> +
                    (((uc::L | cr<"_">)+many(uc::L | uc::Nl | uc::Mn | uc::Mc |
                                             uc::Nd | uc::Pc | cr<"_0-9@">) +
                      option(cr<"!?">)) |
                     AtomOperatorTok))> {};

  // The native tokenizer recognizes an operator keyword only together with
  // its colon. A standalone `%{}`/`{}` literal here would steal empty maps
  // and tuples from the ordinary expression lexer.
  struct OPERATOR_KEYWORD : terminal<as<[](std::string_view text) {
      return text.substr(0, text.size() - 1);
    }>(AtomOperatorTok + lit<":", &cr<" \t\r\n">>)> {};

  struct CODEPOINT : terminal<as<dropFirst>(
                         lit<"?"> + ((lit<"\\"> + any) | (!lit<"\\"> + any)))> {
  };

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
  struct Block;
  struct ClauseBlock;
  struct InfixHigh;
  struct Membership;
  struct Section;
  struct ClauseBody;
  struct AmpWrapExpr;
  struct InfixLow1;
  struct UnaryOpExpr;
  struct PostfixExpr;
  struct NamedPrimary;
  struct AtExpr;
  static constexpr auto Reserved = ref<TRUE> | ref<FALSE> | ref<NIL> | ref<WHEN> |
      ref<AND> | ref<OR> | ref<NOT> | ref<IN> | ref<FN> | ref<DO> | ref<END> |
      ref<CATCH> | ref<RESCUE> | ref<AFTER> | ref<ELSE>;
  struct VariableName : rule<as<[](std::string_view s) { return s; }>(
                            !Reserved + ref<VARIABLE>)> {};
  static constexpr auto KeywordNameTok = ref<VARIABLE> | Reserved;
  struct VarRef : rule<assign<&ast::VarRef::name>(ref<VariableName>)> {};
  static constexpr auto AnyOperatorTok =
      lit<"**"> | lit<"^^^"> | lit<"..."> | lit<"+"> | lit<"-"> | lit<"!"> | lit<"^"> | ref<NOT> | lit<"~~~"> |
      lit<"*"> | lit<"/"> | lit<"++"> | lit<"--"> | lit<"+++"> | lit<"---"> |
      lit<".."> | lit<"<>"> | ref<IN> | lit<"<"> | lit<">"> | lit<"<="> |
      lit<">="> | lit<"=="> | lit<"!="> | lit<"=~"> | lit<"==="> |
      lit<"!=="> | lit<"&&"> | lit<"&&&"> | ref<AND> | lit<"||"> |
      lit<"|||"> | ref<OR> | lit<"<<<"> | lit<">>>"> | lit<"<<~"> |
      lit<"~>>"> | lit<"<~"> | lit<"~>"> | lit<"<~>"> | lit<"<|>">;
  struct OperatorLit
      : rule<assign<&ast::OperatorLit::text>(AnyOperatorTok)> {};

  struct Bool
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ref<TRUE>) | ref<FALSE>)> {};
  struct Nil : rule<create<ast::NilLit> + ref<NIL>> {};
  struct Alias : rule<assign<&ast::AliasLit::name>(ref<ALIAS>)> {};

  struct IntegerLit
      : rule<assign<&ast::IntegerLit::text>(ref<INTEGER>)> {};
  struct HexLit : rule<assign<&ast::HexLit::text>(ref<HEXADECIMAL>)> {};
  struct OctalLit : rule<assign<&ast::OctalLit::text>(ref<OCTAL>)> {};
  struct BinLit : rule<assign<&ast::BinLit::text>(ref<BINARY>)> {};
  struct FloatLit : rule<assign<&ast::FloatLit::text>(ref<FLOAT>)> {};

  struct CAPTURE_INDEX : terminal<lit<"&", &d>> {};
  struct CAPTURE_OP : terminal<lit<"&", !d>> {};
  struct CaptureInt
      : rule<create<ast::AmpExpr>(ref<CAPTURE_INDEX> +
                 assign<&ast::AmpExpr::operand>(ref<IntegerLit> |
                     ref<HexLit> | ref<OctalLit> | ref<BinLit>))> {};

  // The string is raw; only an interpolation restores normal trivia.
  template<auto Texts, auto Holes, class Fragment, cursiv::str Delimiter>
  static constexpr auto LiteralBody = tight(skip(ignore<SPACES>, hide<COMMENT>, lit<Delimiter>) + append<Texts>(ref<Fragment>) +
      many(ref<INTERP_OPEN> + skip(ignore<SPACES>, hide<COMMENT>,
                append<Holes>(ref<Block>) + lit<"}">) +
           append<Texts>(ref<Fragment>)) + lit<Delimiter>);
  struct StrText : terminal<some((lit<"\\"> + any) | lit<"#", !lit<"{">> | cr<R"(^"\\#)">)> {};
  struct StrFragment : rule<option(ref<StrText>)> {};
  struct StringLit : rule<create<ast::StringLit>(LiteralBody<&ast::StringLit::texts, &ast::StringLit::holes, StrFragment, "\"">)> {};
  using MultiStrText = StrText;
  struct MultiStrQuotes : terminal<lit<"\"\""> | lit<"\"">> {};
  struct MultiStrFragment : rule<many(ref<MultiStrText> | ref<MultiStrQuotes>)> {};
  struct MultiStringLit : rule<create<ast::MultiStringLit>(LiteralBody<&ast::MultiStringLit::texts, &ast::MultiStringLit::holes, MultiStrFragment, "\"\"\"">)> {};
  struct CharsText : terminal<some((lit<"\\"> + any) | lit<"#", !lit<"{">> | cr<R"(^'\\#)">)> {};
  struct CharsFragment : rule<option(ref<CharsText>)> {};
  struct CharlistLit : rule<create<ast::CharlistLit>(LiteralBody<&ast::CharlistLit::texts, &ast::CharlistLit::holes, CharsFragment, "'">)> {};
  using MultiCharsText = CharsText;
  struct MultiCharsQuotes : terminal<lit<"''"> | lit<"'">> {};
  struct MultiCharsFragment : rule<many(ref<MultiCharsText> | ref<MultiCharsQuotes>)> {};
  struct MultiCharlistLit : rule<create<ast::MultiCharlistLit>(LiteralBody<&ast::MultiCharlistLit::texts, &ast::MultiCharlistLit::holes, MultiCharsFragment, "'''">)> {};
  struct QuotedAtom : rule<create<ast::QuotedAtomLit>(lit<":"> + tight((LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, StrFragment, "\""> |
       LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, CharsFragment, "'">)))> {};
  struct AtomLit : rule<assign<&ast::AtomLit::text>(ref<ATOM>)> {};
  struct CodepointLit
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
  template <cursiv::str Close>
  struct SigilEnd
      : terminal<as<[](std::string_view s) { return s.substr(Close.size()); }>(
            lit<Close> + many(cr<"a-zA-Z0-9">))> {};
  template<cursiv::str Open, cursiv::str Close> static constexpr auto SigilBody =
      assign<&ast::SigilLit::delimiter>(lit<Open>) +
      append<&ast::SigilLit::texts>(ref<LowerSigilFragment<Close>>) +
      many(ref<INTERP_OPEN> + skip(ignore<SPACES>, hide<COMMENT>,
          append<&ast::SigilLit::holes>(ref<Block>) + lit<"}">) +
          append<&ast::SigilLit::texts>(ref<LowerSigilFragment<Close>>)) +
      assign<&ast::SigilLit::modifiers>(ref<SigilEnd<Close>>);
  template<cursiv::str Open, cursiv::str Close> static constexpr auto RawSigilBody =
      assign<&ast::SigilLit::delimiter>(lit<Open>) +
      append<&ast::SigilLit::texts>(ref<UpperSigilFragment<Close>>) +
      assign<&ast::SigilLit::modifiers>(ref<SigilEnd<Close>>);
  struct SigilNameLower : terminal<cr<"a-z">> {};
  struct SigilNameUpper : terminal<cr<"A-Z"> + many(cr<"A-Z0-9">)> {};
  struct SigilLit : rule<create<ast::SigilLit>(lit<"~"> + tight(((assign<&ast::SigilLit::name>(ref<SigilNameLower>) +
        (SigilBody<"/", "/"> | SigilBody<"|", "|"> | SigilBody<"(", ")"> |
         SigilBody<"[", "]"> | SigilBody<"{", "}"> | SigilBody<"<", ">"> |
         SigilBody<"\"", "\""> | SigilBody<"'", "'"> |
         SigilBody<"\"\"\"", "\"\"\""> | SigilBody<"'''", "'''">)) |
       (assign<&ast::SigilLit::name>(ref<SigilNameUpper>) +
        (RawSigilBody<"/", "/"> | RawSigilBody<"|", "|"> | RawSigilBody<"(", ")"> |
         RawSigilBody<"[", "]"> | RawSigilBody<"{", "}"> | RawSigilBody<"<", ">"> |
         RawSigilBody<"\"", "\""> | RawSigilBody<"'", "'"> |
         RawSigilBody<"\"\"\"", "\"\"\""> | RawSigilBody<"'''", "'''">)))))> {};

  struct NullaryOperator : rule<assign<&ast::OperatorLit::text>(lit<"..."> | lit<"..">)> {};
  struct QuotedKey : rule<create<ast::QuotedAtomLit>(
      LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, StrFragment, "\""> |
      LiteralBody<&ast::QuotedAtomLit::texts, &ast::QuotedAtomLit::holes, CharsFragment, "'">)> {};
  template<class Value> struct KeywordOption : rule<create<ast::OptionEntry>(
      (((assign<&ast::OptionEntry::key>(KeywordNameTok) |
         assign<&ast::OptionEntry::quotedKey>(ref<QuotedKey>)) + gap::none + lit<":">) |
       assign<&ast::OptionEntry::key>(ref<OPERATOR_KEYWORD>)) + NLs +
      assign<&ast::OptionEntry::value>(ref<Value>))> {};
  using KeywordOptionRule = KeywordOption<Expression>;
  using NoDoKeywordOptionRule = KeywordOption<NoDoExpression>;
  static constexpr auto ListItem = append<&ast::ListExpr::keywords>(ref<KeywordOptionRule>) |
                                  append<&ast::ListExpr::items>(ref<AmpWrapExpr>);
  struct List : rule<create<ast::ListExpr>(lit<"["> + NLs +
      option(some(ListItem, Comma) + option(lit<"|"> + NLs + assign<&ast::ListExpr::tail>(ref<AmpWrapExpr>)) + option(Comma) + NLs) + lit<"]">)> {};
  struct Tuple : rule<create<ast::TupleExpr>(lit<"{"> + NLs +
      option(some(append<&ast::TupleExpr::items>(ref<Expression>), Comma) + option(Comma) + NLs) + lit<"}">)> {};
  struct MapEntry : rule<assign<&ast::MapEntry::key>(ref<Expression>) + lit<"=>"> + NLs +
      assign<&ast::MapEntry::value>(ref<Expression>)> {};
  static constexpr auto MapItem = append<&ast::MapExpr::keywords>(ref<KeywordOptionRule>) |
                                 append<&ast::MapExpr::entries>(ref<MapEntry>);
  struct Map : rule<create<ast::MapExpr>(lit<"%"> +
      option(!lit<"{"> + assign<&ast::MapExpr::structType>(ref<UnaryOpExpr>)) + lit<"{"> + NLs +
      option(assign<&ast::MapExpr::base>(ref<AmpWrapExpr>) + lit<"|"> + NLs) +
      option(some(MapItem, Comma) + option(lit<",">) + NLs) + lit<"}">)> {};
  struct Bitstring : rule<create<ast::BitstringExpr>(lit<"<<"> + NLs +
      option(some(append<&ast::BitstringExpr::items>(ref<Expression>), Comma) + option(Comma) + NLs) + lit<">>">)> {};
  struct IndexArg : rule<create<ast::ListExpr>(some(append<&ast::ListExpr::keywords>(ref<KeywordOptionRule>), Comma) + option(Comma))> {};
  struct NestedExpr : rule<lit<"("> + NLs + assign<&ast::NestedExpr::inner>(ref<Expression> | ref<ClauseBlock>) + NLs + lit<")">> {};
  struct DoBlock : rule<create<ast::DoBlock>(ref<DO> +
      assign<&ast::DoBlock::body>(ref<ClauseBlock>) +
      many(append<&ast::DoBlock::sections>(ref<Section>)) + ref<END>)> {};
  struct Section : rule<create<ast::BlockSection>(
      assign<&ast::BlockSection::kind>(ref<AFTER> | ref<RESCUE> | ref<CATCH> | ref<ELSE>) +
      assign<&ast::BlockSection::body>(ref<ClauseBlock>))> {};
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
  struct Block : rule<create<ast::Block>(ExpressionBlock)> {};
  struct ClauseBlock : rule<create<ast::Block>(option(Eoe) +
      option(some(append<&ast::Block::body>(ref<Clause<ast::ClauseExpr>> | ref<Expression>), Eoe) + option(Eoe)))> {};
  struct AnonFn : rule<ref<FN> + NLs +
      some(append<&ast::AnonFn::clauses>(ref<Clause<ast::AnonFnClause>>), Eoe) + option(Eoe) + ref<END>> {};
  static constexpr auto DotNameTok = KeywordNameTok;
  struct DotName : rule<assign<&ast::VarRef::name>(DotNameTok)> {};
  struct DotMember : rule<ref<DotName> | ref<Alias> | ref<OperatorLit> |
      ref<AtomLit> | ref<StringLit> | ref<Tuple>> {};
  struct Primary : rule<ref<AnonFn> | ref<NestedExpr> | ref<AtExpr> |
      ref<List> | ref<Tuple> | ref<Map> | ref<Bool> | ref<Bitstring> |
      ref<NamedPrimary> | ref<QuotedAtom> | ref<AtomLit> | ref<CaptureInt> | ref<IntegerLit> | ref<HexLit> |
      ref<OctalLit> | ref<BinLit> | ref<FloatLit> | ref<SigilLit> |
      ref<StringLit> | ref<MultiStringLit> | ref<CharlistLit> | ref<MultiCharlistLit> |
      ref<Alias> | ref<CodepointLit> | ref<Nil> | ref<NullaryOperator>> {};
  static constexpr auto CallArgItem = append<&ast::CallExpr::options>(ref<KeywordOptionRule>) |
                                     append<&ast::CallExpr::args>(ref<Expression>);
  static constexpr auto NoDoArgItem = append<&ast::CallExpr::options>(ref<NoDoKeywordOptionRule>) |
                                     append<&ast::CallExpr::args>(ref<NoDoExpression>);
  static constexpr auto ParenArgs = NLs + option(some(CallArgItem, Comma) + option(Comma) + NLs);
  // Native @ precedence (320) is above dot/access (310). Keep the
  // attribute's own call, then let PostfixExpr apply .member or [index].
  struct AtExpr : rule<create<ast::AtExpr>(lit<"@"> + NLs +
      assign<&ast::AtExpr::operand>(ref<Primary> + option(
          nest<ast::CallExpr, &ast::CallExpr::callee>(
              gap::none + lit<"("> + ParenArgs + lit<")">))))> {};
  static constexpr auto NoParenArgs = !gap::none + !lit<"("> + !lit<".."> +
      ((!(lit<"+"> | lit<"-">)) | &(lit<"+", !s> | lit<"-", !s>)) +
      flag<&ast::CallExpr::noParens>(some(NoDoArgItem, Comma));
  struct NamedPrimary : rule<ref<VarRef> + option(
      nest<ast::CallExpr, &ast::CallExpr::callee>(NoParenArgs))> {};
  struct PostfixExpr : rule<ref<Primary> + many(
      nest<ast::IndexExpr, &ast::IndexExpr::target>(gap::none + lit<"["> + NLs +
          assign<&ast::IndexExpr::index>(ref<IndexArg> | ref<Expression>) + option(Comma) + NLs + lit<"]">) |
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          option(flag<&ast::CallExpr::dotCall>(lit<".">)) + gap::none + lit<"("> + ParenArgs + lit<")">) |
      (nest<ast::DotExpr, &ast::DotExpr::target>(lit<"."> + NLs + assign<&ast::DotExpr::member>(ref<DotMember>)) +
       option(nest<ast::CallExpr, &ast::CallExpr::callee>(NoParenArgs))))> {};
  struct UnaryOpExpr : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"!"> | lit<"^"> | ref<NOT> | lit<"~~~">) +
      NLs + assign<&ast::UnaryExpr::operand>(ref<UnaryOpExpr>)) | ref<PostfixExpr>> {};
  struct PIPEOP_OP : rule<as<ast::Operator::pipe_op>(lit<"|>"> + NLs)> {};
  struct TILDEGTGT_OP : rule<as<ast::Operator::tilde_gtgt>(lit<"~>>"> + NLs)> {
  };
  struct TILDEGT_OP : rule<as<ast::Operator::tilde_gt>(lit<"~>"> + NLs)> {};
  struct GT3_OP : rule<as<ast::Operator::gt3>(lit<">>>"> + NLs)> {};
  struct GREATEREQ_OP : rule<as<ast::Operator::greater_eq>(lit<">="> + NLs)> {};
  struct GREATER_OP : rule<as<ast::Operator::greater>(lit<">"> + NLs)> {};

  struct InfixHigh
      : infix<UnaryOpExpr, ast::BinaryExpr,
              levels(left(as<ast::Operator::power>(lit<"**"> + NLs)), left(as<ast::Operator::multiply>(lit<"*"> + NLs) | as<ast::Operator::divide>(lit<"/"> + NLs)),
                     left(as<ast::Operator::add>(lit<"+"> + NLs) | as<ast::Operator::subtract>(lit<"-"> + NLs)),
                     right(as<ast::Operator::concat3>(lit<"+++"> + NLs) | as<ast::Operator::list_sub3>(lit<"---"> + NLs) | as<ast::Operator::concat>(lit<"++"> + NLs) |
                           as<ast::Operator::list_sub>(lit<"--"> + NLs) | as<ast::Operator::range>(lit<".."> + NLs) | as<ast::Operator::diamond>(lit<"<>"> + NLs)),
                     right(as<ast::Operator::range_step>(lit<"//"> + NLs)), left(as<ast::Operator::xor_>(lit<"^^^"> + NLs)))> {};
  struct MembershipTail : rule<ref<UnaryOpExpr> + some(
      nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(as<ast::Operator::in_>(ref<IN> + NLs)) +
          assign<&ast::BinaryExpr::right>(ref<InfixHigh>))) + !ref<IN>> {};
  struct NegatedMembership : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(ref<NOT> | lit<"!">) + NLs +
      assign<&ast::UnaryExpr::operand>(ref<MembershipTail>))> {};
  struct MembershipBase : rule<ref<NegatedMembership> | ref<InfixHigh>> {};
  struct Membership
      : infix<MembershipBase, ast::BinaryExpr,
              left(as<ast::Operator::not_in>(ref<NOT> + ref<IN> + NLs) |
                   as<ast::Operator::in_>(ref<IN> + NLs))> {};
  struct InfixMid : infix<Membership, ast::BinaryExpr, levels(
               left(ref<PIPEOP_OP> | as<ast::Operator::lt3>(lit<"<<<"> + NLs) | ref<GT3_OP> |
                    as<ast::Operator::ltlt_tilde>(lit<"<<~"> + NLs) | ref<TILDEGTGT_OP> | as<ast::Operator::lt_tilde_gt>(lit<"<~>"> + NLs) |
                    as<ast::Operator::lt_tilde>(lit<"<~"> + NLs) | ref<TILDEGT_OP> | as<ast::Operator::lt_pipe_gt>(lit<"<|>"> + NLs)),
               left(as<ast::Operator::less_eq>(lit<"<="> + NLs) | ref<GREATEREQ_OP> | as<ast::Operator::less>(lit<"<"> + NLs) |
                    ref<GREATER_OP>),
               left(as<ast::Operator::strict_equal>(lit<"==="> + NLs) | as<ast::Operator::strict_not_equal>(lit<"!=="> + NLs) | as<ast::Operator::equal>(lit<"=="> + NLs) |
                    as<ast::Operator::not_equal>(lit<"!="> + NLs) | as<ast::Operator::match_op>(lit<"=~"> + NLs)),
               left(as<ast::Operator::and_and_and>(lit<"&&&"> + NLs) | as<ast::Operator::and_and>(lit<"&&"> + NLs) | as<ast::Operator::and_kw>(ref<AND> + NLs)),
               left(as<ast::Operator::or_or_or>(lit<"|||"> + NLs) | as<ast::Operator::or_or>(lit<"||"> + NLs) | as<ast::Operator::or_kw>(ref<OR> + NLs)))> {};

  struct MatchExpr
      : rule<ref<InfixMid> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::pattern_match>(lit<"="> + NLs)) +
                 assign<&ast::BinaryExpr::right>(ref<AmpWrapExpr>)))> {};

  struct CapturedOperatorArity : rule<create<ast::BinaryExpr>(
      assign<&ast::BinaryExpr::left>(ref<OperatorLit>) +
      assign<&ast::BinaryExpr::op>(as<ast::Operator::divide>(lit<"/"> + NLs)) +
      assign<&ast::BinaryExpr::right>(ref<IntegerLit>))> {};

  struct AmpWrapExpr
      : rule<create<ast::AmpExpr>(
                 ref<CAPTURE_OP> +
                 assign<&ast::AmpExpr::operand>(ref<CapturedOperatorArity> | ref<AmpWrapExpr>))
             | ref<MatchExpr>> {};


  struct InfixLow1
      : infix<AmpWrapExpr, ast::BinaryExpr,
              levels(right(as<ast::Operator::prepend>(lit<"|"> + NLs)), right(as<ast::Operator::type_op>(lit<"::"> + NLs)),
                     right(as<ast::Operator::when_op>(ref<WHEN> + NLs)))> {};

  struct NoDoExpression
      : infix<InfixLow1, ast::BinaryExpr,
              left(as<ast::Operator::larrow>(lit<"<-"> + NLs) | as<ast::Operator::default_value>(lit<"\\\\"> + NLs))> {};

  struct Expression : rule<ref<NoDoExpression> + option(
      nest<ast::BlockCallExpr, &ast::BlockCallExpr::callee>(assign<&ast::BlockCallExpr::block>(ref<DoBlock>)))> {};
  using entry = Block;
  using ignored = SPACES;
  using hidden = COMMENT;
};
}  // namespace elixir
