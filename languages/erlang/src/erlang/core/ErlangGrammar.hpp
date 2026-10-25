#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "erlang/core/ast.hpp"

namespace erlang {

inline constexpr auto unquoteAtom = [](std::string_view t) {
  if (t.empty() || t.front() != '\'') return t;
  return t.substr(1, t.size() - 1 - 1);
};
using UnquotedAtom = cursiv::text_conv<unquoteAtom>;

struct Erlang : cursiv::GrammarSpec {
  static constexpr auto kLower = cr<"a-z"> | cr<"ß-öø-ÿ">;
  static constexpr auto kUpper = cr<"A-Z"> | cr<"À-ÖØ-Þ">;
  static constexpr auto kIdCont = cr<"0-9a-zA-Z_@"> | cr<"À-ÖØ-Þß-öø-ÿ">;

  static constexpr auto word_boundary = kIdCont;

  struct WS : terminal<some(cr<R"(\x00- )"> | cr<"\u0080-\u00a0">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\n)">)> {};

  struct VAR : terminal<(kUpper | lit<"_">) + many(kIdCont)> {};

  struct ATOM
      : terminal<(kLower + many(kIdCont)) |
                     (lit<"'"> +
                      many((lit<"\\"> + (cr<R"(^\\)"> | lit<"\\">)) |
                           cr<R"(^\\')">) +
                      lit<"'">),
                 UnquotedAtom> {};

  static constexpr auto Digits = some(cr<"0-9"> + option(lit<"_">));
  static constexpr auto BasedDigits = some(cr<"0-9a-zA-Z"> + option(lit<"_">));
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + Digits;

  struct INTEGER : terminal<Digits + option(lit<"#"> + BasedDigits)> {};
  struct FLOATT : terminal<
      (Digits + lit<"."> + Digits + option(Exponent)) |
      (Digits + lit<"#"> + BasedDigits + lit<"."> + BasedDigits +
       lit<"#"> + Exponent)> {};

  static constexpr auto Escape = lit<"\\"> +
      ((lit<"x{"> + some(cr<"0-9a-fA-F">) + lit<"}">) |
       (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)) |
       (lit<"^"> + any) | repeat<1, 3>(cr<"0-7">) | any);
  struct CHAR : terminal<lit<"$"> + (Escape | any)> {};

  struct QuoteCount {};
  static constexpr auto TripleString =
      &lit<"\"\"\""> + mark<QuoteCount>(some(cr<"\"">)) +
      many(cr<" \t\r"> | cr<"\u0080-\u00a0">) +
      through(&lit<"\n">, lit<"\n"> +
          many(cr<" \t\r"> | cr<"\u0080-\u00a0">) + same<QuoteCount>);
  static constexpr auto NormalString =
      lit<"\""> + many(Escape | cr<R"(^\\")">) + lit<"\"">;
  struct STRING : terminal<TripleString | NormalString> {};

