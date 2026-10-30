// The oncrpc language: XDR (RFC 4506) type/constant definitions plus
// ONCRPCv2's (RFC 5531) `program`/`version`/procedure declarations on top.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "oncrpc/core/ast.hpp"

namespace oncrpc {

struct Oncrpc : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // rpcsvc-proto1.4.4 scanner symbols; runtime-reserved typedef names are
  // a separate semantic check and remain available to the parser.
  static constexpr auto Reserved =
      kw<"const"> | kw<"union"> | kw<"switch"> | kw<"case"> | kw<"default"> |
      kw<"struct"> | kw<"typedef"> | kw<"enum"> | kw<"opaque"> | kw<"bool"> |
      kw<"void"> | kw<"char"> | kw<"int"> | kw<"unsigned"> | kw<"short"> |
      kw<"long"> | kw<"hyper"> | kw<"float"> | kw<"double"> | kw<"string"> |
      kw<"program"> | kw<"version">;
  struct ID : terminal<!Reserved + cr<"a-zA-Z_"> + many(w)> {};

  struct NUMBER : terminal<option(cr<"+-">) +
      ((lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) | some(d))> {};
  static constexpr auto escape = lit<"\\"> + any;
  struct STRING : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">> {};
  struct CHARACTER : terminal<lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">> {};
  static constexpr auto ValueExpr = ref<NUMBER> | ref<ID>;



  struct PrimitiveType
      : rule<assign<&ast::PrimitiveType::kind>(
            as<ast::Primitive::unsigned_char>(kw<"unsigned"> + kw<"char">) | as<ast::Primitive::unsigned_short>(kw<"unsigned"> + kw<"short"> + option(kw<"int">)) | as<ast::Primitive::unsigned_long>(kw<"unsigned"> + kw<"long"> + option(kw<"int">)) |
            as<ast::Primitive::unsigned_hyper>(kw<"unsigned"> + kw<"hyper"> + option(kw<"int">)) | as<ast::Primitive::unsigned_int>(kw<"unsigned"> + option(kw<"int">)) | as<ast::Primitive::int_>(kw<"int">) |
            as<ast::Primitive::hyper>(kw<"hyper"> + option(kw<"int">)) | as<ast::Primitive::char_>(kw<"char">) | as<ast::Primitive::short_>(kw<"short"> + option(kw<"int">)) | as<ast::Primitive::long_>(kw<"long"> + option(kw<"int">)) | as<ast::Primitive::float_>(kw<"float">) | as<ast::Primitive::double_>(kw<"double">) |
            as<ast::Primitive::quadruple>(kw<"quadruple">) | as<ast::Primitive::bool_>(kw<"bool">))> {};

  struct NamedType : rule<
      option(assign<&ast::NamedType::prefix>(kw<"struct"> | kw<"enum"> | kw<"union">)) +
      assign<&ast::NamedType::name>(ref<ID>)> {};

  struct OpaqueType : rule<create<ast::OpaqueType> + kw<"opaque">> {};
  struct StringType : rule<create<ast::StringType> + kw<"string">> {};

  struct Declaration;

