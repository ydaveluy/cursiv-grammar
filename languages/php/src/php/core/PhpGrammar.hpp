#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "php/core/ast.hpp"

namespace php {

inline constexpr auto unheredoc = [](std::string_view t) -> std::string_view {
  const auto first = t.find_first_of("\r\n");
  if (first == std::string_view::npos) return t;
  auto begin = first + 1;
  if (t[first] == '\r' && begin < t.size() && t[begin] == '\n') ++begin;
  auto end = t.find_last_of("\r\n");
  if (end > 0 && t[end] == '\n' && t[end - 1] == '\r') --end;
  return end < begin ? std::string_view{} : t.substr(begin, end - begin);
};
using Unheredoc = cursiv::text_conv<unheredoc>;

struct Php : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<R"(a-zA-Z0-9_\u{80}-\u{10FFFF})">;

  struct SHEBANG : terminal<lit<"#!"> + many(cr<R"(^\r\n)">)> {};

  struct HTML_TEXT : terminal<some(cr<"^<"> | lit<"<", !lit<"?">>)> {};

  struct XML_DECL : terminal<through(lit<"<?xml">, lit<"?>">)> {};

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct SL_COMMENT
      : terminal<(lit<"//"> | lit<"#", !lit<"[">>) +
                 many(cr<R"(^\r\n?)"> | lit<"?", !lit<">">>)> {
  };
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<R"(a-zA-Z_\u{80}-\u{10FFFF})"> + many(word_boundary)> {};
  struct VARNAME
      : terminal<lit<"$"> + cr<R"(a-zA-Z_\u{80}-\u{10FFFF})"> +
                 many(cr<R"(a-zA-Z0-9_\u{80}-\u{10FFFF})">)> {};

  static constexpr auto LNUM = some(cr<"0-9">) + many(lit<"_"> + some(cr<"0-9">));
  static constexpr auto EXP = cr<"eE"> + option(cr<"+-">) + LNUM;

  struct OCTAL_LIT
      : terminal<lit<"0"> + option(cr<"oO">) + some(cr<"0-7">) +
                 many(lit<"_"> + some(cr<"0-7">))> {};
  struct DECIMAL_LIT
      : terminal<lit<"0"> | (cr<"1-9"> + many(cr<"0-9">) +
                             many(lit<"_"> + some(cr<"0-9">)))> {};
  struct HEX_LIT
      : terminal<lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">) +
                 many(lit<"_"> + some(cr<"0-9a-fA-F">))> {};
  struct BINARY_LIT
      : terminal<lit<"0"> + cr<"bB"> + some(cr<"01">) +
                 many(lit<"_"> + some(cr<"01">))> {};
  struct REAL_LIT
      : terminal<((((LNUM + lit<"."> + option(LNUM)) | (lit<"."> + LNUM)) +
                  option(EXP)) |
                 (LNUM + EXP))> {};

  struct TRUE_LIT : terminal<ikw<"true">, value<true>> {};
  struct FALSE_LIT : terminal<ikw<"false">, value<false>> {};

