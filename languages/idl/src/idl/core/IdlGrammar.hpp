// The idl language: its grammar. OMG CORBA IDL. All keywords are
// hard-reserved (no soft-keyword position), and
// scoped names are kept as text (see ast.hpp): the grammar below never
// resolves a cross reference, only matches the syntax.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "idl/core/ast.hpp"

namespace idl {

struct Idl : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$"> | uc::XID_Continue;

  // IDL restricts identifier letters/digits to specific old-Unicode ranges
  // narrower than `uc::L`/`uc::Nd`, which over-accept.
  struct ID : terminal<(uc::L | cr<"_$">)+many(uc::L | uc::Nd | cr<"_$">)> {};

  struct DEC_LIT
      : terminal<(lit<"0"> | (cr<"1-9"> + many(d))) + option(cr<"lL">)> {};
  struct OCT_LIT : terminal<lit<"0"> + some(cr<"0-7">) + option(cr<"lL">)> {};
  struct HEX_LIT : terminal<lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">) +
                            option(cr<"lL">)> {};

  static constexpr auto float_suffix = cr<"fFdD">;
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  // IDL's fixed-point literal syntax is exactly its floating-point syntax:
  // the floating reading always wins the tie, so no separate terminal is
  // ported for it.
  struct FLOAT_LIT : terminal<(some(d) + lit<"."> + many(d) + option(exponent) +
                               option(float_suffix)) |
                              (lit<"."> + some(d) + option(exponent) +
                               option(float_suffix)) |
                              (some(d) + exponent + option(float_suffix)) |
                              (some(d) + option(exponent) + float_suffix)> {};

  static constexpr auto simple_escape = lit<"\\"> + cr<R"(btnfr"'\\\\)">;
  static constexpr auto unicode_escape =
      lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto octal_escape =
      lit<"\\"> + cr<"0-7"> + option(cr<"0-7"> + option(cr<"0-7">));
  static constexpr auto escape_seq =
      simple_escape | unicode_escape | octal_escape;
  static constexpr auto char_body =
      lit<"'"> + (escape_seq | cr<R"(^'\\\\)">)+lit<"'">;
  static constexpr auto string_body =
      lit<"\""> + many(escape_seq | cr<R"(^"\\\\)">) + lit<"\"">;
  struct CHAR_LIT : terminal<char_body> {};
  struct WCHAR_LIT : terminal<lit<"L"> + char_body> {};
  struct STR_LIT : terminal<string_body> {};
  struct WSTR_LIT : terminal<lit<"L"> + string_body> {};

  // --- forward declarations ----------------------------------------------
  struct ConstExprRule;
  struct ScopedName;
  struct BaseTypeSpecRule;
  struct NamedTypeRefRule;
  struct SequenceTypeRule;
  struct SetTypeRule;
  struct MapTypeRule;
  struct StringTypeRule;
  struct WStringTypeRule;
  struct FixedPtTypeRule;
  struct StructDeclRule;
  struct UnionDeclRule;
  struct EnumDeclRule;
  struct BitsetDeclRule;
  struct BitmaskDeclRule;
  struct DeclaratorRule;
  struct ExportRule;
  struct ParamRule;

  // Reuse constant expressions for annotation arguments, preserving their AST.
  struct AnnotationArgument
      : rule<option(assign<&ast::AnnotationArgument::name>(ref<ID>) +
                    lit<"=">) +
             assign<&ast::AnnotationArgument::value>(ref<ConstExprRule>)> {};
  static constexpr auto annotation_appl = create<ast::AnnotationUse>(
      lit<"@"> + assign<&ast::AnnotationUse::name>(ref<ScopedName>) +
      option(
          flag<&ast::AnnotationUse::hasArguments>(lit<"(">) +
          many(append<&ast::AnnotationUse::arguments>(ref<AnnotationArgument>),
               lit<",">) +
          lit<")">));
  static constexpr auto annapps =
      many(append<&ast::Annotated::annotations>(annotation_appl));

  struct ScopedName
      : rule<option(lit<"::">) + ref<ID> + many(lit<"::"> + ref<ID>)> {};

