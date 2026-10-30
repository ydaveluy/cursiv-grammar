#pragma once
#include "capnproto/core/StringValue.hpp"
#include "capnproto/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"
namespace capnproto {
struct Capnproto : cursiv::GrammarSpec {
  struct WS : terminal<some(s | lit<"\xef\xbb\xbf">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  struct NAME : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};
  struct STRUCT : terminal<kw<"struct">> {};
  struct INTERFACE : terminal<kw<"interface">> {};
  struct ENUM : terminal<kw<"enum">> {};
  struct ANNOTATION : terminal<kw<"annotation">> {};
  struct CONST : terminal<kw<"const">> {};
  struct USING : terminal<kw<"using">> {};
  struct UNION : terminal<kw<"union">> {};
  struct GROUP : terminal<kw<"group">> {};
  struct EXTENDS : terminal<kw<"extends">> {};
  struct IMPORT : terminal<kw<"import">> {};
  struct EMBED : terminal<kw<"embed">> {};
  struct STREAM : terminal<kw<"stream">> {};
  static constexpr auto Name =
      ref<NAME> | ref<STRUCT> | ref<INTERFACE> | ref<ENUM> | ref<ANNOTATION> |
      ref<CONST> | ref<USING> | ref<UNION> | ref<GROUP> | ref<EXTENDS> |
      ref<IMPORT> | ref<EMBED> | ref<STREAM>;
  static constexpr auto Escape =
      lit<"\\"> + (cr<R"(abfnrtv'"\\?)"> |
                   (lit<"x"> + repeat<2, 2>(cr<"0-9a-fA-F">)) |
                   repeat<1, 3>(cr<"0-7">));
  struct TEXT : terminal<as<decode_text>(
                    lit<"\""> + many(Escape | cr<R"(^"\\\n)">) + lit<"\"">)> {};
  struct LINE_TEXT
      : terminal<as<decode_text>(lit<"`"> + many(cr<R"(^\r\n)">))> {};
  struct INTEGER : terminal<(lit<"0x"> + some(cr<"0-9a-fA-F">)) | some(d)> {};
  struct FLOAT
      : terminal<some(d) + ((lit<"."> + many(d) +
                             option(cr<"eE"> + option(cr<"+-">) + some(d))) |
                            (cr<"eE"> + option(cr<"+-">) + some(d)))> {};
  struct BYTES
      : terminal<as<decode_bytes>(
            lit<"0x\""> + many(s) +
            many(repeat<2, 2>(cr<"0-9a-fA-F">) + many(s)) + lit<"\"">)> {};
  struct Expression;
  struct Decl;
  struct Annotation;
  struct TupleField
      : rule<create<ast::ConstFieldAssign>(
            option(assign<&ast::ConstFieldAssign::name>(Name) + lit<"=">) +
            assign<&ast::ConstFieldAssign::value>(ref<Expression>))> {};
  static constexpr auto TupleFields = option(
      some(append<&ast::ConstStruct::fields>(ref<TupleField>), lit<",">) +
      option(lit<",">));
  struct TextValue : rule<as<join_text>(some(ref<TEXT> | ref<LINE_TEXT>))> {};
  struct NameRule : rule<create<ast::NameExpr>(
                        option(flag<&ast::NameExpr::absolute>(lit<".">)) +
                        assign<&ast::NameExpr::name>(Name))> {};
  struct BaseExpr
      : rule<create<ast::NegativeExpr>(
                 lit<"-"> +
                 assign<&ast::NegativeExpr::operand>(ref<BaseExpr>)) |
             create<ast::IntegerLiteral>(
                 assign<&ast::IntegerLiteral::text>(ref<INTEGER>)) |
             create<ast::FloatLiteral>(
                 assign<&ast::FloatLiteral::text>(ref<FLOAT>)) |
             create<ast::TextLiteral>(
                 assign<&ast::TextLiteral::value>(ref<TextValue>)) |
             create<ast::BytesLiteral>(
                 assign<&ast::BytesLiteral::value>(ref<BYTES>)) |
             create<ast::ImportExpr>(
                 ref<IMPORT> +
                 assign<&ast::ImportExpr::path>(ref<TEXT> | ref<LINE_TEXT>)) |
             create<ast::EmbedExpr>(
                 ref<EMBED> +
                 assign<&ast::EmbedExpr::path>(ref<TEXT> | ref<LINE_TEXT>)) |
             create<ast::ConstList>(
                 lit<"["> +
                 option(some(append<&ast::ConstList::items>(ref<Expression>),
                             lit<",">) +
                        option(lit<",">)) +
                 lit<"]">) |
             create<ast::ConstStruct>(lit<"("> + TupleFields + lit<")">) |
             ref<NameRule>> {};
  static constexpr auto Suffix =
      nest<ast::MemberExpr, &ast::MemberExpr::parentExpr>(
          lit<"."> + assign<&ast::MemberExpr::name>(Name)) |
      nest<ast::ApplicationExpr, &ast::ApplicationExpr::function>(
          lit<"("> +
          option(
              some(append<&ast::ApplicationExpr::params>(ref<TupleField>),
                   lit<",">) +
              option(lit<",">)) +
          lit<")">);
  struct Expression : rule<ref<BaseExpr> + many(Suffix)> {};
  struct Id
      : rule<create<ast::Id>(lit<"@"> + assign<&ast::Id::text>(ref<INTEGER>))> {
  };
  struct Annotation
      : rule<create<ast::AnnotationReference>(
            lit<"$"> +
            assign<&ast::AnnotationReference::expression>(ref<Expression>))> {};
  static constexpr auto Annotations =
      many(append<&ast::Declaration::annotations>(ref<Annotation>));
  static constexpr auto Head =
      assign<&ast::Declaration::name>(Name) +
      option(assign<&ast::Declaration::id>(ref<Id>));
  static constexpr auto Body =
      lit<"{"> + many(append<&ast::Declaration::content>(ref<Decl>)) +
      lit<"}">;
  struct Struct
      : rule<create<ast::StructDef>(
            ref<STRUCT> + Head +
            option(lit<"("> +
                   option(some(append<&ast::StructDef::typeParams>(Name),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<")">) +
            Annotations + Body)> {};
  struct Interface
      : rule<create<ast::InterfaceDef>(
            ref<INTERFACE> + Head +
            option(lit<"("> +
                   option(some(append<&ast::InterfaceDef::typeParams>(Name),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<")">) +
            option(ref<EXTENDS> + lit<"("> +
                   option(some(append<&ast::InterfaceDef::extends>(
                                   ref<Expression>),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<")">) +
            Annotations + Body)> {};
  struct Enum
      : rule<create<ast::EnumDef>(ref<ENUM> + Head + Annotations + Body)> {};
  struct AnnotationDef
      : rule<create<ast::AnnotationDef>(
            ref<ANNOTATION> + Head + lit<"("> +
            option(some(append<&ast::AnnotationDef::targets>(Name | lit<"*">),
                        lit<",">) +
                   option(lit<",">)) +
            lit<")"> + lit<":"> +
            assign<&ast::AnnotationDef::valueType>(ref<Expression>) +
            Annotations + lit<";">)> {};
  struct Const
      : rule<create<ast::ConstDef>(
            ref<CONST> + Head + lit<":"> +
            assign<&ast::ConstDef::type>(ref<Expression>) + lit<"="> +
            assign<&ast::ConstDef::value>(ref<Expression>) + Annotations +
            lit<";">)> {};
  struct Using
      : rule<create<ast::UsingAlias>(
            ref<USING> +
            option(assign<&ast::UsingAlias::name>(Name) + lit<"=">) +
            assign<&ast::UsingAlias::target>(ref<Expression>) + lit<";">)> {};
  struct Field
      : rule<create<ast::FieldDef>(
            Head + lit<":"> + assign<&ast::FieldDef::type>(ref<Expression>) +
            option(lit<"="> +
                   assign<&ast::FieldDef::defaultValue>(ref<Expression>)) +
            Annotations + lit<";">)> {};
  struct Enumerant
      : rule<create<ast::EnumContent>(Head + Annotations + lit<";">)> {};
  struct Group : rule<create<ast::GroupDef>(
                         assign<&ast::GroupDef::name>(Name) + lit<":"> +
                         ref<GROUP> + Annotations + Body)> {};
  struct Union
      : rule<create<ast::UnionDef>(
            (option(
                 assign<&ast::UnionDef::name>(Name) +
                 option(assign<&ast::UnionDef::id>(ref<Id>) +
                        option(flag<&ast::UnionDef::legacy>(lit<"!">))) +
                 lit<":">) +
             ref<UNION>)+Annotations +
            Body)> {};
  struct Param
      : rule<create<ast::Param>(
            assign<&ast::Param::name>(Name) + lit<":"> +
            assign<&ast::Param::type>(ref<Expression>) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expression>)) +
            many(append<&ast::Param::annotations>(ref<Annotation>)))> {};
  struct ParamList
      : rule<create<ast::ParamList>(
            (lit<"("> +
             option(some(append<&ast::ParamList::params>(ref<Param>),
                         lit<",">) +
                    option(lit<",">)) +
             lit<")">) |
            flag<&ast::ParamList::stream>(ref<STREAM>) |
            (!(lit<"("> | ref<STREAM>)+assign<&ast::ParamList::type>(
                ref<Expression>)))> {};
  struct Function
      : rule<create<ast::FunctionDef>(
            Head +
            option(lit<"["> +
                   option(some(append<&ast::FunctionDef::typeParams>(Name),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<"]">) +
            assign<&ast::FunctionDef::params>(ref<ParamList>) +
            option(lit<"->"> +
                   assign<&ast::FunctionDef::results>(ref<ParamList>)) +
            Annotations + lit<";">)> {};
  struct Decl
      : rule<ref<Struct> | ref<Interface> | ref<Enum> |
             ref<AnnotationDef> | ref<Const> | ref<Using> |
             ref<Group> | ref<Union> | ref<Field> |
             ref<Enumerant> | ref<Function> |
             create<ast::FileId>(assign<&ast::FileId::value>(ref<Id>) +
                                 lit<";">) |
             create<ast::AnnotationStmt>(
                 assign<&ast::AnnotationStmt::annotation>(ref<Annotation>) +
                 lit<";">)> {};
  struct Document : rule<create<ast::Document>(many(
                            append<&ast::Document::content>(ref<Decl>)))> {
  };
  using entry = Document;
  using ignored = WS;
  using hidden = list<COMMENT>;
};
}  // namespace capnproto