  struct SINGLE_STRING
      : terminal<option(cr<"bB">) + lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\\\)">) + lit<"'">> {
  };
  struct BACKQUOTE_STRING : terminal<lit<"`"> + many(cr<"^`">) + lit<"`">> {};

  struct STR_TEXT : terminal<some((lit<"\\"> + any) | cr<R"(^${"\\\\)">)> {};
  struct STR_LONE_CHAR
      : terminal<lit<"$", !cr<R"(a-zA-Z_\u{80}-\u{10FFFF}{)">> | lit<"{", !lit<"$">>> {};

  struct SIMPLE_INT : terminal<option(lit<"-">) + some(cr<"0-9">)> {};

  struct HeredocTag {};
  static constexpr auto HeredocOpen = option(cr<"bB">) + lit<"<<<"> + many(cr<R"( \t)">);
  static constexpr auto HeredocId = some(word_boundary);
  static constexpr auto HeredocNewline = lit<"\r\n"> | cr<R"(\r\n)">;
  static constexpr auto HeredocClose =
      HeredocNewline + many(cr<R"( \t)">) + same<HeredocTag> + !word_boundary;
  static constexpr auto HeredocBody =
      through(&HeredocNewline, HeredocClose);

  struct HEREDOC_BODY
      : terminal<HeredocOpen + mark<HeredocTag>(HeredocId) + HeredocBody,
                 Unheredoc> {};
  struct HEREDOC_BODY_QUOTED
      : terminal<HeredocOpen + lit<"\""> + mark<HeredocTag>(HeredocId) +
                     lit<"\""> + HeredocBody,
                 Unheredoc> {};
  struct NOWDOC_BODY
      : terminal<HeredocOpen + lit<"'"> + mark<HeredocTag>(HeredocId) +
                     lit<"'"> + HeredocBody,
                 Unheredoc> {};

  struct PHP_OPEN
      : terminal<lit<"<?"> + cr<"pP"> + cr<"hH"> + cr<"pP"> +
                 !cr<R"(^ \t\r\n)">> {};
  struct PHP_CLOSE : terminal<lit<"?>"> + option(HeredocNewline)> {};

  static constexpr auto SEMI =
      implied(lit<";">, &ref<PHP_CLOSE> | eof);

  struct NameRule
      : rule<(flag<&ast::Name::nsRelative>(ikw<"namespace"> + lit<"\\">) |
              option(flag<&ast::Name::rooted>(lit<"\\">))) +
             some(append<&ast::Name::parts>(ref<ID>), lit<"\\">)> {};

  struct TypeRefRule;

  struct PrimitiveTypeRefRule
      : rule<create<ast::PrimitiveTypeRef>(assign<&ast::PrimitiveTypeRef::keyword>(
            ikw<"bool"> | ikw<"int"> | ikw<"float"> | ikw<"string"> | ikw<"object"> |
            ikw<"array"> | ikw<"callable"> | ikw<"iterable"> | ikw<"mixed"> |
            ikw<"void"> | ikw<"never"> | ikw<"null"> | ikw<"true"> | ikw<"false">))> {};

  static constexpr auto GenericArgs =
      lit<"<:"> + some(append<&ast::NamedTypeRef::genericArgs>(ref<TypeRefRule>), lit<",">) +
      lit<":>">;

  struct SingleTypeRefRule
      : rule<create<ast::StaticTypeRef>(ikw<"static">) | ref<PrimitiveTypeRefRule> |
             create<ast::NamedTypeRef>(assign<&ast::NamedTypeRef::name>(ref<NameRule>) +
                                       option(GenericArgs))> {};

  struct TypeAtomRule
      : rule<create<ast::NullableTypeRef>(lit<"?"> +
                 assign<&ast::NullableTypeRef::type>(ref<SingleTypeRefRule>)) |
             (lit<"("> + ref<TypeRefRule> + lit<")">) |
             ref<SingleTypeRefRule>> {};

  static constexpr auto TypeAmp = lit<"&", !(many(ref<WS> | ref<ML_COMMENT> |
      ref<SL_COMMENT>) + (lit<"$"> | lit<"...">))>;

  struct IntersectionTypeRefRule
      : rule<create<ast::IntersectionTypeRef>(
                 append<&ast::IntersectionTypeRef::options>(ref<TypeAtomRule>) +
                 TypeAmp +
                 some(append<&ast::IntersectionTypeRef::options>(ref<TypeAtomRule>), TypeAmp)) |
             ref<TypeAtomRule>> {};

  struct TypeRefRule
      : rule<create<ast::UnionTypeRef>(
                 append<&ast::UnionTypeRef::options>(ref<IntersectionTypeRefRule>) + lit<"|"> +
                 some(append<&ast::UnionTypeRef::options>(ref<IntersectionTypeRefRule>), lit<"|">)) |
             ref<IntersectionTypeRefRule>> {};

  struct ArgRule;

  struct AttributeRule
      : rule<assign<&ast::Attribute::name>(ref<NameRule>) +
             option(lit<"("> + many(append<&ast::Attribute::args>(ref<ArgRule>), lit<",">) +
                    option(lit<",">) + lit<")">)> {};

  struct AttributeGroupRule
      : rule<lit<"#["> +
             option(assign<&ast::AttributeGroup::target>(ref<ID>) + lit<":">) +
             some(append<&ast::AttributeGroup::items>(ref<AttributeRule>), lit<",">) +
             option(lit<",">) + lit<"]">> {};

  struct Expression;
  struct ChainExpr;
  struct ClosureRule;
  struct MatchExprRule;
  struct NewExprRule;
  struct ArrayExprRule;
  struct BlockStmtRule;
  struct YieldExprRule;
  struct ClassMemberRule;

  static constexpr auto FieldNameExpr =
      create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>)) |
      (lit<"{"> + ref<Expression> + lit<"}">) |
      create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>));

  struct SimpleIndexKey
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)) |
             create<ast::IntLit>(assign<&ast::IntLit::text>(ref<SIMPLE_INT>)) |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<ID>))> {};

  struct PlainVariableRule
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>))> {};

  static constexpr auto SimplePostfixStart =
      lit<"["> | lit<"->", &ref<ID>> | lit<"?->", &ref<ID>>;
  struct SIMPLE_PLAIN_VAR : terminal<ref<VARNAME> + !SimplePostfixStart> {};

  struct SimpleVarInterp
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<SIMPLE_PLAIN_VAR>)) |
             (ref<PlainVariableRule> +
              (nest<ast::IndexExpr, &ast::IndexExpr::base>(
                   lit<"["> + assign<&ast::IndexExpr::index>(ref<SimpleIndexKey>) + lit<"]">) |
               nest<ast::MemberExpr, &ast::MemberExpr::object>(
                   (flag<&ast::MemberExpr::nullsafe>(lit<"?->", &ref<ID>>) |
                    lit<"->", &ref<ID>>) + assign<&ast::MemberExpr::member>(
                       create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))))))> {};

  static constexpr auto LegacyNameStart = ref<ID> + cr<"[}">;
  struct LegacyVariableHead
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<ID>))> {};

  struct LegacyVarInterp
      : rule<lit<"${", &LegacyNameStart> + ref<LegacyVariableHead> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::base>(lit<"["> +
                 assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">)) + lit<"}">> {};

  struct CurlyInterp
      : rule<&(lit<"{", &lit<"$">> | lit<"${">) +
             skip(ignored<WS>, hide<SL_COMMENT, ML_COMMENT>,
                 (lit<"{", &lit<"$">> + ref<Expression> + lit<"}">) |
                 ref<LegacyVarInterp> |
                 create<ast::Var>(lit<"${", !LegacyNameStart> +
                     assign<&ast::Var::nameExpr>(ref<Expression>) + lit<"}">))> {};

  static constexpr auto StringPiece =
      append<&ast::InterpString::pieces>(
          create<ast::StrText>(assign<&ast::StrText::text>(ref<STR_TEXT> | ref<STR_LONE_CHAR>))) |
      append<&ast::InterpString::pieces>(ref<SimpleVarInterp>) |
      append<&ast::InterpString::pieces>(ref<CurlyInterp>);

  struct DoubleQuotedRule
      : rule<create<ast::InterpString>(tight((lit<"b\""> | lit<"B\""> | lit<"\"">) + many(StringPiece) + lit<"\"">))> {};

  struct StringRule
      : rule<create<ast::StringLit>(assign<&ast::StringLit::text>(
                 ref<SINGLE_STRING> | ref<HEREDOC_BODY> | ref<HEREDOC_BODY_QUOTED> |
                 ref<NOWDOC_BODY>))> {};

  struct ArgRule
      : rule<(option(assign<&ast::Arg::name>(ref<ID>) + lit<":">) +
              ((flag<&ast::Arg::spread>(lit<"...">) +
                option(assign<&ast::Arg::value>(ref<Expression>))) |
               assign<&ast::Arg::value>(ref<Expression>))) |
             (flag<&ast::Arg::byRef>(lit<"&">) + assign<&ast::Arg::value>(ref<ChainExpr>))> {
  };

  template <auto Ptr>
  static constexpr auto ArgsInto = lit<"("> +
      many(append<Ptr>(ref<ArgRule>), lit<",">) + option(lit<",">) + lit<")">;
  static constexpr auto CallArgs = ArgsInto<&ast::CallExpr::args>;

  struct VariableBaseRule
      : rule<ref<PlainVariableRule> |
             create<ast::Var>(lit<"${"> + assign<&ast::Var::nameExpr>(ref<Expression>) + lit<"}">) |
             create<ast::Var>(lit<"$"> + assign<&ast::Var::nameExpr>(ref<VariableBaseRule>))> {};

  struct KeyedVariableRule
      : rule<ref<VariableBaseRule> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> + option(assign<&ast::IndexExpr::index>(ref<Expression>)) + lit<"]">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"{"> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"}">))> {};

  struct ChainOrigin
      : rule<ref<KeyedVariableRule> | ref<ArrayExprRule> | ref<DoubleQuotedRule> |
             create<ast::NameExpr>(flag<&ast::NameExpr::isStatic>(ikw<"static">)) |
             create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<NameRule>)) |
             (lit<"("> + ref<Expression> + lit<")">) | ref<NewExprRule> |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<SINGLE_STRING>))> {};

  struct ChainExpr
      : rule<ref<ChainOrigin> +
             many(
                 nest<ast::MemberExpr, &ast::MemberExpr::object>(
                     (flag<&ast::MemberExpr::nullsafe>(lit<"?->">) |
                      lit<"->">)+assign<&ast::MemberExpr::
                                            member>(FieldNameExpr)) |
                 nest<ast::StaticAccessExpr, &ast::StaticAccessExpr::scope>(
                     lit<"::"> +
                     assign<&ast::StaticAccessExpr::member>(FieldNameExpr)) |
                 nest<ast::CallExpr, &ast::CallExpr::callee>(CallArgs) |
                 nest<ast::IndexExpr, &ast::IndexExpr::base>(
                     lit<"["> +
                     option(assign<&ast::IndexExpr::index>(ref<Expression>)) +
                     lit<"]">) |
                 nest<ast::IndexExpr, &ast::IndexExpr::base>(
                     lit<"{"> +
                     assign<&ast::IndexExpr::index>(ref<Expression>) +
                     lit<"}">))> {};

  static constexpr auto CastKeyword =
      ikw<"bool"> | ikw<"boolean"> | ikw<"int8"> | ikw<"int16"> | ikw<"int"> |
      ikw<"integer"> | ikw<"int64"> | ikw<"uint8"> | ikw<"uint16"> | ikw<"uint64"> |
      ikw<"uint"> | ikw<"real"> | ikw<"double"> | ikw<"float"> | ikw<"string"> |
      ikw<"binary"> | ikw<"unicode"> | ikw<"array"> | ikw<"object"> |
      ikw<"resource"> | ikw<"unset"> | ikw<"void">;

  static constexpr auto NumberLit =
      create<ast::FloatLit>(assign<&ast::FloatLit::text>(ref<REAL_LIT>)) |
      create<ast::IntLit>(assign<&ast::IntLit::text>(
          ref<HEX_LIT> | ref<BINARY_LIT> | ref<OCTAL_LIT> | ref<DECIMAL_LIT>));

  struct ScalarExpr
      : rule<create<ast::BoolLit>(assign<&ast::BoolLit::value>(ref<TRUE_LIT> | ref<FALSE_LIT>)) |
             create<ast::NullLit>(ikw<"null">) | NumberLit | ref<StringRule>> {};

  struct ArrayItemRule
      : rule<(flag<&ast::ArrayItem::byRef>(lit<"&">) +
              assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
             (assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>"> +
              flag<&ast::ArrayItem::byRef>(lit<"&">) +
              assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
             (option(assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>">) +
              option(flag<&ast::ArrayItem::spread>(lit<"...">)) +
              assign<&ast::ArrayItem::value>(ref<Expression>))> {};

  static constexpr auto ArrayItems =
      many(append<&ast::ArrayExpr::items>(ref<ArrayItemRule>), lit<",">) + option(lit<",">);

  struct ArrayExprRule
      : rule<create<ast::ArrayExpr>((ikw<"array"> + lit<"("> + ArrayItems + lit<")">) |
                                    (lit<"["> + ArrayItems + lit<"]">))> {};

  struct ParamRule;

  template <auto Ptr>
  static constexpr auto ClosureParams = lit<"("> +
      many(append<Ptr>(ref<ParamRule>), lit<",">) + option(lit<",">) + lit<")">;

  struct ClosureRule
      : rule<create<ast::Closure>(
            many(append<&ast::Closure::attributes>(ref<AttributeGroupRule>)) +
            option(flag<&ast::Closure::isStatic>(ikw<"static">)) +
            ((ikw<"function"> + option(flag<&ast::Closure::byRef>(lit<"&">)) +
              ClosureParams<&ast::Closure::params> +
              option(ikw<"use"> + lit<"("> +
                     many(append<&ast::Closure::uses>(create<ast::Var>(
                         option(flag<&ast::Var::byRef>(lit<"&">)) +
                         assign<&ast::Var::name>(ref<VARNAME>))), lit<",">) +
                     option(lit<",">) + lit<")">) +
              option(lit<":"> + assign<&ast::Closure::returnType>(ref<TypeRefRule>)) +
              assign<&ast::Closure::body>(ref<BlockStmtRule>)) |
             (ikw<"fn"> + option(flag<&ast::Closure::byRef>(lit<"&">)) +
              ClosureParams<&ast::Closure::params> +
              option(lit<":"> + assign<&ast::Closure::returnType>(ref<TypeRefRule>)) +
              lit<"=>"> + assign<&ast::Closure::arrowBody>(ref<Expression>))))> {};

  struct MatchExprRule
      : rule<create<ast::MatchExpr>(
            ikw<"match"> + lit<"("> + assign<&ast::MatchExpr::subject>(ref<Expression>) + lit<")"> +
            lit<"{"> +
            many(append<&ast::MatchExpr::arms>(create<ast::MatchArm>(
                     (ikw<"default"> |
                      some(append<&ast::MatchArm::conditions>(ref<Expression>), lit<",">)) +
                     lit<"=>"> + assign<&ast::MatchArm::value>(ref<Expression>))),
                 lit<",">) +
            option(lit<",">) + lit<"}">)> {};

  struct ListPatternRule;

  static constexpr auto ListItemBody =
      option(assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>">) +
      ((flag<&ast::ArrayItem::byRef>(lit<"&">) +
        assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
       assign<&ast::ArrayItem::value>(ref<ListPatternRule> | ref<ChainExpr>));

  struct ListItemRule : rule<ListItemBody> {};
  struct ListSlotRule : rule<create<ast::ArrayItem>(option(ListItemBody))> {};

  static constexpr auto ListItems =
      many(append<&ast::ArrayExpr::items>(ref<ListSlotRule>) + lit<",">) +
      option(append<&ast::ArrayExpr::items>(ref<ListItemRule>));

  struct ListPatternRule
      : rule<create<ast::ArrayExpr>((ikw<"list"> + lit<"("> + ListItems + lit<")">) |
                                    (lit<"["> + ListItems + lit<"]">))> {};

  struct AnonClassTailRule
      : rule<create<ast::ClassDecl>(
            option(ikw<"extends"> + assign<&ast::ClassDecl::extendsName>(ref<NameRule>)) +
            option(ikw<"implements"> +
                   some(append<&ast::ClassDecl::implementsList>(ref<NameRule>), lit<",">)) +
            lit<"{"> + many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>)) + lit<"}">)> {
  };

  static constexpr auto NewArgs = ArgsInto<&ast::NewExpr::args>;

  struct NewTargetRule
      : rule<(ref<KeyedVariableRule> |
              create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<NameRule>)) |
              (lit<"("> + ref<Expression> + lit<")">)) +
             many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"->"> + assign<&ast::MemberExpr::member>(FieldNameExpr)) |
                  nest<ast::StaticAccessExpr, &ast::StaticAccessExpr::scope>(
                      lit<"::"> + assign<&ast::StaticAccessExpr::member>(FieldNameExpr)) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">))> {};

  struct NewExprRule
      : rule<create<ast::NewExpr>(
            ikw<"new"> +
            ((flag<&ast::NewExpr::isStatic>(ikw<"static">) + option(NewArgs)) |
             (ikw<"class"> + option(NewArgs) +
              assign<&ast::NewExpr::anonClass>(ref<AnonClassTailRule>)) |
             (assign<&ast::NewExpr::target>(ref<NewTargetRule>) + option(NewArgs))))> {};

  struct UnaryLevel;
  struct AssignLevel;

  struct AtomLevel
      : rule<create<ast::CloneExpr>(ikw<"clone"> + assign<&ast::CloneExpr::operand>(ref<AtomLevel>)) |
             create<ast::PrintExpr>(ikw<"print"> + assign<&ast::PrintExpr::operand>(ref<AssignLevel>)) |
             create<ast::IncludeExpr>(
                 assign<&ast::IncludeExpr::keyword>(ikw<"include_once"> | ikw<"include"> |
                                                    ikw<"require_once"> | ikw<"require">) +
                 assign<&ast::IncludeExpr::operand>(ref<Expression>)) |
             create<ast::ThrowExpr>(ikw<"throw"> + assign<&ast::ThrowExpr::operand>(ref<Expression>)) |
             ref<ClosureRule> | ref<MatchExprRule> |
             create<ast::IncDecExpr>(assign<&ast::IncDecExpr::operand>(ref<ChainExpr>) +
                                     assign<&ast::IncDecExpr::op>(lit<"++"> | lit<"--">)) |
             ref<ChainExpr> | ref<ScalarExpr> |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<BACKQUOTE_STRING>)) |
             ref<YieldExprRule> |
             create<ast::IssetExpr>(ikw<"isset"> + lit<"("> +
                                    some(append<&ast::IssetExpr::args>(ref<ChainExpr>), lit<",">) +
                                    lit<")">) |
             create<ast::EmptyExpr>(ikw<"empty"> + lit<"("> +
                                    assign<&ast::EmptyExpr::operand>(ref<Expression>) + lit<")">) |
             create<ast::EvalExpr>(ikw<"eval"> + lit<"("> +
                                   assign<&ast::EvalExpr::operand>(ref<Expression>) + lit<")">) |
             create<ast::ExitExpr>((ikw<"die"> | ikw<"exit">) +
                                   option(ArgsInto<&ast::ExitExpr::args>))> {};

  struct PowLevel
      : rule<create<ast::BinaryExpr>(assign<&ast::BinaryExpr::left>(ref<AtomLevel>) +
                                     assign<&ast::BinaryExpr::op>(lit<"**">) +
                                     assign<&ast::BinaryExpr::right>(ref<UnaryLevel>))  //
             | ref<AtomLevel>> {};

  struct UnaryLevel
      : rule<create<ast::CastExpr>(lit<"("> + assign<&ast::CastExpr::type>(CastKeyword) + lit<")"> +
                                   assign<&ast::CastExpr::operand>(ref<UnaryLevel>)) |
             create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"~"> | lit<"@">) +
                                    assign<&ast::UnaryExpr::operand>(ref<UnaryLevel>)) |
             create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                                    assign<&ast::UnaryExpr::operand>(ref<UnaryLevel>)) |
             create<ast::IncDecExpr>(
                 flag<&ast::IncDecExpr::prefix>(assign<&ast::IncDecExpr::op>(lit<"++"> | lit<"--">)) +
                 assign<&ast::IncDecExpr::operand>(ref<ChainExpr>)) |
             ref<PowLevel>> {};

  struct InstanceofLevel
      : rule<create<ast::InstanceofExpr>(assign<&ast::InstanceofExpr::value>(ref<UnaryLevel>) +
                                         ikw<"instanceof"> +
                                         assign<&ast::InstanceofExpr::type>(ref<ChainExpr>)) |
             ref<UnaryLevel>> {};

  struct NotLevel
      : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"!">) +
                                    assign<&ast::UnaryExpr::operand>(ref<NotLevel>)) |
             ref<InstanceofLevel>> {};

  struct BinaryLevel
      : infix<NotLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | lit<"%">) >>
               left(lit<"+"> | lit<"-">) >> left(lit<"<<"> | lit<">>">) >>
               left(lit<".">) >> left(lit<"|>">) >>
               left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) >>
               left(lit<"==="> | lit<"!=="> | lit<"=="> | lit<"<>"> |
                    lit<"!="> | lit<"<=>">) >>
               left(lit<"&", !cr<"&=">>) >> left(lit<"^">) >>
               left(lit<"|", !cr<"|=">>) >> left(lit<"&&">) >>
               left(lit<"||">))> {};

  struct CoalesceLevel
      : infix<BinaryLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, right(lit<"??">)> {};
  struct ConditionalLevel
      : rule<ref<CoalesceLevel> +
             option(nest<ast::ConditionalExpr, &ast::ConditionalExpr::cond>(
                 lit<"?"> + option(assign<&ast::ConditionalExpr::thenValue>(ref<Expression>)) +
                 lit<":"> + assign<&ast::ConditionalExpr::elseValue>(ref<ConditionalLevel>)))> {};

  struct AssignableRule : rule<ref<ListPatternRule> | ref<ChainExpr>> {};

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"**="> | lit<"/="> | lit<".="> | lit<"%="> |
      lit<"&="> | lit<"|="> | lit<"^="> | lit<"<<="> | lit<">>="> | lit<"??="> |
      lit<"=", !cr<"=>">>;

  struct AssignLevel
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<AssignableRule>) +
                 lit<"="> + flag<&ast::AssignExpr::byRef>(lit<"&">) +
                 assign<&ast::AssignExpr::value>(ref<ChainExpr>))  //
             | create<ast::AssignExpr>(
                   assign<&ast::AssignExpr::target>(ref<AssignableRule>) +
                   assign<&ast::AssignExpr::op>(AssignOp) +
                   assign<&ast::AssignExpr::value>(ref<AssignLevel>))  //
             | ref<ConditionalLevel>> {};

  struct Expression
      : infix<AssignLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(ikw<"and">) >> left(ikw<"xor">) >> left(ikw<"or">))> {};

  struct YieldExprRule
      : rule<create<ast::YieldExpr>(
            ikw<"yield"> + option(
                (flag<&ast::YieldExpr::from>(ikw<"from">) +
                 assign<&ast::YieldExpr::value>(ref<AssignLevel>)) |
                (assign<&ast::YieldExpr::key>(ref<AssignLevel>) + lit<"=>"> +
                 assign<&ast::YieldExpr::value>(ref<AssignLevel>)) |
                assign<&ast::YieldExpr::value>(ref<AssignLevel>)))> {};

  struct MEMBER_MODIFIER
      : terminal<((ikw<"public"> | ikw<"protected"> | ikw<"private">) +
                  lit<"("> + ikw<"set"> + lit<")">) |
                 ikw<"public"> | ikw<"protected"> | ikw<"private"> | ikw<"static"> |
                 ikw<"abstract"> | ikw<"final"> | ikw<"readonly">> {};
  static constexpr auto MemberModifier = ref<MEMBER_MODIFIER>;

  struct PropertyHookRule;

  struct ParamRule
      : rule<create<ast::Param>(
            many(append<&ast::Param::attributes>(ref<AttributeGroupRule>)) +
            many(append<&ast::Param::modifiers>(MemberModifier)) +
            option(assign<&ast::Param::type>(ref<TypeRefRule>)) +
            option(flag<&ast::Param::byRef>(lit<"&">)) +
            option(flag<&ast::Param::variadic>(lit<"...">)) +
            assign<&ast::Param::name>(create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>))) +
            option(lit<"="> + assign<&ast::Param::defaultValue>(ref<Expression>)) +
            option(lit<"{"> + many(append<&ast::Param::hooks>(ref<PropertyHookRule>)) + lit<"}">))> {};

  static constexpr auto ParamList =
      many(append<&ast::FunctionDecl::params>(ref<ParamRule>), lit<",">) + option(lit<",">);

  static constexpr auto ReturnTypeTail =
      option(lit<":"> + assign<&ast::FunctionDecl::returnType>(ref<TypeRefRule>));

  struct InnerStatementRule;

  struct GenericArgsBareRule
      : rule<lit<"<:"> + many(cr<"^:"> | lit<":", !lit<">">>) + lit<":>">> {};

  struct IdentifierRule : rule<ref<ID>> {};

  static constexpr auto FunctionHead =
      ikw<"function"> + option(flag<&ast::FunctionDecl::byRef>(lit<"&">)) +
      assign<&ast::FunctionDecl::name>(ref<IdentifierRule>) +
      option(ref<GenericArgsBareRule>) + lit<"("> + ParamList + lit<")"> + ReturnTypeTail;

  struct FunctionDeclRule
      : rule<create<ast::FunctionDecl>(
            many(append<&ast::FunctionDecl::attributes>(ref<AttributeGroupRule>)) +
            FunctionHead + assign<&ast::FunctionDecl::body>(ref<BlockStmtRule>))> {};

  struct FunctionDeclOrProtoRule
      : rule<create<ast::FunctionDecl>(FunctionHead +
            (assign<&ast::FunctionDecl::body>(ref<BlockStmtRule>) | SEMI))> {};

  struct ClassDeclRule;

  static constexpr auto ClassHeaderTail =
      assign<&ast::ClassDecl::kind>(ikw<"class"> | ikw<"trait">) +
      assign<&ast::ClassDecl::name>(ref<IdentifierRule>) + option(ref<GenericArgsBareRule>) +
      option(ikw<"extends"> + assign<&ast::ClassDecl::extendsName>(ref<NameRule>)) +
      option(ikw<"implements"> +
             some(append<&ast::ClassDecl::implementsList>(ref<NameRule>), lit<",">));

  static constexpr auto InterfaceHeaderTail =
      assign<&ast::ClassDecl::kind>(ikw<"interface">) +
      assign<&ast::ClassDecl::name>(ref<IdentifierRule>) + option(ref<GenericArgsBareRule>) +
      option(ikw<"extends"> + some(append<&ast::ClassDecl::extendsList>(ref<NameRule>), lit<",">));

  struct ClassDeclRule
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::attributes>(ref<AttributeGroupRule>)) +
            option(flag<&ast::ClassDecl::isPrivate>(ikw<"private">)) +
            many(append<&ast::ClassDecl::modifiers>(ikw<"abstract"> | ikw<"final"> | ikw<"readonly">)) +
            option(flag<&ast::ClassDecl::isPartial>(ikw<"partial">)) +
            (ClassHeaderTail | InterfaceHeaderTail) + lit<"{"> +
            many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>)) + lit<"}">)> {};

  struct ClassConstRule
      : rule<create<ast::ClassConst>(assign<&ast::ClassConst::name>(ref<IdentifierRule>) +
                                     lit<"="> + assign<&ast::ClassConst::value>(ref<Expression>))> {
  };

  struct PropertyHookRule
      : rule<create<ast::PropertyHook>(
            many(append<&ast::PropertyHook::attributes>(ref<AttributeGroupRule>)) +
            many(append<&ast::PropertyHook::modifiers>(MemberModifier)) +
            option(flag<&ast::PropertyHook::byRef>(lit<"&">)) +
            assign<&ast::PropertyHook::name>(ref<ID>) +
            option(ClosureParams<&ast::PropertyHook::params>) +
            ((lit<"=>"> + assign<&ast::PropertyHook::expression>(ref<Expression>) + SEMI) |
             assign<&ast::PropertyHook::body>(ref<BlockStmtRule>) | SEMI))> {};

  struct PropertyDeclRule
      : rule<create<ast::PropertyDecl>(
            assign<&ast::PropertyDecl::name>(create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>))) +
            option(lit<"="> + assign<&ast::PropertyDecl::defaultValue>(ref<Expression>)) +
            option(lit<"{"> + many(append<&ast::PropertyDecl::hooks>(ref<PropertyHookRule>)) + lit<"}">))> {};

  struct TraitPrecedenceRule
      : rule<create<ast::TraitPrecedence>(
            assign<&ast::TraitPrecedence::traitName>(ref<NameRule>) + lit<"::"> +
            assign<&ast::TraitPrecedence::methodName>(ref<IdentifierRule>) + ikw<"insteadof"> +
            some(append<&ast::TraitPrecedence::insteadOf>(ref<NameRule>), lit<",">) + SEMI)> {};

  struct TraitAliasRule
      : rule<create<ast::TraitAlias>(
            option(assign<&ast::TraitAlias::traitName>(ref<NameRule>) + lit<"::">) +
            assign<&ast::TraitAlias::methodName>(ref<IdentifierRule>) + ikw<"as"> +
            option(assign<&ast::TraitAlias::asModifier>(MemberModifier)) +
            option(assign<&ast::TraitAlias::asName>(ref<IdentifierRule>)) + SEMI)> {};

  struct UseTraitMemberRule
      : rule<create<ast::UseTraitMember>(
            ikw<"use"> + some(append<&ast::UseTraitMember::traits>(ref<NameRule>), lit<",">) +
            (SEMI | (lit<"{"> +
                     many(append<&ast::UseTraitMember::precedences>(ref<TraitPrecedenceRule>) |
                          append<&ast::UseTraitMember::aliases>(ref<TraitAliasRule>)) +
                     lit<"}">)))> {};

  struct MethodMemberRule
      : rule<create<ast::MethodMember>(
            many(append<&ast::MethodMember::attributes>(ref<AttributeGroupRule>)) +
            many(append<&ast::MethodMember::modifiers>(MemberModifier)) +
            assign<&ast::MethodMember::function>(ref<FunctionDeclOrProtoRule>))> {};

  struct ClassMemberRule
      : rule<create<ast::ConstMember>(
                 many(append<&ast::ConstMember::attributes>(ref<AttributeGroupRule>)) +
                 many(append<&ast::ConstMember::modifiers>(MemberModifier)) + ikw<"const"> +
                 option(assign<&ast::ConstMember::type>(ref<TypeRefRule>)) +
                 some(append<&ast::ConstMember::consts>(ref<ClassConstRule>), lit<",">) + SEMI)  //
             | create<ast::PropertyMember>(
                   many(append<&ast::PropertyMember::attributes>(ref<AttributeGroupRule>)) +
                   ((some(append<&ast::PropertyMember::modifiers>(MemberModifier)) +
                     option(assign<&ast::PropertyMember::type>(ref<TypeRefRule>))) |
                    append<&ast::PropertyMember::modifiers>(ikw<"var">)) +
                   some(append<&ast::PropertyMember::props>(ref<PropertyDeclRule>), lit<",">) +
                   option(SEMI))  //
             | ref<MethodMemberRule> | ref<UseTraitMemberRule>> {};

  struct EnumCaseRule
      : rule<create<ast::EnumCase>(many(append<&ast::EnumCase::attributes>(ref<AttributeGroupRule>)) +
                                   ikw<"case"> + assign<&ast::EnumCase::name>(ref<IdentifierRule>) +
                                   option(lit<"="> + assign<&ast::EnumCase::value>(ref<Expression>)) +
                                   SEMI)> {};

  struct EnumDeclRule
      : rule<create<ast::EnumDecl>(
            many(append<&ast::EnumDecl::attributes>(ref<AttributeGroupRule>)) +
            ikw<"enum"> + assign<&ast::EnumDecl::name>(ref<IdentifierRule>) +
            option(lit<":"> + assign<&ast::EnumDecl::backingType>(ikw<"int"> | ikw<"string">)) +
            option(ikw<"implements"> +
                   some(append<&ast::EnumDecl::implementsList>(ref<NameRule>), lit<",">)) +
            lit<"{"> +
            many(append<&ast::EnumDecl::cases>(ref<EnumCaseRule>) |
                 append<&ast::EnumDecl::members>(ref<ClassMemberRule>)) +
            lit<"}">)> {};

  struct UseGroupInnerRule
      : rule<create<ast::UseItem>(option(assign<&ast::UseItem::kind>(ikw<"function"> | ikw<"const">)) +
                                  assign<&ast::UseItem::name>(ref<NameRule>) +
                                  option(ikw<"as"> + assign<&ast::UseItem::alias>(ref<IdentifierRule>)))> {
  };

  struct UseEntryRule
      : rule<create<ast::UseItem>(assign<&ast::UseItem::name>(ref<NameRule>) + lit<"\\"> +
                                  lit<"{"> +
                                  some(append<&ast::UseItem::group>(ref<UseGroupInnerRule>), lit<",">) +
                                  option(lit<",">) + lit<"}">)  //
             | create<ast::UseItem>(assign<&ast::UseItem::name>(ref<NameRule>) +
                                    option(ikw<"as"> + assign<&ast::UseItem::alias>(ref<IdentifierRule>)))> {
  };

  struct UseDeclRule
      : rule<create<ast::UseDecl>(
            ikw<"use"> + option(assign<&ast::UseDecl::kind>(ikw<"function"> | ikw<"const">)) +
            some(append<&ast::UseDecl::items>(ref<UseEntryRule>), lit<",">) + SEMI)> {};

  struct TopLevelItemRule;

  struct NamespaceDeclRule
      : rule<create<ast::NamespaceDecl>(
            ikw<"namespace"> +
            ((option(assign<&ast::NamespaceDecl::name>(ref<NameRule>)) +
              flag<&ast::NamespaceDecl::braced>(lit<"{">) +
              many(append<&ast::NamespaceDecl::items>(ref<TopLevelItemRule>)) + lit<"}">)  //
             | (assign<&ast::NamespaceDecl::name>(ref<NameRule>) + SEMI)))> {};

  struct ImportDeclRule
      : rule<create<ast::ImportDecl>(ikw<"import"> + ikw<"namespace"> +
                                     assign<&ast::ImportDecl::name>(ref<NameRule>) + SEMI)> {};

  struct GlobalConstDeclRule
      : rule<create<ast::GlobalConstDecl>(
            many(append<&ast::GlobalConstDecl::attributes>(ref<AttributeGroupRule>)) +
            ikw<"const"> +
            some(append<&ast::GlobalConstDecl::consts>(ref<ClassConstRule>), lit<",">) + SEMI)> {};

  inline static constexpr auto bridge_text = [](std::string_view text) {
    const std::size_t n = text.size() - text.rfind("<?");
    const std::size_t begin = text.substr(2).starts_with("\r\n") ? 4
                            : text.size() > 2 && (text[2] == '\r' || text[2] == '\n') ? 3 : 2;
    return text.substr(begin, text.size() - begin - n);
  };
  static constexpr auto BridgeBody =
      lit<"?>"> + many(lit<"<", !lit<"?">> | cr<"^<">);
  struct HTML_BRIDGE
      : terminal<BridgeBody + (ref<PHP_OPEN> | lit<"<?">),
                 text_conv<bridge_text>> {};
  struct HTML_ECHO_BRIDGE
      : terminal<BridgeBody + lit<"<?=">,
                 text_conv<bridge_text>> {};
  struct HtmlBridgeRule
      : rule<create<ast::InlineHtmlStmt>(
                 assign<&ast::InlineHtmlStmt::text>(ref<HTML_BRIDGE>)) |
             create<ast::BlockStmt>(
                 append<&ast::BlockStmt::items>(create<ast::InlineHtmlStmt>(
                     assign<&ast::InlineHtmlStmt::text>(ref<HTML_ECHO_BRIDGE>))) +
                 append<&ast::BlockStmt::items>(create<ast::EchoStmt>(
                     some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)))> {};

  struct StatementRule;

  struct BlockStmtRule
      : rule<create<ast::BlockStmt>(lit<"{"> +
                                    many(append<&ast::BlockStmt::items>(ref<InnerStatementRule>)) +
                                    lit<"}">)> {};

  static constexpr auto AltBody =
      create<ast::BlockStmt>(many(append<&ast::BlockStmt::items>(ref<InnerStatementRule>)));

  struct ElseChainRule
      : rule<create<ast::IfStmt>(
                 ikw<"elseif"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseChainRule>)))  //
             | (ikw<"else"> + ref<StatementRule>)> {};

  struct ElseColonChainRule
      : rule<create<ast::IfStmt>(
                 ikw<"elseif"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 lit<":"> + assign<&ast::IfStmt::thenBranch>(AltBody) +
                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseColonChainRule>)))  //
             | (ikw<"else"> + lit<":"> + AltBody)> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(ikw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                                 lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<StatementRule>) +
                                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseChainRule>)))  //
             | create<ast::IfStmt>(ikw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                                   lit<")"> + lit<":"> + assign<&ast::IfStmt::thenBranch>(AltBody) +
                                   option(assign<&ast::IfStmt::elseBranch>(ref<ElseColonChainRule>)) +
                                   ikw<"endif"> + SEMI)> {};

  struct WhileStmtRule
      : rule<create<ast::WhileStmt>(
            ikw<"while"> + lit<"("> + assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
            (assign<&ast::WhileStmt::body>(ref<StatementRule>)  //
             | (lit<":"> + assign<&ast::WhileStmt::body>(AltBody) + ikw<"endwhile"> + SEMI)))> {};

  struct DoWhileStmtRule
      : rule<create<ast::DoWhileStmt>(ikw<"do"> + assign<&ast::DoWhileStmt::body>(ref<StatementRule>) +
                                      ikw<"while"> + lit<"("> +
                                      assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")"> +
                                      SEMI)> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt>(
            ikw<"for"> + lit<"("> +
            many(append<&ast::ForStmt::init>(ref<Expression>), lit<",">) + SEMI +
            many(append<&ast::ForStmt::cond>(ref<Expression>), lit<",">) + SEMI +
            many(append<&ast::ForStmt::update>(ref<Expression>), lit<",">) + lit<")"> +
            (assign<&ast::ForStmt::body>(ref<StatementRule>)  //
             | (lit<":"> + assign<&ast::ForStmt::body>(AltBody) + ikw<"endfor"> + SEMI)))> {};

  struct CaseArmRule
      : rule<create<ast::CaseArm>(
            ((ikw<"case"> + assign<&ast::CaseArm::value>(ref<Expression>)) | ikw<"default">) +
            (lit<":"> | SEMI) +
            many(append<&ast::CaseArm::body>(ref<InnerStatementRule>)))> {};

  struct SwitchStmtRule
      : rule<create<ast::SwitchStmt>(
            ikw<"switch"> + lit<"("> + assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
            ((lit<"{"> + option(SEMI) + many(append<&ast::SwitchStmt::cases>(ref<CaseArmRule>)) +
              lit<"}">)  //
             | (lit<":"> + option(SEMI) + many(append<&ast::SwitchStmt::cases>(ref<CaseArmRule>)) +
                ikw<"endswitch"> + SEMI)))> {};

  struct BreakStmtRule
      : rule<create<ast::BreakStmt>(ikw<"break"> + option(assign<&ast::BreakStmt::level>(ref<Expression>)) +
                                    SEMI)> {};
  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(
            ikw<"continue"> + option(assign<&ast::ContinueStmt::level>(ref<Expression>)) + SEMI)> {};
  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            ikw<"return"> + option(assign<&ast::ReturnStmt::value>(ref<Expression>)) + SEMI)> {};

  struct GlobalStmtRule
      : rule<create<ast::GlobalStmt>(
            ikw<"global"> +
            some(append<&ast::GlobalStmt::vars>(
                     create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)) |
                     (lit<"$"> + ref<ChainExpr>) |
                     (lit<"$"> + lit<"{"> + ref<Expression> + lit<"}">)),
                 lit<",">) +
            SEMI)> {};

  struct StaticVarStmtRule
      : rule<create<ast::StaticVarStmt>(
            ikw<"static"> + some(append<&ast::StaticVarStmt::vars>(ref<PropertyDeclRule>), lit<",">) +
            SEMI)> {};

  struct EchoStmtRule
      : rule<create<ast::EchoStmt>(
            ikw<"echo"> + some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>) + SEMI)> {};

  struct UnsetStmtRule
      : rule<create<ast::UnsetStmt>(ikw<"unset"> + lit<"("> +
                                    some(append<&ast::UnsetStmt::targets>(ref<ChainExpr>), lit<",">) +
                                    lit<")"> + SEMI)> {};

  struct ForeachStmtRule
      : rule<create<ast::ForeachStmt>(
            ikw<"foreach"> + lit<"("> + assign<&ast::ForeachStmt::source>(ref<Expression>) +
            ikw<"as"> + option(assign<&ast::ForeachStmt::key>(ref<AssignableRule>) + lit<"=>">) +
            option(flag<&ast::ForeachStmt::valueByRef>(lit<"&">)) +
            assign<&ast::ForeachStmt::value>(ref<AssignableRule>) + lit<")"> +
            (assign<&ast::ForeachStmt::body>(ref<StatementRule>)  //
             | (lit<":"> + assign<&ast::ForeachStmt::body>(AltBody) + ikw<"endforeach"> + SEMI)))> {
  };

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            ikw<"catch"> + lit<"("> + some(append<&ast::CatchClause::types>(ref<NameRule>), lit<"|">) +
            option(assign<&ast::CatchClause::var>(
                create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)))) +
            lit<")"> + assign<&ast::CatchClause::body>(ref<BlockStmtRule>))> {};

  struct TryStmtRule
      : rule<create<ast::TryStmt>(
            ikw<"try"> + assign<&ast::TryStmt::body>(ref<BlockStmtRule>) +
            ((some(append<&ast::TryStmt::catches>(ref<CatchClauseRule>)) +
              option(ikw<"finally"> + assign<&ast::TryStmt::finallyBody>(ref<BlockStmtRule>)))  //
             | (ikw<"finally"> + assign<&ast::TryStmt::finallyBody>(ref<BlockStmtRule>))))> {};

  struct ThrowStmtRule
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(create<ast::ThrowExpr>(
                ikw<"throw"> + assign<&ast::ThrowExpr::operand>(ref<Expression>))) +
            SEMI)> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(ikw<"goto"> + assign<&ast::GotoStmt::target>(ref<IdentifierRule>) +
                                   SEMI)> {};

  struct DirectiveRule
      : rule<create<ast::Directive>(
            assign<&ast::Directive::name>(ikw<"ticks"> | ikw<"encoding"> | ikw<"strict_types">) +
            lit<"="> + assign<&ast::Directive::value>(ref<Expression>))> {};

  struct DeclareStmtRule
      : rule<create<ast::DeclareStmt>(
            ikw<"declare"> + lit<"("> +
            some(append<&ast::DeclareStmt::directives>(ref<DirectiveRule>), lit<",">) + lit<")"> +
            (assign<&ast::DeclareStmt::body>(ref<StatementRule>)  //
             | (lit<":"> + assign<&ast::DeclareStmt::body>(AltBody) + ikw<"enddeclare"> + SEMI)  //
             | SEMI))> {};

  struct LabelStmtRule
      : rule<create<ast::LabelStmt>(assign<&ast::LabelStmt::name>(ref<IdentifierRule>) + lit<":">)> {
  };

  struct EmptyStmtRule : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct StatementRule
      : rule<ref<HtmlBridgeRule> | ref<BlockStmtRule> | ref<IfStmtRule> |
             ref<WhileStmtRule> | ref<DoWhileStmtRule> | ref<ForStmtRule> |
             ref<SwitchStmtRule> | ref<BreakStmtRule> | ref<ContinueStmtRule> |
             ref<ReturnStmtRule> | ref<GlobalStmtRule> |
             ref<StaticVarStmtRule> | ref<EchoStmtRule> | ref<UnsetStmtRule> |
             ref<ForeachStmtRule> | ref<TryStmtRule> | ref<ThrowStmtRule> |
             ref<GotoStmtRule> | ref<DeclareStmtRule> | ref<EmptyStmtRule> |
             ref<LabelStmtRule> |
             ref<ExprStmtRule>> {};

  struct InnerStatementRule
      : rule<ref<StatementRule> | ref<FunctionDeclRule> | ref<ClassDeclRule>> {};

  struct TopLevelItemRule
      : rule<ref<ImportDeclRule> | ref<UseDeclRule> | ref<NamespaceDeclRule> |
             ref<FunctionDeclRule> | ref<ClassDeclRule> | ref<GlobalConstDeclRule> |
             ref<EnumDeclRule> | ref<StatementRule>> {};

  static constexpr auto PhpItems =
      many(append<&ast::PhpBlock::items>(ref<TopLevelItemRule>));
  static constexpr auto PhpClose = ref<PHP_CLOSE>;
  static constexpr auto PhpOpen = ref<PHP_OPEN> | lit<"<?">;
  static constexpr auto PhpContent =
      (PhpOpen | (lit<"<?="> +
       append<&ast::PhpBlock::items>(create<ast::EchoStmt>(
           some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)))) + PhpItems;

  struct PhpBlockRule
      : rule<create<ast::PhpBlock>(
                 &(PhpOpen | lit<"<?=">) +
                 skip(ignored<WS>, hide<SL_COMMENT, ML_COMMENT>, PhpContent + (PhpClose | eof)))> {};

  struct HtmlChunkRule
      : rule<create<ast::InlineHtmlStmt>(
            assign<&ast::InlineHtmlStmt::text>(ref<HTML_TEXT> | ref<XML_DECL>))> {};

  struct DocumentRule
      : rule<create<ast::Document>(
            option(assign<&ast::Document::shebang>(ref<SHEBANG>)) +
            many(append<&ast::Document::items>(ref<HtmlChunkRule> | ref<PhpBlockRule>)))> {};

  using entry = DocumentRule;
  using ignore = list<>;
  using hidden = list<>;
};

}  // namespace php
