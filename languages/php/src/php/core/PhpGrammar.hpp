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
      : terminal<as<unheredoc>(HeredocOpen + mark<HeredocTag>(HeredocId) +
                               HeredocBody)> {};
  struct HEREDOC_BODY_QUOTED
      : terminal<as<unheredoc>(HeredocOpen + lit<"\""> +
                               mark<HeredocTag>(HeredocId) + lit<"\""> +
                               HeredocBody)> {};
  struct NOWDOC_BODY : terminal<as<unheredoc>(HeredocOpen + lit<"'"> +
                                              mark<HeredocTag>(HeredocId) +
                                              lit<"'"> + HeredocBody)> {};

  struct PHP_OPEN
      : terminal<lit<"<?"> + cr<"pP"> + cr<"hH"> + cr<"pP"> +
                 !cr<R"(^ \t\r\n)">> {};
  struct PHP_CLOSE : terminal<lit<"?>"> + option(HeredocNewline)> {};

  static constexpr auto SEMI =
      implied(lit<";">, &ref<PHP_CLOSE> | eof);

  struct Name
      : rule<(flag<&ast::Name::nsRelative>(ikw<"namespace"> + lit<"\\">) |
              option(flag<&ast::Name::rooted>(lit<"\\">))) +
             some(append<&ast::Name::parts>(ref<ID>), lit<"\\">)> {};

  template <bool AllowStatic> struct TypeRef_;
  using TypeRef = TypeRef_<true>;
  using TypeWithoutStatic = TypeRef_<false>;

  struct TYPE_KEYWORD
      : terminal<(
            ikw<"bool"> | ikw<"int"> | ikw<"float"> | ikw<"string"> | ikw<"object"> |
            ikw<"array"> | ikw<"callable"> | ikw<"iterable"> | ikw<"mixed"> |
            ikw<"void"> | ikw<"never"> | ikw<"null"> | ikw<"true"> | ikw<"false">) +
            !lit<"\\">> {};
  struct STATIC_TYPE : terminal<ikw<"static"> + !lit<"\\">> {};

  struct PrimitiveTypeRef
      : rule<assign<&ast::PrimitiveTypeRef::keyword>(ref<TYPE_KEYWORD>)> {};

  static constexpr auto OrdinaryType = ref<PrimitiveTypeRef> |
      (!ref<STATIC_TYPE> + assign<&ast::NamedTypeRef::name>(ref<Name>));
  template <bool AllowStatic>
  static constexpr auto SingleType = [] {
    if constexpr (AllowStatic)
      return create<ast::StaticTypeRef>(ref<STATIC_TYPE>) | OrdinaryType;
    else return OrdinaryType;
  }();
  template <bool AllowStatic>
  struct SingleTypeRef : rule<SingleType<AllowStatic>> {};

  static constexpr auto TypeAmp = lit<"&", !(many(ref<WS> | ref<ML_COMMENT> |
      ref<SL_COMMENT>) + (lit<"$"> | lit<"...">))>;

  template <bool AllowStatic>
  struct IntersectionTypeRef
      : rule<create<ast::IntersectionTypeRef>(
                 append<&ast::IntersectionTypeRef::options>(ref<SingleTypeRef<AllowStatic>>) +
                 TypeAmp +
                 some(append<&ast::IntersectionTypeRef::options>(ref<SingleTypeRef<AllowStatic>>), TypeAmp))> {};

  template <bool AllowStatic>
  struct UnionTypeElement
      : rule<ref<SingleTypeRef<AllowStatic>> |
             (lit<"("> + ref<IntersectionTypeRef<AllowStatic>> + lit<")">)> {};

  template <bool AllowStatic>
  struct TypeRef_
      : rule<create<ast::UnionTypeRef>(
                 append<&ast::UnionTypeRef::options>(ref<UnionTypeElement<AllowStatic>>) + lit<"|"> +
                 some(append<&ast::UnionTypeRef::options>(ref<UnionTypeElement<AllowStatic>>), lit<"|">)) |
             ref<IntersectionTypeRef<AllowStatic>> |
             create<ast::NullableTypeRef>(lit<"?"> +
                 assign<&ast::NullableTypeRef::type>(ref<SingleTypeRef<AllowStatic>>)) |
             ref<SingleTypeRef<AllowStatic>>> {};

  struct Arg;

  struct Attribute
      : rule<assign<&ast::Attribute::name>(ref<Name>) +
             option(lit<"("> + option(some(append<&ast::Attribute::args>(ref<Arg>), lit<",">) +
                           option(lit<",">)) + lit<")">)> {};

  struct AttributeGroup
      : rule<lit<"#["> +
             option(assign<&ast::AttributeGroup::target>(ref<ID>) + lit<":">) +
             some(append<&ast::AttributeGroup::items>(ref<Attribute>), lit<",">) +
             option(lit<",">) + lit<"]">> {};

  struct Expression;
  struct ChainExpr;
  struct Closure;
  struct MatchExpr;
  struct NewExpr;
  struct ArrayExpr;
  struct BlockStmt;
  struct YieldExpr;
  struct ClassMember;

  static constexpr auto FieldNameExpr =
      create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>)) |
      (lit<"{"> + ref<Expression> + lit<"}">) |
      create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>));

  struct SimpleIndexKey
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)) |
             create<ast::IntLit>(assign<&ast::IntLit::text>(ref<SIMPLE_INT>)) |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<ID>))> {};

  struct PlainVariable
      : rule<assign<&ast::Var::name>(ref<VARNAME>)> {};

  static constexpr auto SimplePostfixStart =
      lit<"["> | lit<"->", &ref<ID>> | lit<"?->", &ref<ID>>;
  struct SIMPLE_PLAIN_VAR : terminal<ref<VARNAME> + !SimplePostfixStart> {};

  struct SimpleVarInterp
      : rule<create<ast::Var>(assign<&ast::Var::name>(ref<SIMPLE_PLAIN_VAR>)) |
             (ref<PlainVariable> +
              (nest<ast::IndexExpr, &ast::IndexExpr::base>(
                   lit<"["> + assign<&ast::IndexExpr::index>(ref<SimpleIndexKey>) + lit<"]">) |
               nest<ast::MemberExpr, &ast::MemberExpr::object>(
                   (flag<&ast::MemberExpr::nullsafe>(lit<"?->", &ref<ID>>) |
                    lit<"->", &ref<ID>>) + assign<&ast::MemberExpr::member>(
                       create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))))))> {};

  static constexpr auto LegacyNameStart = ref<ID> + cr<"[}">;
  struct LegacyVariableHead
      : rule<assign<&ast::Var::name>(ref<ID>)> {};

  struct LegacyVarInterp
      : rule<lit<"${", &LegacyNameStart> + ref<LegacyVariableHead> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::base>(lit<"["> +
                 assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">)) + lit<"}">> {};

  struct CurlyInterp
      : rule<&(lit<"{", &lit<"$">> | lit<"${">) +
             skip(ignore<WS>, hide<SL_COMMENT, ML_COMMENT>,
                 (lit<"{", &lit<"$">> + ref<Expression> + lit<"}">) |
                 ref<LegacyVarInterp> |
                 create<ast::Var>(lit<"${", !LegacyNameStart> +
                     assign<&ast::Var::nameExpr>(ref<Expression>) + lit<"}">))> {};

  static constexpr auto StringPiece =
      append<&ast::InterpString::pieces>(
          create<ast::StrText>(assign<&ast::StrText::text>(ref<STR_TEXT> | ref<STR_LONE_CHAR>))) |
      append<&ast::InterpString::pieces>(ref<SimpleVarInterp>) |
      append<&ast::InterpString::pieces>(ref<CurlyInterp>);

  struct DoubleQuoted
      : rule<create<ast::InterpString>(tight((lit<"b\""> | lit<"B\""> | lit<"\"">) + many(StringPiece) + lit<"\"">))> {};

  struct String
      : rule<create<ast::StringLit>(assign<&ast::StringLit::text>(
                 ref<SINGLE_STRING> | ref<HEREDOC_BODY> | ref<HEREDOC_BODY_QUOTED> |
                 ref<NOWDOC_BODY>))> {};

  struct Arg
      : rule<(option(assign<&ast::Arg::name>(ref<ID>) + lit<":">) +
              ((flag<&ast::Arg::spread>(lit<"...">) +
                option(assign<&ast::Arg::value>(ref<Expression>))) |
               assign<&ast::Arg::value>(ref<Expression>))) |
             (flag<&ast::Arg::byRef>(lit<"&">) + assign<&ast::Arg::value>(ref<ChainExpr>))> {
  };

  template <auto Ptr>
  static constexpr auto ArgsInto = lit<"("> +
      option(some(append<Ptr>(ref<Arg>), lit<",">) + option(lit<",">)) + lit<")">;
  static constexpr auto CallArgs = ArgsInto<&ast::CallExpr::args>;

  struct VariableBase
      : rule<ref<PlainVariable> |
             create<ast::Var>(lit<"${"> + assign<&ast::Var::nameExpr>(ref<Expression>) + lit<"}">) |
             create<ast::Var>(lit<"$"> + assign<&ast::Var::nameExpr>(ref<VariableBase>))> {};

  struct KeyedVariable
      : rule<ref<VariableBase> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> + option(assign<&ast::IndexExpr::index>(ref<Expression>)) + lit<"]">) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"{"> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"}">))> {};

  struct ChainOrigin
      : rule<ref<KeyedVariable> | ref<ArrayExpr> | ref<DoubleQuoted> |
             create<ast::NameExpr>(flag<&ast::NameExpr::isStatic>(ikw<"static">)) |
             create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<Name>)) |
             (lit<"("> + ref<Expression> + lit<")">) | ref<NewExpr> |
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
      : rule<create<ast::BoolLit>(assign<&ast::BoolLit::value>(as<true>(ikw<"true">) | as<false>(ikw<"false">))) |
             create<ast::NullLit>(ikw<"null">) | NumberLit | ref<String>> {};

  struct ArrayItem
      : rule<(flag<&ast::ArrayItem::byRef>(lit<"&">) +
              assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
             (assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>"> +
              flag<&ast::ArrayItem::byRef>(lit<"&">) +
              assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
             (option(assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>">) +
              option(flag<&ast::ArrayItem::spread>(lit<"...">)) +
              assign<&ast::ArrayItem::value>(ref<Expression>))> {};

  static constexpr auto ArrayItems =
      option(some(append<&ast::ArrayExpr::items>(ref<ArrayItem>), lit<",">) + option(lit<",">));

  struct ArrayExpr
      : rule<create<ast::ArrayExpr>((ikw<"array"> + lit<"("> + ArrayItems + lit<")">) |
                                    (lit<"["> + ArrayItems + lit<"]">))> {};

  struct Param;

  template <auto Ptr>
  static constexpr auto ClosureParams = lit<"("> +
      option(some(append<Ptr>(ref<Param>), lit<",">) + option(lit<",">)) + lit<")">;

  struct Closure
      : rule<create<ast::Closure>(
            many(append<&ast::Closure::attributes>(ref<AttributeGroup>)) +
            option(flag<&ast::Closure::isStatic>(ikw<"static">)) +
            ((ikw<"function"> + option(flag<&ast::Closure::byRef>(lit<"&">)) +
              ClosureParams<&ast::Closure::params> +
              option(ikw<"use"> + lit<"("> +
                     many(append<&ast::Closure::uses>(create<ast::Var>(
                         option(flag<&ast::Var::byRef>(lit<"&">)) +
                         assign<&ast::Var::name>(ref<VARNAME>))), lit<",">) +
                     option(lit<",">) + lit<")">) +
              option(lit<":"> + assign<&ast::Closure::returnType>(ref<TypeRef>)) +
              assign<&ast::Closure::body>(ref<BlockStmt>)) |
             (ikw<"fn"> + option(flag<&ast::Closure::byRef>(lit<"&">)) +
              ClosureParams<&ast::Closure::params> +
              option(lit<":"> + assign<&ast::Closure::returnType>(ref<TypeRef>)) +
              lit<"=>"> + assign<&ast::Closure::arrowBody>(ref<Expression>))))> {};

  struct MatchExpr
      : rule<create<ast::MatchExpr>(
            ikw<"match"> + lit<"("> + assign<&ast::MatchExpr::subject>(ref<Expression>) + lit<")"> +
            lit<"{"> +
            many(append<&ast::MatchExpr::arms>(create<ast::MatchArm>(
                     (ikw<"default"> |
                      some(append<&ast::MatchArm::conditions>(ref<Expression>), lit<",">)) +
                     lit<"=>"> + assign<&ast::MatchArm::value>(ref<Expression>))),
                 lit<",">) +
            option(lit<",">) + lit<"}">)> {};

  struct ListPattern;

  static constexpr auto ListItemBody =
      option(assign<&ast::ArrayItem::key>(ref<Expression>) + lit<"=>">) +
      ((flag<&ast::ArrayItem::byRef>(lit<"&">) +
        assign<&ast::ArrayItem::value>(ref<ChainExpr>)) |
       assign<&ast::ArrayItem::value>(ref<ListPattern> | ref<ChainExpr>));

  struct ListItem : rule<ListItemBody> {};
  struct ListSlot : rule<create<ast::ArrayItem>(option(ListItemBody))> {};

  static constexpr auto ListItems =
      many(append<&ast::ArrayExpr::items>(ref<ListSlot>) + lit<",">) +
      option(append<&ast::ArrayExpr::items>(ref<ListItem>));

  struct ListPattern
      : rule<create<ast::ArrayExpr>((ikw<"list"> + lit<"("> + ListItems + lit<")">) |
                                    (lit<"["> + ListItems + lit<"]">))> {};

  struct AnonClassTail
      : rule<create<ast::ClassDecl>(
            option(ikw<"extends"> + assign<&ast::ClassDecl::extendsName>(ref<Name>)) +
            option(ikw<"implements"> +
                   some(append<&ast::ClassDecl::implementsList>(ref<Name>), lit<",">)) +
            lit<"{"> + many(append<&ast::ClassDecl::members>(ref<ClassMember>)) + lit<"}">)> {
  };

  static constexpr auto NewArgs = ArgsInto<&ast::NewExpr::args>;

  struct NewTarget
      : rule<(ref<KeyedVariable> |
              create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<Name>)) |
              (lit<"("> + ref<Expression> + lit<")">)) +
             many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"->"> + assign<&ast::MemberExpr::member>(FieldNameExpr)) |
                  nest<ast::StaticAccessExpr, &ast::StaticAccessExpr::scope>(
                      lit<"::"> + assign<&ast::StaticAccessExpr::member>(FieldNameExpr)) |
                  nest<ast::IndexExpr, &ast::IndexExpr::base>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">))> {};

  struct NewExpr
      : rule<create<ast::NewExpr>(
            ikw<"new"> +
            ((flag<&ast::NewExpr::isStatic>(ikw<"static">) + option(NewArgs)) |
             (many(append<&ast::NewExpr::anonymousAttributes>(ref<AttributeGroup>)) +
              many(append<&ast::NewExpr::anonymousModifiers>(ikw<"readonly"> | ikw<"final"> | ikw<"abstract">)) +
              ikw<"class"> + option(NewArgs) +
              assign<&ast::NewExpr::anonClass>(ref<AnonClassTail>)) |
             (assign<&ast::NewExpr::target>(ref<NewTarget>) + option(NewArgs))))> {};

  struct UnaryLevel;
  struct AssignLevel;
  struct Assignable;
  struct Assignment;

  // Distinguish the native PHP8.5 function form through productions:
  // a first plain expression needs a comma; named/spread arguments do not.
  static constexpr auto CloneSpecialArg = create<ast::Arg>(
      (assign<&ast::Arg::name>(ref<ID>) + lit<":"> + assign<&ast::Arg::value>(ref<Expression>)) |
      (flag<&ast::Arg::spread>(lit<"...">) + assign<&ast::Arg::value>(ref<Expression>)));
  struct CloneFunction : rule<create<ast::CloneExpr>(ikw<"clone"> +
      flag<&ast::CloneExpr::callSyntax>(lit<"(">) +
      (lit<")"> |
       (append<&ast::CloneExpr::args>(CloneSpecialArg) +
        many(lit<","> + append<&ast::CloneExpr::args>(ref<Arg>)) + option(lit<",">) + lit<")">) |
       (append<&ast::CloneExpr::args>(create<ast::Arg>(assign<&ast::Arg::value>(ref<Expression>))) + lit<","> +
        option(some(append<&ast::CloneExpr::args>(ref<Arg>), lit<",">) + option(lit<",">)) + lit<")">) |
       (append<&ast::CloneExpr::args>(create<ast::Arg>(flag<&ast::Arg::spread>(lit<"...">))) + lit<")">)))> {};

  struct AtomLevel
      : rule<ref<CloneFunction> | create<ast::CloneExpr>(ikw<"clone"> + assign<&ast::CloneExpr::operand>(ref<AtomLevel>)) |
             create<ast::PrintExpr>(ikw<"print"> + assign<&ast::PrintExpr::operand>(ref<AssignLevel>)) |
             create<ast::IncludeExpr>(
                 assign<&ast::IncludeExpr::keyword>(ikw<"include_once"> | ikw<"include"> |
                                                    ikw<"require_once"> | ikw<"require">) +
                 assign<&ast::IncludeExpr::operand>(ref<Expression>)) |
             create<ast::ThrowExpr>(ikw<"throw"> + assign<&ast::ThrowExpr::operand>(ref<Expression>)) |
             ref<Closure> | ref<MatchExpr> |
             create<ast::IncDecExpr>(assign<&ast::IncDecExpr::operand>(ref<ChainExpr>) +
                                     assign<&ast::IncDecExpr::op>(lit<"++"> | lit<"--">)) |
             ref<ChainExpr> | ref<ScalarExpr> |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<BACKQUOTE_STRING>)) |
             ref<YieldExpr> |
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
      : infix<NotLevel, ast::BinaryExpr,
              levels(left(lit<"*"> | lit<"/"> | lit<"%">),
                     left(lit<"+"> | lit<"-">), left(lit<"<<"> | lit<">>">),
                     left(lit<".">), left(lit<"|>">),
                     nonassoc(lit<"<="> | lit<">="> | lit<"<"> | lit<">">),
                     nonassoc(lit<"==="> | lit<"!=="> | lit<"=="> | lit<"<>"> |
                          lit<"!="> | lit<"<=>">),
                     left(lit<"&", !cr<"&=">>), left(lit<"^">),
                     left(lit<"|", !cr<"|=">>), left(lit<"&&">),
                     left(lit<"||">))> {};

  struct CoalesceLevel
      : infix<BinaryLevel, ast::BinaryExpr, right(lit<"??">)> {};
  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"**="> | lit<"/="> | lit<".="> | lit<"%="> |
      lit<"&="> | lit<"|="> | lit<"^="> | lit<"<<="> | lit<">>="> | lit<"?" "?="> |
      lit<"=", !cr<"=>">>;

  struct ConditionalElse : rule<ref<Assignment> | ref<CoalesceLevel>> {};
  // Full ternaries are non-associative in PHP8. Short ?: chains remain
  // left-associative; parentheses are supplied by ChainOrigin.
  struct ShortConditional : rule<ref<CoalesceLevel> |
      create<ast::ConditionalExpr>(assign<&ast::ConditionalExpr::cond>(ref<ShortConditional>) +
          lit<"?"> + lit<":"> + assign<&ast::ConditionalExpr::elseValue>(ref<ConditionalElse>))> {};
  struct ConditionalLevel : rule<
      create<ast::ConditionalExpr>(assign<&ast::ConditionalExpr::cond>(ref<CoalesceLevel>) +
          lit<"?"> + assign<&ast::ConditionalExpr::thenValue>(ref<Expression>) + lit<":"> +
          assign<&ast::ConditionalExpr::elseValue>(ref<ConditionalElse>)) |
      ref<ShortConditional>> {};

  struct Assignable : rule<ref<ListPattern> | ref<ChainExpr>> {};

  struct Assignment
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<Assignable>) +
                 assign<&ast::AssignExpr::op>(lit<"=">) + flag<&ast::AssignExpr::byRef>(lit<"&">) +
                 assign<&ast::AssignExpr::value>(ref<ChainExpr>))  //
             | create<ast::AssignExpr>(
                   assign<&ast::AssignExpr::target>(ref<Assignable>) +
                   assign<&ast::AssignExpr::op>(AssignOp) +
                   assign<&ast::AssignExpr::value>(ref<AssignLevel>))  //
             > {};
  struct AssignLevel : rule<ref<Assignment> | ref<ConditionalLevel>> {};

  struct Expression
      : infix<AssignLevel, ast::BinaryExpr,
              levels(left(ikw<"and">), left(ikw<"xor">), left(ikw<"or">))> {};

  struct YieldExpr
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
  struct PARAM_MODIFIER
      : terminal<((ikw<"public"> | ikw<"protected"> | ikw<"private">) +
                  option(lit<"("> + ikw<"set"> + lit<")">)) |
                 ikw<"final"> | ikw<"readonly">> {};

  struct PropertyHook;

  struct Param
      : rule<create<ast::Param>(
            many(append<&ast::Param::attributes>(ref<AttributeGroup>)) +
            many(append<&ast::Param::modifiers>(ref<PARAM_MODIFIER>)) +
            option(assign<&ast::Param::type>(ref<TypeWithoutStatic>)) +
            option(flag<&ast::Param::byRef>(lit<"&">)) +
            option(flag<&ast::Param::variadic>(lit<"...">)) +
            assign<&ast::Param::name>(create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>))) +
            option(lit<"="> + assign<&ast::Param::defaultValue>(ref<Expression>)) +
            option(lit<"{"> + some(append<&ast::Param::hooks>(ref<PropertyHook>)) + lit<"}">))> {};

  static constexpr auto ParamList =
      option(some(append<&ast::FunctionDecl::params>(ref<Param>), lit<",">) + option(lit<",">));

  static constexpr auto ReturnTypeTail =
      option(lit<":"> + assign<&ast::FunctionDecl::returnType>(ref<TypeRef>));

  struct InnerStatement;

  struct Identifier : rule<ref<ID>> {};

  static constexpr auto FunctionHead =
      ikw<"function"> + option(flag<&ast::FunctionDecl::byRef>(lit<"&">)) +
      assign<&ast::FunctionDecl::name>(ref<Identifier>) +
      lit<"("> + ParamList + lit<")"> + ReturnTypeTail;

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
            many(append<&ast::FunctionDecl::attributes>(ref<AttributeGroup>)) +
            FunctionHead + assign<&ast::FunctionDecl::body>(ref<BlockStmt>))> {};

  struct FunctionDeclOrProto
      : rule<create<ast::FunctionDecl>(FunctionHead +
            (assign<&ast::FunctionDecl::body>(ref<BlockStmt>) | SEMI))> {};

  struct ClassDecl;

  static constexpr auto ClassHeaderTail =
      assign<&ast::ClassDecl::kind>(ikw<"class"> | ikw<"trait">) +
      assign<&ast::ClassDecl::name>(ref<Identifier>) +
      option(ikw<"extends"> + assign<&ast::ClassDecl::extendsName>(ref<Name>)) +
      option(ikw<"implements"> +
             some(append<&ast::ClassDecl::implementsList>(ref<Name>), lit<",">));

  static constexpr auto InterfaceHeaderTail =
      assign<&ast::ClassDecl::kind>(ikw<"interface">) +
      assign<&ast::ClassDecl::name>(ref<Identifier>) +
      option(ikw<"extends"> + some(append<&ast::ClassDecl::extendsList>(ref<Name>), lit<",">));

  struct ClassDecl
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::attributes>(ref<AttributeGroup>)) +
            option(flag<&ast::ClassDecl::isPrivate>(ikw<"private">)) +
            many(append<&ast::ClassDecl::modifiers>(ikw<"abstract"> | ikw<"final"> | ikw<"readonly">)) +
            option(flag<&ast::ClassDecl::isPartial>(ikw<"partial">)) +
            (ClassHeaderTail | InterfaceHeaderTail) + lit<"{"> +
            many(append<&ast::ClassDecl::members>(ref<ClassMember>)) + lit<"}">)> {};

  struct ClassConst
      : rule<create<ast::ClassConst>(assign<&ast::ClassConst::name>(ref<Identifier>) +
                                     lit<"="> + assign<&ast::ClassConst::value>(ref<Expression>))> {
  };

  struct PropertyHook
      : rule<create<ast::PropertyHook>(
            many(append<&ast::PropertyHook::attributes>(ref<AttributeGroup>)) +
            many(append<&ast::PropertyHook::modifiers>(ikw<"final">)) +
            option(flag<&ast::PropertyHook::byRef>(lit<"&">)) +
            assign<&ast::PropertyHook::name>(ref<ID>) +
            option(flag<&ast::PropertyHook::hasParams>(lit<"(">) +
                   option(some(append<&ast::PropertyHook::params>(ref<Param>), lit<",">) +
                          option(lit<",">)) + lit<")">) +
            ((lit<"=>"> + assign<&ast::PropertyHook::expression>(ref<Expression>) + SEMI) |
             assign<&ast::PropertyHook::body>(ref<BlockStmt>) | SEMI))> {};

  static constexpr auto PropertyHead =
      assign<&ast::PropertyDecl::name>(create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>))) +
      option(lit<"="> + assign<&ast::PropertyDecl::defaultValue>(ref<Expression>));
  struct PropertyDecl : rule<create<ast::PropertyDecl>(PropertyHead)> {};
  struct HookedProperty : rule<create<ast::PropertyDecl>(PropertyHead + lit<"{"> +
      some(append<&ast::PropertyDecl::hooks>(ref<PropertyHook>)) + lit<"}">)> {};

  struct TraitPrecedence
      : rule<create<ast::TraitPrecedence>(
            assign<&ast::TraitPrecedence::traitName>(ref<Name>) + lit<"::"> +
            assign<&ast::TraitPrecedence::methodName>(ref<Identifier>) + ikw<"insteadof"> +
            some(append<&ast::TraitPrecedence::insteadOf>(ref<Name>), lit<",">) + SEMI)> {};

  struct TraitAlias
      : rule<create<ast::TraitAlias>(
            option(assign<&ast::TraitAlias::traitName>(ref<Name>) + lit<"::">) +
            assign<&ast::TraitAlias::methodName>(ref<Identifier>) + ikw<"as"> +
            option(assign<&ast::TraitAlias::asModifier>(MemberModifier)) +
            option(assign<&ast::TraitAlias::asName>(ref<Identifier>)) + SEMI)> {};

  struct UseTraitMember
      : rule<create<ast::UseTraitMember>(
            ikw<"use"> + some(append<&ast::UseTraitMember::traits>(ref<Name>), lit<",">) +
            (SEMI | (lit<"{"> +
                     many(append<&ast::UseTraitMember::precedences>(ref<TraitPrecedence>) |
                          append<&ast::UseTraitMember::aliases>(ref<TraitAlias>)) +
                     lit<"}">)))> {};

  struct MethodMember
      : rule<create<ast::MethodMember>(
            many(append<&ast::MethodMember::attributes>(ref<AttributeGroup>)) +
            many(append<&ast::MethodMember::modifiers>(MemberModifier)) +
            assign<&ast::MethodMember::function>(ref<FunctionDeclOrProto>))> {};

  struct ClassMember
      : rule<create<ast::ConstMember>(
                 many(append<&ast::ConstMember::attributes>(ref<AttributeGroup>)) +
                 many(append<&ast::ConstMember::modifiers>(MemberModifier)) + ikw<"const"> +
                 option(assign<&ast::ConstMember::type>(ref<TypeRef>)) +
                 some(append<&ast::ConstMember::consts>(ref<ClassConst>), lit<",">) + SEMI)  //
             | create<ast::PropertyMember>(
                   many(append<&ast::PropertyMember::attributes>(ref<AttributeGroup>)) +
                   ((some(append<&ast::PropertyMember::modifiers>(MemberModifier)) +
                     option(assign<&ast::PropertyMember::type>(ref<TypeWithoutStatic>))) |
                    append<&ast::PropertyMember::modifiers>(ikw<"var">)) +
                   ((some(append<&ast::PropertyMember::props>(ref<PropertyDecl>), lit<",">) + SEMI) |
                    append<&ast::PropertyMember::props>(ref<HookedProperty>)))  //
             | ref<MethodMember> | ref<UseTraitMember>> {};

  struct EnumCase
      : rule<create<ast::EnumCase>(many(append<&ast::EnumCase::attributes>(ref<AttributeGroup>)) +
                                   ikw<"case"> + assign<&ast::EnumCase::name>(ref<Identifier>) +
                                   option(lit<"="> + assign<&ast::EnumCase::value>(ref<Expression>)) +
                                   SEMI)> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
            many(append<&ast::EnumDecl::attributes>(ref<AttributeGroup>)) +
            ikw<"enum"> + assign<&ast::EnumDecl::name>(ref<Identifier>) +
            option(lit<":"> + assign<&ast::EnumDecl::backingType>(ikw<"int"> | ikw<"string">)) +
            option(ikw<"implements"> +
                   some(append<&ast::EnumDecl::implementsList>(ref<Name>), lit<",">)) +
            lit<"{"> +
            many(append<&ast::EnumDecl::cases>(ref<EnumCase>) |
                 append<&ast::EnumDecl::members>(ref<ClassMember>)) +
            lit<"}">)> {};

  struct UseGroupInner
      : rule<create<ast::UseItem>(option(assign<&ast::UseItem::kind>(ikw<"function"> | ikw<"const">)) +
                                  assign<&ast::UseItem::name>(ref<Name>) +
                                  option(ikw<"as"> + assign<&ast::UseItem::alias>(ref<Identifier>)))> {
  };

  struct UseEntry
      : rule<create<ast::UseItem>(assign<&ast::UseItem::name>(ref<Name>) + lit<"\\"> +
                                  lit<"{"> +
                                  some(append<&ast::UseItem::group>(ref<UseGroupInner>), lit<",">) +
                                  option(lit<",">) + lit<"}">)  //
             | create<ast::UseItem>(assign<&ast::UseItem::name>(ref<Name>) +
                                    option(ikw<"as"> + assign<&ast::UseItem::alias>(ref<Identifier>)))> {
  };

  struct UseDecl
      : rule<create<ast::UseDecl>(
            ikw<"use"> + option(assign<&ast::UseDecl::kind>(ikw<"function"> | ikw<"const">)) +
            some(append<&ast::UseDecl::items>(ref<UseEntry>), lit<",">) + SEMI)> {};

  struct TopLevelItem;

  struct NamespaceDecl
      : rule<create<ast::NamespaceDecl>(
            ikw<"namespace"> +
            ((option(assign<&ast::NamespaceDecl::name>(ref<Name>)) +
              flag<&ast::NamespaceDecl::braced>(lit<"{">) +
              many(append<&ast::NamespaceDecl::items>(ref<TopLevelItem>)) + lit<"}">)  //
             | (assign<&ast::NamespaceDecl::name>(ref<Name>) + SEMI)))> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(ikw<"import"> + ikw<"namespace"> +
                                     assign<&ast::ImportDecl::name>(ref<Name>) + SEMI)> {};

  struct GlobalConstDecl
      : rule<create<ast::GlobalConstDecl>(
            many(append<&ast::GlobalConstDecl::attributes>(ref<AttributeGroup>)) +
            ikw<"const"> +
            some(append<&ast::GlobalConstDecl::consts>(ref<ClassConst>), lit<",">) + SEMI)> {};

  inline static constexpr auto bridge_text = [](std::string_view text) {
    const std::size_t n = text.size() - text.rfind("<?");
    const std::size_t begin = text.substr(2).starts_with("\r\n") ? 4
                            : text.size() > 2 && (text[2] == '\r' || text[2] == '\n') ? 3 : 2;
    return text.substr(begin, text.size() - begin - n);
  };
  static constexpr auto BridgeBody =
      lit<"?>"> + many(lit<"<", !lit<"?">> | cr<"^<">);
  struct HTML_BRIDGE
      : terminal<as<bridge_text>(BridgeBody + (ref<PHP_OPEN> | lit<"<?">))> {};
  struct HTML_ECHO_BRIDGE : terminal<as<bridge_text>(BridgeBody + lit<"<?=">)> {
  };
  struct HtmlBridge
      : rule<create<ast::InlineHtmlStmt>(
                 assign<&ast::InlineHtmlStmt::text>(ref<HTML_BRIDGE>)) |
             create<ast::BlockStmt>(
                 append<&ast::BlockStmt::items>(create<ast::InlineHtmlStmt>(
                     assign<&ast::InlineHtmlStmt::text>(ref<HTML_ECHO_BRIDGE>))) +
                 append<&ast::BlockStmt::items>(create<ast::EchoStmt>(
                     some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)))> {};

  struct Statement;

  struct BlockStmt
      : rule<create<ast::BlockStmt>(lit<"{"> +
                                    many(append<&ast::BlockStmt::items>(ref<InnerStatement>)) +
                                    lit<"}">)> {};

  static constexpr auto AltBody =
      create<ast::BlockStmt>(many(append<&ast::BlockStmt::items>(ref<InnerStatement>)));

  struct ElseChain
      : rule<create<ast::IfStmt>(
                 ikw<"elseif"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseChain>)))  //
             | (ikw<"else"> + ref<Statement>)> {};

  struct ElseColonChain
      : rule<create<ast::IfStmt>(
                 ikw<"elseif"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 lit<":"> + assign<&ast::IfStmt::thenBranch>(AltBody) +
                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseColonChain>)))  //
             | (ikw<"else"> + lit<":"> + AltBody)> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(ikw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                                 lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                                 option(assign<&ast::IfStmt::elseBranch>(ref<ElseChain>)))  //
             | create<ast::IfStmt>(ikw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                                   lit<")"> + lit<":"> + assign<&ast::IfStmt::thenBranch>(AltBody) +
                                   option(assign<&ast::IfStmt::elseBranch>(ref<ElseColonChain>)) +
                                   ikw<"endif"> + SEMI)> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            ikw<"while"> + lit<"("> + assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
            (assign<&ast::WhileStmt::body>(ref<Statement>)  //
             | (lit<":"> + assign<&ast::WhileStmt::body>(AltBody) + ikw<"endwhile"> + SEMI)))> {};

  struct DoWhileStmt
      : rule<create<ast::DoWhileStmt>(ikw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
                                      ikw<"while"> + lit<"("> +
                                      assign<&ast::DoWhileStmt::cond>(ref<Expression>) + lit<")"> +
                                      SEMI)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            ikw<"for"> + lit<"("> +
            many(append<&ast::ForStmt::init>(ref<Expression>), lit<",">) + SEMI +
            many(append<&ast::ForStmt::cond>(ref<Expression>), lit<",">) + SEMI +
            many(append<&ast::ForStmt::update>(ref<Expression>), lit<",">) + lit<")"> +
            (assign<&ast::ForStmt::body>(ref<Statement>)  //
             | (lit<":"> + assign<&ast::ForStmt::body>(AltBody) + ikw<"endfor"> + SEMI)))> {};

  struct CaseArm
      : rule<create<ast::CaseArm>(
            ((ikw<"case"> + assign<&ast::CaseArm::value>(ref<Expression>)) | ikw<"default">) +
            (lit<":"> | SEMI) +
            many(append<&ast::CaseArm::body>(ref<InnerStatement>)))> {};

  struct SwitchStmt
      : rule<create<ast::SwitchStmt>(
            ikw<"switch"> + lit<"("> + assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
            ((lit<"{"> + option(SEMI) + many(append<&ast::SwitchStmt::cases>(ref<CaseArm>)) +
              lit<"}">)  //
             | (lit<":"> + option(SEMI) + many(append<&ast::SwitchStmt::cases>(ref<CaseArm>)) +
                ikw<"endswitch"> + SEMI)))> {};

  struct BreakStmt
      : rule<create<ast::BreakStmt>(ikw<"break"> + option(assign<&ast::BreakStmt::level>(ref<Expression>)) +
                                    SEMI)> {};
  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(
            ikw<"continue"> + option(assign<&ast::ContinueStmt::level>(ref<Expression>)) + SEMI)> {};
  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"return"> + option(assign<&ast::ReturnStmt::value>(ref<Expression>)) + SEMI)> {};

  struct GlobalStmt
      : rule<create<ast::GlobalStmt>(
            ikw<"global"> +
            some(append<&ast::GlobalStmt::vars>(
                     create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)) |
                     (lit<"$"> + ref<ChainExpr>) |
                     (lit<"$"> + lit<"{"> + ref<Expression> + lit<"}">)),
                 lit<",">) +
            SEMI)> {};

  struct StaticVarStmt
      : rule<create<ast::StaticVarStmt>(
            ikw<"static"> + some(append<&ast::StaticVarStmt::vars>(ref<PropertyDecl>), lit<",">) +
            SEMI)> {};

  struct EchoStmt
      : rule<create<ast::EchoStmt>(
            ikw<"echo"> + some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)> {};

  struct ExprStmt
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>) + SEMI)> {};

  struct UnsetStmt
      : rule<create<ast::UnsetStmt>(ikw<"unset"> + lit<"("> +
                                    some(append<&ast::UnsetStmt::targets>(ref<ChainExpr>), lit<",">) +
                                    lit<")"> + SEMI)> {};

  struct ForeachStmt
      : rule<create<ast::ForeachStmt>(
            ikw<"foreach"> + lit<"("> + assign<&ast::ForeachStmt::source>(ref<Expression>) +
            ikw<"as"> + option(assign<&ast::ForeachStmt::key>(ref<Assignable>) + lit<"=>">) +
            option(flag<&ast::ForeachStmt::valueByRef>(lit<"&">)) +
            assign<&ast::ForeachStmt::value>(ref<Assignable>) + lit<")"> +
            (assign<&ast::ForeachStmt::body>(ref<Statement>)  //
             | (lit<":"> + assign<&ast::ForeachStmt::body>(AltBody) + ikw<"endforeach"> + SEMI)))> {
  };

  struct CatchClause
      : rule<create<ast::CatchClause>(
            ikw<"catch"> + lit<"("> + some(append<&ast::CatchClause::types>(ref<Name>), lit<"|">) +
            option(assign<&ast::CatchClause::var>(
                create<ast::Var>(assign<&ast::Var::name>(ref<VARNAME>)))) +
            lit<")"> + assign<&ast::CatchClause::body>(ref<BlockStmt>))> {};

  struct TryStmt
      : rule<create<ast::TryStmt>(
            ikw<"try"> + assign<&ast::TryStmt::body>(ref<BlockStmt>) +
            ((some(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
              option(ikw<"finally"> + assign<&ast::TryStmt::finallyBody>(ref<BlockStmt>)))  //
             | (ikw<"finally"> + assign<&ast::TryStmt::finallyBody>(ref<BlockStmt>))))> {};

  struct ThrowStmt
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(create<ast::ThrowExpr>(
                ikw<"throw"> + assign<&ast::ThrowExpr::operand>(ref<Expression>))) +
            SEMI)> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(ikw<"goto"> + assign<&ast::GotoStmt::target>(ref<Identifier>) +
                                   SEMI)> {};

  struct Directive
      : rule<create<ast::Directive>(
            assign<&ast::Directive::name>(ikw<"ticks"> | ikw<"encoding"> | ikw<"strict_types">) +
            lit<"="> + assign<&ast::Directive::value>(ref<Expression>))> {};

  struct DeclareStmt
      : rule<create<ast::DeclareStmt>(
            ikw<"declare"> + lit<"("> +
            some(append<&ast::DeclareStmt::directives>(ref<Directive>), lit<",">) + lit<")"> +
            (assign<&ast::DeclareStmt::body>(ref<Statement>)  //
             | (lit<":"> + assign<&ast::DeclareStmt::body>(AltBody) + ikw<"enddeclare"> + SEMI)  //
             | SEMI))> {};

  struct LabelStmt
      : rule<create<ast::LabelStmt>(assign<&ast::LabelStmt::name>(ref<Identifier>) + lit<":">)> {
  };

  struct EmptyStmt : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct Statement
      : rule<ref<HtmlBridge> | ref<BlockStmt> | ref<IfStmt> |
             ref<WhileStmt> | ref<DoWhileStmt> | ref<ForStmt> |
             ref<SwitchStmt> | ref<BreakStmt> | ref<ContinueStmt> |
             ref<ReturnStmt> | ref<GlobalStmt> |
             ref<StaticVarStmt> | ref<EchoStmt> | ref<UnsetStmt> |
             ref<ForeachStmt> | ref<TryStmt> | ref<ThrowStmt> |
             ref<GotoStmt> | ref<DeclareStmt> | ref<EmptyStmt> |
             ref<LabelStmt> |
             ref<ExprStmt>> {};

  struct InnerStatement
      : rule<ref<Statement> | ref<FunctionDecl> | ref<ClassDecl>> {};

  struct TopLevelItem
      : rule<ref<ImportDecl> | ref<UseDecl> | ref<NamespaceDecl> |
             ref<FunctionDecl> | ref<ClassDecl> | ref<GlobalConstDecl> |
             ref<EnumDecl> | ref<Statement>> {};

  static constexpr auto PhpItems =
      many(append<&ast::PhpBlock::items>(ref<TopLevelItem>));
  static constexpr auto PhpClose = ref<PHP_CLOSE>;
  static constexpr auto PhpOpen = ref<PHP_OPEN> | lit<"<?">;
  static constexpr auto PhpContent =
      (PhpOpen | (lit<"<?="> +
       append<&ast::PhpBlock::items>(create<ast::EchoStmt>(
           some(append<&ast::EchoStmt::values>(ref<Expression>), lit<",">) + SEMI)))) + PhpItems;

  struct PhpBlock
      : rule<create<ast::PhpBlock>(
                 &(PhpOpen | lit<"<?=">) +
                 skip(ignore<WS>, hide<SL_COMMENT, ML_COMMENT>, PhpContent + (PhpClose | eof)))> {};

  struct HtmlChunk
      : rule<create<ast::InlineHtmlStmt>(
            assign<&ast::InlineHtmlStmt::text>(ref<HTML_TEXT> | ref<XML_DECL>))> {};

  struct Document
      : rule<create<ast::Document>(
            option(assign<&ast::Document::shebang>(ref<SHEBANG>)) +
            many(append<&ast::Document::items>(ref<HtmlChunk> | ref<PhpBlock>)))> {};

  using entry = Document;
  using ignored = list<>;
  using hidden = list<>;
};

}  // namespace php
