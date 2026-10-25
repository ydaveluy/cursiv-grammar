
#pragma once

#include <memory_resource>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "objc/core/ast.hpp"

namespace objc {

inline constexpr auto identifier_text = [](std::string_view text,
                                           std::pmr::memory_resource* mem) {
  if (text.find('\\') == text.npos) return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size();) {
    if (text[i] != '\\' || i + 1 >= text.size() ||
        (text[i + 1] != 'u' && text[i + 1] != 'U')) {
      room[n++] = text[i++];
      continue;
    }
    const unsigned width = text[i + 1] == 'u' ? 4 : 8;
    unsigned value = 0;
    if (i + 2 + width > text.size()) return text;
    for (unsigned j = 0; j < width; ++j) {
      const char c = text[i + 2 + j];
      value = value * 16 + (c >= '0' && c <= '9'   ? c - '0'
                            : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                   : c - 'A' + 10);
    }
    if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return text;
    i += 2 + width;
    if (value < 0x80)
      room[n++] = static_cast<char>(value);
    else if (value < 0x800) {
      room[n++] = static_cast<char>(0xC0 | (value >> 6));
      room[n++] = static_cast<char>(0x80 | (value & 63));
    } else if (value < 0x10000) {
      room[n++] = static_cast<char>(0xE0 | (value >> 12));
      room[n++] = static_cast<char>(0x80 | ((value >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (value & 63));
    } else {
      room[n++] = static_cast<char>(0xF0 | (value >> 18));
      room[n++] = static_cast<char>(0x80 | ((value >> 12) & 63));
      room[n++] = static_cast<char>(0x80 | ((value >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (value & 63));
    }
  }
  return std::string_view(room.data(), n);
};

struct Objc : cursiv::GrammarSpec {
  static constexpr auto LineSplice = lit<"\\"> + option(lit<"\r">) + lit<"\n">;
  struct WS : terminal<some(s | LineSplice)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(LineSplice | cr<R"(^\r\n)">)> {
  };
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct DIRECTIVE
      : terminal<lit<"#"> +
                 many(LineSplice | through(lit<"/*">, lit<"*/">) |
                      (lit<"//"> + many(LineSplice | cr<R"(^\r\n)">)) |
                      (!cr<R"(\r\n)"> + any))> {};
  struct NS_ASSUME_NONNULL_BEGIN_T : terminal<kw<"NS_ASSUME_NONNULL_BEGIN">> {};
  struct NS_ASSUME_NONNULL_END_T : terminal<kw<"NS_ASSUME_NONNULL_END">> {};
  struct EXTERN_SUFFIX_T : terminal<some(cr<"_A-Z">) + lit<"_EXTERN">> {};

  static constexpr auto IdentStart = cr<"$_a-zA-Z"> | uc::XID_Start;
  static constexpr auto IdentCont = cr<"$_a-zA-Z0-9"> | uc::XID_Continue;
  static constexpr auto word_boundary = IdentCont;

  static constexpr auto UCN = (lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">)) |
                              (lit<"\\U"> + repeat<8>(cr<"0-9a-fA-F">));
  struct IDENTIFIER : terminal<(IdentStart | UCN) + many(IdentCont | UCN),
                               text_conv<identifier_text>> {};

  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto Escape = LineSplice | (lit<"\\"> + any);
  struct CHAR_LIT : terminal<option(lit<"u8"> | cr<"LuU">) + lit<"'"> +
                             many(Escape | cr<R"(^'\\)">) + lit<"'">> {};
  struct STRING_LIT
      : terminal<((lit<"@"> + many(s | LineSplice | ref<SL_COMMENT> |
                                    ref<ML_COMMENT>)) |
                  option(lit<"u8"> | cr<"LuU">)) +
                 lit<"\""> + many(Escape | cr<R"(^"\\)">) + lit<"\"">> {};
  struct NUMBER
      : terminal<(d | (lit<"."> + d)) + many((cr<"eEpP"> + cr<"+-">) |
                                             IdentCont | lit<"."> | lit<"'">)> {
  };
  struct VERSION : terminal<some(d) + many(lit<"."> + some(d))> {};

  static constexpr auto K_BOOL = kw<"BOOL">;
  static constexpr auto K_Class = kw<"Class">;
  static constexpr auto K_bycopy = kw<"bycopy">;
  static constexpr auto K_byref = kw<"byref">;
  static constexpr auto K_id = kw<"id">;
  static constexpr auto K_IMP = kw<"IMP">;
  static constexpr auto K_in = kw<"in">;
  static constexpr auto K_inout = kw<"inout">;
  static constexpr auto K_oneway = kw<"oneway">;
  static constexpr auto K_out = kw<"out">;
  static constexpr auto K_Protocol = kw<"Protocol">;
  static constexpr auto K_SEL = kw<"SEL">;
  static constexpr auto K_self = kw<"self">;
  static constexpr auto K_super = kw<"super">;
  static constexpr auto K_atomic = kw<"atomic">;
  static constexpr auto K_nonatomic = kw<"nonatomic">;
  static constexpr auto K_retain = kw<"retain">;
  static constexpr auto K_direct = kw<"direct">;
  static constexpr auto K_register = kw<"register">;
  static constexpr auto K_autoreleasing = kw<"__autoreleasing">;
  static constexpr auto K_block = kw<"__block">;
  static constexpr auto K_bridge = kw<"__bridge">;
  static constexpr auto K_bridge_retained = kw<"__bridge_retained">;
  static constexpr auto K_bridge_transfer = kw<"__bridge_transfer">;
  static constexpr auto K_covariant = kw<"__covariant">;
  static constexpr auto K_contravariant = kw<"__contravariant">;
  static constexpr auto K_deprecated = kw<"__deprecated">;
  static constexpr auto K_kindof = kw<"__kindof">;
  static constexpr auto K_strong_q = kw<"__strong">;
  static constexpr auto K_unused = kw<"__unused">;
  static constexpr auto K_weak_q = kw<"__weak">;
  static constexpr auto K_unsafe_unretained_q = kw<"__unsafe_unretained">;
  static constexpr auto K_inline_attr = kw<"__inline__">;
  static constexpr auto K_extension = kw<"__extension__">;
  static constexpr auto K_align_of = kw<"_Alignof">;
  static constexpr auto K_typeof =
      kw<"typeof"> | kw<"__typeof"> | kw<"__typeof__">;
  static constexpr auto K_null_unspecified = kw<"null_unspecified"> |
                                             kw<"__null_unspecified"> |
                                             kw<"_Null_unspecified">;
  static constexpr auto K_nullable =
      kw<"nullable"> | kw<"__nullable"> | kw<"_Nullable">;
  static constexpr auto K_nonnull =
      kw<"nonnull"> | kw<"__nonnull"> | kw<"_Nonnull">;
  static constexpr auto K_null_resettable = kw<"null_resettable">;
  static constexpr auto K_ns_inline = kw<"NS_INLINE">;
  static constexpr auto K_ns_enum = kw<"NS_ENUM">;
  static constexpr auto K_ns_closed_enum = kw<"NS_CLOSED_ENUM">;
  static constexpr auto K_ns_error_enum = kw<"NS_ERROR_ENUM">;
  static constexpr auto K_ns_options = kw<"NS_OPTIONS">;
  static constexpr auto K_ns_swift_name = kw<"NS_SWIFT_NAME">;
  static constexpr auto K_ns_noescape = kw<"NS_NOESCAPE">;
  static constexpr auto K_ns_unavailable = kw<"NS_UNAVAILABLE">;
  static constexpr auto K_ns_swift_unavailable = kw<"NS_SWIFT_UNAVAILABLE">;
  static constexpr auto K_api_available = kw<"API_AVAILABLE">;
  static constexpr auto K_api_unavailable = kw<"API_UNAVAILABLE">;
  static constexpr auto K_assign = kw<"assign">;
  static constexpr auto K_copy = kw<"copy">;
  static constexpr auto K_getter = kw<"getter">;
  static constexpr auto K_setter = kw<"setter">;
  static constexpr auto K_strong = kw<"strong">;
  static constexpr auto K_readonly = kw<"readonly">;
  static constexpr auto K_readwrite = kw<"readwrite">;
  static constexpr auto K_weak = kw<"weak">;
  static constexpr auto K_unsafe_unretained = kw<"unsafe_unretained">;
  static constexpr auto K_ib_outlet = kw<"IBOutlet">;
  static constexpr auto K_ib_outlet_collection = kw<"IBOutletCollection">;
  static constexpr auto K_ib_inspectable = kw<"IBInspectable">;
  static constexpr auto K_ib_designable = kw<"IB_DESIGNABLE">;

  static constexpr auto NameText =
      ref<IDENTIFIER> | K_BOOL | K_Class | K_bycopy | K_byref | K_id | K_IMP |
      K_in | K_inout | K_oneway | K_out | K_Protocol | K_SEL | K_self |
      K_super | K_atomic | K_nonatomic | K_retain | K_register |
      K_autoreleasing | K_block | K_bridge_retained | K_bridge_transfer |
      K_covariant | K_contravariant | K_deprecated | K_kindof | K_unused |
      K_ns_inline | K_ns_enum | K_ns_options | K_ns_swift_name |
      K_null_unspecified | K_nullable | K_nonnull | K_null_resettable |
      K_assign | K_copy | K_getter | K_setter | K_strong | K_readonly |
      K_readwrite | K_weak | K_unsafe_unretained | K_ib_outlet |
      K_ib_outlet_collection | K_ib_inspectable | K_ib_designable;

  static constexpr auto StorageClass =
      kw<"auto"> | K_register | kw<"static"> | kw<"extern"> | kw<"typedef"> |
      kw<"_Thread_local"> | kw<"thread_local"> | kw<"__thread"> |
      kw<"constexpr">;
  struct AtomicQualifier : rule<kw<"_Atomic"> + !lit<"(">, std::string_view> {};
  static constexpr auto TypeQualifierBase =
      kw<"const"> | kw<"volatile"> | kw<"restrict"> | ref<AtomicQualifier>;
  static constexpr auto ProtocolQualifier =
      K_in | K_out | K_inout | K_bycopy | K_byref | K_oneway;
  static constexpr auto TypeQualifier = TypeQualifierBase | ProtocolQualifier;
  static constexpr auto ArcBehaviour =
      K_weak_q | K_strong_q | K_autoreleasing | K_unsafe_unretained_q;
  static constexpr auto NullabilitySpec =
      K_null_unspecified | K_nullable | K_nonnull | K_null_resettable;
  static constexpr auto TypePrefix =
      K_bridge | K_bridge_transfer | K_bridge_retained | K_block |
      kw<"inline"> | kw<"_Noreturn"> | K_ns_inline | K_inline_attr | K_kindof;
  static constexpr auto ArcNullQual =
      ArcBehaviour | NullabilitySpec | TypeQualifier;
  static constexpr auto HardBuiltinWord =
      kw<"void"> | kw<"char"> | kw<"short"> | kw<"int"> | kw<"long"> |
      kw<"float"> | kw<"double"> | kw<"signed"> | kw<"unsigned"> | kw<"_Bool"> |
      kw<"_Complex"> | kw<"__int128">;
  static constexpr auto BuiltinWord = HardBuiltinWord | kw<"bool">;
  static constexpr auto MessageSelectorWord =
      NameText | kw<"auto"> | kw<"break"> | kw<"case"> | kw<"char"> |
      kw<"const"> | kw<"continue"> | kw<"default"> | kw<"do"> | kw<"double"> |
      kw<"else"> | kw<"enum"> | kw<"extern"> | kw<"float"> | kw<"for"> |
      kw<"goto"> | kw<"if"> | kw<"inline"> | kw<"int"> | kw<"long"> |
      kw<"restrict"> | kw<"return"> | kw<"short"> | kw<"signed"> |
      kw<"sizeof"> | kw<"static"> | kw<"struct"> | kw<"switch"> |
      kw<"typedef"> | kw<"union"> | kw<"unsigned"> | kw<"void"> |
      kw<"volatile"> | kw<"while"> | kw<"_Alignas"> | kw<"_Alignof"> |
      kw<"_Atomic"> | kw<"_Bool"> | kw<"_Complex"> | kw<"_Generic"> |
      kw<"_Imaginary"> | kw<"_Noreturn"> | kw<"_Static_assert"> |
      kw<"_Thread_local">;
  static constexpr auto Selector = MessageSelectorWord;

  struct DeclarationSpecifiers;
  struct TypeSpecifier;
  struct TypeName;
  struct Declarator;
  struct AbstractDeclarator;
  struct Param;
  struct Expression;
  struct AssignmentExpression;
  struct ConditionalExpression;
  struct ConstantExpression;
  struct CastExpression;
  struct UnaryExpression;
  struct PostfixExpression;
  static constexpr auto KnownTypeHead =
      HardBuiltinWord | kw<"struct"> | kw<"union"> | kw<"enum"> |
      kw<"_Atomic"> | kw<"_BitInt"> | K_typeof;
  static constexpr auto BareOperandName = NameText + &(lit<","> | lit<")">);
  template <class ExprRule>
  struct TypeOrExpression
      : rule<(&KnownTypeHead + ref<TypeName>) |
             (!KnownTypeHead +
              (create<ast::UnresolvedName>(
                   &BareOperandName +
                   assign<&ast::UnresolvedName::name>(NameText)) |
               (!BareOperandName + (ref<ExprRule> | ref<TypeName>))))> {};

  struct GenericAssociation
      : rule<(kw<"default"> |
              assign<&ast::GenericAssociation::type>(ref<TypeName>)) +
             lit<":"> +
             assign<&ast::GenericAssociation::value>(
                 ref<AssignmentExpression>)> {};
  struct GenericSelection
      : rule<kw<"_Generic"> + lit<"("> +
             assign<&ast::GenericExpr::control>(
                 ref<TypeOrExpression<AssignmentExpression>>) +
             lit<","> +
             some(append<&ast::GenericExpr::associations>(
                      ref<GenericAssociation>),
                  lit<",">) +
             lit<")">> {};

  struct PrimaryExpression;
  struct Statement;
  struct CompoundStatement;
  struct Initializer;
  struct InitializerItem;
  struct Macro;
  struct Attribute;
  struct ClassInterfaceMember;
  struct ClassImplementationMember;

  struct AttributeParameter
      : rule<create<ast::AttributeParam>(
                 assign<&ast::AttributeParam::nested>(ref<Attribute>)) |
             create<ast::AttributeParam>(
                 assign<&ast::AttributeParam::value>(ref<PrimaryExpression>)) |
             create<ast::AttributeParam>(
                 assign<&ast::AttributeParam::assignName>(kw<"const"> |
                                                          NameText) +
                 lit<"="> +
                 (assign<&ast::AttributeParam::assignText>(kw<"const"> |
                                                           NameText) |
                  assign<&ast::AttributeParam::assignValue>(
                      ref<PrimaryExpression>)))> {};

  struct Attribute
      : rule<assign<&ast::Attribute::name>(kw<"const"> | NameText) +
             option(lit<"("> +
                    option(some(append<&ast::Attribute::params>(
                                    ref<AttributeParameter>),
                                lit<",">)) +
                    lit<")">)> {};

  template <auto Member>
  static constexpr auto AttributeSpecifierExpr =
      kw<"__attribute__"> + lit<"("> + lit<"("> +
      some(append<Member>(ref<Attribute>), lit<",">) + lit<")"> + lit<")">;

  struct BuiltinTypeSpecifier
      : rule<assign<&ast::BuiltinType::keyword>(BuiltinWord)> {};

  struct TypeofSpecifier
      : rule<K_typeof + lit<"("> +
             assign<&ast::TypeofType::operand>(
                 ref<TypeOrExpression<Expression>>) +
             lit<")"> +
             many(append<&ast::TypeofType::qualifiers>(ArcNullQual))> {};

  struct FieldDeclaration;
  struct StaticAssertDeclaration;
  struct StructOrUnionSpecifier
      : rule<assign<&ast::TagType::keyword>(kw<"struct"> | kw<"union">) +
             ((option(assign<&ast::TagType::name>(NameText)) +
               flag<&ast::TagType::defined>(lit<"{">) +
               many(append<&ast::TagType::members>(
                   ref<FieldDeclaration> | ref<StaticAssertDeclaration>)) +
               lit<"}">) |
              assign<&ast::TagType::name>(NameText))> {};

  struct Enumerator
      : rule<assign<&ast::Enumerator::name>(NameText | kw<"default">) +
             option(lit<"="> + assign<&ast::Enumerator::value>(
                                   ref<AssignmentExpression>))> {};

  struct EnumSpecifier
      : rule<kw<"enum"> + option(assign<&ast::EnumType::name>(NameText)) +
             option(lit<":"> +
                    assign<&ast::EnumType::underlying>(ref<TypeName>)) +
             option(flag<&ast::EnumType::defined>(lit<"{">) +
                    option(some(append<&ast::EnumType::enumerators>(
                                    ref<Enumerator>),
                                lit<",">) +
                           option(lit<",">)) +
                    lit<"}">)> {};

  struct NsEnumOrOptionSpecifier
      : rule<assign<&ast::EnumType::nsMacro>(K_ns_options | K_ns_enum |
                                             K_ns_closed_enum |
                                             K_ns_error_enum) +
             lit<"("> + assign<&ast::EnumType::underlying>(ref<TypeName>) +
             lit<","> + assign<&ast::EnumType::name>(NameText) + lit<")"> +
             option(flag<&ast::EnumType::defined>(lit<"{">) +
                    some(append<&ast::EnumType::enumerators>(ref<Enumerator>),
                         lit<",">) +
                    option(lit<",">) + lit<"}">)> {};

  struct ProtocolRef : rule<option(assign<&ast::ProtocolRef::variance>(
                                K_covariant | K_contravariant)) +
                            assign<&ast::ProtocolRef::name>(NameText)> {};

  template <auto Member>
  static constexpr auto ProtocolListInto =
      some(append<Member>(ref<ProtocolRef>), lit<",">);

  struct TypeArgument
      : rule<option(assign<&ast::TypeArgument::variance>(K_covariant |
                                                         K_contravariant)) +
             assign<&ast::TypeArgument::type>(ref<TypeName>) +
             option(lit<":"> +
                    assign<&ast::TypeArgument::bound>(ref<TypeName>))> {};
  struct AngleClause
      : rule<some(append<&ast::AngleClause::args>(ref<TypeArgument>),
                  lit<",">)> {};

  struct IdTypeSpecifier
      : rule<K_id +
             many(lit<"<"> + append<&ast::IdType::clauses>(ref<AngleClause>) +
                  lit<">">) +
             many(append<&ast::IdType::qualifiers>(ArcNullQual))> {};

  struct GenericTypeSpecifier
      : rule<assign<&ast::ClassRef::name>(NameText) +
             many(lit<"<"> + append<&ast::ClassRef::clauses>(ref<AngleClause>) +
                  lit<">">) +
             many(append<&ast::ClassRef::qualifiers>(ArcNullQual))> {};

  struct AtomicTypeSpecifier
      : rule<kw<"_Atomic"> + lit<"("> +
             assign<&ast::AtomicType::type>(ref<TypeName>) + lit<")">> {};
  struct BitIntTypeSpecifier
      : rule<kw<"_BitInt"> + lit<"("> +
             assign<&ast::BitIntType::width>(ref<AssignmentExpression>) +
             lit<")">> {};
  struct AlignmentSpecifier
      : rule<(kw<"_Alignas"> | kw<"alignas">)+lit<"("> +
             assign<&ast::Alignment::operand>(
                 ref<TypeOrExpression<AssignmentExpression>>) +
             lit<")">> {};
  struct TypeSpecifier
      : rule<ref<BuiltinTypeSpecifier> | ref<AtomicTypeSpecifier> |
             ref<BitIntTypeSpecifier> | ref<TypeofSpecifier> |
             ref<StructOrUnionSpecifier> | ref<EnumSpecifier> |
             ref<NsEnumOrOptionSpecifier> | ref<IdTypeSpecifier> |
             ref<GenericTypeSpecifier>> {};
  static constexpr auto DeclSpecifierItem =
      append<&ast::Specifiers::qualifiers>(StorageClass | ArcBehaviour |
                                           NullabilitySpec | K_ns_noescape |
                                           TypePrefix | TypeQualifier) |
      AttributeSpecifierExpr<&ast::Specifiers::attributes> |
      append<&ast::Specifiers::alignments>(ref<AlignmentSpecifier>) |
      (flag<&ast::Specifiers::ibOutletCollection>(K_ib_outlet_collection) +
       lit<"("> + append<&ast::Specifiers::ibOutletCollectionTypes>(NameText) +
       lit<")">) |
      flag<&ast::Specifiers::ibOutlet>(K_ib_outlet);
  struct DeclarationSpecifiers
      : rule<many(DeclSpecifierItem |
                  append<&ast::Specifiers::qualifiers>(ref<EXTERN_SUFFIX_T>)) +
             append<&ast::Specifiers::types>(ref<TypeSpecifier>) +
             many(DeclSpecifierItem |
                  append<&ast::Specifiers::types>(ref<BuiltinTypeSpecifier> |
                                                  ref<BitIntTypeSpecifier>))> {
  };

  struct PointerPart
      : rule<(lit<"*"> | flag<&ast::Pointer::isBlock>(lit<"^">)) +
             many(append<&ast::Pointer::qualifiers>(ArcNullQual |
                                                    K_ns_noescape))> {};
  struct ArraySuffix
      : rule<lit<"["> +
             many(append<&ast::ArraySuffix::qualifiers>(TypeQualifier) |
                  flag<&ast::ArraySuffix::isStatic>(kw<"static">)) +
             option(assign<&ast::ArraySuffix::size>(ref<AssignmentExpression>) |
                    lit<"*">) +
             lit<"]">> {};
  struct FunctionSuffix
      : rule<lit<"("> +
             option(
                 (some(append<&ast::FunctionSuffix::params>(ref<Param>),
                       lit<",">) +
                  option(lit<","> + flag<&ast::FunctionSuffix::variadic>(
                                        lit<"...">))) |
                 flag<&ast::FunctionSuffix::variadic>(lit<"...">)) +
             lit<")">> {};
  struct DeclaratorSuffix : rule<ref<ArraySuffix> | ref<FunctionSuffix>> {};
  struct Declarator
      : rule<many(append<&ast::Declarator::pointers>(ref<PointerPart>)) +
             (assign<&ast::Declarator::name>(NameText) |
              (lit<"("> + assign<&ast::Declarator::nested>(ref<Declarator>) +
               lit<")">)) +
             many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>)) +
             many(AttributeSpecifierExpr<&ast::Declarator::attributes>)> {};
  static constexpr auto AbstractTail =
      (lit<"("> + assign<&ast::Declarator::nested>(ref<AbstractDeclarator>) +
       lit<")"> +
       many(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>))) |
      some(append<&ast::Declarator::suffixes>(ref<DeclaratorSuffix>));
  struct AbstractDeclarator
      : rule<(some(append<&ast::Declarator::pointers>(ref<PointerPart>)) +
              option(AbstractTail)) |
             AbstractTail> {};
  struct Param
      : rule<assign<&ast::Param::specifiers>(ref<DeclarationSpecifiers>) +
             option(assign<&ast::Param::declarator>(
                 ref<Declarator> | ref<AbstractDeclarator>))> {};
  template <auto Member>
  static constexpr auto BlockParametersInto =
      lit<"("> + option(some(append<Member>(ref<Param>), lit<",">)) + lit<")">;
  struct TypeName
      : rule<assign<&ast::TypeName::specifiers>(ref<DeclarationSpecifiers>) +
             option(
                 assign<&ast::TypeName::declarator>(ref<AbstractDeclarator>))> {
  };

  struct Identifier : rule<assign<&ast::Ident::name>(NameText)> {};
  struct SelfIdent : rule<assign<&ast::Ident::name>(K_self)> {};
  struct SuperIdent : rule<assign<&ast::Ident::name>(K_super)> {};

  struct NumberLiteral : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct CharacterLiteral : rule<assign<&ast::CharLit::text>(ref<CHAR_LIT>)> {};
  struct StringLiteral
      : rule<some(append<&ast::StringLit::pieces>(ref<STRING_LIT>))> {};
  struct KeywordLiteral : rule<assign<&ast::KeywordLit::keyword>(
                              kw<"__objc_yes"> | kw<"__objc_no">)> {};
  struct Constant
      : rule<ref<GenericSelection> | ref<NumberLiteral> |
             ref<CharacterLiteral> | ref<KeywordLiteral> | ref<Identifier>> {};

  struct DictPair
      : rule<assign<&ast::DictPair::key>(ref<AssignmentExpression>) + lit<":"> +
             assign<&ast::DictPair::value>(ref<AssignmentExpression>)> {};

  struct DictionaryLiteralExpression
      : rule<lit<"@"> + lit<"{"> +
             option(
                 some(append<&ast::DictExpr::pairs>(ref<DictPair>), lit<",">) +
                 option(lit<",">)) +
             lit<"}">> {};

  struct ArrayLiteralExpression
      : rule<lit<"@"> + lit<"["> +
             option(
                 some(append<&ast::ArrayExpr::items>(ref<AssignmentExpression>),
                      lit<",">) +
                 option(lit<",">)) +
             lit<"]">> {};

  struct BoxedExpression
      : rule<lit<"@"> +
             ((lit<"("> + assign<&ast::BoxedExpr::value>(ref<Expression>) +
               lit<")">) |
              assign<&ast::BoxedExpr::value>(ref<Constant>))> {};

  struct BlockExpression
      : rule<lit<"^"> +
             option(assign<&ast::BlockExpr::returnType>(
                        ref<DeclarationSpecifiers>) +
                    option(assign<&ast::BlockExpr::returnDeclarator>(
                        ref<AbstractDeclarator>))) +
             option(assign<&ast::BlockExpr::nullability>(NullabilitySpec)) +
             option(BlockParametersInto<&ast::BlockExpr::params>) +
             assign<&ast::BlockExpr::body>(ref<CompoundStatement>)> {};

  struct MessageArg
      : rule<option(assign<&ast::MessageArg::label>(MessageSelectorWord)) +
             lit<":"> +
             assign<&ast::MessageArg::value>(ref<AssignmentExpression>)> {};
  struct MessageExpression
      : rule<lit<"["> +
             (assign<&ast::MessageExpr::receiver>(ref<Expression>) |
              assign<&ast::MessageExpr::receiverType>(ref<TypeName>)) +
             (assign<&ast::MessageExpr::unarySelector>(MessageSelectorWord) |
              (some(append<&ast::MessageExpr::args>(ref<MessageArg>)) +
               many(lit<","> + append<&ast::MessageExpr::variadicArgs>(
                                   ref<AssignmentExpression>)))) +
             lit<"]">> {};
  struct SelectorPart
      : rule<option(assign<&ast::SelectorPart::name>(MessageSelectorWord)) +
             lit<":">> {};
  struct SelectorName
      : rule<some(append<&ast::SelectorExpr::parts>(ref<SelectorPart>)) |
             assign<&ast::SelectorExpr::simple>(MessageSelectorWord)> {};

  struct SelectorExpression
      : rule<lit<"@selector"> + lit<"("> +
             (ref<SelectorName> | (lit<"("> + ref<SelectorName> + lit<")">)) +
             lit<")">> {};

  struct ProtocolExpression
      : rule<lit<"@protocol"> + lit<"("> +
             option(assign<&ast::ProtocolExpr::variance>(K_covariant |
                                                         K_contravariant)) +
             assign<&ast::ProtocolExpr::name>(NameText) + lit<")">> {};

  struct EncodeExpression
      : rule<lit<"@encode"> + lit<"("> +
             assign<&ast::EncodeExpr::type>(ref<TypeName>) + lit<")">> {};

  struct BracedInit;
  struct AvailabilityClause
      : rule<(assign<&ast::AvailabilityClause::platform>(NameText) +
              assign<&ast::AvailabilityClause::version>(ref<VERSION>)) |
             flag<&ast::AvailabilityClause::wildcard>(lit<"*">)> {};
  struct AvailableExpression
      : rule<kw<"@available"> + lit<"("> +
             some(append<&ast::AvailableExpr::clauses>(ref<AvailabilityClause>),
                  lit<",">) +
             lit<")">> {};

  struct PrimaryExpression
      : rule<(lit<"("> + ref<Expression> + lit<")">) |
             create<ast::StmtExpr>(
                 option(K_extension) + lit<"("> +
                 assign<&ast::StmtExpr::body>(ref<CompoundStatement>) +
                 lit<")">) |
             ref<GenericSelection> | ref<NumberLiteral> |
             ref<CharacterLiteral> | ref<StringLiteral> | ref<KeywordLiteral> |
             ref<MessageExpression> | ref<SelectorExpression> |
             ref<ProtocolExpression> | ref<EncodeExpression> |
             ref<DictionaryLiteralExpression> | ref<ArrayLiteralExpression> |
             ref<BoxedExpression> | ref<BlockExpression> | ref<SelfIdent> |
             ref<SuperIdent> | ref<AvailableExpression> | ref<Identifier>> {};

  struct ArgumentExpression
      : rule<ref<AssignmentExpression> |
             create<ast::ArgumentTypeExpr>(
                 assign<&ast::ArgumentTypeExpr::type>(ref<TypeSpecifier>))> {};

  struct PostfixExpression
      : rule<(ref<PrimaryExpression> |
              create<ast::CompoundLiteralExpr>(
                  option(K_extension) + lit<"("> +
                  assign<&ast::CompoundLiteralExpr::type>(ref<TypeName>) +
                  lit<")"> +
                  assign<&ast::CompoundLiteralExpr::init>(ref<BracedInit>))) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      option(some(
                          append<&ast::CallExpr::args>(ref<ArgumentExpression>),
                          lit<",">)) +
                      lit<")">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      flag<&ast::MemberExpr::arrow>(lit<"->">) +
                      assign<&ast::MemberExpr::field>(NameText)) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> + assign<&ast::MemberExpr::field>(NameText)) |
                  nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                      assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {
  };

