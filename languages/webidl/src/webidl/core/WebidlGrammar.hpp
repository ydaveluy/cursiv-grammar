#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "webidl/core/ast.hpp"

namespace webidl {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;
inline constexpr auto unescape = [](std::string_view text) {
  return text.starts_with('_') ? text.substr(1) : text;
};

struct Webidl : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Za-z0-9_-">;
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n\u2028\u2029)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto Identifier =
      option(cr<"_-">) + cr<"A-Za-z_"> + many(cr<"0-9A-Za-z_-">);
  struct RAW_ID : terminal<Identifier> {};
  struct ID : terminal<Identifier, text_conv<unescape>> {};

  struct INTEGER
      : terminal<option(lit<"-">) +
                 ((lit<"0"> +
                   ((cr<"xX"> + some(cr<"0-9A-Fa-f">)) | many(cr<"0-7">))) |
                  (cr<"1-9"> + many(d)))> {};

  struct DECIMAL
      : terminal<option(lit<"-">) +
                 ((((some(d) + lit<"."> + many(d)) |
                    (many(d) + lit<"."> + some(d))) +
                   option(cr<"eE"> + option(cr<"+-">) + some(d))) |
                  (some(d) + cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  struct STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  struct ExtendedAttributeRule;
  struct ArgumentRule;
  struct UnionTypeRule;
  struct TypeRule;

  static constexpr auto ArgumentName = ref<ID>;
  static constexpr auto AttributeName = ref<ID>;
  static constexpr auto OperationName = ref<ID>;

  struct KShort : rule<kw<"short">, value<ast::Primitive::short_>> {};
  struct KLongLong
      : rule<kw<"long"> + kw<"long">, value<ast::Primitive::long_long>> {};
  struct KLong : rule<kw<"long">, value<ast::Primitive::long_>> {};
  struct KUShort : rule<kw<"unsigned"> + kw<"short">,
                        value<ast::Primitive::unsigned_short>> {};
  struct KULongLong
      : rule<kw<"unsigned"> + kw<"long"> + kw<"long">,
             value<ast::Primitive::unsigned_long_long>> {};
  struct KULong : rule<kw<"unsigned"> + kw<"long">,
                       value<ast::Primitive::unsigned_long>> {};
  struct KFloat : rule<kw<"float">, value<ast::Primitive::float_>> {};
  struct KUnrestrictedFloat
      : rule<kw<"unrestricted"> + kw<"float">,
             value<ast::Primitive::unrestricted_float>> {};
  struct KDouble : rule<kw<"double">, value<ast::Primitive::double_>> {};
  struct KUnrestrictedDouble
      : rule<kw<"unrestricted"> + kw<"double">,
             value<ast::Primitive::unrestricted_double>> {};
  struct KUndefined
      : rule<kw<"undefined">, value<ast::Primitive::undefined_>> {};
  struct KVoid : rule<kw<"void">, value<ast::Primitive::void_>> {};
  struct KBoolean : rule<kw<"boolean">, value<ast::Primitive::boolean_>> {};
  struct KByte : rule<kw<"byte">, value<ast::Primitive::byte_>> {};
  struct KOctet : rule<kw<"octet">, value<ast::Primitive::octet_>> {};
  struct KBigint : rule<kw<"bigint">, value<ast::Primitive::bigint_>> {};

  static constexpr auto PrimitiveKw =
      ref<KULongLong> | ref<KULong> | ref<KUShort> | ref<KLongLong> |
      ref<KLong> | ref<KShort> | ref<KUnrestrictedFloat> |
      ref<KUnrestrictedDouble> | ref<KFloat> | ref<KDouble> |
      ref<KUndefined> | ref<KVoid> | ref<KBoolean> | ref<KByte> |
      ref<KOctet> | ref<KBigint>;

  static constexpr auto BufferTypeKw =
      kw<"ArrayBuffer"> | kw<"SharedArrayBuffer"> | kw<"DataView"> | kw<"Int8Array"> |
      kw<"Int16Array"> | kw<"Int32Array"> | kw<"Uint8Array"> |
      kw<"Uint16Array"> | kw<"Uint32Array"> | kw<"Uint8ClampedArray"> |
      kw<"BigInt64Array"> | kw<"BigUint64Array"> | kw<"Float16Array"> |
      kw<"Float32Array"> | kw<"Float64Array">;

  static constexpr auto Nullable =
      option(flag<&ast::Type::nullable>(lit<"?">));
  static constexpr auto TypeAttrs =
      option(lit<"["> +
             append<&ast::Type::attrs>(ref<ExtendedAttributeRule>) +
             many(lit<","> +
                  append<&ast::Type::attrs>(ref<ExtendedAttributeRule>)) +
             lit<"]">);

  struct PrimitiveTypeRule
      : rule<create<ast::PrimitiveType>(
            TypeAttrs + assign<&ast::PrimitiveType::kind>(PrimitiveKw) +
            Nullable)> {};

  struct StringTypeRule
      : rule<create<ast::StringType>(
            TypeAttrs +
            assign<&ast::StringType::name>(kw<"ByteString"> |
                                           kw<"DOMString"> |
                                           kw<"USVString">) +
            Nullable)> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(
            TypeAttrs + assign<&ast::NamedType::name>(ref<ID>) + Nullable)> {
  };

  struct AnyTypeRule : rule<create<ast::AnyType>(TypeAttrs + kw<"any"> + Nullable)> {};

  struct ObjectTypeRule
      : rule<create<ast::ObjectType>(TypeAttrs + kw<"object"> + Nullable)> {};

  struct SymbolTypeRule
      : rule<create<ast::SymbolType>(TypeAttrs + kw<"symbol"> + Nullable)> {};

  struct BufferTypeRule
      : rule<create<ast::BufferType>(
            TypeAttrs + assign<&ast::BufferType::name>(BufferTypeKw) +
            Nullable)> {};

  struct SequenceTypeRule
      : rule<create<ast::SequenceType>(
            TypeAttrs +
            (flag<&ast::SequenceType::async_>(kw<"async_sequence">) |
             kw<"sequence">) + lit<"<"> +
            assign<&ast::SequenceType::element>(ref<TypeRule>) + lit<">"> +
            Nullable)> {};

  struct FrozenArrayTypeRule
      : rule<create<ast::FrozenArrayType>(
            TypeAttrs + kw<"FrozenArray"> + lit<"<"> +
            assign<&ast::FrozenArrayType::element>(ref<TypeRule>) +
            lit<">"> + Nullable)> {};

  struct ObservableArrayTypeRule
      : rule<create<ast::ObservableArrayType>(
            TypeAttrs + kw<"ObservableArray"> + lit<"<"> +
            assign<&ast::ObservableArrayType::element>(ref<TypeRule>) +
            lit<">"> + Nullable)> {};

  struct RecordTypeRule
      : rule<create<ast::RecordType>(
            TypeAttrs + kw<"record"> + lit<"<"> +
            assign<&ast::RecordType::keyType>(ref<StringTypeRule>) +
            lit<","> + assign<&ast::RecordType::valueType>(ref<TypeRule>) +
            lit<">"> + Nullable)> {};

  struct PromiseTypeRule
      : rule<create<ast::PromiseType>(
            TypeAttrs + kw<"Promise"> + lit<"<"> +
            assign<&ast::PromiseType::value>(ref<TypeRule>) + lit<">"> + Nullable)> {};

  static constexpr auto DistinguishableTypeChoice =
      ref<PrimitiveTypeRule> | ref<StringTypeRule> | ref<NamedTypeRule> |
      ref<SequenceTypeRule> | ref<ObjectTypeRule> | ref<SymbolTypeRule> |
      ref<BufferTypeRule> | ref<FrozenArrayTypeRule> |
      ref<ObservableArrayTypeRule> | ref<RecordTypeRule>;

  struct UnionTypeRule
      : rule<create<ast::UnionType>(
            TypeAttrs + lit<"("> +
            many(append<&ast::UnionType::members>(ref<TypeRule>), kw<"or">) +
            lit<")"> + Nullable)> {};

  struct TypeRule
      : rule<DistinguishableTypeChoice | ref<UnionTypeRule> |
             ref<AnyTypeRule> | ref<PromiseTypeRule>> {};

  struct BoolValueRule
      : rule<create<ast::BoolValue>(
            flag<&ast::BoolValue::value>(kw<"true">) | kw<"false">)> {};
  struct IntValueRule
      : rule<create<ast::IntValue>(
            assign<&ast::IntValue::text>(ref<INTEGER>))> {};
  struct FloatValueRule
      : rule<create<ast::FloatValue>(assign<&ast::FloatValue::text>(
            ref<DECIMAL> | kw<"-Infinity"> | kw<"Infinity"> | kw<"NaN">))> {};
  struct StringValueRule
      : rule<create<ast::StringValue>(
            assign<&ast::StringValue::text>(ref<STRING>))> {};
  struct NullValueRule : rule<create<ast::NullValue> + kw<"null">> {};
  struct EmptySequenceValueRule
      : rule<create<ast::EmptySequenceValue> + lit<"["> + lit<"]">> {};
  struct EmptyDictionaryValueRule
      : rule<create<ast::EmptyDictionaryValue> + lit<"{"> + lit<"}">> {};

  static constexpr auto ConstValueChoice =
      ref<BoolValueRule> | ref<FloatValueRule> | ref<IntValueRule>;
  static constexpr auto DefaultValueChoice =
      ConstValueChoice | ref<StringValueRule> | ref<EmptySequenceValueRule> |
      ref<EmptyDictionaryValueRule> | ref<NullValueRule>;

  static constexpr auto ArgumentAttrs =
      option(lit<"["> +
             append<&ast::Argument::attrs>(ref<ExtendedAttributeRule>) +
             many(lit<","> +
                  append<&ast::Argument::attrs>(ref<ExtendedAttributeRule>)) +
             lit<"]">);

  struct ArgumentRule
      : rule<create<ast::Argument>(
            ArgumentAttrs +
            option(flag<&ast::Argument::optional_>(kw<"optional">)) +
            assign<&ast::Argument::type>(ref<TypeRule>) +
            option(flag<&ast::Argument::variadic>(lit<"...">)) +
            assign<&ast::Argument::name>(ArgumentName) +
            option(lit<"="> +
                   assign<&ast::Argument::default_>(DefaultValueChoice)))> {};

  struct IdentifierValueRule
      : rule<assign<&ast::IdentifierValue::text>(ref<ID>)> {};
  struct AttributeDecimalRule
      : rule<assign<&ast::FloatValue::text>(ref<DECIMAL>)> {};
  static constexpr auto ExtendedValueChoice =
      ref<IdentifierValueRule> | ref<StringValueRule> |
      ref<AttributeDecimalRule> | ref<IntValueRule>;

  struct ExtendedAttributeRule
      : rule<create<ast::ExtendedAttribute>(
            assign<&ast::ExtendedAttribute::name>(ref<RAW_ID>) +
            option(lit<"="> +
                   (flag<&ast::ExtendedAttribute::wildcard>(lit<"*">) |
                    assign<&ast::ExtendedAttribute::rhs>(ExtendedValueChoice) |
                    (flag<&ast::ExtendedAttribute::hasList>(lit<"(">) +
                     many(append<&ast::ExtendedAttribute::values>(ExtendedValueChoice), lit<",">) +
                     lit<")">))) +
            option(flag<&ast::ExtendedAttribute::hasArguments>(lit<"(">) +
                   many(append<&ast::ExtendedAttribute::args>(ref<ArgumentRule>), lit<",">) +
                   lit<")">))> {};

  static constexpr auto MemberAttrs =
      option(lit<"["> +
             append<&ast::Member::attrs>(ref<ExtendedAttributeRule>) +
             many(lit<","> +
                  append<&ast::Member::attrs>(ref<ExtendedAttributeRule>)) +
             lit<"]">);

  static constexpr auto AttributeRestBody =
      kw<"attribute"> + assign<&ast::Attribute::type>(ref<TypeRule>) +
      assign<&ast::Attribute::name>(AttributeName) + lit<";">;

  static constexpr auto RegularOperationBody =
      assign<&ast::Operation::returnType>(ref<TypeRule>) +
      option(assign<&ast::Operation::name>(OperationName)) + lit<"("> +
      many(append<&ast::Operation::params>(ref<ArgumentRule>), lit<",">) +
      lit<")"> + lit<";">;

  static constexpr auto MaplikeRestBody =
      kw<"maplike"> + lit<"<"> +
      assign<&ast::Maplike::keyType>(ref<TypeRule>) + lit<","> +
      assign<&ast::Maplike::valueType>(ref<TypeRule>) + lit<">"> + lit<";">;

  static constexpr auto SetlikeRestBody =
      kw<"setlike"> + lit<"<"> + assign<&ast::Setlike::type>(ref<TypeRule>) +
      lit<">"> + lit<";">;

  struct KGetter : rule<kw<"getter">, value<ast::Special::getter_>> {};
  struct KSetter : rule<kw<"setter">, value<ast::Special::setter_>> {};
  struct KDeleter : rule<kw<"deleter">, value<ast::Special::deleter_>> {};
  static constexpr auto SpecialKw =
      ref<KGetter> | ref<KSetter> | ref<KDeleter>;

  struct ConstMemberRule
      : rule<create<ast::ConstMember>(
            MemberAttrs + kw<"const"> +
            assign<&ast::ConstMember::type>(ref<TypeRule>) +
            assign<&ast::ConstMember::name>(ref<ID>) + lit<"="> +
            assign<&ast::ConstMember::value>(ConstValueChoice) +
            lit<";">)> {};

  struct OperationRule
      : rule<create<ast::Operation>(
            MemberAttrs +
            option(assign<&ast::Operation::special>(SpecialKw) |
                   flag<&ast::Operation::static_>(kw<"static">) |
                   flag<&ast::Operation::stringifier>(kw<"stringifier">)) +
            RegularOperationBody)> {};

  struct AttributeRule
      : rule<create<ast::Attribute>(
            MemberAttrs +
            option(flag<&ast::Attribute::inherit_>(kw<"inherit">) |
                   flag<&ast::Attribute::static_>(kw<"static">) |
                   flag<&ast::Attribute::stringifier>(kw<"stringifier">)) +
            option(flag<&ast::Attribute::readonly_>(kw<"readonly">)) +
            AttributeRestBody)> {};

  struct StringifierBareRule
      : rule<create<ast::Stringifier>(MemberAttrs + kw<"stringifier"> + lit<";">)> {};

  struct MaplikeRule
      : rule<create<ast::Maplike>(MemberAttrs +
            option(flag<&ast::Maplike::readonly_>(kw<"readonly">)) +
            MaplikeRestBody)> {};

  struct SetlikeRule
      : rule<create<ast::Setlike>(MemberAttrs +
            option(flag<&ast::Setlike::readonly_>(kw<"readonly">)) +
            SetlikeRestBody)> {};

  struct ConstructorRule
      : rule<create<ast::Constructor>(
            MemberAttrs + kw<"constructor"> + lit<"("> +
            many(append<&ast::Constructor::params>(ref<ArgumentRule>),
                 lit<",">) +
            lit<")"> + lit<";">)> {};

  struct IterableRule
      : rule<create<ast::Iterable>(
            MemberAttrs + kw<"iterable"> + lit<"<"> +
            assign<&ast::Iterable::keyType>(ref<TypeRule>) +
            option(lit<","> +
                   assign<&ast::Iterable::valueType>(ref<TypeRule>)) +
            lit<">"> + lit<";">)> {};

  struct AsyncIterableRule
      : rule<create<ast::AsyncIterable>(
            MemberAttrs +
            ((flag<&ast::AsyncIterable::legacySyntax>(kw<"async">) +
              kw<"iterable">) | kw<"async_iterable">) + lit<"<"> +
            assign<&ast::AsyncIterable::keyType>(ref<TypeRule>) +
            option(lit<","> +
                   assign<&ast::AsyncIterable::valueType>(ref<TypeRule>)) +
            lit<">"> +
            option(lit<"("> +
                   many(append<&ast::AsyncIterable::params>(ref<ArgumentRule>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  static constexpr auto MemberChoice =
      ref<ConstMemberRule> | ref<ConstructorRule> | ref<OperationRule> |
      ref<StringifierBareRule> | ref<AttributeRule> | ref<IterableRule> |
      ref<AsyncIterableRule> | ref<MaplikeRule> | ref<SetlikeRule>;

  static constexpr auto DictMemberAttrs =
      option(lit<"["> +
             append<&ast::DictionaryMember::attrs>(ref<ExtendedAttributeRule>) +
             many(lit<","> + append<&ast::DictionaryMember::attrs>(
                                  ref<ExtendedAttributeRule>)) +
             lit<"]">);

  struct DictionaryMemberRule
      : rule<create<ast::DictionaryMember>(
            DictMemberAttrs +
            option(flag<&ast::DictionaryMember::required_>(kw<"required">)) +
            assign<&ast::DictionaryMember::type>(ref<TypeRule>) +
            assign<&ast::DictionaryMember::name>(ref<ID>) +
            option(lit<"="> + assign<&ast::DictionaryMember::default_>(DefaultValueChoice)) +
            lit<";">)> {};

  static constexpr auto DefinitionAttrs =
      option(lit<"["> +
             append<&ast::Definition::attrs>(ref<ExtendedAttributeRule>) +
             many(lit<","> + append<&ast::Definition::attrs>(
                                  ref<ExtendedAttributeRule>)) +
             lit<"]">);

  struct InterfaceRule
      : rule<create<ast::Interface>(
            DefinitionAttrs +
            option(flag<&ast::Interface::partial_>(kw<"partial">)) +
            kw<"interface"> + assign<&ast::Interface::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Interface::base>(ref<ID>)) +
            lit<"{"> +
            many(append<&ast::Interface::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct MixinRule
      : rule<create<ast::Mixin>(
            DefinitionAttrs +
            option(flag<&ast::Mixin::partial_>(kw<"partial">)) +
            kw<"interface"> + kw<"mixin"> +
            assign<&ast::Mixin::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::Mixin::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct CallbackInterfaceRule
      : rule<create<ast::CallbackInterface>(
            DefinitionAttrs + kw<"callback"> + kw<"interface"> +
            assign<&ast::CallbackInterface::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::CallbackInterface::members>(
                MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct CallbackRule
      : rule<create<ast::Callback>(
            DefinitionAttrs + kw<"callback"> +
            assign<&ast::Callback::name>(ref<ID>) + lit<"="> +
            assign<&ast::Callback::returnType>(ref<TypeRule>) + lit<"("> +
            many(append<&ast::Callback::params>(ref<ArgumentRule>),
                 lit<",">) +
            lit<")"> + lit<";">)> {};

  struct NamespaceRule
      : rule<create<ast::Namespace>(
            DefinitionAttrs +
            option(flag<&ast::Namespace::partial_>(kw<"partial">)) +
            kw<"namespace"> + assign<&ast::Namespace::name>(ref<ID>) +
            lit<"{"> +
            many(append<&ast::Namespace::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct DictionaryRule
      : rule<create<ast::Dictionary>(
            DefinitionAttrs +
            option(flag<&ast::Dictionary::partial_>(kw<"partial">)) +
            kw<"dictionary"> + assign<&ast::Dictionary::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Dictionary::base>(ref<ID>)) +
            lit<"{"> +
            many(append<&ast::Dictionary::members>(ref<DictionaryMemberRule>)) +
            lit<"}"> + lit<";">)> {};

  struct EnumRule
      : rule<create<ast::Enum>(
            DefinitionAttrs + kw<"enum"> +
            assign<&ast::Enum::name>(ref<ID>) + lit<"{"> +
            append<&ast::Enum::values>(ref<STRING>) +
            many(lit<","> + append<&ast::Enum::values>(ref<STRING>)) +
            option(lit<",">) + lit<"}"> + lit<";">)> {};

  struct TypedefRule
      : rule<create<ast::Typedef>(
            DefinitionAttrs + kw<"typedef"> +
            assign<&ast::Typedef::type>(ref<TypeRule>) +
            assign<&ast::Typedef::name>(ref<ID>) + lit<";">)> {};

  struct IncludesStatementRule
      : rule<create<ast::IncludesStatement>(
            DefinitionAttrs +
            assign<&ast::IncludesStatement::target>(ref<ID>) +
            kw<"includes"> +
            assign<&ast::IncludesStatement::mixin>(ref<ID>) + lit<";">)> {};

  static constexpr auto DefinitionChoice =
      ref<InterfaceRule> | ref<MixinRule> | ref<CallbackInterfaceRule> |
      ref<CallbackRule> | ref<NamespaceRule> | ref<DictionaryRule> |
      ref<EnumRule> | ref<TypedefRule> | ref<IncludesStatementRule>;

  struct DocumentRule
      : rule<create<ast::Document>(
            many(append<&ast::Document::definitions>(DefinitionChoice)))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace webidl
