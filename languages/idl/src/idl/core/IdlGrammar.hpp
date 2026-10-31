// The idl language: CORBA-family and selected OMG IDL4.2 building blocks.
// Full keyword/case collision checks are not implemented;
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

  // The inherited lexer keeps Unicode/$ extensions. IDL4.2 specifies ASCII
  // identifiers; this broader syntax is documented rather than called normative.
  struct ID : terminal<(uc::L | cr<"_$">)+many(uc::L | uc::Nd | cr<"_$">)> {};

  struct DEC_LIT
      : terminal<(lit<"0"> | (cr<"1-9"> + many(d))) + option(cr<"lL">)> {};
  struct OCT_LIT : terminal<lit<"0"> + some(cr<"0-7">) + option(cr<"lL">)> {};
  struct HEX_LIT : terminal<lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">) +
                            option(cr<"lL">)> {};

  static constexpr auto float_suffix = cr<"fF">; // legacy float suffix extension
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FIXED_LIT
      : terminal<((some(d) + option(lit<"."> + many(d))) |
                  (lit<"."> + some(d))) + cr<"dD">> {};
  struct FLOAT_LIT : terminal<(some(d) + lit<"."> + many(d) + option(exponent) +
                               option(float_suffix)) |
                              (lit<"."> + some(d) + option(exponent) +
                               option(float_suffix)) |
                              (some(d) + exponent + option(float_suffix)) |
                              (some(d) + option(exponent) + float_suffix)> {};

  static constexpr auto simple_escape = lit<"\\"> + cr<R"(abtnvfr?"'\\\\)">;
  static constexpr auto unicode_escape =
      lit<"\\u"> + repeat<1, 4>(cr<"0-9a-fA-F">);
  static constexpr auto hex_escape =
      lit<"\\x"> + repeat<1, 2>(cr<"0-9a-fA-F">);
  static constexpr auto octal_escape =
      lit<"\\"> + cr<"0-7"> + option(cr<"0-7"> + option(cr<"0-7">));
  static constexpr auto escape_seq =
      simple_escape | unicode_escape | hex_escape | octal_escape;
  static constexpr auto char_body =
      lit<"'"> + (escape_seq | cr<R"(^'\\\\\x00-\x1f\x7f-\x9f)">)+lit<"'">;
  static constexpr auto string_body =
      lit<"\""> + many(escape_seq | cr<R"(^"\\\\\x00-\x1f\x7f-\x9f)">) + lit<"\"">;
  struct CHAR_LIT : terminal<char_body> {};
  struct WCHAR_LIT : terminal<lit<"L"> + char_body> {};
  struct STR_LIT : terminal<string_body> {};
  struct WSTR_LIT : terminal<lit<"L"> + string_body> {};
  struct StringSpelling : rule<some(ref<STR_LIT>)> {};

  // --- forward declarations ----------------------------------------------
  struct ConstExpr;
  struct ScopedName;
  struct BaseTypeSpec;
  struct NamedTypeRef;
  struct SequenceType;
  struct SetType;
  struct MapType;
  struct StringType;
  struct WStringType;
  struct FixedPtType;
  struct StructDecl;
  struct UnionDecl;
  struct EnumDecl;
  struct BitsetDecl;
  struct BitmaskDecl;
  struct Declarator;
  struct Export;
  struct Param;

  // Reuse constant expressions for annotation arguments, preserving their AST.
  struct AnnotationArgument
      : rule<assign<&ast::AnnotationArgument::value>(ref<ConstExpr>)> {};
  struct NamedAnnotationArgument
      : rule<assign<&ast::AnnotationArgument::name>(ref<ID>) + lit<"="> +
             assign<&ast::AnnotationArgument::value>(ref<ConstExpr>)> {};
  static constexpr auto annotation_appl = create<ast::AnnotationUse>(
      lit<"@"> + assign<&ast::AnnotationUse::name>(ref<ScopedName>) +
      option(
          flag<&ast::AnnotationUse::hasArguments>(lit<"(">) +
          (some(append<&ast::AnnotationUse::arguments>(ref<NamedAnnotationArgument>),
                lit<",">) |
           append<&ast::AnnotationUse::arguments>(ref<AnnotationArgument>)) +
          lit<")">));
  static constexpr auto annapps =
      many(append<&ast::Annotated::annotations>(annotation_appl));

  struct ScopedName
      : rule<option(lit<"::">) + ref<ID> + many(lit<"::"> + ref<ID>)> {};

  // --- constant expressions -----------------------------------------------
  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<ConstExpr>) +
            lit<")">)> {};
  struct FloatLit : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {};
  struct FixedLit : rule<assign<&ast::FixedLit::text>(ref<FIXED_LIT>)> {};
  struct IntLit : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(
                          ref<DEC_LIT> | ref<OCT_LIT> | ref<HEX_LIT>))> {};
  struct WCharLit : rule<assign<&ast::WCharLit::text>(ref<WCHAR_LIT>)> {};
  struct CharLit
      : rule<assign<&ast::CharLit::text>(ref<CHAR_LIT>)> {
  };
  struct WStringLit : rule<create<ast::WStringLit>(
                              assign<&ast::WStringLit::text>(ref<WSTR_LIT>) +
                              many(append<&ast::WStringLit::continuations>(ref<WSTR_LIT>)))> {
  };
  struct StringLit : rule<create<ast::StringLit>(
                             assign<&ast::StringLit::text>(ref<STR_LIT>) +
                             many(append<&ast::StringLit::continuations>(ref<STR_LIT>)))> {};
  struct BoolLit
      : rule<create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"TRUE">) |
                                  kw<"FALSE">)> {};
  struct NameExpr : rule<assign<&ast::NameExpr::name>(ref<ScopedName>)> {};

  struct PrimaryExpr : rule<ref<ParenExpr>    //
                                | ref<FloatLit>   //
                                | ref<FixedLit>   //
                                | ref<IntLit>     //
                                | ref<WCharLit>   //
                                | ref<CharLit>    //
                                | ref<WStringLit> //
                                | ref<StringLit>  //
                                | ref<BoolLit>    //
                                | ref<NameExpr>> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+"> | lit<"~">) +
                 assign<&ast::UnaryExpr::operand>(ref<PrimaryExpr>)) //
             | ref<PrimaryExpr>> {};

  struct ConstExpr : infix<UnaryExpr, ast::BinaryExpr,
                               levels(left(lit<"*"> | lit<"/"> | lit<"%">),  //
                                      left(lit<"+"> | lit<"-">),             //
                                      left(lit<"<<"> | lit<">>">),           //
                                      left(lit<"&">),                        //
                                      left(lit<"^">),                        //
                                      left(lit<"|">))> {};

  // --- base types -----------------------------------------------------------

  // Every base-type keyword is accepted at every base-type position (a
  // const's own type excludes `any`/`Object`): a uniform position
  // over-accepts a handful of nonsensical types, never rejects a valid one.
  static constexpr auto base_type_kw =
      as<ast::BaseType::long_double>(kw<"long"> + kw<"double">) | as<ast::BaseType::float_>(kw<"float">) | as<ast::BaseType::double_>(kw<"double">) |          //
      as<ast::BaseType::long_long>(kw<"long"> + kw<"long">) | as<ast::BaseType::long_>(kw<"long">) | as<ast::BaseType::int8_>(kw<"int8">) | as<ast::BaseType::int16_>(kw<"int16">) | //
      as<ast::BaseType::int32_>(kw<"int32">) | as<ast::BaseType::int64_>(kw<"int64">) |                              //
      as<ast::BaseType::ulong_long>(kw<"unsigned"> + kw<"long"> + kw<"long">) | as<ast::BaseType::ulong_>(kw<"unsigned"> + kw<"long">) | as<ast::BaseType::ushort_>(kw<"unsigned"> + kw<"short">) | as<ast::BaseType::short_>(kw<"short">) | as<ast::BaseType::uint8_>(kw<"uint8">) |
      as<ast::BaseType::uint16_>(kw<"uint16">) | as<ast::BaseType::uint32_>(kw<"uint32">) | as<ast::BaseType::uint64_>(kw<"uint64">) |               //
      as<ast::BaseType::char_>(kw<"char">) | as<ast::BaseType::wchar_>(kw<"wchar">) | as<ast::BaseType::boolean_>(kw<"boolean">) | as<ast::BaseType::octet_>(kw<"octet">) |   //
      as<ast::BaseType::any_>(kw<"any">) | as<ast::BaseType::object_>(kw<"Object">) | as<ast::BaseType::value_base>(kw<"ValueBase">) | as<ast::BaseType::fixed_>(kw<"fixed">) | //
      as<ast::BaseType::void_>(kw<"void">);

  struct BaseTypeSpec
      : rule<create<ast::BaseTypeRef>(
            assign<&ast::BaseTypeRef::kind>(base_type_kw))> {};

  struct NamedTypeRef
      : rule<assign<&ast::NamedTypeRef::name>(ref<ScopedName>)> {};

  // --- template types: sequence/set/map/string/wstring/fixed --------------
  static constexpr auto element_type_spec =
      ref<BaseTypeSpec> | ref<SequenceType> | ref<SetType> |
      ref<MapType> | ref<StringType> | ref<WStringType> |
      ref<FixedPtType> | ref<NamedTypeRef>;

  struct SequenceType
      : rule<create<ast::SequenceTypeRef>(
            kw<"sequence"> + lit<"<"> + annapps +
            assign<&ast::SequenceTypeRef::element>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::SequenceTypeRef::bound>(ref<ConstExpr>)) +
            lit<">">)> {};

  struct SetType
      : rule<create<ast::SetTypeRef>(
            kw<"set"> + lit<"<"> +
            assign<&ast::SetTypeRef::element>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::SetTypeRef::bound>(ref<ConstExpr>)) +
            lit<">">)> {};

  struct MapType
      : rule<create<ast::MapTypeRef>(
            kw<"map"> + lit<"<"> +
            assign<&ast::MapTypeRef::keyType>(element_type_spec) + lit<","> +
            assign<&ast::MapTypeRef::valueType>(element_type_spec) +
            option(lit<","> +
                   assign<&ast::MapTypeRef::bound>(ref<ConstExpr>)) +
            lit<">">)> {};

  struct StringType
      : rule<create<ast::StringTypeRef>(
            kw<"string"> +
            option(lit<"<"> +
                   assign<&ast::StringTypeRef::bound>(ref<ConstExpr>) +
                   lit<">">))> {};
  struct WStringType
      : rule<create<ast::WStringTypeRef>(
            kw<"wstring"> +
            option(lit<"<"> +
                   assign<&ast::WStringTypeRef::bound>(ref<ConstExpr>) +
                   lit<">">))> {};
  struct FixedPtType
      : rule<create<ast::FixedPtTypeRef>(
            kw<"fixed"> + lit<"<"> +
            assign<&ast::FixedPtTypeRef::digits>(ref<ConstExpr>) +
            lit<","> + assign<&ast::FixedPtTypeRef::scale>(ref<ConstExpr>) +
            lit<">">)> {};

  // simple_type_spec: a base type, a template type, or a named reference.
  static constexpr auto simple_type_spec = element_type_spec;
  static constexpr auto const_type_spec =
      ref<BaseTypeSpec> | ref<StringType> | ref<WStringType> |
      ref<NamedTypeRef>;
  // The Anonymous Types building block permits template types directly in
  // parameter, return and attribute positions, not just behind typedefs.
  static constexpr auto param_type_spec = simple_type_spec;
  // constr_type_spec: a fresh struct/union/enum/bitset/bitmask used inline
  // as a type. type_spec = simple_type_spec | constr_type_spec.
  static constexpr auto constr_type_spec =
      ref<StructDecl> | ref<UnionDecl> | ref<EnumDecl> |
      ref<BitsetDecl> | ref<BitmaskDecl>;
  static constexpr auto type_spec = simple_type_spec | constr_type_spec;

  // --- declarators and members ---------------------------------------------
  struct Declarator
      : rule<annapps +
             create<ast::Declarator>(
                 assign<&ast::Declarator::name>(ref<ID>) +
                 many(lit<"["> +
                      append<&ast::Declarator::arraySizes>(ref<ConstExpr>) +
                      lit<"]">))> {};

  struct Member
      : rule<annapps +
             create<ast::Member>(
                 assign<&ast::Member::type>(type_spec) +
                 some(append<&ast::Member::declarators>(ref<Declarator>),
                      lit<",">)) +
             lit<";">> {};

  // --- struct/union/enum/bitset/bitmask ------------------------------------
  static constexpr auto struct_decl = create<ast::StructDecl>(
      kw<"struct"> + annapps + assign<&ast::StructDecl::name>(ref<ID>) +
      option(lit<":"> + assign<&ast::StructDecl::base>(ref<ScopedName>)) +
      lit<"{"> + many(append<&ast::StructDecl::members>(ref<Member>)) +
      lit<"}">);
  struct StructDecl : rule<struct_decl> {};

  struct CaseLabel
      : rule<annapps + create<ast::CaseLabel>(
                           (kw<"case"> +
                            assign<&ast::CaseLabel::value>(ref<ConstExpr>) +
                            lit<":">) |
                           flag<&ast::CaseLabel::default_>(kw<"default"> +
                                                                lit<":">))> {};
  struct ElementSpec
      : rule<annapps +
             create<ast::ElementSpec>(
                 assign<&ast::ElementSpec::type>(type_spec) +
                 assign<&ast::ElementSpec::declarator>(ref<Declarator>))> {
  };
  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            some(append<&ast::CaseStmt::labels>(ref<CaseLabel>)) +
            assign<&ast::CaseStmt::element>(ref<ElementSpec>) + lit<";">)> {
  };
  static constexpr auto switch_type_spec =
      ref<BaseTypeSpec> | ref<EnumDecl> | ref<NamedTypeRef>;
  static constexpr auto union_decl = create<ast::UnionDecl>(
      kw<"union"> + annapps + assign<&ast::UnionDecl::name>(ref<ID>) +
      kw<"switch"> + lit<"("> + annapps +
      assign<&ast::UnionDecl::switchType>(switch_type_spec) + lit<")"> +
      lit<"{"> + some(append<&ast::UnionDecl::cases>(ref<CaseStmt>)) +
      lit<"}">);
  struct UnionDecl : rule<union_decl> {};

  struct Enumerator
      : rule<annapps + create<ast::EnumeratorDecl>(
                           assign<&ast::EnumeratorDecl::name>(ref<ID>))> {};
  static constexpr auto enum_decl = create<ast::EnumDecl>(
      kw<"enum"> + annapps + assign<&ast::EnumDecl::name>(ref<ID>) + lit<"{"> +
      some(append<&ast::EnumDecl::enumerators>(ref<Enumerator>), lit<",">) +
      lit<"}">);
  struct EnumDecl : rule<enum_decl> {};

  struct BitfieldSpec
      : rule<create<ast::BitfieldSpec>(
            annapps + kw<"bitfield"> + lit<"<"> +
            assign<&ast::BitfieldSpec::width>(ref<ConstExpr>) +
            option(lit<","> + assign<&ast::BitfieldSpec::type>(base_type_kw)) +
            lit<">"> +
            option(annapps + append<&ast::BitfieldSpec::names>(ref<ID>) +
                   many(option(lit<",">) + annapps +
                        append<&ast::BitfieldSpec::names>(ref<ID>))) +
            lit<";">)> {};
  static constexpr auto bitset_decl = create<ast::BitsetDecl>(
      kw<"bitset"> + annapps + assign<&ast::BitsetDecl::name>(ref<ID>) +
      option(lit<":"> + assign<&ast::BitsetDecl::base>(ref<ScopedName>)) +
      lit<"{"> + many(append<&ast::BitsetDecl::fields>(ref<BitfieldSpec>)) +
      lit<"}">);
  struct BitsetDecl : rule<bitset_decl> {};

  static constexpr auto bitmask_decl = create<ast::BitmaskDecl>(
      kw<"bitmask"> + annapps + assign<&ast::BitmaskDecl::name>(ref<ID>) +
      lit<"{"> + some(append<&ast::BitmaskDecl::values>(ref<Enumerator>), lit<",">) +
      lit<"}">);
  struct BitmaskDecl : rule<bitmask_decl> {};

  // --- typedef / native / forward struct-union -----------------------------
  static constexpr auto typedef_decl = create<ast::TypedefDecl>(
      kw<"typedef"> + annapps + assign<&ast::TypedefDecl::type>(type_spec) +
      some(append<&ast::TypedefDecl::declarators>(ref<Declarator>),
           lit<",">));
  struct TypedefDecl : rule<typedef_decl> {};
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
      kw<"const"> + annapps + assign<&ast::ConstDecl::type>(const_type_spec) +
      annapps + assign<&ast::ConstDecl::name>(ref<ID>) + lit<"="> +
      assign<&ast::ConstDecl::value>(ref<ConstExpr>));
  struct ConstDecl : rule<const_decl> {};

  static constexpr auto exception_decl = create<ast::ExceptionDecl>(
      kw<"exception"> + annapps + assign<&ast::ExceptionDecl::name>(ref<ID>) +
      lit<"{"> + many(append<&ast::ExceptionDecl::members>(ref<Member>)) +
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


  struct Param
      : rule<annapps +
             create<ast::Param>(
                 assign<&ast::Param::dir>(as<ast::ParamDir::in_>(kw<"in">) | as<ast::ParamDir::out_>(kw<"out">) |
                                          as<ast::ParamDir::inout_>(kw<"inout">)) +
                 annapps + assign<&ast::Param::type>(param_type_spec) +
                 annapps + assign<&ast::Param::name>(ref<ID>))> {};

  static constexpr auto op_decl = create<ast::OpDecl>(
      option(flag<&ast::OpDecl::oneway_>(kw<"oneway">)) + annapps +
      assign<&ast::OpDecl::returnType>(param_type_spec) + annapps +
      assign<&ast::OpDecl::name>(ref<ID>) + lit<"("> +
      option(some(append<&ast::OpDecl::params>(ref<Param>), lit<",">)) +
      lit<")"> +
      option(kw<"raises"> + lit<"("> +
             some(append<&ast::OpDecl::raises>(ref<ScopedName>), lit<",">) +
             lit<")">) +
      option(kw<"context"> + lit<"("> +
             some(append<&ast::OpDecl::context>(ref<StringSpelling>), lit<",">) +
             lit<")">));

  // --- import / type_id / type_prefix ---------------------------------------
  static constexpr auto import_decl = kw<"import"> + annapps +
      create<ast::ImportDecl>(assign<&ast::ImportDecl::imported>(
          ref<ScopedName> | ref<StringSpelling>));
  struct ImportDecl : rule<annapps + import_decl> {};
  static constexpr auto typeid_decl = create<ast::TypeIdDecl>(
      kw<"typeid"> + annapps +
      assign<&ast::TypeIdDecl::target>(ref<ScopedName>) +
      assign<&ast::TypeIdDecl::repositoryId>(ref<StringSpelling>));
  static constexpr auto typeprefix_decl = create<ast::TypePrefixDecl>(
      kw<"typeprefix"> + annapps +
      assign<&ast::TypePrefixDecl::target>(ref<ScopedName>) +
      assign<&ast::TypePrefixDecl::prefix>(ref<StringSpelling>));

  // --- annotation definitions
  // -------------------------------------------------
  struct AnnotationMember
      : rule<create<ast::AnnotationMember>(
            assign<&ast::AnnotationMember::type>(const_type_spec) +
            assign<&ast::AnnotationMember::name>(ref<ID>) +
            option(kw<"default"> + assign<&ast::AnnotationMember::defaultValue>(
                                       ref<ConstExpr>)) +
            lit<";">)> {};
  static constexpr auto annotation_body_item =
      ref<AnnotationMember>         //
      | (ref<EnumDecl> + lit<";">)  //
      | (ref<ConstDecl> + lit<";">) //
      | (ref<TypedefDecl> + lit<";">);

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
      lit<"{"> + many(append<&ast::InterfaceDecl::exports>(ref<Export>)) +
      lit<"}">);

  struct Export
      : rule<annapps +
             (attr_decl | op_decl | typeid_decl | typeprefix_decl |
              exception_decl | type_decl | const_decl | import_decl) +
             lit<";">> {};

  // --- module
  // ------------------------------------------------------------------
  struct Definition;
  static constexpr auto module_decl = create<ast::ModuleDecl>(
      kw<"module"> + annapps + assign<&ast::ModuleDecl::name>(ref<ID>) +
      lit<"{"> +
      some(append<&ast::ModuleDecl::definitions>(ref<Definition>)) +
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
                 some(append<&ast::ValueAbsDecl::supports_>(ref<ScopedName>), lit<",">)) +
          lit<"{"> +
          many(append<&ast::ValueAbsDecl::exports>(ref<Export>)) +
          lit<"}">);

  struct StateMember
      : rule<annapps +
             create<ast::StateMember>(
                 (flag<&ast::StateMember::public_>(kw<"public">) |
                  kw<"private">)+annapps +
                 assign<&ast::StateMember::type>(type_spec) +
                 some(append<&ast::StateMember::declarators>(
                          ref<Declarator>),
                      lit<",">)) +
             lit<";">> {};

  struct InitDecl
      : rule<annapps + kw<"factory"> +
             create<ast::InitDecl>(
                 annapps + assign<&ast::InitDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::InitDecl::params>(ref<Param>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        some(append<&ast::InitDecl::raises>(ref<ScopedName>), lit<",">) +
                        lit<")">)) +
             lit<";">> {};

  static constexpr auto value_element =
      ref<Export> | ref<StateMember> | ref<InitDecl>;

  static constexpr auto value_decl = create<ast::ValueDecl>(
      option(flag<&ast::ValueDecl::custom_>(kw<"custom">)) + kw<"valuetype"> +
      annapps + assign<&ast::ValueDecl::name>(ref<ID>) +
      option(lit<":"> +
             option(flag<&ast::ValueDecl::truncatable_>(kw<"truncatable">)) +
             some(append<&ast::ValueDecl::valueBases>(ref<ScopedName>), lit<",">)) +
      option(kw<"supports"> +
             some(append<&ast::ValueDecl::supports_>(ref<ScopedName>), lit<",">)) +
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
                 some(append<&ast::EventAbsDecl::supports_>(ref<ScopedName>), lit<",">)) +
          lit<"{"> +
          many(append<&ast::EventAbsDecl::exports>(ref<Export>)) +
          lit<"}">);

  static constexpr auto event_decl = create<ast::EventDecl>(
      option(flag<&ast::EventDecl::custom_>(kw<"custom">)) + kw<"eventtype"> +
      annapps + assign<&ast::EventDecl::name>(ref<ID>) +
      option(lit<":"> +
             option(flag<&ast::EventDecl::truncatable_>(kw<"truncatable">)) +
             some(append<&ast::EventDecl::valueBases>(ref<ScopedName>), lit<",">)) +
      option(kw<"supports"> +
             some(append<&ast::EventDecl::supports_>(ref<ScopedName>), lit<",">)) +
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

  struct ComponentExport
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
             some(append<&ast::ComponentDecl::supports_>(ref<ScopedName>), lit<",">)) +
      lit<"{"> +
      many(append<&ast::ComponentDecl::exports>(ref<ComponentExport>)) +
      lit<"}">);

  // --- home
  // ----------------------------------------------------------------------
  struct FactoryDecl
      : rule<annapps + kw<"factory"> +
             create<ast::FactoryDecl>(
                 annapps + assign<&ast::FactoryDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::FactoryDecl::params>(ref<Param>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        append<&ast::FactoryDecl::raises>(ref<ScopedName>) +
                        many(lit<","> + append<&ast::FactoryDecl::raises>(
                                            ref<ScopedName>)) +
                        lit<")">)) +
             lit<";">> {};
  struct FinderDecl
      : rule<annapps + kw<"finder"> +
             create<ast::FinderDecl>(
                 annapps + assign<&ast::FinderDecl::name>(ref<ID>) + lit<"("> +
                 option(some(append<&ast::FinderDecl::params>(ref<Param>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"raises"> + lit<"("> +
                        append<&ast::FinderDecl::raises>(ref<ScopedName>) +
                        many(lit<","> + append<&ast::FinderDecl::raises>(
                                            ref<ScopedName>)) +
                        lit<")">)) +
             lit<";">> {};

  static constexpr auto home_export =
      ref<Export> | ref<FactoryDecl> | ref<FinderDecl>;

  static constexpr auto home_decl = create<ast::HomeDecl>(
      kw<"home"> + annapps + assign<&ast::HomeDecl::name>(ref<ID>) +
      option(lit<":"> + annapps +
             assign<&ast::HomeDecl::base>(ref<ScopedName>)) +
      option(
          kw<"supports"> + some(append<&ast::HomeDecl::supports_>(ref<ScopedName>), lit<",">)) +
      kw<"manages"> + assign<&ast::HomeDecl::manages_>(ref<ScopedName>) +
      option(kw<"primarykey"> +
             assign<&ast::HomeDecl::primaryKey>(ref<ScopedName>)) +
      lit<"{"> + many(append<&ast::HomeDecl::exports>(home_export)) + lit<"}">);

  // --- top-level definition and specification
  // -------------------------------------
  static constexpr auto definition_body =
             (type_decl | const_decl | exception_decl | interface_decl |
              interface_forward_decl | module_decl | value_decl |
              value_abs_decl | value_box_decl | value_forward_decl |
              typeid_decl | typeprefix_decl | event_decl | event_abs_decl |
              event_forward_decl | component_decl | component_forward_decl |
              home_decl | annotation_decl | annotation_forward_decl);
  struct Definition
      : rule<annapps + (definition_body | import_decl) + lit<";">> {};
  struct NonImportDefinition : rule<annapps + definition_body + lit<";">> {};

  struct Specification
      : rule<create<ast::Specification>(
            !eof +
            many(append<&ast::Specification::imports>(ref<ImportDecl>) +
                 lit<";">) +
            option(append<&ast::Specification::definitions>(ref<NonImportDefinition>) +
                   many(append<&ast::Specification::definitions>(ref<Definition>))))> {};

  using entry = Specification;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace idl