  static constexpr auto OpUnary =
      lit<"&"> | lit<"*"> | lit<"+"> | lit<"-"> | lit<"~"> | lit<"!">;

  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>)) |
             create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(OpUnary) +
                 assign<&ast::PrefixExpr::operand>(ref<CastExpression>)) |
             create<ast::SizeofTypeExpr>(
                 (kw<"sizeof"> | K_align_of) + lit<"("> +
                 assign<&ast::SizeofTypeExpr::type>(ref<TypeName>) + lit<")">) |
             create<ast::SizeofExpr>(
                 kw<"sizeof"> +
                 assign<&ast::SizeofExpr::operand>(ref<UnaryExpression>)) |
             create<ast::LabelAddrExpr>(
                 lit<"&&"> +
                 assign<&ast::LabelAddrExpr::label>(ref<IDENTIFIER>)) |
             ref<PostfixExpression>> {};

  struct CastExpression
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeName>) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<CastExpression>)) |
             ref<UnaryExpression>> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;

  static constexpr auto OpShift = lit<"<<"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;

  struct BinaryExpression
      : infix<CastExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >> left(OpRel) >>
               left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >>
               left(OpBitOr) >> left(OpLogAnd) >> left(OpLogOr))> {};

  struct ConditionalExpression
      : rule<
            create<ast::CondExpr>(
                assign<&ast::CondExpr::cond>(ref<BinaryExpression>) + lit<"?"> +
                option(assign<&ast::CondExpr::thenValue>(ref<Expression>)) +
                lit<":"> +
                assign<&ast::CondExpr::elseValue>(ref<ConditionalExpression>)) |
            ref<BinaryExpression>> {};

  static constexpr auto OpAssign =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | lit<"=">;

  struct AssignmentExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<UnaryExpression>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<AssignmentExpression>)) |
             ref<ConditionalExpression>> {};

  struct Expression
      : infix<AssignmentExpression, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right, left(lit<",">)> {};

  struct ConstantExpression : rule<ref<ConditionalExpression>> {};

  struct Designator
      : rule<create<ast::FieldDesignator>(
                 lit<"."> + assign<&ast::FieldDesignator::name>(NameText)) |
             create<ast::IndexDesignator>(
                 lit<"["> +
                 assign<&ast::IndexDesignator::index>(ref<ConstantExpression>) +
                 option(lit<"..."> + assign<&ast::IndexDesignator::rangeEnd>(
                                         ref<ConstantExpression>)) +
                 lit<"]">)> {};
  struct InitializerItem
      : rule<create<ast::DesignatedInit>(
                 some(append<&ast::DesignatedInit::designators>(
                     ref<Designator>)) +
                 lit<"="> +
                 assign<&ast::DesignatedInit::value>(ref<Initializer>)) |
             ref<Initializer>> {};
  struct BracedInit
      : rule<create<ast::ListInit>(
            lit<"{"> +
            option(some(append<&ast::ListInit::items>(ref<InitializerItem>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};
  struct Initializer : rule<ref<BracedInit> |
                            create<ast::ExprInit>(assign<&ast::ExprInit::value>(
                                ref<AssignmentExpression>))> {};

  struct FieldDeclarator
      : rule<(assign<&ast::FieldDeclarator::declarator>(ref<Declarator>) +
              option(lit<":"> + assign<&ast::FieldDeclarator::bitWidth>(
                                    ref<ConstantExpression>))) |
             (lit<":"> + assign<&ast::FieldDeclarator::bitWidth>(
                             ref<ConstantExpression>))> {};

  struct FieldDeclaration
      : rule<assign<&ast::FieldDeclaration::specifiers>(
                 ref<DeclarationSpecifiers>) +
             option(some(append<&ast::FieldDeclaration::declarators>(
                             ref<FieldDeclarator>),
                         lit<",">)) +
             option(assign<&ast::FieldDeclaration::trailingMacro>(ref<Macro>)) +
             lit<";">> {};

  static constexpr auto AccessModifier =
      kw<"@private"> | kw<"@protected"> | kw<"@package"> | kw<"@public">;

  struct VisibilitySection
      : rule<assign<&ast::VisibilitySection::access>(AccessModifier) +
             many(append<&ast::VisibilitySection::fields>(
                 ref<FieldDeclaration>))> {};

  struct LeadingFieldSection
      : rule<some(
            append<&ast::VisibilitySection::fields>(ref<FieldDeclaration>))> {};

  struct InstanceVariables
      : rule<lit<"{"> +
             option(append<&ast::InstanceVariables::sections>(
                 ref<LeadingFieldSection>)) +
             many(append<&ast::InstanceVariables::sections>(
                 ref<VisibilitySection>)) +
             lit<"}">> {};

  static constexpr auto SwiftAliasExpression =
      append<&ast::SwiftNameMacro::aliasPath>(NameText) +
      many(lit<"."> + append<&ast::SwiftNameMacro::aliasPath>(NameText));

  static constexpr auto SwiftSelectorWord = NameText | lit<"_"> | kw<"for">;
  static constexpr auto SwiftSelectorExpression =
      assign<&ast::SwiftNameMacro::selectorName>(NameText) + lit<"("> +
      many(append<&ast::SwiftNameMacro::selectorParts>(SwiftSelectorWord) +
           lit<":">) +
      lit<")">;

  struct Version : rule<ref<VERSION>> {};

  struct ApiAvailableOsVersion
      : rule<assign<&ast::ApiVersion::platform>(NameText) + lit<"("> +
             assign<&ast::ApiVersion::version>(ref<Version>) + lit<")">> {};

  struct ClangAttributeArgument
      : rule<(assign<&ast::ClangAttrArg::assignName>(NameText) + lit<"="> +
              (assign<&ast::ClangAttrArg::assignVersion>(ref<Version>) |
               assign<&ast::ClangAttrArg::assignValue>(ref<StringLiteral>))) |
             assign<&ast::ClangAttrArg::value>(ref<StringLiteral>) |
             assign<&ast::ClangAttrArg::text>(NameText | ref<NUMBER>)> {};

  struct ClangAttribute : rule<assign<&ast::ClangAttr::name>(NameText) +
                               option(lit<"("> +
                                      some(append<&ast::ClangAttr::args>(
                                               ref<ClangAttributeArgument>),
                                           lit<",">) +
                                      lit<")">)> {};

  struct Macro
      : rule<create<ast::CallMacro>(assign<&ast::CallMacro::name>(NameText) +
                                    option(lit<"("> +
                                           some(append<&ast::CallMacro::args>(
                                                    ref<PrimaryExpression>),
                                                lit<",">) +
                                           lit<")">)) |
             create<ast::CallMacro>(
                 assign<&ast::CallMacro::name>(K_ns_unavailable)) |
             create<ast::SwiftNameMacro>(
                 K_ns_swift_name + lit<"("> +
                 (SwiftAliasExpression | SwiftSelectorExpression) + lit<")">) |
             create<ast::ApiAvailableMacro>(
                 K_api_available + lit<"("> +
                 some(append<&ast::ApiAvailableMacro::versions>(
                          ref<ApiAvailableOsVersion>),
                      lit<",">) +
                 lit<")">) |
             create<ast::ApiUnavailableMacro>(
                 K_api_unavailable + lit<"("> +
                 some(append<&ast::ApiUnavailableMacro::platforms>(NameText),
                      lit<",">) +
                 lit<")">) |
             create<ast::SwiftUnavailableMacro>(
                 K_ns_swift_unavailable + lit<"("> +
                 assign<&ast::SwiftUnavailableMacro::message>(
                     ref<StringLiteral>) +
                 lit<")">) |
             create<ast::AttributeMacro>(
                 kw<"__attribute__"> + lit<"("> + lit<"("> +
                 some(append<&ast::AttributeMacro::attrs>(ref<ClangAttribute>),
                      lit<",">) +
                 lit<")"> + lit<")">)> {};

  struct UnresolvedCallOrDeclaration
      : rule<option(AttributeSpecifierExpr<&ast::UnresolvedCallOrDeclaration::attrsBefore>) +
             assign<&ast::UnresolvedCallOrDeclaration::name>(NameText) +
             option(AttributeSpecifierExpr<&ast::UnresolvedCallOrDeclaration::attrsAfter>) +
             lit<"("> + assign<&ast::UnresolvedCallOrDeclaration::declarator>(ref<Declarator>) +
             lit<")"> +
             option(assign<&ast::UnresolvedCallOrDeclaration::trailingMacro>(ref<Macro>)) +
             lit<";">> {};

  struct InitDeclarator
      : rule<assign<&ast::InitDeclarator::declarator>(ref<Declarator>) +
             option(lit<"="> +
                    assign<&ast::InitDeclarator::init>(ref<Initializer>))> {};

  struct VarDeclaration
      : rule<assign<&ast::Declaration::specifiers>(ref<DeclarationSpecifiers>) +
             option(some(
                 append<&ast::Declaration::declarators>(ref<InitDeclarator>),
                 lit<",">)) +
             option(assign<&ast::Declaration::trailingMacro>(ref<Macro>)) +
             lit<";">> {};

  struct StaticAssertDeclaration
      : rule<(kw<"_Static_assert"> | kw<"static_assert">)+lit<"("> +
             assign<&ast::StaticAssert::condition>(ref<AssignmentExpression>) +
             option(lit<","> +
                    assign<&ast::StaticAssert::message>(ref<StringLiteral>)) +
             lit<")"> + lit<";">> {};
  struct Declaration : rule<ref<StaticAssertDeclaration> |
                            ref<UnresolvedCallOrDeclaration> | ref<VarDeclaration>> {};
  struct FunctionDefinition
      : rule<option(assign<&ast::FunctionDef::specifiers>(
                 ref<DeclarationSpecifiers>)) +
             assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
             many(append<&ast::FunctionDef::paramDeclarations>(
                 ref<VarDeclaration>)) +
             assign<&ast::FunctionDef::body>(ref<CompoundStatement>)> {};

  struct ExpressionStatement
      : rule<option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct BlockItem : rule<ref<Declaration> | ref<Statement>> {};

  struct CompoundStatement
      : rule<lit<"{"> +
             many(append<&ast::CompoundStmt::items>(ref<BlockItem>)) +
             lit<"}">> {};

  struct IfStatement
      : rule<kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
             lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct CaseStatement
      : rule<((kw<"case"> +
               assign<&ast::CaseStmt::value>(ref<ConstantExpression>) +
               option(lit<"..."> + assign<&ast::CaseStmt::rangeEnd>(
                                       ref<ConstantExpression>))) |
              flag<&ast::CaseStmt::isDefault>(kw<"default">)) +
             lit<":"> + option(assign<&ast::CaseStmt::body>(ref<BlockItem>))> {
  };

  struct SwitchStatement
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
             assign<&ast::SwitchStmt::body>(ref<Statement>)> {};

  struct WhileStatement
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoStatement
      : rule<kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  struct ForLoopInitializer
      : rule<create<ast::ForInit>(
            (assign<&ast::ForInit::specifiers>(ref<DeclarationSpecifiers>) +
             some(append<&ast::ForInit::declarators>(ref<InitDeclarator>),
                  lit<",">)) |
            assign<&ast::ForInit::expr>(ref<Expression>))> {};

  struct ForStatement
      : rule<kw<"for"> + lit<"("> +
             option(assign<&ast::ForStmt::init>(ref<ForLoopInitializer>)) +
             lit<";"> + option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
             lit<";"> + option(assign<&ast::ForStmt::step>(ref<Expression>)) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct ForInStatement
      : rule<kw<"for"> + lit<"("> +
             assign<&ast::ForInStmt::init>(ref<ForLoopInitializer>) + K_in +
             assign<&ast::ForInStmt::collection>(ref<Expression>) + lit<")"> +
             assign<&ast::ForInStmt::body>(ref<Statement>)> {};

  struct JumpStatement
      : rule<(create<ast::GotoStmt>(kw<"goto"> + assign<&ast::GotoStmt::target>(
                                                     ref<IDENTIFIER>)) |
              create<ast::ContinueStmt>(kw<"continue">) |
              create<ast::BreakStmt>(kw<"break">) |
              create<ast::ReturnStmt>(
                  kw<"return"> +
                  option(assign<&ast::ReturnStmt::value>(ref<Expression>)))) +
             lit<";">> {};

  struct LabeledStatement
      : rule<assign<&ast::Label::name>(NameText) + lit<":"> +
             option(assign<&ast::Label::body>(ref<BlockItem>))> {};

  struct ThrowStatement
      : rule<lit<"@throw"> +
             option(assign<&ast::ThrowStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct CatchStatement
      : rule<lit<"@"> + kw<"catch"> + lit<"("> +
             ((assign<&ast::CatchClause::specifiers>(
                   ref<DeclarationSpecifiers>) +
               assign<&ast::CatchClause::declarator>(ref<Declarator>)) |
              flag<&ast::CatchClause::isEllipsis>(lit<"...">)) +
             lit<")"> +
             assign<&ast::CatchClause::body>(ref<CompoundStatement>)> {};

  struct TryBlock
      : rule<lit<"@"> + kw<"try"> +
             assign<&ast::TryStmt::tryBlock>(ref<CompoundStatement>) +
             many(append<&ast::TryStmt::catches>(ref<CatchStatement>)) +
             option(
                 lit<"@"> + kw<"finally"> +
                 assign<&ast::TryStmt::finallyBlock>(ref<CompoundStatement>))> {
  };

  struct SynchronizedStatement
      : rule<lit<"@synchronized"> + lit<"("> +
             assign<&ast::SynchronizedStmt::lock>(ref<Expression>) + lit<")"> +
             assign<&ast::SynchronizedStmt::body>(ref<CompoundStatement>)> {};

  struct AutoreleaseStatement
      : rule<lit<"@autoreleasepool"> +
             assign<&ast::AutoreleaseStmt::body>(ref<CompoundStatement>)> {};

  struct Statement
      : rule<(ref<LabeledStatement> | ref<CaseStatement> |
              ref<CompoundStatement> | ref<IfStatement> | ref<SwitchStatement> |
              ref<WhileStatement> | ref<DoStatement> | ref<ForInStatement> |
              ref<ForStatement> | ref<SynchronizedStatement> |
              ref<AutoreleaseStatement> | ref<ThrowStatement> | ref<TryBlock> |
              ref<JumpStatement> | ref<ExpressionStatement>)> {};

  struct MethodType : rule<lit<"("> + ref<TypeName> + lit<")">> {};

  struct KeywordDeclarator
      : rule<option(assign<&ast::MethodParam::label>(Selector)) + lit<":"> +
             option(lit<"("> + assign<&ast::MethodParam::type>(ref<TypeName>) +
                    lit<")">) +
             option(assign<&ast::MethodParam::arc>(ArcBehaviour)) +
             many(AttributeSpecifierExpr<&ast::MethodParam::attributes>) +
             assign<&ast::MethodParam::name>(NameText)> {};

  static constexpr auto MethodDeclBody =
      option(lit<"("> + assign<&ast::MethodDecl::returnType>(ref<TypeName>) +
             lit<")">) +
      (assign<&ast::MethodDecl::unarySelector>(Selector) |
       some(append<&ast::MethodDecl::params>(ref<KeywordDeclarator>))) +
      many(lit<","> + append<&ast::MethodDecl::extraParams>(ref<Param>)) +
      option(lit<","> + flag<&ast::MethodDecl::variadic>(lit<"...">)) +
      many(append<&ast::MethodDecl::macros>(ref<Macro>)) + lit<";">;

  static constexpr auto MethodDefBody =
      option(lit<"("> + assign<&ast::MethodDef::returnType>(ref<TypeName>) +
             lit<")">) +
      (assign<&ast::MethodDef::unarySelector>(Selector) |
       some(append<&ast::MethodDef::params>(ref<KeywordDeclarator>))) +
      many(lit<","> + append<&ast::MethodDef::extraParams>(ref<Param>)) +
      option(lit<","> + flag<&ast::MethodDef::variadic>(lit<"...">)) +
      many(append<&ast::MethodDef::macros>(ref<Macro>)) + option(lit<";">) +
      assign<&ast::MethodDef::body>(ref<CompoundStatement>);

  static constexpr auto PropertyAttributeWord =
      K_atomic | K_nonatomic | K_strong | K_weak | K_retain | K_assign |
      K_unsafe_unretained | K_copy | K_readonly | K_readwrite | K_direct;

  struct PropertyAttribute
      : rule<create<ast::PropertyAttr>(
                 assign<&ast::PropertyAttr::keyword>(PropertyAttributeWord)) |
             create<ast::PropertyAttr>(
                 K_getter + lit<"="> +
                 assign<&ast::PropertyAttr::getterName>(MessageSelectorWord)) |
             create<ast::PropertyAttr>(
                 K_setter + lit<"="> +
                 assign<&ast::PropertyAttr::setterName>(MessageSelectorWord) +
                 lit<":">) |
             create<ast::PropertyAttr>(
                 assign<&ast::PropertyAttr::keyword>(NullabilitySpec)) |
             create<ast::PropertyAttr>(
                 assign<&ast::PropertyAttr::keyword>(NameText))> {};

  struct PropertyDeclarationBody
      : rule<kw<"@property"> +
             option(
                 lit<"("> +
                 some(append<&ast::PropertyDecl::attrs>(ref<PropertyAttribute>),
                      lit<",">) +
                 lit<")">) +
             option(
                 (flag<&ast::PropertyDecl::ibOutletCollection>(
                      K_ib_outlet_collection) +
                  lit<"("> +
                  assign<&ast::PropertyDecl::ibOutletCollectionType>(NameText) +
                  lit<")">) |
                 flag<&ast::PropertyDecl::ibOutlet>(K_ib_outlet)) +
             option(flag<&ast::PropertyDecl::ibInspectable>(
                 K_ib_inspectable)) +
             assign<&ast::PropertyDecl::field>(ref<FieldDeclaration>)> {};

  struct SynthesizeItem
      : rule<assign<&ast::SynthesizeItem::name>(NameText) +
             option(lit<"="> +
                    assign<&ast::SynthesizeItem::backing>(NameText))> {};

  struct PropertyImplementationBody
      : rule<((kw<"@synthesize"> +
               some(append<&ast::PropertyImpl::items>(ref<SynthesizeItem>),
                    lit<",">)) |
              (flag<&ast::PropertyImpl::dynamic>(kw<"@dynamic">) +
               some(append<&ast::PropertyImpl::items>(ref<SynthesizeItem>),
                    lit<",">))) +
             lit<";">> {};

  struct ClassInterfaceMember
      : rule<create<ast::MethodDecl>(
                 flag<&ast::MethodDecl::isClassMethod>(lit<"+">) +
                 MethodDeclBody) |
             create<ast::MethodDecl>(lit<"-"> + MethodDeclBody) |
             ref<PropertyDeclarationBody>

             | ref<Declaration>> {};

  template <auto Member>
  static constexpr auto InterfaceMemberOrSemi =
      append<Member>(ref<ClassInterfaceMember>) | lit<";">;

  struct ClassInterface
      : rule<
            many(AttributeSpecifierExpr<&ast::ClassInterface::attributes>) +
            option(flag<&ast::ClassInterface::ibDesignable>(
                K_ib_designable)) +
            kw<"@interface"> +
            assign<&ast::ClassInterface::className>(ref<GenericTypeSpecifier>) +
            option(lit<":"> + assign<&ast::ClassInterface::superclassName>(
                                  ref<IDENTIFIER>)) +
            many(lit<"<"> +
                 append<&ast::ClassInterface::clauses>(ref<AngleClause>) +
                 lit<">">) +
            option(
                assign<&ast::ClassInterface::ivars>(ref<InstanceVariables>)) +
            option(some(InterfaceMemberOrSemi<&ast::ClassInterface::members>)) +
            kw<"@end">> {};

  struct CategoryInterface
      : rule<many(AttributeSpecifierExpr<&ast::CategoryInterface::attributes>) +
             kw<"@interface"> +
             assign<&ast::CategoryInterface::className>(
                 ref<GenericTypeSpecifier>) +
             lit<"("> +
             option(assign<&ast::CategoryInterface::categoryName>(
                 ref<IDENTIFIER>)) +
             lit<")"> +
             option(lit<"<"> +
                    assign<&ast::CategoryInterface::clause>(
                        create<ast::AngleClause>(some(
                            append<&ast::AngleClause::args>(ref<TypeArgument>),
                            lit<",">))) +
                    lit<">">) +
             option(assign<&ast::CategoryInterface::ivars>(
                 ref<InstanceVariables>)) +
             option(some(
                 InterfaceMemberOrSemi<&ast::CategoryInterface::members>)) +
             kw<"@end">> {};

  struct ClassImplementationMember
      : rule<ref<FunctionDefinition> |
             create<ast::MethodDef>(
                 flag<&ast::MethodDef::isClassMethod>(lit<"+">) +
                 MethodDefBody) |
             create<ast::MethodDef>(lit<"-"> + MethodDefBody) |
             ref<PropertyImplementationBody> | ref<Declaration>> {};

  template <auto Member>
  static constexpr auto ImplementationMemberOrSemi =
      append<Member>(ref<ClassImplementationMember>) | lit<";">;

  struct ClassImplementation
      : rule<
            many(
                AttributeSpecifierExpr<&ast::ClassImplementation::attributes>) +
            kw<"@implementation"> +
            assign<&ast::ClassImplementation::className>(
                ref<GenericTypeSpecifier>) +
            option(lit<":"> + assign<&ast::ClassImplementation::superclassName>(
                                  ref<IDENTIFIER>)) +
            option(assign<&ast::ClassImplementation::ivars>(
                ref<InstanceVariables>)) +
            option(some(ImplementationMemberOrSemi<
                        &ast::ClassImplementation::members>)) +
            kw<"@end">> {};

  struct CategoryImplementation
      : rule<many(AttributeSpecifierExpr<
                  &ast::CategoryImplementation::attributes>) +
             kw<"@implementation"> +
             assign<&ast::CategoryImplementation::className>(
                 ref<GenericTypeSpecifier>) +
             lit<"("> +
             assign<&ast::CategoryImplementation::categoryName>(
                 ref<IDENTIFIER>) +
             lit<")"> +
             option(some(ImplementationMemberOrSemi<
                         &ast::CategoryImplementation::members>)) +
             kw<"@end">> {};

  struct ProtocolSection
      : rule<create<ast::ProtocolSection>(
                 assign<&ast::ProtocolSection::modifier>(kw<"@required">) +
                 many(InterfaceMemberOrSemi<&ast::ProtocolSection::members>)) |
             create<ast::ProtocolSection>(
                 assign<&ast::ProtocolSection::modifier>(kw<"@optional">) +
                 many(InterfaceMemberOrSemi<&ast::ProtocolSection::members>))> {
  };

  struct LeadingProtocolSection
      : rule<create<ast::ProtocolSection>(
            some(InterfaceMemberOrSemi<&ast::ProtocolSection::members>))> {};

  struct ProtocolDeclaration
      : rule<many(AttributeSpecifierExpr<&ast::ProtocolDecl::attributes>) +
             option(assign<&ast::ProtocolDecl::macro>(ref<Macro>)) +
             kw<"@protocol"> +
             option(assign<&ast::ProtocolDecl::variance>(K_covariant |
                                                         K_contravariant)) +
             assign<&ast::ProtocolDecl::name>(NameText) +
             option(lit<"<"> +
                    assign<&ast::ProtocolDecl::clause>(create<ast::AngleClause>(
                        some(append<&ast::AngleClause::args>(ref<TypeArgument>),
                             lit<",">))) +
                    lit<">">) +
             option(append<&ast::ProtocolDecl::sections>(
                 ref<LeadingProtocolSection>)) +
             many(append<&ast::ProtocolDecl::sections>(ref<ProtocolSection>)) +
             kw<"@end">> {};

  struct ProtocolDeclarationList
      : rule<kw<"@protocol"> +
             some(
                 append<&ast::ProtocolForwardDecl::protocols>(ref<ProtocolRef>),
                 lit<",">) +
             lit<";">> {};

  struct ClassDeclarationList
      : rule<kw<"@class"> +
             some(append<&ast::ClassForwardDecl::classes>(
                      ref<GenericTypeSpecifier>),
                  lit<",">) +
             lit<";">> {};

  struct ImportDeclaration
      : rule<lit<"@import"> +
             some(append<&ast::ImportDecl::path>(ref<IDENTIFIER>), lit<".">) +
             lit<";">> {};

  struct CompatibilityAliasDeclaration
      : rule<kw<"@compatibility_alias"> +
             assign<&ast::CompatibilityAlias::name>(NameText) +
             assign<&ast::CompatibilityAlias::original>(NameText) + lit<";">> {
  };

  struct TopLevelDeclaration
      : rule<ref<CompatibilityAliasDeclaration> | ref<ImportDeclaration>

             | ref<Declaration> | ref<ClassInterface> |
             ref<ClassImplementation> | ref<CategoryInterface> |
             ref<CategoryImplementation> | ref<ProtocolDeclaration> |
             ref<ProtocolDeclarationList> | ref<ClassDeclarationList> |
             ref<FunctionDefinition>> {};

  struct TranslationUnit
      : rule<create<ast::TranslationUnit> +
             many(append<&ast::TranslationUnit::items>(
                      ref<TopLevelDeclaration>) |
                  append<&ast::TranslationUnit::items>(
                      create<ast::AnnotationMarker>(
                          assign<&ast::AnnotationMarker::text>(
                              ref<NS_ASSUME_NONNULL_BEGIN_T> |
                              ref<NS_ASSUME_NONNULL_END_T>))) |
                  lit<";">)> {};

  using entry = TranslationUnit;
  using ignore = list<WS, DIRECTIVE>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace objc