  template<auto Open, auto Close, auto Body>
  static constexpr auto SigilBody = Open + many(Escape | Body) + Close;
  static constexpr auto EscapedSigil = TripleString |
      SigilBody<lit<"\"">, lit<"\"">, cr<R"(^\\")">> |
      SigilBody<lit<"'">, lit<"'">, cr<R"(^\\')">> |
      SigilBody<lit<"/">, lit<"/">, cr<R"(^\\/)">> |
      SigilBody<lit<"|">, lit<"|">, cr<R"(^\\|)">> |
      SigilBody<lit<"#">, lit<"#">, cr<R"(^\\#)">> |
      SigilBody<lit<"`">, lit<"`">, cr<R"(^\\`)">> |
      SigilBody<lit<"(">, lit<")">, cr<R"(^\\))">> |
      SigilBody<lit<"[">, lit<"]">, cr<R"(^\\])">> |
      SigilBody<lit<"{">, lit<"}">, cr<R"(^\\})">> |
      SigilBody<lit<"<">, lit<">">, cr<R"(^\\>)">>;
  static constexpr auto VerbatimSigil = TripleString |
      through(lit<"\"">, lit<"\"">) | through(lit<"'">, lit<"'">) |
      through(lit<"/">, lit<"/">) | through(lit<"|">, lit<"|">) |
      through(lit<"#">, lit<"#">) | through(lit<"`">, lit<"`">) |
      through(lit<"(">, lit<")">) | through(lit<"[">, lit<"]">) |
      through(lit<"{">, lit<"}">) | through(lit<"<">, lit<">">);
  struct SIGIL : terminal<lit<"~"> +
      ((option(cr<"bs">) + EscapedSigil) |
       (some(kIdCont) + VerbatimSigil)) + many(kIdCont)> {};

  struct ATTR_NAME
      : terminal<lit<"-"> + many(ref<WS> | ref<COMMENT>) +
                 (lit<"spec", !kIdCont> | lit<"callback", !kIdCont>),
                 cursiv::text_conv<[](std::string_view t) {
                   return t.substr(t.size() - (t.ends_with("callback") ? 8 : 4));
                 }>> {};

  struct OpMatch : terminal<lit<"=">, value<ast::Operator::match>> {};
  struct OpSend : terminal<lit<"!">, value<ast::Operator::send>> {};
  struct OpAndAlso : terminal<kw<"andalso">, value<ast::Operator::andalso_>> {
  };
  struct OpOrElse : terminal<kw<"orelse">, value<ast::Operator::orelse_>> {};
  struct OpExactEq : terminal<lit<"=:=">, value<ast::Operator::exact_eq>> {};
  struct OpExactNeq : terminal<lit<"=/=">, value<ast::Operator::exact_neq>> {
  };
  struct OpLe : terminal<lit<"=<">, value<ast::Operator::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct OpEq : terminal<lit<"==">, value<ast::Operator::eq>> {};
  struct OpNeq : terminal<lit<"/=">, value<ast::Operator::neq>> {};
  struct OpLt : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct OpListAppend
      : terminal<lit<"++">, value<ast::Operator::list_append>> {};
  struct OpListSubtract
      : terminal<lit<"--">, value<ast::Operator::list_subtract>> {};
  struct OpBor : terminal<kw<"bor">, value<ast::Operator::bor_>> {};
  struct OpBxor : terminal<kw<"bxor">, value<ast::Operator::bxor_>> {};
  struct OpBsl : terminal<kw<"bsl">, value<ast::Operator::bsl_>> {};
  struct OpBsr : terminal<kw<"bsr">, value<ast::Operator::bsr_>> {};
  struct OpOr : terminal<kw<"or">, value<ast::Operator::or_>> {};
  struct OpXor : terminal<kw<"xor">, value<ast::Operator::xor_>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::Operator::sub>> {};
  struct OpIntDiv : terminal<kw<"div">, value<ast::Operator::int_div>> {};
  struct OpRem : terminal<kw<"rem">, value<ast::Operator::rem_>> {};
  struct OpBand : terminal<kw<"band">, value<ast::Operator::band_>> {};
  struct OpAnd : terminal<kw<"and">, value<ast::Operator::and_>> {};
  struct OpDivide : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct OpMultiply : terminal<lit<"*">, value<ast::Operator::multiply>> {};

  static constexpr auto OpMult = ref<OpDivide> | ref<OpMultiply> |
                                 ref<OpIntDiv> | ref<OpRem> | ref<OpBand> |
                                 ref<OpAnd>;
  static constexpr auto OpAddLevel = ref<OpAdd> | ref<OpSub> | ref<OpBor> |
                                     ref<OpBxor> | ref<OpBsl> | ref<OpBsr> |
                                     ref<OpOr> | ref<OpXor>;
  static constexpr auto OpListLevel = ref<OpListAppend> | ref<OpListSubtract>;
  static constexpr auto OpCompareLevel = ref<OpExactEq> | ref<OpExactNeq> |
                                         ref<OpLe> | ref<OpGe> | ref<OpEq> |
                                         ref<OpNeq> | ref<OpLt> | ref<OpGt>;
  static constexpr auto PrefixOpText =
      lit<"+"> | lit<"-"> | kw<"bnot"> | kw<"not">;

  struct ExprRule;
  struct UnaryRule;
  struct PrimaryChainRule;
  struct ExprMaxRule;

  struct ExprInfixRule
      : infix<UnaryRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
             &ast::BinaryExpr::right,
             (left(OpMult) >>          //
              left(OpAddLevel) >>      //
              right(OpListLevel) >>    //
              left(OpCompareLevel) >>  //
              right(ref<OpAndAlso>) >>  //
              right(ref<OpOrElse>) >>   //
              right(ref<OpMatch> | ref<OpSend>))> {};

  struct ExprRule : rule<create<ast::CatchExpr>(
                             kw<"catch"> +
                             assign<&ast::CatchExpr::operand>(ref<ExprRule>))
                         | ref<ExprInfixRule>> {};

  struct UnaryRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(PrefixOpText) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryRule>))
             | ref<PrimaryChainRule>> {};

  static constexpr auto MapFieldBody =
      create<ast::MapField>(
          assign<&ast::MapField::key>(ref<ExprRule>) +
          ((lit<"=>"> + assign<&ast::MapField::value>(ref<ExprRule>))
           | (lit<":="> +
              flag<&ast::MapField::exact>(
                  assign<&ast::MapField::value>(ref<ExprRule>)))));

  struct MapFieldRule : rule<MapFieldBody> {};

  static constexpr auto MapTupleBody =
      lit<"{"> +
      many(append<&ast::MapExpr::fields>(ref<MapFieldRule>), lit<",">) +
      lit<"}">;

  struct RecordFieldRule
      : rule<create<ast::RecordField>(
                 assign<&ast::RecordField::name>(ref<VAR> | ref<ATOM>) +
                 lit<"="> +
                 assign<&ast::RecordField::value>(ref<ExprRule>))> {};

  static constexpr auto RecordBody =
      assign<&ast::RecordExpr::name>(ref<ATOM>) +
      ((lit<"."> + assign<&ast::RecordExpr::accessField>(ref<ATOM>)) |
       (lit<"{"> +
        many(append<&ast::RecordExpr::fields>(ref<RecordFieldRule>),
             lit<",">) +
        lit<"}">));

  struct PrimaryChainRule
      : rule<(ref<ExprMaxRule> +
              option(nest<ast::ModuleRef, &ast::ModuleRef::module>(
                  lit<":"> +
                  assign<&ast::ModuleRef::member>(ref<ExprMaxRule>))) +
              option(nest<ast::Call, &ast::Call::target>(
                  lit<"("> +
                  many(append<&ast::Call::args>(ref<ExprRule>), lit<",">) +
                  lit<")">)) +
              many(nest<ast::MapExpr, &ast::MapExpr::base>(lit<"#"> +
                                                            MapTupleBody) |
                   nest<ast::RecordExpr, &ast::RecordExpr::base>(
                       lit<"#"> + RecordBody)))
             | ((create<ast::MapExpr>(lit<"#"> + MapTupleBody) |
                 create<ast::RecordExpr>(lit<"#"> + RecordBody)) +
                many(nest<ast::MapExpr, &ast::MapExpr::base>(lit<"#"> +
                                                              MapTupleBody) |
                     nest<ast::RecordExpr, &ast::RecordExpr::base>(
                         lit<"#"> + RecordBody)))> {};

  struct ListLitRule
      : rule<create<ast::ListLit>(
                 lit<"["> + lit<"]">) |
             create<ast::ListLit>(
                 lit<"["> +
                 some(append<&ast::ListLit::items>(ref<ExprRule>), lit<",">) +
                 option(lit<"|"> +
                        assign<&ast::ListLit::tail>(ref<ExprRule>)) +
                 lit<"]">)> {};

  struct TupleLitRule
      : rule<create<ast::TupleLit>(
                 lit<"{"> +
                 many(append<&ast::TupleLit::items>(ref<ExprRule>), lit<",">) +
                 lit<"}">)> {};

  static constexpr auto BitTypeOne =
      ref<ATOM> + option(lit<":"> + ref<INTEGER>);
  struct BitTypeList : rule<some(BitTypeOne, lit<"-">), std::string_view> {};

  static constexpr auto BitExprValue =
      (create<ast::UnaryExpr>(
           assign<&ast::UnaryExpr::op>(PrefixOpText) +
           assign<&ast::UnaryExpr::operand>(ref<ExprMaxRule>))
       | ref<ExprMaxRule>);

  struct BinElementRule
      : rule<create<ast::BinElement>(
                 assign<&ast::BinElement::value>(BitExprValue) +
                 option(lit<":"> +
                        assign<&ast::BinElement::size>(ref<ExprMaxRule>)) +
                 option(lit<"/"> + assign<&ast::BinElement::bitTypes>(ref<BitTypeList>)))> {};

  struct BinaryLitRule
      : rule<create<ast::BinaryLit>(lit<"<<"> + lit<">>">) |
             create<ast::BinaryLit>(
                 lit<"<<"> +
                 some(append<&ast::BinaryLit::elements>(ref<BinElementRule>),
                      lit<",">) +
                 lit<">>">)> {};

  struct LcClauseRule
      : rule<create<ast::LcClause>(
          assign<&ast::LcClause::left>(ref<ExprRule>) +
          option(lit<":="> + assign<&ast::LcClause::mapValue>(ref<ExprRule>)) +
          option(assign<&ast::LcClause::op>(
                     lit<"<:-"> | lit<"<:="> | lit<"<-"> | lit<"<=">) +
                 assign<&ast::LcClause::right>(ref<ExprRule>)))> {};

  struct QualifierRule
      : rule<create<ast::ZipQualifier>(
          append<&ast::ZipQualifier::generators>(ref<LcClauseRule>) +
          lit<"&&"> + some(append<&ast::ZipQualifier::generators>(ref<LcClauseRule>), lit<"&&">)) |
          ref<LcClauseRule>> {};

  struct ListComprRule
      : rule<create<ast::ListCompr>(
          lit<"["> + assign<&ast::ListCompr::head>(ref<ExprRule>) + lit<"||"> +
          some(append<&ast::ListCompr::quals>(ref<QualifierRule>), lit<",">) + lit<"]">)> {};

  struct BinComprRule
      : rule<create<ast::BinCompr>(
          lit<"<<"> + assign<&ast::BinCompr::head>(ref<ExprMaxRule>) + lit<"||"> +
          some(append<&ast::BinCompr::quals>(ref<QualifierRule>), lit<",">) + lit<">>">)> {};

  struct MapComprRule
      : rule<create<ast::MapCompr>(
          lit<"#"> + lit<"{"> + assign<&ast::MapCompr::head>(ref<MapFieldRule>) + lit<"||"> +
          some(append<&ast::MapCompr::quals>(ref<QualifierRule>), lit<",">) + lit<"}">)> {};

  struct GuardGroupRule
      : rule<create<ast::GuardGroup>(
                 some(append<&ast::GuardGroup::exprs>(ref<ExprRule>),
                      lit<",">))> {};

  struct GuardRule
      : rule<create<ast::Guard>(
                 some(append<&ast::Guard::groups>(ref<GuardGroupRule>),
                      lit<";">))> {};

  struct IfClauseRule
      : rule<create<ast::IfClause>(
                 assign<&ast::IfClause::guard>(ref<GuardRule>) + lit<"->"> +
                 some(append<&ast::IfClause::body>(ref<ExprRule>),
                      lit<",">))> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
                 kw<"if"> +
                 some(append<&ast::IfExpr::clauses>(ref<IfClauseRule>),
                      lit<";">) +
                 kw<"end">)> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
                 assign<&ast::CaseClause::pattern>(ref<ExprRule>) +
                 option(kw<"when"> +
                        assign<&ast::CaseClause::guard>(ref<GuardRule>)) +
                 lit<"->"> +
                 some(append<&ast::CaseClause::body>(ref<ExprRule>),
                      lit<",">))> {};

  struct MaybeItemRule
      : rule<create<ast::MaybeMatch>(
          assign<&ast::MaybeMatch::left>(ref<ExprRule>) + lit<"?="> +
          assign<&ast::MaybeMatch::right>(ref<ExprRule>)) | ref<ExprRule>> {};

  struct MaybeExprRule
      : rule<create<ast::MaybeExpr>(
          kw<"maybe"> + some(append<&ast::MaybeExpr::body>(ref<MaybeItemRule>), lit<",">) +
          option(flag<&ast::MaybeExpr::hasElse>(kw<"else">) +
              some(append<&ast::MaybeExpr::elseClauses>(ref<CaseClauseRule>), lit<";">)) + kw<"end">)> {};

  struct CaseExprRule
      : rule<create<ast::CaseExpr>(
                 kw<"case"> + assign<&ast::CaseExpr::subject>(ref<ExprRule>) +
                 kw<"of"> +
                 some(append<&ast::CaseExpr::clauses>(ref<CaseClauseRule>),
                      lit<";">) +
                 kw<"end">)> {};

  struct ReceiveExprRule
      : rule<create<ast::ReceiveExpr>(
                 kw<"receive"> +
                 option(some(
                     append<&ast::ReceiveExpr::clauses>(ref<CaseClauseRule>),
                     lit<";">)) +
                 option(flag<&ast::ReceiveExpr::hasAfter>(kw<"after">) +
                        assign<&ast::ReceiveExpr::afterTimeout>(
                            ref<ExprRule>) +
                        lit<"->"> +
                        some(append<&ast::ReceiveExpr::afterBody>(
                                 ref<ExprRule>),
                             lit<",">)) +
                 kw<"end">)> {};

  struct FunClauseRule
      : rule<create<ast::FunClause>(
                 option(assign<&ast::FunClause::name>(ref<VAR>)) + lit<"("> +
                 many(append<&ast::FunClause::params>(ref<ExprRule>),
                      lit<",">) +
                 lit<")"> +
                 option(kw<"when"> +
                        assign<&ast::FunClause::guard>(ref<GuardRule>)) +
                 lit<"->"> +
                 some(append<&ast::FunClause::body>(ref<ExprRule>),
                      lit<",">))> {};

  struct FunExprRule
      : rule<create<ast::FunNameRef>(
                 kw<"fun"> + assign<&ast::FunNameRef::name>(ref<ATOM>) +
                 lit<"/"> +
                 assign<&ast::FunNameRef::arity>(ref<INTEGER>)) |
             create<ast::FunRemoteRef>(
                 kw<"fun"> +
                 assign<&ast::FunRemoteRef::module>(ref<ATOM> | ref<VAR>) +
                 lit<":"> +
                 assign<&ast::FunRemoteRef::function>(ref<ATOM> | ref<VAR>) +
                 lit<"/"> +
                 assign<&ast::FunRemoteRef::arity>(ref<INTEGER> |
                                                    ref<VAR>)) |
             create<ast::FunExpr>(
                 kw<"fun"> +
                 some(append<&ast::FunExpr::clauses>(ref<FunClauseRule>),
                      lit<";">) +
                 kw<"end">)> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
                 option(assign<&ast::CatchClause::exClass>(ref<ATOM> |
                                                            ref<VAR>) +
                        lit<":">) +
                 assign<&ast::CatchClause::pattern>(ref<ExprRule>) +
                 option(lit<":"> +
                        assign<&ast::CatchClause::stackVar>(ref<VAR>)) +
                 option(kw<"when"> +
                        assign<&ast::CatchClause::guard>(ref<GuardRule>)) +
                 lit<"->"> +
                 some(append<&ast::CatchClause::body>(ref<ExprRule>),
                      lit<",">))> {};

  struct TryExprRule
      : rule<create<ast::TryExpr>(
                 kw<"try"> +
                 some(append<&ast::TryExpr::body>(ref<ExprRule>), lit<",">) +
                 option(flag<&ast::TryExpr::hasOf>(kw<"of">) +
                        some(append<&ast::TryExpr::ofClauses>(
                                 ref<CaseClauseRule>),
                             lit<";">)) +
                 ((kw<"catch"> +
                   some(append<&ast::TryExpr::catchClauses>(
                            ref<CatchClauseRule>),
                        lit<";">) +
                   option(flag<&ast::TryExpr::hasAfter>(kw<"after">) +
                          some(append<&ast::TryExpr::afterBody>(
                                   ref<ExprRule>),
                               lit<",">))) |
                  (flag<&ast::TryExpr::hasAfter>(kw<"after">) +
                   some(append<&ast::TryExpr::afterBody>(ref<ExprRule>),
                        lit<",">))) +
                 kw<"end">)> {};

  struct ExprMaxRule
      : rule<assign<&ast::VarRef::name>(ref<VAR>) |
             assign<&ast::CharLit::value>(ref<CHAR>) |
             assign<&ast::IntegerLit::value>(ref<INTEGER>) |
             assign<&ast::FloatLit::value>(ref<FLOATT>) |
             assign<&ast::AtomLit::name>(ref<ATOM>) |
             some(append<&ast::StringLit::parts>(ref<STRING>)) |
             ref<ListLitRule> | ref<BinaryLitRule> | ref<ListComprRule> |
             ref<BinComprRule> | ref<MapComprRule> | ref<TupleLitRule> |
             (lit<"("> + ref<ExprRule> + lit<")">) |
             create<ast::BeginEnd>(
                 kw<"begin"> +
                 some(append<&ast::BeginEnd::body>(ref<ExprRule>), lit<",">) +
                 kw<"end">) |
             ref<IfExprRule> | ref<CaseExprRule> | ref<ReceiveExprRule> |
             ref<FunExprRule> | ref<TryExprRule> | ref<MaybeExprRule> |
             assign<&ast::SigilLit::spelling>(ref<SIGIL>) |
             create<ast::MacroExpr>(
                 (flag<&ast::MacroExpr::stringify>(lit<"??">) | lit<"?">) +
                 assign<&ast::MacroExpr::name>(ref<VAR> | ref<ATOM>))> {};

  struct TypeExprRule;

  template <auto Part>
  static constexpr auto TypeLeaf = append<&ast::TypeExpr::parts>(Part);

  static constexpr auto NestedType =
      append<&ast::TypeExpr::parts>(ref<TypeExprRule>);

  static constexpr auto TypeAtom =
      (TypeLeaf<lit<"(">> + NestedType + TypeLeaf<lit<")">>) | TypeLeaf<ref<VAR>> |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<":">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<":">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> +
       some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<")">>) |
      (TypeLeaf<lit<"[">> + TypeLeaf<lit<"]">>) | (TypeLeaf<lit<"[">> + NestedType + TypeLeaf<lit<"]">>) |
      (TypeLeaf<lit<"[">> + NestedType + TypeLeaf<lit<",">> + TypeLeaf<lit<"...">> + TypeLeaf<lit<"]">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<lit<"{">> +
       some(NestedType + (TypeLeaf<lit<"=>">> | TypeLeaf<lit<":=">>) + NestedType, TypeLeaf<lit<",">>) +
       TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"{">> + some(NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"{">> + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"#">> + TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"{">> +
       some(TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"::">> + NestedType, TypeLeaf<lit<",">>) + TypeLeaf<lit<"}">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + NestedType + TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<"*">> + NestedType +
       TypeLeaf<lit<">>">>) |
      (TypeLeaf<lit<"<<">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<":">> + NestedType + TypeLeaf<lit<",">> + TypeLeaf<ref<VAR>> +
       TypeLeaf<lit<":">> + TypeLeaf<ref<VAR>> + TypeLeaf<lit<"*">> + NestedType + TypeLeaf<lit<">>">>) |
      TypeLeaf<ref<INTEGER>> | TypeLeaf<ref<CHAR>> | (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> + TypeLeaf<lit<")">>) |
      (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> + TypeLeaf<lit<"...">> + TypeLeaf<lit<")">> + TypeLeaf<lit<"->">> +
       NestedType + TypeLeaf<lit<")">>) |
      (TypeLeaf<kw<"fun">> + TypeLeaf<lit<"(">> +
       ((TypeLeaf<lit<"(">> + option(some(NestedType, TypeLeaf<lit<",">>)) + TypeLeaf<lit<")">> +
         TypeLeaf<lit<"->">> + NestedType)) +
       TypeLeaf<lit<")">>) |
      TypeLeaf<ref<ATOM>>;

  static constexpr auto TypeTerm = option(TypeLeaf<PrefixOpText>) + TypeAtom;
  static constexpr auto TypeArith =
      TypeTerm + many((TypeLeaf<lit<"*">> | TypeLeaf<lit<"/">> | TypeLeaf<kw<"div">> |
                       TypeLeaf<kw<"rem">> | TypeLeaf<kw<"band">> | TypeLeaf<kw<"and">> | TypeLeaf<lit<"+">> |
                       TypeLeaf<lit<"-">> | TypeLeaf<kw<"bor">> | TypeLeaf<kw<"bxor">> | TypeLeaf<kw<"bsl">> |
                       TypeLeaf<kw<"bsr">> | TypeLeaf<kw<"or">> | TypeLeaf<kw<"xor">>) +
                          TypeTerm);
  static constexpr auto TypeRange = TypeArith + option(TypeLeaf<lit<"..">> + TypeArith);
  static constexpr auto TypeUnion =
      TypeRange + many(TypeLeaf<lit<"|">> + TypeRange);

  struct TypeExprRule
      : rule<create<ast::TypeExpr>(
            option(TypeLeaf<ref<VAR>> + TypeLeaf<lit<"::">>) + TypeUnion)> {};

  struct TypeGuardRule
      : rule<create<ast::TypeExpr>(
                 (TypeLeaf<ref<ATOM>> + TypeLeaf<lit<"(">> + some(NestedType, TypeLeaf<lit<",">>) +
                  TypeLeaf<lit<")">>) |
                 (TypeLeaf<ref<VAR>> + TypeLeaf<lit<"::">> + NestedType))> {};

  struct FunTypeRule
      : rule<create<ast::TypeExpr>(
                 TypeLeaf<lit<"(">> + option(some(NestedType, TypeLeaf<lit<",">>)) + TypeLeaf<lit<")">> +
                 TypeLeaf<lit<"->">> + NestedType)> {};

  static constexpr auto TypeSigOne =
      append<&ast::TypeSig::signatures>(ref<FunTypeRule>) +
      option(kw<"when"> +
             some(append<&ast::TypeSig::signatures>(ref<TypeGuardRule>),
                  lit<",">));

  struct TypeSigRule
      : rule<create<ast::TypeSig>(
                 lit<"("> +
                 ((assign<&ast::TypeSig::module>(ref<ATOM>) + lit<":"> +
                   assign<&ast::TypeSig::function>(ref<ATOM>)) |
                  assign<&ast::TypeSig::function>(ref<ATOM>)) +
                 some(TypeSigOne, lit<";">) + lit<")">) |
             create<ast::TypeSig>(
                 ((assign<&ast::TypeSig::module>(ref<ATOM>) + lit<":"> +
                   assign<&ast::TypeSig::function>(ref<ATOM>)) |
                  assign<&ast::TypeSig::function>(ref<ATOM>)) +
                 some(TypeSigOne, lit<";">))> {};

  struct TypedFieldRule
      : rule<create<ast::TypedField>(
                 assign<&ast::TypedField::field>(ref<ExprRule>) +
                 option(lit<"::"> +
                        assign<&ast::TypedField::type>(ref<TypeExprRule>)))> {
  };

  struct TypedRecordRule
      : rule<create<ast::TypedRecord>(
                 assign<&ast::TypedRecord::head>(ref<ExprRule>) +
                 ((lit<"::"> +
                   assign<&ast::TypedRecord::type>(ref<TypeExprRule>)) |
                  (lit<","> + lit<"{"> +
                   many(append<&ast::TypedRecord::fields>(
                            ref<TypedFieldRule>),
                        lit<",">) +
                   lit<"}">)))> {};

  struct AttributeRule
      : rule<create<ast::Attribute>(
                 lit<"-"> + assign<&ast::Attribute::name>(ref<ATOM>) +
                 option((option(lit<"(">) +
                   some(append<&ast::Attribute::args>(ref<ExprRule>),
                        lit<",">) +
                   option(lit<")">)) |
                  assign<&ast::Attribute::typedRecord>(
                      ref<TypedRecordRule>) |
                  (lit<"("> +
                   assign<&ast::Attribute::typedRecord>(
                       ref<TypedRecordRule>) +
                   lit<")">))) |
             create<ast::Attribute>(
                 assign<&ast::Attribute::name>(ref<ATTR_NAME>) +
                 assign<&ast::Attribute::spec>(ref<TypeSigRule>))> {};

  struct FunctionClauseRule
      : rule<create<ast::FunctionClause>(
                 assign<&ast::FunctionClause::name>(ref<ATOM>) + lit<"("> +
                 many(append<&ast::FunctionClause::params>(ref<ExprRule>),
                      lit<",">) +
                 lit<")"> +
                 option(kw<"when"> +
                        assign<&ast::FunctionClause::guard>(ref<GuardRule>)) +
                 lit<"->"> +
                 some(append<&ast::FunctionClause::body>(ref<ExprRule>),
                      lit<",">))> {};

  struct FunctionDeclRule
      : rule<create<ast::FunctionDecl>(
                 some(append<&ast::FunctionDecl::clauses>(
                          ref<FunctionClauseRule>),
                      lit<";">))> {};

  struct FormsRule
      : rule<create<ast::Forms>(many(
                 append<&ast::Forms::forms>(ref<AttributeRule> |
                                             ref<FunctionDeclRule>) +
                 lit<".">))> {};

  using entry = FormsRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace erlang
