#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "alloy/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace alloy {

struct Alloy : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\u0000-\u0020)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"--">)+many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<lit<"/*"> + many(!lit<"*/"> + any) + lit<"*/">> {
  };

  static constexpr auto IdentifierStart =
      uc::L | uc::Nl | uc::Sc | uc::Pc | cr<"\u1885\u1886\u30FB\uFF65">;
  static constexpr auto IdentifierPart = IdentifierStart | uc::Nd | uc::Mn |
                                         uc::Mc | uc::Cf | cr<"\u1369-\u1371"> |
                                         cr<R"('"\u007F-\u009F)">;
  static constexpr auto Reserved =
      (lit<"abstract"> | lit<"all"> | lit<"and"> | lit<"as"> | lit<"assert"> |
       lit<"but"> | lit<"check"> | lit<"disj"> | lit<"disjoint"> | lit<"else"> |
       lit<"enum"> | lit<"exactly"> | lit<"expect"> | lit<"extends"> |
       lit<"fact"> | lit<"for"> | lit<"fun"> | lit<"iden"> | lit<"iff"> |
       lit<"implies"> | lit<"in"> | lit<"int"> | lit<"Int"> | lit<"let"> |
       lit<"lone"> | lit<"module"> | lit<"no"> | lit<"none"> | lit<"not"> |
       lit<"one"> | lit<"open"> | lit<"or"> | lit<"pred"> | lit<"private"> |
       lit<"run"> | lit<"seq"> | lit<"set"> | lit<"sig"> | lit<"some"> |
       lit<"String"> | lit<"sum"> | lit<"this"> | lit<"univ">)+!IdentifierPart;
  struct ID : terminal<!Reserved + IdentifierStart + many(IdentifierPart)> {};
  struct NUMBER : terminal<some(cr<"0-9">), std::int64_t> {};
  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                           lit<"\"">> {};
  inline static constexpr auto string_name =
      [](std::string_view text, std::pmr::memory_resource *resource) {
        auto *value = static_cast<char *>(resource->allocate(text.size()));
        std::size_t size = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
          unsigned char c = text[i];
          if (c == '\\' && i + 1 < text.size()) {
            c = text[++i];
            if (c == 'n')
              c = '\n';
          } else if (c < 32 && c != '\r' && c != '\n' && c != '\t')
            c = ' ';
          value[size++] = static_cast<char>(c);
        }
        return std::string_view(value, size);
      };
  struct QuotedName : terminal<ref<STRING>, text_conv<string_name>> {};
  struct QualName : rule<option(lit<"this/"> | lit<"seq/">) + ref<ID> +
                         many(lit<"/"> + ref<ID>)> {};
  struct SigName : rule<ref<QualName> | kw<"Int"> | kw<"String"> | kw<"univ"> |
                        kw<"none"> | lit<"seq/Int">> {};

  struct OpNot : rule<lit<"!"> | kw<"not">, value<ast::UnaryOp::not_>> {};
  struct OpNo : rule<kw<"no">, value<ast::UnaryOp::no>> {};
  struct OpLone : rule<kw<"lone">, value<ast::UnaryOp::lone>> {};
  struct OpSome : rule<kw<"some">, value<ast::UnaryOp::some>> {};
  struct OpOne : rule<kw<"one">, value<ast::UnaryOp::one>> {};
  struct OpSetU : rule<kw<"set">, value<ast::UnaryOp::set>> {};
  struct OpCard : rule<lit<"#">, value<ast::UnaryOp::card>> {};
  struct OpTranspose : rule<lit<"~">, value<ast::UnaryOp::transpose>> {};
  struct OpRClosure : rule<lit<"*">, value<ast::UnaryOp::rclosure>> {};
  struct OpClosure : rule<lit<"^">, value<ast::UnaryOp::closure>> {};

  struct OpOr : rule<lit<"||"> | kw<"or">, value<ast::BinOp::or_>> {};
  struct OpAnd : rule<lit<"&&"> | kw<"and">, value<ast::BinOp::and_>> {};
  struct OpIff : rule<lit<"<=>"> | kw<"iff">, value<ast::BinOp::iff>> {};
  struct OpIntersect : rule<lit<"&">, value<ast::BinOp::intersect>> {};
  struct OpOverride : rule<lit<"++">, value<ast::BinOp::override_>> {};
  struct OpAdd : rule<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSubtract : rule<lit<"-">, value<ast::BinOp::subtract>> {};
  struct OpDomainRestrict
      : rule<lit<"<:">, value<ast::BinOp::domain_restrict>> {};
  struct OpRangeRestrict : rule<lit<":>">, value<ast::BinOp::range_restrict>> {
  };
  struct OpJoin : rule<lit<"."> | lit<"::">, value<ast::BinOp::join>> {};

  struct OpInteger : rule<kw<"sum"> | kw<"int">, value<ast::UnaryOp::integer>> {
  };
  struct OpSequence : rule<kw<"seq">, value<ast::UnaryOp::sequence>> {};
  struct OpIntegerAdd : rule<lit<"fun/add">, value<ast::BinOp::integer_add>> {};
  struct OpIntegerSubtract
      : rule<lit<"fun/sub">, value<ast::BinOp::integer_subtract>> {};
  struct OpMultiply : rule<lit<"fun/mul">, value<ast::BinOp::multiply>> {};
  struct OpDivide : rule<lit<"fun/div">, value<ast::BinOp::divide>> {};
  struct OpRemainder : rule<lit<"fun/rem">, value<ast::BinOp::remainder>> {};
  struct OpShiftLeft : rule<lit<"<<">, value<ast::BinOp::shift_left>> {};
  struct OpShiftRight : rule<lit<">>>">, value<ast::BinOp::shift_right>> {};
  struct OpShiftArithmetic
      : rule<lit<">>">, value<ast::BinOp::shift_arithmetic>> {};

  struct CmpEq : rule<lit<"=">, value<ast::CompareOp::eq>> {};
  struct CmpNotEq
      : rule<(lit<"!"> | kw<"not">)+lit<"=">, value<ast::CompareOp::neq>> {};
  struct CmpLt : rule<lit<"<">, value<ast::CompareOp::lt>> {};
  struct CmpNotLt
      : rule<(lit<"!"> | kw<"not">)+lit<"<">, value<ast::CompareOp::not_lt>> {};
  struct CmpGt : rule<lit<">">, value<ast::CompareOp::gt>> {};
  struct CmpNotGt
      : rule<(lit<"!"> | kw<"not">)+lit<">">, value<ast::CompareOp::not_gt>> {};
  struct CmpLe : rule<(lit<"=<"> | lit<"<=">), value<ast::CompareOp::le>> {};
  struct CmpNotLe : rule<(lit<"!"> | kw<"not">)+(lit<"=<"> | lit<"<=">),
                         value<ast::CompareOp::not_le>> {};
  struct CmpGe : rule<lit<">=">, value<ast::CompareOp::ge>> {};
  struct CmpNotGe
      : rule<(lit<"!"> | kw<"not">)+lit<">=">, value<ast::CompareOp::not_ge>> {
  };
  struct CmpIn : rule<kw<"in">, value<ast::CompareOp::in>> {};
  struct CmpNotIn
      : rule<(lit<"!"> | kw<"not">)+kw<"in">, value<ast::CompareOp::not_in>> {};
  static constexpr auto CompareOp = ref<CmpNotEq> | ref<CmpEq> | ref<CmpNotLt> |
                                    ref<CmpLt> | ref<CmpNotGt> | ref<CmpGt> |
                                    ref<CmpNotLe> | ref<CmpLe> | ref<CmpNotGe> |
                                    ref<CmpGe> | ref<CmpNotIn> | ref<CmpIn>;

  struct MultLone : rule<kw<"lone">, value<ast::Mult::lone>> {};
  struct MultSome : rule<kw<"some">, value<ast::Mult::some>> {};
  struct MultOne : rule<kw<"one">, value<ast::Mult::one>> {};
  struct MultSet : rule<kw<"set">, value<ast::Mult::set>> {};
  static constexpr auto Mult = ref<MultLone> | ref<MultSome> | ref<MultOne>;
  static constexpr auto MultOrSet =
      ref<MultLone> | ref<MultSome> | ref<MultOne> | ref<MultSet>;

  struct QuantAll : rule<kw<"all">, value<ast::Quantifier::all>> {};
  struct QuantNo : rule<kw<"no">, value<ast::Quantifier::no>> {};
  struct QuantSum : rule<kw<"sum">, value<ast::Quantifier::sum>> {};
  struct QuantLone : rule<kw<"lone">, value<ast::Quantifier::lone>> {};
  struct QuantSome : rule<kw<"some">, value<ast::Quantifier::some>> {};
  struct QuantOne : rule<kw<"one">, value<ast::Quantifier::one>> {};
  static constexpr auto Quant = ref<QuantAll> | ref<QuantNo> | ref<QuantSum> |
                                ref<QuantLone> | ref<QuantSome> | ref<QuantOne>;

  struct KindRun : rule<kw<"run">, value<ast::CmdKind::run>> {};
  struct KindCheck : rule<kw<"check">, value<ast::CmdKind::check>> {};

  struct ExprRule;
  struct IffRule;
  struct ImpliesRule;
  struct AndRule;
  struct GeneralNegRule;
  struct CompareExprRule;
  struct QuantPrefixRule;
  struct AddSubRule;
  struct CardinalityRule;
  struct MultiplyRule;
  struct ShiftRule;
  struct RangeRule;
  struct IntersectOverrideRule;
  struct ArrowExprRule;
  struct RestrictRule;
  struct BoxJoinRule;
  struct UnaryTightRule;
  struct PrimaryExprRule;
  struct DeclRule;
  struct ComprehensionDeclRule;
  struct LetDeclRule;
  struct BlockRule;
  struct BlockOrBarRule;

  struct DeclNamesRule
      : rule<create<ast::Decl> +
             option(flag<&ast::Decl::private_>(kw<"private">)) +
             option(flag<&ast::Decl::namesDisjoint>(kw<"disj"> |
                                                         kw<"disjoint">)) +
             some(append<&ast::Decl::names>(ref<ID>), lit<",">)> {};
  static constexpr auto DeclType =
      option(flag<&ast::Decl::typeDisjoint>(kw<"disj"> | kw<"disjoint">)) +
      assign<&ast::Decl::type>(ref<ExprRule>);
  struct DeclRule
      : rule<ref<DeclNamesRule> +
             (lit<":"> | flag<&ast::Decl::defined>(lit<"=">)) + DeclType> {
  };
  struct ComprehensionDeclRule
      : rule<ref<DeclNamesRule> + lit<":"> + DeclType> {};

  struct LetDeclRule
      : rule<create<ast::LetDecl> + assign<&ast::LetDecl::name>(ref<ID>) +
             lit<"="> + assign<&ast::LetDecl::value>(ref<ExprRule>)> {};

  struct BlockRule
      : rule<create<ast::Block> + lit<"{"> +
             many(append<&ast::Block::exprs>(ref<ExprRule>)) + lit<"}">> {};
  struct BlockOrBarRule
      : rule<ref<BlockRule> |
             create<ast::Block>(lit<"|"> +
                                append<&ast::Block::exprs>(ref<ExprRule>))> {};

  struct PrimaryExprRule
      : rule<create<ast::NumberExpr>(
                 option(flag<&ast::NumberExpr::negative>(lit<"-">)) +
                 assign<&ast::NumberExpr::value>(ref<NUMBER>)) |
             create<ast::StringExpr>(
                 assign<&ast::StringExpr::raw>(ref<STRING>)) |
             create<ast::NoneExpr>(kw<"none">) |
             create<ast::UnivExpr>(kw<"univ">) |
             create<ast::IdenExpr>(kw<"iden">) |
             create<ast::FieldExpr>(
                 lit<"@"> + assign<&ast::FieldExpr::name>(ref<QualName>)) |
             create<ast::ThisExpr>(kw<"this">) |
             create<ast::LetExpr>(
                 kw<"let"> +
                 some(append<&ast::LetExpr::decls>(ref<LetDeclRule>),
                      lit<",">) +
                 assign<&ast::LetExpr::body>(ref<BlockOrBarRule>)) |
             create<ast::QuantExpr>(
                 assign<&ast::QuantExpr::quant>(Quant) +
                 some(append<&ast::QuantExpr::decls>(ref<DeclRule>), lit<",">) +
                 assign<&ast::QuantExpr::body>(ref<BlockOrBarRule>)) |
             create<ast::SetComprExpr>(
                 lit<"{"> +
                 some(append<&ast::SetComprExpr::decls>(
                          ref<ComprehensionDeclRule>),
                      lit<",">) +
                 option(assign<&ast::SetComprExpr::body>(ref<BlockOrBarRule>)) +
                 lit<"}">) |
             (lit<"("> + ref<ExprRule> + lit<")">) | ref<BlockRule> |
             create<ast::NameExpr>(assign<&ast::NameExpr::name>(
                 ref<SigName> | kw<"disj"> | kw<"disjoint"> |
                 lit<"pred/totalOrder"> | lit<"fun/min"> | lit<"fun/max"> |
                 lit<"fun/next"> | kw<"int"> | kw<"sum">))> {};
  struct UnaryTightRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpTranspose> |
                                             ref<OpRClosure> | ref<OpClosure>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryTightRule>)) |
             ref<PrimaryExprRule>> {};

  struct BoxJoinRule
      : rule<ref<UnaryTightRule> +
             many(nest<ast::UnaryExpr, &ast::UnaryExpr::operand>(
                      (lit<"."> |
                       lit<"::">)+assign<&ast::UnaryExpr::op>(ref<OpInteger>)) |
                  nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                      assign<&ast::BinaryExpr::op>(ref<OpJoin>) +
                      assign<&ast::BinaryExpr::right>(ref<UnaryTightRule>)) |
                  nest<ast::BoxJoinExpr, &ast::BoxJoinExpr::callee>(
                      lit<"["> +
                      many(append<&ast::BoxJoinExpr::args>(ref<ExprRule>),
                           lit<",">) +
                      lit<"]">))> {};

  struct RangeRule
      : infix<BoxJoinRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpRangeRestrict>)> {};
  struct RestrictRule
      : infix<RangeRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpDomainRestrict>)> {};

  struct ArrowExprRule
      : rule<ref<RestrictRule> +
             option(nest<ast::ArrowExpr, &ast::ArrowExpr::left>(
                 option(assign<&ast::ArrowExpr::leftMult>(MultOrSet)) +
                 lit<"->"> +
                 option(assign<&ast::ArrowExpr::rightMult>(MultOrSet)) +
                 assign<&ast::ArrowExpr::right>(ref<ArrowExprRule>)))> {};

  struct IntersectOverrideRule
      : infix<ArrowExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OpIntersect>) >> left(ref<OpOverride>))> {};
  struct CardinalityRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpCard> | ref<OpInteger>) +
                 assign<&ast::UnaryExpr::operand>(ref<CardinalityRule>)) |
             ref<IntersectOverrideRule>> {};

  struct MultiplyRule
      : infix<CardinalityRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<OpMultiply> | ref<OpDivide> | ref<OpRemainder>)> {};
  struct AddSubRule : infix<MultiplyRule, &ast::BinaryExpr::left,
                            &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                            left(ref<OpAdd> | ref<OpSubtract> |
                                 ref<OpIntegerAdd> | ref<OpIntegerSubtract>)> {
  };
  struct ShiftRule : infix<AddSubRule, &ast::BinaryExpr::left,
                           &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                           left(ref<OpShiftLeft> | ref<OpShiftRight> |
                                ref<OpShiftArithmetic>)> {};
  struct QuantPrefixRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpNo> | ref<OpLone> |
                                             ref<OpSome> | ref<OpOne> |
                                             ref<OpSetU> | ref<OpSequence>) +
                 assign<&ast::UnaryExpr::operand>(ref<QuantPrefixRule>)) |
             ref<ShiftRule>> {};

  struct CompareExprRule
      : infix<QuantPrefixRule, &ast::CompareExpr::left, &ast::CompareExpr::op,
              &ast::CompareExpr::right, left(CompareOp)> {};
  struct GeneralNegRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpNot>) +
                 assign<&ast::UnaryExpr::operand>(ref<GeneralNegRule>)) |
             ref<CompareExprRule>> {};

  struct AndRule
      : infix<GeneralNegRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpAnd>)> {};
  struct ImpliesRule
      : rule<ref<AndRule> +
             option(nest<ast::IfExpr, &ast::IfExpr::cond>(
                 (lit<"=>"> |
                  kw<"implies">)+assign<&ast::IfExpr::then>(ref<ImpliesRule>) +
                 option(kw<"else"> +
                        assign<&ast::IfExpr::else_>(ref<ImpliesRule>))))> {};

  struct IffRule
      : infix<ImpliesRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpIff>)> {};
  struct ExprRule : infix<IffRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
                          &ast::BinaryExpr::right, left(ref<OpOr>)> {};

  struct SigExtRule
      : rule<create<ast::SigExtends>(
                 kw<"extends"> +
                 assign<&ast::SigExtends::target>(ref<SigName>)) |
             create<ast::SigIn>(
                 (kw<"in"> | flag<&ast::SigIn::exact>(lit<"=">)) +
                 some(append<&ast::SigIn::targets>(ref<SigName>), lit<"+">))> {
  };

  struct QualAbstract
      : rule<kw<"abstract">, value<ast::SigQualifier::abstract_>> {};
  struct QualLone : rule<kw<"lone">, value<ast::SigQualifier::lone>> {};
  struct QualOne : rule<kw<"one">, value<ast::SigQualifier::one>> {};
  struct QualSome : rule<kw<"some">, value<ast::SigQualifier::some>> {};
  struct QualPrivate : rule<kw<"private">, value<ast::SigQualifier::private_>> {
  };
  struct SigDeclRule
      : rule<create<ast::SigDecl>(
            many(append<&ast::SigDecl::qualifiers>(
                ref<QualAbstract> | ref<QualLone> | ref<QualOne> |
                ref<QualSome> | ref<QualPrivate>)) +
            kw<"sig"> + some(append<&ast::SigDecl::names>(ref<ID>), lit<",">) +
            option(assign<&ast::SigDecl::ext>(ref<SigExtRule>)) + lit<"{"> +
            many(lit<",">) +
            option(append<&ast::SigDecl::decls>(ref<DeclRule>) +
                   many(lit<","> +
                        option(append<&ast::SigDecl::decls>(ref<DeclRule>)))) +
            lit<"}"> + option(assign<&ast::SigDecl::facts>(ref<BlockRule>)))> {
  };

  struct FactDeclRule
      : rule<create<ast::FactDecl>(
            kw<"fact"> +
            option(assign<&ast::FactDecl::name>(ref<ID> | ref<QuotedName>)) +
            assign<&ast::FactDecl::body>(ref<BlockRule>))> {};

  struct PredDeclRule
      : rule<create<ast::PredDecl>(
            option(flag<&ast::PredDecl::private_>(kw<"private">)) +
            kw<"pred"> +
            option(assign<&ast::PredDecl::qualifier>(ref<SigName>) + lit<".">) +
            assign<&ast::PredDecl::name>(ref<ID>) +
            option(
                (lit<"("> + many(lit<",">) +
                 option(append<&ast::PredDecl::params>(ref<DeclRule>) +
                        many(lit<","> + option(append<&ast::PredDecl::params>(
                                            ref<DeclRule>)))) +
                 lit<")">) |
                (lit<"["> + many(lit<",">) +
                 option(append<&ast::PredDecl::params>(ref<DeclRule>) +
                        many(lit<","> + option(append<&ast::PredDecl::params>(
                                            ref<DeclRule>)))) +
                 lit<"]">)) +
            assign<&ast::PredDecl::body>(ref<BlockRule>))> {};

  struct FunDeclRule
      : rule<create<ast::FunDecl>(
            option(flag<&ast::FunDecl::private_>(kw<"private">)) +
            kw<"fun"> +
            option(assign<&ast::FunDecl::qualifier>(ref<SigName>) + lit<".">) +
            assign<&ast::FunDecl::name>(ref<ID>) +
            option((lit<"("> + many(lit<",">) +
                    option(append<&ast::FunDecl::params>(ref<DeclRule>) +
                           many(lit<","> + option(append<&ast::FunDecl::params>(
                                               ref<DeclRule>)))) +
                    lit<")">) |
                   (lit<"["> + many(lit<",">) +
                    option(append<&ast::FunDecl::params>(ref<DeclRule>) +
                           many(lit<","> + option(append<&ast::FunDecl::params>(
                                               ref<DeclRule>)))) +
                    lit<"]">)) +
            lit<":"> + assign<&ast::FunDecl::returnType>(ref<ExprRule>) +
            assign<&ast::FunDecl::body>(ref<BlockRule>))> {};

  struct AssertDeclRule
      : rule<create<ast::AssertDecl>(
            kw<"assert"> +
            option(assign<&ast::AssertDecl::name>(ref<ID> | ref<QuotedName>)) +
            assign<&ast::AssertDecl::body>(ref<BlockRule>))> {};

  struct TypeScopeRule
      : rule<create<ast::TypeScope>(
            option(flag<&ast::TypeScope::exactly_>(kw<"exactly">)) +
            assign<&ast::TypeScope::count>(ref<NUMBER>) +
            option(lit<".."> + assign<&ast::TypeScope::end>(ref<NUMBER>)) +
            option(lit<":"> + assign<&ast::TypeScope::step>(ref<NUMBER>)) +
            assign<&ast::TypeScope::type>(ref<SigName> | kw<"int"> |
                                          kw<"seq">))> {};

  struct ScopeRule
      : rule<create<ast::Scope>(
            kw<"for"> +
            (some(append<&ast::Scope::typescopes>(ref<TypeScopeRule>),
                  lit<",">) |
             (assign<&ast::Scope::overall>(ref<NUMBER>) +
                 option(kw<"but"> + some(append<&ast::Scope::typescopes>(
                                             ref<TypeScopeRule>),
                                         lit<",">)))))> {};

  struct CmdDeclRule
      : rule<create<ast::CmdDecl>(
            option(assign<&ast::CmdDecl::name>(ref<ID>) + lit<":">) +
            option(assign<&ast::CmdDecl::kind>(ref<KindRun> | ref<KindCheck>)) +
            assign<&ast::CmdDecl::target>(ref<QualName> | ref<BlockRule>) +
            option(assign<&ast::CmdDecl::scope>(ref<ScopeRule>)) +
            option(kw<"expect"> + assign<&ast::CmdDecl::expects>(ref<NUMBER>)) +
            option(lit<"=>"> +
                   assign<&ast::CmdDecl::next>(ref<CmdDeclRule>)))> {};

  struct EnumDeclRule
      : rule<create<ast::EnumDecl> +
             option(flag<&ast::EnumDecl::private_>(kw<"private">)) +
             kw<"enum"> + assign<&ast::EnumDecl::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::EnumDecl::members>(ref<ID>), lit<",">) +
             lit<"}">> {};
  struct MacroDeclRule
      : rule<create<ast::MacroDecl> +
             option(flag<&ast::MacroDecl::private_>(kw<"private">)) +
             kw<"let"> + assign<&ast::MacroDecl::name>(ref<ID>) +
             option(
                 (lit<"("> +
                     many(append<&ast::MacroDecl::params>(ref<ID>), lit<",">) +
                     lit<")">) |
                 (lit<"["> +
                     many(append<&ast::MacroDecl::params>(ref<ID>), lit<",">) +
                     lit<"]">)) +
             assign<&ast::MacroDecl::body>(ref<BlockRule> |
                                           (lit<"="> + ref<ExprRule>))> {};

  struct ParagraphRule
      : rule<ref<SigDeclRule> | ref<FactDeclRule> | ref<PredDeclRule> |
             ref<FunDeclRule> | ref<AssertDeclRule> | ref<EnumDeclRule> |
             ref<MacroDeclRule> | ref<CmdDeclRule>> {};

  struct ImportRule
      : rule<create<ast::Import>(
            option(flag<&ast::Import::private_>(kw<"private">)) +
            kw<"open"> +
            some(append<&ast::Import::paths>(ref<QualName>), lit<",">) +
            option(lit<"["> +
                   many(append<&ast::Import::args>(ref<SigName>), lit<",">) +
                   lit<"]">) +
            option(kw<"as"> + assign<&ast::Import::alias>(ref<ID>)))> {};

  struct ModuleParamRule
      : rule<create<ast::ModuleParam> +
             option(flag<&ast::ModuleParam::exactly_>(kw<"exactly">)) +
             assign<&ast::ModuleParam::name>(ref<QualName>)> {};
  struct ModuleDeclRule
      : rule<create<ast::ModuleDecl>(
            kw<"module"> + assign<&ast::ModuleDecl::path>(ref<QualName>) +
            option(lit<"["> +
                   many(append<&ast::ModuleDecl::names>(ref<ModuleParamRule>),
                        lit<",">) +
                   lit<"]">))> {};

  struct AlloyModuleRule
      : rule<create<ast::Module> +
             option(assign<&ast::Module::moduleDecl>(ref<ModuleDeclRule>)) +
             many(append<&ast::Module::imports>(ref<ImportRule>) |
                  append<&ast::Module::paragraphs>(ref<ParagraphRule>))> {};

  using entry = AlloyModuleRule;
  using ignore = list<WS, SL_COMMENT, ML_COMMENT>;
};

} // namespace alloy
