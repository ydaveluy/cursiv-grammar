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

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  struct NUMBER : terminal<option(cr<"+-">) +
      ((lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) | some(d))> {};
  static constexpr auto escape = lit<"\\"> + any;
  struct STRING : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">> {};
  struct CHARACTER : terminal<lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">> {};
  static constexpr auto ValueExpr = ref<NUMBER> | ref<ID>;

  struct UnsignedIntSpec
      : rule<kw<"unsigned"> + option(kw<"int">), value<ast::Primitive::unsigned_int>> {
  };
  struct IntSpec : rule<kw<"int">, value<ast::Primitive::int_>> {};
  struct UnsignedHyperSpec
      : rule<kw<"unsigned"> + kw<"hyper"> + option(kw<"int">),
             value<ast::Primitive::unsigned_hyper>> {};
  struct HyperSpec : rule<kw<"hyper"> + option(kw<"int">), value<ast::Primitive::hyper>> {};
  struct FloatSpec : rule<kw<"float">, value<ast::Primitive::float_>> {};
  struct DoubleSpec : rule<kw<"double">, value<ast::Primitive::double_>> {};
  struct QuadrupleSpec
      : rule<kw<"quadruple">, value<ast::Primitive::quadruple>> {};
  struct BoolSpec : rule<kw<"bool">, value<ast::Primitive::bool_>> {};

  struct CharSpec : rule<kw<"char">, value<ast::Primitive::char_>> {};
  struct ShortSpec : rule<kw<"short"> + option(kw<"int">), value<ast::Primitive::short_>> {};
  struct LongSpec : rule<kw<"long"> + option(kw<"int">), value<ast::Primitive::long_>> {};
  struct UnsignedCharSpec : rule<kw<"unsigned"> + kw<"char">, value<ast::Primitive::unsigned_char>> {};
  struct UnsignedShortSpec : rule<kw<"unsigned"> + kw<"short"> + option(kw<"int">), value<ast::Primitive::unsigned_short>> {};
  struct UnsignedLongSpec : rule<kw<"unsigned"> + kw<"long"> + option(kw<"int">), value<ast::Primitive::unsigned_long>> {};

  struct PrimitiveTypeRule
      : rule<assign<&ast::PrimitiveType::kind>(
            ref<UnsignedCharSpec> | ref<UnsignedShortSpec> | ref<UnsignedLongSpec> |
            ref<UnsignedHyperSpec> | ref<UnsignedIntSpec> | ref<IntSpec> |
            ref<HyperSpec> | ref<CharSpec> | ref<ShortSpec> | ref<LongSpec> | ref<FloatSpec> | ref<DoubleSpec> |
            ref<QuadrupleSpec> | ref<BoolSpec>)> {};

  struct NamedTypeRule : rule<
      option(assign<&ast::NamedType::prefix>(kw<"struct"> | kw<"enum"> | kw<"union">)) +
      assign<&ast::NamedType::name>(ref<ID>)> {};

  struct OpaqueTypeRule : rule<create<ast::OpaqueType> + kw<"opaque">> {};
  struct StringTypeRule : rule<create<ast::StringType> + kw<"string">> {};

  struct DeclarationRule;

  struct EnumConstRule
      : rule<assign<&ast::EnumConst::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::EnumConst::value>(ValueExpr))> {};

  struct EnumBodyRule
      : rule<lit<"{"> +
             some(append<&ast::EnumType::consts>(ref<EnumConstRule>),
                  lit<",">) +
             lit<"}">> {};

  struct EnumTypeSpecRule : rule<kw<"enum"> + ref<EnumBodyRule>> {};

  struct StructBodyRule
      : rule<lit<"{"> +
             some(append<&ast::StructType::members>(ref<DeclarationRule>) +
                  lit<";">) +
             lit<"}">> {};

  struct StructTypeSpecRule : rule<kw<"struct"> + ref<StructBodyRule>> {};

  struct CaseLabelRule
      : rule<kw<"case"> + assign<&ast::CaseLabel::value>(ValueExpr | ref<CHARACTER>) +
             lit<":">> {};

  struct CaseSpecRule
      : rule<some(append<&ast::CaseSpec::labels>(ref<CaseLabelRule>)) +
             assign<&ast::CaseSpec::body>(ref<DeclarationRule>) + lit<";">> {
  };

  struct UnionBodyRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::UnionType::discriminant>(ref<DeclarationRule>) +
             lit<")"> + lit<"{"> +
             some(append<&ast::UnionType::cases>(ref<CaseSpecRule>)) +
             option(kw<"default"> + lit<":"> +
                    assign<&ast::UnionType::defaultCase>(
                        ref<DeclarationRule>) +
                    lit<";">) +
             lit<"}">> {};

  struct UnionTypeSpecRule : rule<kw<"union"> + ref<UnionBodyRule>> {};

  struct TypeSpecifierRule
      : rule<ref<PrimitiveTypeRule> | ref<EnumTypeSpecRule> |
             ref<StructTypeSpecRule> | ref<UnionTypeSpecRule> |
             ref<OpaqueTypeRule> | ref<StringTypeRule> | ref<NamedTypeRule>> {};

  static constexpr auto ArrayOpen =
      ref<rule<lit<"[">, value<ast::Bound::fixed>>>;
  static constexpr auto VarOpen =
      ref<rule<lit<"<">, value<ast::Bound::variable>>>;

  static constexpr auto ArraySuffix =
      (assign<&ast::ValueDecl::bound>(ArrayOpen) +
       assign<&ast::ValueDecl::size>(ValueExpr) + lit<"]">) |
      (assign<&ast::ValueDecl::bound>(VarOpen) +
       option(assign<&ast::ValueDecl::size>(ValueExpr)) + lit<">">);

  struct VoidDeclRule : rule<create<ast::VoidDecl> + kw<"void">> {};

  struct ValueDeclRule
      : rule<assign<&ast::ValueDecl::type>(ref<TypeSpecifierRule>) +
             option(flag<&ast::ValueDecl::pointer>(lit<"*">)) +
             assign<&ast::ValueDecl::name>(ref<ID>) + option(ArraySuffix)> {};

  struct ArgumentRule : rule<
      create<ast::VoidDecl>(kw<"void"> + option(assign<&ast::VoidDecl::name>(ref<ID>))) |
      create<ast::ValueDecl>(assign<&ast::ValueDecl::type>(ref<TypeSpecifierRule>) +
          option((flag<&ast::ValueDecl::pointer>(lit<"*">) +
                  option(assign<&ast::ValueDecl::name>(ref<ID>))) |
                 (assign<&ast::ValueDecl::name>(ref<ID>) +
                  option(flag<&ast::ValueDecl::pointer>(lit<"*">) +
                         option(append<&ast::ValueDecl::extraNames>(ref<ID>))))) +
          option(ArraySuffix))> {};

  struct DeclarationRule
      : rule<ref<VoidDeclRule> | ref<ValueDeclRule>> {};

  struct TypedefDefRule
      : rule<kw<"typedef"> + assign<&ast::TypedefDef::decl>(
                                  ref<DeclarationRule>) +
             lit<";">> {};

  struct EnumDefRule
      : rule<kw<"enum"> + assign<&ast::EnumDef::name>(ref<ID>) +
             assign<&ast::EnumDef::body>(ref<EnumBodyRule>) + lit<";">> {};

  struct StructDefRule
      : rule<kw<"struct"> + assign<&ast::StructDef::name>(ref<ID>) +
             assign<&ast::StructDef::body>(ref<StructBodyRule>) + lit<";">> {
  };

  struct UnionDefRule
      : rule<kw<"union"> + assign<&ast::UnionDef::name>(ref<ID>) +
             assign<&ast::UnionDef::body>(ref<UnionBodyRule>) + lit<";">> {};

  struct ConstantDefRule
      : rule<kw<"const"> + assign<&ast::ConstantDef::name>(ref<ID>) +
             lit<"="> + assign<&ast::ConstantDef::value>(ValueExpr | ref<STRING>) +
             lit<";">> {};

  struct DefinitionRule
      : rule<ref<TypedefDefRule> | ref<EnumDefRule> | ref<StructDefRule> |
             ref<UnionDefRule> | ref<ConstantDefRule>> {};

  struct ProcedureDefRule
      : rule<(assign<&ast::ProcedureDef::returnType>(
                  ref<TypeSpecifierRule>) |
              kw<"void">) +
             assign<&ast::ProcedureDef::name>(ref<ID>) + lit<"("> +
             many(append<&ast::ProcedureDef::arguments>(ref<ArgumentRule>), lit<",">) +
             lit<")"> + lit<"="> +
             assign<&ast::ProcedureDef::number>(ValueExpr) + lit<";">> {};

  struct VersionDefRule
      : rule<kw<"version"> + assign<&ast::VersionDef::name>(ref<ID>) +
             lit<"{"> +
             some(append<&ast::VersionDef::procedures>(
                 ref<ProcedureDefRule>)) +
             lit<"}"> + lit<"="> +
             assign<&ast::VersionDef::number>(ValueExpr) + lit<";">> {};

  struct ProgramDefRule
      : rule<kw<"program"> + assign<&ast::ProgramDef::name>(ref<ID>) +
             lit<"{"> +
             some(append<&ast::ProgramDef::versions>(ref<VersionDefRule>)) +
             lit<"}"> + lit<"="> +
             assign<&ast::ProgramDef::number>(ValueExpr) + lit<";">> {};

  struct SpecificationRule
      : rule<many(append<&ast::Specification::items>(
            ref<DefinitionRule> | ref<ProgramDefRule>))> {};

  using entry = SpecificationRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace oncrpc