  // --- constant expressions -----------------------------------------------
  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<ConstExprRule>) +
            lit<")">)> {};
  struct FloatLitRule : rule<create<ast::FloatLit>(
                            assign<&ast::FloatLit::text>(ref<FLOAT_LIT>))> {};
  struct IntLitRule : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(
                          ref<DEC_LIT> | ref<OCT_LIT> | ref<HEX_LIT>))> {};
  struct WCharLitRule : rule<create<ast::WCharLit>(
                            assign<&ast::WCharLit::text>(ref<WCHAR_LIT>))> {};
  struct CharLitRule
      : rule<create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR_LIT>))> {
  };
  struct WStringLitRule : rule<create<ast::WStringLit>(
                              assign<&ast::WStringLit::text>(ref<WSTR_LIT>))> {
  };
  struct StringLitRule : rule<create<ast::StringLit>(
                             assign<&ast::StringLit::text>(ref<STR_LIT>))> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"TRUE">) |
                                  kw<"FALSE">)> {};
  struct NameExprRule : rule<create<ast::NameExpr>(
                            assign<&ast::NameExpr::name>(ref<ScopedName>))> {};

  struct PrimaryExprRule : rule<ref<ParenExprRule>    //
                                | ref<FloatLitRule>   //
                                | ref<IntLitRule>     //
                                | ref<WCharLitRule>   //
                                | ref<CharLitRule>    //
                                | ref<WStringLitRule> //
                                | ref<StringLitRule>  //
                                | ref<BoolLitRule>    //
                                | ref<NameExprRule>> {};

  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+"> | lit<"~">) +
                 assign<&ast::UnaryExpr::operand>(ref<PrimaryExprRule>)) //
             | ref<PrimaryExprRule>> {};

  struct ConstExprRule : infix<UnaryExprRule, &ast::BinaryExpr::left,
                               &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                               (left(lit<"*"> | lit<"/"> | lit<"%">) >> //
                                left(lit<"+"> | lit<"-">) >>            //
                                left(lit<"<<"> | lit<">>">) >>          //
                                left(lit<"&">) >>                       //
                                left(lit<"^">) >>                       //
                                left(lit<"|">))> {};

  // --- base types -----------------------------------------------------------
  struct KLongDouble
      : rule<kw<"long"> + kw<"double">, value<ast::BaseType::long_double>> {};
  struct KFloat : rule<kw<"float">, value<ast::BaseType::float_>> {};
  struct KDouble : rule<kw<"double">, value<ast::BaseType::double_>> {};
  struct KLongLong
      : rule<kw<"long"> + kw<"long">, value<ast::BaseType::long_long>> {};
  struct KLong : rule<kw<"long">, value<ast::BaseType::long_>> {};
  struct KInt8 : rule<kw<"int8">, value<ast::BaseType::int8_>> {};
  struct KInt16 : rule<kw<"int16">, value<ast::BaseType::int16_>> {};
  struct KInt32 : rule<kw<"int32">, value<ast::BaseType::int32_>> {};
  struct KInt64 : rule<kw<"int64">, value<ast::BaseType::int64_>> {};
  struct KULongLong : rule<kw<"unsigned"> + kw<"long"> + kw<"long">,
                           value<ast::BaseType::ulong_long>> {};
  struct KULong
      : rule<kw<"unsigned"> + kw<"long">, value<ast::BaseType::ulong_>> {};
  struct KUShort
      : rule<kw<"unsigned"> + kw<"short">, value<ast::BaseType::ushort_>> {};
  struct KShort : rule<kw<"short">, value<ast::BaseType::short_>> {};
  struct KUInt8 : rule<kw<"uint8">, value<ast::BaseType::uint8_>> {};
  struct KUInt16 : rule<kw<"uint16">, value<ast::BaseType::uint16_>> {};
  struct KUInt32 : rule<kw<"uint32">, value<ast::BaseType::uint32_>> {};
  struct KUInt64 : rule<kw<"uint64">, value<ast::BaseType::uint64_>> {};
  struct KChar : rule<kw<"char">, value<ast::BaseType::char_>> {};
  struct KWChar : rule<kw<"wchar">, value<ast::BaseType::wchar_>> {};
  struct KBoolean : rule<kw<"boolean">, value<ast::BaseType::boolean_>> {};
  struct KOctet : rule<kw<"octet">, value<ast::BaseType::octet_>> {};
  struct KAny : rule<kw<"any">, value<ast::BaseType::any_>> {};
  struct KObject : rule<kw<"Object">, value<ast::BaseType::object_>> {};
  struct KValueBase : rule<kw<"ValueBase">, value<ast::BaseType::value_base>> {
  };
  struct KFixed : rule<kw<"fixed">, value<ast::BaseType::fixed_>> {};
  struct KVoid : rule<kw<"void">, value<ast::BaseType::void_>> {};

  // Every base-type keyword is accepted at every base-type position (a
  // const's own type excludes `any`/`Object`): a uniform position
  // over-accepts a handful of nonsensical types, never rejects a valid one.
  static constexpr auto base_type_kw =
      ref<KLongDouble> | ref<KFloat> | ref<KDouble> |          //
      ref<KLongLong> | ref<KLong> | ref<KInt8> | ref<KInt16> | //
      ref<KInt32> | ref<KInt64> |                              //
      ref<KULongLong> | ref<KULong> | ref<KUShort> | ref<KShort> | ref<KUInt8> |
      ref<KUInt16> | ref<KUInt32> | ref<KUInt64> |               //
      ref<KChar> | ref<KWChar> | ref<KBoolean> | ref<KOctet> |   //
      ref<KAny> | ref<KObject> | ref<KValueBase> | ref<KFixed> | //
      ref<KVoid>;

  struct BaseTypeSpecRule
      : rule<create<ast::BaseTypeRef>(
            assign<&ast::BaseTypeRef::kind>(base_type_kw))> {};

  struct NamedTypeRefRule
      : rule<create<ast::NamedTypeRef>(
            assign<&ast::NamedTypeRef::name>(ref<ScopedName>))> {};

  // --- template types: sequence/set/map/string/wstring/fixed --------------
  static constexpr auto element_type_spec =
      ref<BaseTypeSpecRule> | ref<SequenceTypeRule> | ref<SetTypeRule> |
      ref<MapTypeRule> | ref<StringTypeRule> | ref<WStringTypeRule> |
      ref<FixedPtTypeRule> | ref<NamedTypeRefRule>;

  struct SequenceTypeRule
      : rule<create<ast::SequenceTypeRef>(
            kw<"sequence"> + lit<"<"> + annapps +
            assign<&ast::SequenceTypeRef::element>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::SequenceTypeRef::bound>(ref<ConstExprRule>)) +
            lit<">">)> {};

  struct SetTypeRule
      : rule<create<ast::SetTypeRef>(
            kw<"set"> + lit<"<"> +
            assign<&ast::SetTypeRef::element>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::SetTypeRef::bound>(ref<ConstExprRule>)) +
            lit<">">)> {};

  struct MapTypeRule
      : rule<create<ast::MapTypeRef>(
            kw<"map"> + lit<"<"> +
            assign<&ast::MapTypeRef::keyType>(element_type_spec) + lit<","> +
            assign<&ast::MapTypeRef::valueType>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::MapTypeRef::bound>(ref<ConstExprRule>)) +
            lit<">">)> {};

  struct StringTypeRule
      : rule<create<ast::StringTypeRef>(
            kw<"string"> +
            option(lit<"<"> +
                   assign<&ast::StringTypeRef::bound>(ref<ConstExprRule>) +
                   lit<">">))> {};
  struct WStringTypeRule
      : rule<create<ast::WStringTypeRef>(
            kw<"wstring"> +
            option(lit<"<"> +
                   assign<&ast::WStringTypeRef::bound>(ref<ConstExprRule>) +
                   lit<">">))> {};
  struct FixedPtTypeRule
      : rule<create<ast::FixedPtTypeRef>(
            kw<"fixed"> + lit<"<"> +
            assign<&ast::FixedPtTypeRef::digits>(ref<ConstExprRule>) +
            lit<","> + assign<&ast::FixedPtTypeRef::scale>(ref<ConstExprRule>) +
            lit<">">)> {};

  // simple_type_spec: a base type, a template type, or a named reference.
  static constexpr auto simple_type_spec = element_type_spec;
  // param_type_spec: base type, string/wstring, or a named reference (no
  // sequence/set/map/fixed).
  static constexpr auto param_type_spec =
      ref<BaseTypeSpecRule> | ref<StringTypeRule> | ref<WStringTypeRule> |
      ref<NamedTypeRefRule>;
  // constr_type_spec: a fresh struct/union/enum/bitset/bitmask used inline
  // as a type. type_spec = simple_type_spec | constr_type_spec.
  static constexpr auto constr_type_spec =
      ref<StructDeclRule> | ref<UnionDeclRule> | ref<EnumDeclRule> |
      ref<BitsetDeclRule> | ref<BitmaskDeclRule>;
  static constexpr auto type_spec = simple_type_spec | constr_type_spec;

  // --- declarators and members ---------------------------------------------
  struct DeclaratorRule
      : rule<annapps +
             create<ast::Declarator>(
                 assign<&ast::Declarator::name>(ref<ID>) +
                 many(lit<"["> +
                      append<&ast::Declarator::arraySizes>(ref<ConstExprRule>) +
                      lit<"]">))> {};

  struct MemberRule
      : rule<annapps +
             create<ast::Member>(
                 assign<&ast::Member::type>(type_spec) +
                 some(append<&ast::Member::declarators>(ref<DeclaratorRule>),
                      lit<",">)) +
             lit<";">> {};

  // --- struct/union/enum/bitset/bitmask ------------------------------------
  static constexpr auto struct_decl = create<ast::StructDecl>(
      kw<"struct"> + annapps + assign<&ast::StructDecl::name>(ref<ID>) +
      option(lit<":"> + assign<&ast::StructDecl::base>(ref<ScopedName>)) +
      lit<"{"> + many(append<&ast::StructDecl::members>(ref<MemberRule>)) +
      lit<"}">);
  struct StructDeclRule : rule<struct_decl> {};

  struct CaseLabelRule
      : rule<annapps + create<ast::CaseLabel>(
                           (kw<"case"> +
                            assign<&ast::CaseLabel::value>(ref<ConstExprRule>) +
                            lit<":">) |
                           flag<&ast::CaseLabel::default_>(kw<"default"> +
                                                                lit<":">))> {};
  struct ElementSpecRule
      : rule<annapps +
             create<ast::ElementSpec>(
                 assign<&ast::ElementSpec::type>(type_spec) +
                 assign<&ast::ElementSpec::declarator>(ref<DeclaratorRule>))> {
  };
  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            some(append<&ast::CaseStmt::labels>(ref<CaseLabelRule>)) +
            assign<&ast::CaseStmt::element>(ref<ElementSpecRule>) + lit<";">)> {
  };
  static constexpr auto switch_type_spec =
      ref<BaseTypeSpecRule> | ref<EnumDeclRule> | ref<NamedTypeRefRule>;
  static constexpr auto union_decl = create<ast::UnionDecl>(
      kw<"union"> + annapps + assign<&ast::UnionDecl::name>(ref<ID>) +
      kw<"switch"> + lit<"("> + annapps +
      assign<&ast::UnionDecl::switchType>(switch_type_spec) + lit<")"> +
      lit<"{"> + some(append<&ast::UnionDecl::cases>(ref<CaseStmtRule>)) +
      lit<"}">);
  struct UnionDeclRule : rule<union_decl> {};

  struct EnumeratorRule
      : rule<annapps + create<ast::EnumeratorDecl>(
                           assign<&ast::EnumeratorDecl::name>(ref<ID>))> {};
  static constexpr auto enum_decl = create<ast::EnumDecl>(
      kw<"enum"> + annapps + assign<&ast::EnumDecl::name>(ref<ID>) + lit<"{"> +
      some(append<&ast::EnumDecl::enumerators>(ref<EnumeratorRule>), lit<",">) +
      lit<"}">);
  struct EnumDeclRule : rule<enum_decl> {};

  struct BitfieldSpecRule
      : rule<create<ast::BitfieldSpec>(
            annapps + kw<"bitfield"> + lit<"<"> +
            assign<&ast::BitfieldSpec::width>(ref<ConstExprRule>) +
            option(lit<","> + assign<&ast::BitfieldSpec::type>(base_type_kw)) +
            lit<">"> +
            option(annapps + append<&ast::BitfieldSpec::names>(ref<ID>) +
                   many(lit<","> + annapps +
                        append<&ast::BitfieldSpec::names>(ref<ID>))) +
            lit<";">)> {};
  static constexpr auto bitset_decl = create<ast::BitsetDecl>(
      kw<"bitset"> + annapps + assign<&ast::BitsetDecl::name>(ref<ID>) +
      option(lit<":"> + assign<&ast::BitsetDecl::base>(ref<ScopedName>)) +
      lit<"{"> + some(append<&ast::BitsetDecl::fields>(ref<BitfieldSpecRule>)) +
      lit<"}">);
  struct BitsetDeclRule : rule<bitset_decl> {};

  static constexpr auto bitmask_decl = create<ast::BitmaskDecl>(
      kw<"bitmask"> + annapps + assign<&ast::BitmaskDecl::name>(ref<ID>) +
      lit<"{"> + annapps + append<&ast::BitmaskDecl::values>(ref<ID>) +
      many(lit<","> + annapps + append<&ast::BitmaskDecl::values>(ref<ID>)) +
      lit<"}">);
  struct BitmaskDeclRule : rule<bitmask_decl> {};

  // --- typedef / native / forward struct-union -----------------------------
  static constexpr auto typedef_decl = create<ast::TypedefDecl>(
      kw<"typedef"> + annapps + assign<&ast::TypedefDecl::type>(type_spec) +
      some(append<&ast::TypedefDecl::declarators>(ref<DeclaratorRule>),
           lit<",">));
  struct TypedefDeclRule : rule<typedef_decl> {};
  static constexpr auto native_decl = create<ast::NativeDecl>(
      kw<"native"> + annapps + assign<&ast::NativeDecl::name>(ref<ID>));
  static constexpr auto struct_forward_decl = create<ast::StructForwardDecl>(
      kw<"struct"> + assign<&ast::StructForwardDecl::name>(ref<ID>));
  static constexpr auto union_forward_decl = create<ast::UnionForwardDecl>(
      kw<"union"> + assign<&ast::UnionForwardDecl::name>(ref<ID>));

  static constexpr auto type_decl =
      typedef_decl | struct_decl | union_decl | enum_decl | bitset_decl |
      bitmask_decl | native_decl | struct_forward_decl | union_forward_decl;

  // --- const / exception ----------------------------------------------------
  static constexpr auto const_decl = create<ast::ConstDecl>(
      kw<"const"> + annapps + assign<&ast::ConstDecl::type>(param_type_spec) +
      annapps + assign<&ast::ConstDecl::name>(ref<ID>) + lit<"="> +
      assign<&ast::ConstDecl::value>(ref<ConstExprRule>));
  struct ConstDeclRule : rule<const_decl> {};

  static constexpr auto exception_decl = create<ast::ExceptionDecl>(
      kw<"exception"> + annapps + assign<&ast::ExceptionDecl::name>(ref<ID>) +
      lit<"{"> + many(append<&ast::ExceptionDecl::members>(ref<MemberRule>)) +
      lit<"}">);

  // --- attributes and operations --------------------------------------------
  static constexpr auto attr_decl = create<ast::AttrDecl>(
      option(flag<&ast::AttrDecl::readonly_>(kw<"readonly">)) +
      kw<"attribute"> + annapps +
      assign<&ast::AttrDecl::type>(param_type_spec) + annapps +
      append<&ast::AttrDecl::names>(ref<ID>) +
      option(
          (option(kw<"raises"> + lit<"("> + annapps +
                  append<&ast::AttrDecl::getRaises>(ref<ScopedName>) +
                  many(lit<","> + annapps +
                       append<&ast::AttrDecl::getRaises>(ref<ScopedName>)) +
                  lit<")">) +
           option(kw<"getraises"> + lit<"("> + annapps +
                  append<&ast::AttrDecl::getRaises>(ref<ScopedName>) +
                  many(lit<","> + annapps +
                       append<&ast::AttrDecl::getRaises>(ref<ScopedName>)) +
                  lit<")">) +
           option(kw<"setraises"> + lit<"("> + annapps +
                  append<&ast::AttrDecl::setRaises>(ref<ScopedName>) +
                  many(lit<","> + annapps +
                       append<&ast::AttrDecl::setRaises>(ref<ScopedName>)) +
                  lit<")">)) |
          many(lit<","> + annapps + append<&ast::AttrDecl::names>(ref<ID>))));

  struct KParamIn : rule<kw<"in">, value<ast::ParamDir::in_>> {};
  struct KParamOut : rule<kw<"out">, value<ast::ParamDir::out_>> {};
  struct KParamInOut : rule<kw<"inout">, value<ast::ParamDir::inout_>> {};

  struct ParamRule
      : rule<annapps +
             create<ast::Param>(
                 assign<&ast::Param::dir>(ref<KParamIn> | ref<KParamOut> |
                                          ref<KParamInOut>) +
                 annapps + assign<&ast::Param::type>(param_type_spec) +
                 annapps + assign<&ast::Param::name>(ref<ID>))> {};

  static constexpr auto op_decl = create<ast::OpDecl>(
      option(flag<&ast::OpDecl::oneway_>(kw<"oneway">)) + annapps +
      assign<&ast::OpDecl::returnType>(param_type_spec) + annapps +
      assign<&ast::OpDecl::name>(ref<ID>) + lit<"("> +
      option(some(append<&ast::OpDecl::params>(ref<ParamRule>), lit<",">)) +
      lit<")"> +
      option(kw<"raises"> + lit<"("> +
             append<&ast::OpDecl::raises>(ref<ScopedName>) +
             many(lit<","> + append<&ast::OpDecl::raises>(ref<ScopedName>)) +
             lit<")">) +
      option(kw<"context"> + lit<"("> +
             append<&ast::OpDecl::context>(ref<STR_LIT>) +
             many(lit<","> + append<&ast::OpDecl::context>(ref<STR_LIT>)) +
             lit<")">));

  // --- import / type_id / type_prefix ---------------------------------------
  struct ImportDeclRule
      : rule<annapps + kw<"import"> + annapps +
             create<ast::ImportDecl>(assign<&ast::ImportDecl::imported>(
                 ref<ScopedName> | ref<STR_LIT>))> {};
  static constexpr auto typeid_decl = create<ast::TypeIdDecl>(
      kw<"typeid"> + annapps +
      assign<&ast::TypeIdDecl::target>(ref<ScopedName>) +
      assign<&ast::TypeIdDecl::repositoryId>(ref<STR_LIT>));
  static constexpr auto typeprefix_decl = create<ast::TypePrefixDecl>(
      kw<"typeprefix"> + annapps +
      assign<&ast::TypePrefixDecl::target>(ref<ScopedName>) +
      assign<&ast::TypePrefixDecl::prefix>(ref<STR_LIT>));

  // --- annotation definitions
  // -------------------------------------------------
  struct AnnotationMemberRule
      : rule<create<ast::AnnotationMember>(
            assign<&ast::AnnotationMember::type>(param_type_spec) +
            assign<&ast::AnnotationMember::name>(ref<ID>) +
            option(kw<"default"> + assign<&ast::AnnotationMember::defaultValue>(
                                       ref<ConstExprRule>)) +
            lit<";">)> {};
  static constexpr auto annotation_body_item =
      ref<AnnotationMemberRule>         //
      | (ref<EnumDeclRule> + lit<";">)  //
      | (ref<ConstDeclRule> + lit<";">) //
      | (ref<TypedefDeclRule> + lit<";">);

  static constexpr auto annotation_decl =
      kw<"@annotation"> +
      create<ast::AnnotationDecl>(
          annapps + assign<&ast::AnnotationDecl::name>(ref<ID>) +
          option(lit<":"> +
                 assign<&ast::AnnotationDecl::base>(ref<ScopedName>)) +
          lit<"{"> +
          many(append<&ast::AnnotationDecl::members>(annotation_body_item)) +
          lit<"}">);
  static constexpr auto annotation_forward_decl =
      kw<"@annotation"> +
      create<ast::AnnotationForwardDecl>(
          assign<&ast::AnnotationForwardDecl::name>(ref<ScopedName>));

  // --- interface
  // --------------------------------------------------------------
  static constexpr auto interface_forward_decl =
      create<ast::InterfaceForwardDecl>(
          option(flag<&ast::InterfaceForwardDecl::abstract_>(kw<"abstract">) |
                 flag<&ast::InterfaceForwardDecl::local_>(kw<"local">)) +
          kw<"interface"> + annapps +
          assign<&ast::InterfaceForwardDecl::name>(ref<ID>));

  static constexpr auto interface_decl = create<ast::InterfaceDecl>(
      option(flag<&ast::InterfaceDecl::abstract_>(kw<"abstract">) |
             flag<&ast::InterfaceDecl::local_>(kw<"local">)) +
      kw<"interface"> + annapps + assign<&ast::InterfaceDecl::name>(ref<ID>) +
      option(lit<":"> + annapps +
             append<&ast::InterfaceDecl::bases>(ref<ScopedName>) +
             many(lit<","> + annapps +
                  append<&ast::InterfaceDecl::bases>(ref<ScopedName>))) +
      lit<"{"> + many(append<&ast::InterfaceDecl::exports>(ref<ExportRule>)) +
      lit<"}">);

  struct ExportRule
      : rule<annapps +
             (attr_decl | op_decl | typeid_decl | typeprefix_decl |
              exception_decl | type_decl | const_decl) +
             lit<";">> {};

  // --- module
  // ------------------------------------------------------------------
  struct DefinitionRule;
  static constexpr auto module_decl = create<ast::ModuleDecl>(
      kw<"module"> + annapps + assign<&ast::ModuleDecl::name>(ref<ID>) +
      lit<"{"> +
      some(append<&ast::ModuleDecl::definitions>(ref<DefinitionRule>)) +
      lit<"}">);

  // --- valuetype
  // -----------------------------------------------------------------
  static constexpr auto value_forward_decl = create<ast::ValueForwardDecl>(
      option(flag<&ast::ValueForwardDecl::abstract_>(kw<"abstract">)) +
      kw<"valuetype"> + annapps +
      assign<&ast::ValueForwardDecl::name>(ref<ID>));

  static constexpr auto value_box_decl = create<ast::ValueBoxDecl>(
      kw<"valuetype"> + annapps + assign<&ast::ValueBoxDecl::name>(ref<ID>) +
      assign<&ast::ValueBoxDecl::type>(type_spec));

  static constexpr auto value_abs_decl =
      kw<"abstract"> + kw<"valuetype"> +
      create<ast::ValueAbsDecl>(
          annapps + assign<&ast::ValueAbsDecl::name>(ref<ID>) +
          option(lit<":"> +
                 option(flag<&ast::ValueAbsDecl::truncatable_>(
                     kw<"truncatable">)) +
                 append<&ast::ValueAbsDecl::valueBases>(ref<ScopedName>) +
                 many(lit<","> + append<&ast::ValueAbsDecl::valueBases>(
                                     ref<ScopedName>))) +
          option(kw<"supports"> +
                 append<&ast::ValueAbsDecl::supports_>(ref<ScopedName>) +
                 many(lit<","> +
                      append<&ast::ValueAbsDecl::supports_>(ref<ScopedName>))) +
          lit<"{"> +
          many(append<&ast::ValueAbsDecl::exports>(ref<ExportRule>)) +
          lit<"}">);

  struct StateMemberRule
      : rule<annapps +
             create<ast::StateMember>(
                 (flag<&ast::StateMember::public_>(kw<"public">) |
                  kw<"private">)+annapps +
                 assign<&ast::StateMember::type>(type_spec) +
                 some(append<&ast::StateMember::declarators>(
                          ref<DeclaratorRule>),
                      lit<",">)) +
             lit<";">> {};

  struct InitDeclRule
      : rule<annapps + kw<"factory"> +
             create<ast::InitDecl>(
                 annapps + assign<&ast::InitDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::InitDecl::params>(ref<ParamRule>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        append<&ast::InitDecl::raises>(ref<ScopedName>) +
                        many(lit<","> +
                             append<&ast::InitDecl::raises>(ref<ScopedName>)) +
                        lit<")">)) +
             lit<";">> {};

  static constexpr auto value_element =
      ref<ExportRule> | ref<StateMemberRule> | ref<InitDeclRule>;

  static constexpr auto value_decl = create<ast::ValueDecl>(
      option(flag<&ast::ValueDecl::custom_>(kw<"custom">)) + kw<"valuetype"> +
      annapps + assign<&ast::ValueDecl::name>(ref<ID>) +
      option(lit<":"> +
             option(flag<&ast::ValueDecl::truncatable_>(kw<"truncatable">)) +
             append<&ast::ValueDecl::valueBases>(ref<ScopedName>) +
             many(lit<","> +
                  append<&ast::ValueDecl::valueBases>(ref<ScopedName>))) +
      option(kw<"supports"> +
             append<&ast::ValueDecl::supports_>(ref<ScopedName>) +
             many(lit<","> +
                  append<&ast::ValueDecl::supports_>(ref<ScopedName>))) +
      lit<"{"> + many(append<&ast::ValueDecl::elements>(value_element)) +
      lit<"}">);

  // --- eventtype
  // -----------------------------------------------------------------
  static constexpr auto event_forward_decl = create<ast::EventForwardDecl>(
      option(flag<&ast::EventForwardDecl::abstract_>(kw<"abstract">)) +
      kw<"eventtype"> + assign<&ast::EventForwardDecl::name>(ref<ID>));

  static constexpr auto event_abs_decl =
      kw<"abstract"> + kw<"eventtype"> +
      create<ast::EventAbsDecl>(
          annapps + assign<&ast::EventAbsDecl::name>(ref<ID>) +
          option(lit<":"> +
                 option(flag<&ast::EventAbsDecl::truncatable_>(
                     kw<"truncatable">)) +
                 append<&ast::EventAbsDecl::valueBases>(ref<ScopedName>) +
                 many(lit<","> + append<&ast::EventAbsDecl::valueBases>(
                                     ref<ScopedName>))) +
          option(kw<"supports"> +
                 append<&ast::EventAbsDecl::supports_>(ref<ScopedName>) +
                 many(lit<","> +
                      append<&ast::EventAbsDecl::supports_>(ref<ScopedName>))) +
          lit<"{"> +
          many(append<&ast::EventAbsDecl::exports>(ref<ExportRule>)) +
          lit<"}">);

  static constexpr auto event_decl = create<ast::EventDecl>(
      option(flag<&ast::EventDecl::custom_>(kw<"custom">)) + kw<"eventtype"> +
      annapps + assign<&ast::EventDecl::name>(ref<ID>) +
      option(lit<":"> +
             option(flag<&ast::EventDecl::truncatable_>(kw<"truncatable">)) +
             append<&ast::EventDecl::valueBases>(ref<ScopedName>) +
             many(lit<","> +
                  append<&ast::EventDecl::valueBases>(ref<ScopedName>))) +
      option(kw<"supports"> +
             append<&ast::EventDecl::supports_>(ref<ScopedName>) +
             many(lit<","> +
                  append<&ast::EventDecl::supports_>(ref<ScopedName>))) +
      lit<"{"> + many(append<&ast::EventDecl::elements>(value_element)) +
      lit<"}">);

  // --- component
  // ---------------------------------------------------------------
  static constexpr auto interface_type_expr = ref<ScopedName> | kw<"Object">;

  static constexpr auto provides_decl = create<ast::ProvidesDecl>(
      kw<"provides"> + annapps +
      assign<&ast::ProvidesDecl::interfaceType>(interface_type_expr) +
      assign<&ast::ProvidesDecl::name>(ref<ID>));
  static constexpr auto uses_decl = create<ast::UsesDecl>(
      kw<"uses"> + option(flag<&ast::UsesDecl::multiple_>(kw<"multiple">)) +
      annapps + assign<&ast::UsesDecl::interfaceType>(interface_type_expr) +
      assign<&ast::UsesDecl::name>(ref<ID>));
  static constexpr auto emits_decl = create<ast::EmitsDecl>(
      kw<"emits"> + annapps +
      assign<&ast::EmitsDecl::eventType>(ref<ScopedName>) +
      assign<&ast::EmitsDecl::name>(ref<ID>));
  static constexpr auto publishes_decl = create<ast::PublishesDecl>(
      kw<"publishes"> + annapps +
      assign<&ast::PublishesDecl::eventType>(ref<ScopedName>) +
      assign<&ast::PublishesDecl::name>(ref<ID>));
  static constexpr auto consumes_decl = create<ast::ConsumesDecl>(
      kw<"consumes"> + annapps +
      assign<&ast::ConsumesDecl::eventType>(ref<ScopedName>) +
      assign<&ast::ConsumesDecl::name>(ref<ID>));

  struct ComponentExportRule
      : rule<annapps +
             (provides_decl | uses_decl | emits_decl | publishes_decl |
              consumes_decl | attr_decl) +
             lit<";">> {};

  static constexpr auto component_forward_decl =
      create<ast::ComponentForwardDecl>(
          kw<"component"> + assign<&ast::ComponentForwardDecl::name>(ref<ID>));

  static constexpr auto component_decl = create<ast::ComponentDecl>(
      kw<"component"> + annapps + assign<&ast::ComponentDecl::name>(ref<ID>) +
      option(lit<":"> + annapps +
             assign<&ast::ComponentDecl::base>(ref<ScopedName>)) +
      option(kw<"supports"> +
             append<&ast::ComponentDecl::supports_>(ref<ScopedName>) +
             many(lit<","> +
                  append<&ast::ComponentDecl::supports_>(ref<ScopedName>))) +
      lit<"{"> +
      many(append<&ast::ComponentDecl::exports>(ref<ComponentExportRule>)) +
      lit<"}">);

  // --- home
  // ----------------------------------------------------------------------
  struct FactoryDeclRule
      : rule<annapps + kw<"factory"> +
             create<ast::FactoryDecl>(
                 annapps + assign<&ast::FactoryDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::FactoryDecl::params>(ref<ParamRule>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        append<&ast::FactoryDecl::raises>(ref<ScopedName>) +
                        many(lit<","> + append<&ast::FactoryDecl::raises>(
                                            ref<ScopedName>)) +
                        lit<")">)) +
             lit<";">> {};
  struct FinderDeclRule
      : rule<annapps + kw<"finder"> +
             create<ast::FinderDecl>(
                 annapps + assign<&ast::FinderDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::FinderDecl::params>(ref<ParamRule>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        append<&ast::FinderDecl::raises>(ref<ScopedName>) +
                        many(lit<","> + append<&ast::FinderDecl::raises>(
                                            ref<ScopedName>)) +
                        lit<")">)) +
             lit<";">> {};

  static constexpr auto home_export =
      ref<ExportRule> | ref<FactoryDeclRule> | ref<FinderDeclRule>;

  static constexpr auto home_decl = create<ast::HomeDecl>(
      kw<"home"> + annapps + assign<&ast::HomeDecl::name>(ref<ID>) +
      option(lit<":"> + annapps +
             assign<&ast::HomeDecl::base>(ref<ScopedName>)) +
      option(
          kw<"supports"> + append<&ast::HomeDecl::supports_>(ref<ScopedName>) +
          many(lit<","> + append<&ast::HomeDecl::supports_>(ref<ScopedName>))) +
      kw<"manages"> + assign<&ast::HomeDecl::manages_>(ref<ScopedName>) +
      option(kw<"primarykey"> +
             assign<&ast::HomeDecl::primaryKey>(ref<ScopedName>)) +
      lit<"{"> + many(append<&ast::HomeDecl::exports>(home_export)) + lit<"}">);

  // --- top-level definition and specification
  // -------------------------------------
  struct DefinitionRule
      : rule<annapps +
             (type_decl | const_decl | exception_decl | interface_decl |
              interface_forward_decl | module_decl | value_decl |
              value_abs_decl | value_box_decl | value_forward_decl |
              typeid_decl | typeprefix_decl | event_decl | event_abs_decl |
              event_forward_decl | component_decl | component_forward_decl |
              home_decl | annotation_decl | annotation_forward_decl) +
             lit<";">> {};

  struct SpecificationRule
      : rule<create<ast::Specification>(
            many(append<&ast::Specification::imports>(ref<ImportDeclRule>) +
                 lit<";">) +
            some(append<&ast::Specification::definitions>(
                ref<DefinitionRule>)))> {};

  using entry = SpecificationRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace idl