  struct EnumConst
      : rule<assign<&ast::EnumConst::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::EnumConst::value>(ValueExpr))> {};

  struct EnumBody
      : rule<lit<"{"> +
             some(append<&ast::EnumType::consts>(ref<EnumConst>),
                  lit<",">) +
             lit<"}">> {};

  struct EnumTypeSpec : rule<kw<"enum"> + ref<EnumBody>> {};

  struct StructBody
      : rule<lit<"{"> +
             some(append<&ast::StructType::members>(ref<Declaration>) +
                  lit<";">) +
             lit<"}">> {};

  struct StructTypeSpec : rule<kw<"struct"> + ref<StructBody>> {};

  struct CaseLabel
      : rule<kw<"case"> + assign<&ast::CaseLabel::value>(ValueExpr | ref<CHARACTER>) +
             lit<":">> {};

  struct CaseSpec
      : rule<some(append<&ast::CaseSpec::labels>(ref<CaseLabel>)) +
             assign<&ast::CaseSpec::body>(ref<Declaration>) + lit<";">> {
  };

  struct UnionBody
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::UnionType::discriminant>(ref<Declaration>) +
             lit<")"> + lit<"{"> +
             some(append<&ast::UnionType::cases>(ref<CaseSpec>)) +
             option(kw<"default"> + lit<":"> +
                    assign<&ast::UnionType::defaultCase>(
                        ref<Declaration>) +
                    lit<";">) +
             lit<"}">> {};

  struct UnionTypeSpec : rule<kw<"union"> + ref<UnionBody>> {};

  struct TypeSpecifier
      : rule<ref<PrimitiveType> | ref<EnumTypeSpec> |
             ref<StructTypeSpec> | ref<UnionTypeSpec> |
             ref<OpaqueType> | ref<StringType> | ref<NamedType>> {};

  static constexpr auto ArrayOpen = ref<rule<as<ast::Bound::fixed>(lit<"[">)>>;
  static constexpr auto VarOpen = ref<rule<as<ast::Bound::variable>(lit<"<">)>>;

  static constexpr auto ArraySuffix =
      (assign<&ast::ValueDecl::bound>(ArrayOpen) +
       assign<&ast::ValueDecl::size>(ValueExpr) + lit<"]">) |
      (assign<&ast::ValueDecl::bound>(VarOpen) +
       option(assign<&ast::ValueDecl::size>(ValueExpr)) + lit<">">);

  struct VoidDecl : rule<create<ast::VoidDecl> + kw<"void">> {};

  struct ValueDecl
      : rule<assign<&ast::ValueDecl::type>(ref<TypeSpecifier>) +
             option(flag<&ast::ValueDecl::pointer>(lit<"*">)) +
             assign<&ast::ValueDecl::name>(ref<ID>) + option(ArraySuffix)> {};

  struct Argument : rule<
      create<ast::VoidDecl>(kw<"void"> + option(assign<&ast::VoidDecl::name>(ref<ID>))) |
      create<ast::ValueDecl>(assign<&ast::ValueDecl::type>(ref<TypeSpecifier>) +
          option((flag<&ast::ValueDecl::pointer>(lit<"*">) +
                  option(assign<&ast::ValueDecl::name>(ref<ID>))) |
                 (assign<&ast::ValueDecl::name>(ref<ID>) +
                  option(flag<&ast::ValueDecl::pointer>(lit<"*">) +
                         option(append<&ast::ValueDecl::extraNames>(ref<ID>))))) +
          option(ArraySuffix))> {};

  struct Declaration
      : rule<ref<VoidDecl> | ref<ValueDecl>> {};

  struct TypedefDef
      : rule<kw<"typedef"> + assign<&ast::TypedefDef::decl>(
                                  ref<Declaration>) +
             lit<";">> {};

  struct EnumDef
      : rule<kw<"enum"> + assign<&ast::EnumDef::name>(ref<ID>) +
             assign<&ast::EnumDef::body>(ref<EnumBody>) + lit<";">> {};

  struct StructDef
      : rule<kw<"struct"> + assign<&ast::StructDef::name>(ref<ID>) +
             assign<&ast::StructDef::body>(ref<StructBody>) + lit<";">> {
  };

  struct UnionDef
      : rule<kw<"union"> + assign<&ast::UnionDef::name>(ref<ID>) +
             assign<&ast::UnionDef::body>(ref<UnionBody>) + lit<";">> {};

  struct ConstantDef
      : rule<kw<"const"> + assign<&ast::ConstantDef::name>(ref<ID>) +
             lit<"="> + assign<&ast::ConstantDef::value>(ValueExpr | ref<STRING>) +
             lit<";">> {};

  struct Definition
      : rule<ref<TypedefDef> | ref<EnumDef> | ref<StructDef> |
             ref<UnionDef> | ref<ConstantDef>> {};

  struct ProcedureDef
      : rule<(assign<&ast::ProcedureDef::returnType>(
                  ref<TypeSpecifier>) |
              kw<"void">) +
             assign<&ast::ProcedureDef::name>(ref<ID>) + lit<"("> +
             many(append<&ast::ProcedureDef::arguments>(ref<Argument>), lit<",">) +
             lit<")"> + lit<"="> +
             assign<&ast::ProcedureDef::number>(ValueExpr) + lit<";">> {};

  struct VersionDef
      : rule<kw<"version"> + assign<&ast::VersionDef::name>(ref<ID>) +
             lit<"{"> +
             some(append<&ast::VersionDef::procedures>(
                 ref<ProcedureDef>)) +
             lit<"}"> + lit<"="> +
             assign<&ast::VersionDef::number>(ValueExpr) + lit<";">> {};

  struct ProgramDef
      : rule<kw<"program"> + assign<&ast::ProgramDef::name>(ref<ID>) +
             lit<"{"> +
             some(append<&ast::ProgramDef::versions>(ref<VersionDef>)) +
             lit<"}"> + lit<"="> +
             assign<&ast::ProgramDef::number>(ValueExpr) + lit<";">> {};

  struct Specification
      : rule<many(append<&ast::Specification::items>(
            ref<Definition> | ref<ProgramDef>))> {};

  using entry = Specification;
  using ignored = list<WS, COMMENT>;
};

}  // namespace oncrpc
