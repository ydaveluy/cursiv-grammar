#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "webidl/core/ast.hpp"

namespace webidl {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
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
  struct ID : terminal<as<unescape>(Identifier)> {};

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
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  struct ExtendedAttribute;
  struct Argument;
  struct UnionType;
  struct Type;

  static constexpr auto ArgumentName = ref<ID>;
  static constexpr auto AttributeName = ref<ID>;
  static constexpr auto OperationName = ref<ID>;


  static constexpr auto PrimitiveKw =
      as<ast::Primitive::unsigned_long_long>(kw<"unsigned"> + kw<"long"> + kw<"long">) | as<ast::Primitive::unsigned_long>(kw<"unsigned"> + kw<"long">) | as<ast::Primitive::unsigned_short>(kw<"unsigned"> + kw<"short">) | as<ast::Primitive::long_long>(kw<"long"> + kw<"long">) |
      as<ast::Primitive::long_>(kw<"long">) | as<ast::Primitive::short_>(kw<"short">) | as<ast::Primitive::unrestricted_float>(kw<"unrestricted"> + kw<"float">) |
      as<ast::Primitive::unrestricted_double>(kw<"unrestricted"> + kw<"double">) | as<ast::Primitive::float_>(kw<"float">) | as<ast::Primitive::double_>(kw<"double">) |
      as<ast::Primitive::undefined_>(kw<"undefined">) | as<ast::Primitive::void_>(kw<"void">) | as<ast::Primitive::boolean_>(kw<"boolean">) | as<ast::Primitive::byte_>(kw<"byte">) |
      as<ast::Primitive::octet_>(kw<"octet">) | as<ast::Primitive::bigint_>(kw<"bigint">);

  static constexpr auto BufferTypeKw =
      kw<"ArrayBuffer"> | kw<"SharedArrayBuffer"> | kw<"DataView"> | kw<"Int8Array"> |
      kw<"Int16Array"> | kw<"Int32Array"> | kw<"Uint8Array"> |
      kw<"Uint16Array"> | kw<"Uint32Array"> | kw<"Uint8ClampedArray"> |
      kw<"BigInt64Array"> | kw<"BigUint64Array"> | kw<"Float16Array"> |
      kw<"Float32Array"> | kw<"Float64Array">;

  static constexpr auto Nullable =
      option(flag<&ast::Type::nullable>(lit<"?">));
  template <auto Member>
  static constexpr auto Attributes = option(lit<"["> +
      some(append<Member>(ref<ExtendedAttribute>), lit<",">) + lit<"]">);
  static constexpr auto TypeAttrs = Attributes<&ast::Type::attrs>;

  struct PrimitiveType
      : rule<create<ast::PrimitiveType>(
            TypeAttrs + assign<&ast::PrimitiveType::kind>(PrimitiveKw) +
            Nullable)> {};

  struct StringType
      : rule<create<ast::StringType>(
            TypeAttrs +
            assign<&ast::StringType::name>(kw<"ByteString"> |
                                           kw<"DOMString"> |
                                           kw<"USVString">) +
            Nullable)> {};

  struct NamedType
      : rule<create<ast::NamedType>(
            TypeAttrs + assign<&ast::NamedType::name>(ref<ID>) + Nullable)> {
  };

  struct AnyType : rule<create<ast::AnyType>(TypeAttrs + kw<"any"> + Nullable)> {};

  struct ObjectType
      : rule<create<ast::ObjectType>(TypeAttrs + kw<"object"> + Nullable)> {};

  struct SymbolType
      : rule<create<ast::SymbolType>(TypeAttrs + kw<"symbol"> + Nullable)> {};

  struct BufferType
      : rule<create<ast::BufferType>(
            TypeAttrs + assign<&ast::BufferType::name>(BufferTypeKw) +
            Nullable)> {};

  struct SequenceType
      : rule<create<ast::SequenceType>(
            TypeAttrs +
            (flag<&ast::SequenceType::async_>(kw<"async_sequence">) |
             kw<"sequence">) + lit<"<"> +
            assign<&ast::SequenceType::element>(ref<Type>) + lit<">"> +
            Nullable)> {};

  struct FrozenArrayType
      : rule<create<ast::FrozenArrayType>(
            TypeAttrs + kw<"FrozenArray"> + lit<"<"> +
            assign<&ast::FrozenArrayType::element>(ref<Type>) +
            lit<">"> + Nullable)> {};

  struct ObservableArrayType
      : rule<create<ast::ObservableArrayType>(
            TypeAttrs + kw<"ObservableArray"> + lit<"<"> +
            assign<&ast::ObservableArrayType::element>(ref<Type>) +
            lit<">"> + Nullable)> {};

  struct RecordType
      : rule<create<ast::RecordType>(
            TypeAttrs + kw<"record"> + lit<"<"> +
            assign<&ast::RecordType::keyType>(ref<StringType>) +
            lit<","> + assign<&ast::RecordType::valueType>(ref<Type>) +
            lit<">"> + Nullable)> {};

  struct PromiseType
      : rule<create<ast::PromiseType>(
            TypeAttrs + kw<"Promise"> + lit<"<"> +
            assign<&ast::PromiseType::value>(ref<Type>) + lit<">"> + Nullable)> {};

  static constexpr auto DistinguishableTypeChoice =
      ref<PrimitiveType> | ref<StringType> | ref<NamedType> |
      ref<SequenceType> | ref<ObjectType> | ref<SymbolType> |
      ref<BufferType> | ref<FrozenArrayType> |
      ref<ObservableArrayType> | ref<RecordType>;

  struct UnionType
      : rule<create<ast::UnionType>(
            TypeAttrs + lit<"("> +
            many(append<&ast::UnionType::members>(ref<Type>), kw<"or">) +
            lit<")"> + Nullable)> {};

  struct Type
      : rule<DistinguishableTypeChoice | ref<UnionType> |
             ref<AnyType> | ref<PromiseType>> {};

  struct BoolValue
      : rule<create<ast::BoolValue>(
            flag<&ast::BoolValue::value>(kw<"true">) | kw<"false">)> {};
  struct IntValue
      : rule<assign<&ast::IntValue::text>(ref<INTEGER>)> {};
  struct FloatValue
      : rule<create<ast::FloatValue>(assign<&ast::FloatValue::text>(
            ref<DECIMAL> | kw<"-Infinity"> | kw<"Infinity"> | kw<"NaN">))> {};
  struct StringValue
      : rule<assign<&ast::StringValue::text>(ref<STRING>)> {};
  struct NullValue : rule<create<ast::NullValue> + kw<"null">> {};
  struct EmptySequenceValue
      : rule<create<ast::EmptySequenceValue> + lit<"["> + lit<"]">> {};
  struct EmptyDictionaryValue
      : rule<create<ast::EmptyDictionaryValue> + lit<"{"> + lit<"}">> {};

  static constexpr auto ConstValueChoice =
      ref<BoolValue> | ref<FloatValue> | ref<IntValue>;
  static constexpr auto DefaultValueChoice =
      ConstValueChoice | ref<StringValue> | ref<EmptySequenceValue> |
      ref<EmptyDictionaryValue> | ref<NullValue>;

  static constexpr auto ArgumentAttrs = Attributes<&ast::Argument::attrs>;

  struct Argument
      : rule<create<ast::Argument>(
            ArgumentAttrs +
            option(flag<&ast::Argument::optional_>(kw<"optional">)) +
            assign<&ast::Argument::type>(ref<Type>) +
            option(flag<&ast::Argument::variadic>(lit<"...">)) +
            assign<&ast::Argument::name>(ArgumentName) +
            option(lit<"="> +
                   assign<&ast::Argument::default_>(DefaultValueChoice)))> {};

  struct IdentifierValue
      : rule<assign<&ast::IdentifierValue::text>(ref<ID>)> {};
  struct AttributeDecimal
      : rule<assign<&ast::FloatValue::text>(ref<DECIMAL>)> {};
  static constexpr auto ExtendedValueChoice =
      ref<IdentifierValue> | ref<StringValue> |
      ref<AttributeDecimal> | ref<IntValue>;

  struct ExtendedAttribute
      : rule<create<ast::ExtendedAttribute>(
            assign<&ast::ExtendedAttribute::name>(ref<RAW_ID>) +
            option(lit<"="> +
                   (flag<&ast::ExtendedAttribute::wildcard>(lit<"*">) |
                    assign<&ast::ExtendedAttribute::rhs>(ExtendedValueChoice) |
                    (flag<&ast::ExtendedAttribute::hasList>(lit<"(">) +
                     many(append<&ast::ExtendedAttribute::values>(ExtendedValueChoice), lit<",">) +
                     lit<")">))) +
            option(flag<&ast::ExtendedAttribute::hasArguments>(lit<"(">) +
                   many(append<&ast::ExtendedAttribute::args>(ref<Argument>), lit<",">) +
                   lit<")">))> {};

  static constexpr auto MemberAttrs = Attributes<&ast::Member::attrs>;

  static constexpr auto AttributeRestBody =
      kw<"attribute"> + assign<&ast::Attribute::type>(ref<Type>) +
      assign<&ast::Attribute::name>(AttributeName) + lit<";">;

  static constexpr auto RegularOperationBody =
      assign<&ast::Operation::returnType>(ref<Type>) +
      option(assign<&ast::Operation::name>(OperationName)) + lit<"("> +
      many(append<&ast::Operation::params>(ref<Argument>), lit<",">) +
      lit<")"> + lit<";">;

  static constexpr auto MaplikeRestBody =
      kw<"maplike"> + lit<"<"> +
      assign<&ast::Maplike::keyType>(ref<Type>) + lit<","> +
      assign<&ast::Maplike::valueType>(ref<Type>) + lit<">"> + lit<";">;

  static constexpr auto SetlikeRestBody =
      kw<"setlike"> + lit<"<"> + assign<&ast::Setlike::type>(ref<Type>) +
      lit<">"> + lit<";">;

  static constexpr auto SpecialKw =
      as<ast::Special::getter_>(kw<"getter">) | as<ast::Special::setter_>(kw<"setter">) | as<ast::Special::deleter_>(kw<"deleter">);

  struct ConstMember
      : rule<create<ast::ConstMember>(
            MemberAttrs + kw<"const"> +
            assign<&ast::ConstMember::type>(ref<Type>) +
            assign<&ast::ConstMember::name>(ref<ID>) + lit<"="> +
            assign<&ast::ConstMember::value>(ConstValueChoice) +
            lit<";">)> {};

  struct Operation
      : rule<create<ast::Operation>(
            MemberAttrs +
            option(assign<&ast::Operation::special>(SpecialKw) |
                   flag<&ast::Operation::static_>(kw<"static">) |
                   flag<&ast::Operation::stringifier>(kw<"stringifier">)) +
            RegularOperationBody)> {};

  struct Attribute
      : rule<create<ast::Attribute>(
            MemberAttrs +
            option(flag<&ast::Attribute::inherit_>(kw<"inherit">) |
                   flag<&ast::Attribute::static_>(kw<"static">) |
                   flag<&ast::Attribute::stringifier>(kw<"stringifier">)) +
            option(flag<&ast::Attribute::readonly_>(kw<"readonly">)) +
            AttributeRestBody)> {};

  struct StringifierBare
      : rule<create<ast::Stringifier>(MemberAttrs + kw<"stringifier"> + lit<";">)> {};

  struct Maplike
      : rule<create<ast::Maplike>(MemberAttrs +
            option(flag<&ast::Maplike::readonly_>(kw<"readonly">)) +
            MaplikeRestBody)> {};

  struct Setlike
      : rule<create<ast::Setlike>(MemberAttrs +
            option(flag<&ast::Setlike::readonly_>(kw<"readonly">)) +
            SetlikeRestBody)> {};

  struct Constructor
      : rule<create<ast::Constructor>(
            MemberAttrs + kw<"constructor"> + lit<"("> +
            many(append<&ast::Constructor::params>(ref<Argument>),
                 lit<",">) +
            lit<")"> + lit<";">)> {};

  struct Iterable
      : rule<create<ast::Iterable>(
            MemberAttrs + kw<"iterable"> + lit<"<"> +
            assign<&ast::Iterable::keyType>(ref<Type>) +
            option(lit<","> +
                   assign<&ast::Iterable::valueType>(ref<Type>)) +
            lit<">"> + lit<";">)> {};

  struct AsyncIterable
      : rule<create<ast::AsyncIterable>(
            MemberAttrs +
            ((flag<&ast::AsyncIterable::legacySyntax>(kw<"async">) +
              kw<"iterable">) | kw<"async_iterable">) + lit<"<"> +
            assign<&ast::AsyncIterable::keyType>(ref<Type>) +
            option(lit<","> +
                   assign<&ast::AsyncIterable::valueType>(ref<Type>)) +
            lit<">"> +
            option(lit<"("> +
                   many(append<&ast::AsyncIterable::params>(ref<Argument>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  static constexpr auto MemberChoice =
      ref<ConstMember> | ref<Constructor> | ref<Operation> |
      ref<StringifierBare> | ref<Attribute> | ref<Iterable> |
      ref<AsyncIterable> | ref<Maplike> | ref<Setlike>;

  static constexpr auto DictMemberAttrs = Attributes<&ast::DictionaryMember::attrs>;

  struct DictionaryMember
      : rule<create<ast::DictionaryMember>(
            DictMemberAttrs +
            option(flag<&ast::DictionaryMember::required_>(kw<"required">)) +
            assign<&ast::DictionaryMember::type>(ref<Type>) +
            assign<&ast::DictionaryMember::name>(ref<ID>) +
            option(lit<"="> + assign<&ast::DictionaryMember::default_>(DefaultValueChoice)) +
            lit<";">)> {};

  static constexpr auto DefinitionAttrs = Attributes<&ast::Definition::attrs>;

  struct Interface
      : rule<create<ast::Interface>(
            DefinitionAttrs +
            option(flag<&ast::Interface::partial_>(kw<"partial">)) +
            kw<"interface"> + assign<&ast::Interface::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Interface::base>(ref<ID>)) +
            lit<"{"> +
            many(append<&ast::Interface::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct Mixin
      : rule<create<ast::Mixin>(
            DefinitionAttrs +
            option(flag<&ast::Mixin::partial_>(kw<"partial">)) +
            kw<"interface"> + kw<"mixin"> +
            assign<&ast::Mixin::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::Mixin::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct CallbackInterface
      : rule<create<ast::CallbackInterface>(
            DefinitionAttrs + kw<"callback"> + kw<"interface"> +
            assign<&ast::CallbackInterface::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::CallbackInterface::members>(
                MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct Callback
      : rule<create<ast::Callback>(
            DefinitionAttrs + kw<"callback"> +
            assign<&ast::Callback::name>(ref<ID>) + lit<"="> +
            assign<&ast::Callback::returnType>(ref<Type>) + lit<"("> +
            many(append<&ast::Callback::params>(ref<Argument>),
                 lit<",">) +
            lit<")"> + lit<";">)> {};

  struct Namespace
      : rule<create<ast::Namespace>(
            DefinitionAttrs +
            option(flag<&ast::Namespace::partial_>(kw<"partial">)) +
            kw<"namespace"> + assign<&ast::Namespace::name>(ref<ID>) +
            lit<"{"> +
            many(append<&ast::Namespace::members>(MemberChoice)) +
            lit<"}"> + lit<";">)> {};

  struct Dictionary
      : rule<create<ast::Dictionary>(
            DefinitionAttrs +
            option(flag<&ast::Dictionary::partial_>(kw<"partial">)) +
            kw<"dictionary"> + assign<&ast::Dictionary::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Dictionary::base>(ref<ID>)) +
            lit<"{"> +
            many(append<&ast::Dictionary::members>(ref<DictionaryMember>)) +
            lit<"}"> + lit<";">)> {};

  struct Enum
      : rule<create<ast::Enum>(
            DefinitionAttrs + kw<"enum"> +
            assign<&ast::Enum::name>(ref<ID>) + lit<"{"> +
            some(append<&ast::Enum::values>(ref<STRING>), lit<",">) +
            option(lit<",">) + lit<"}"> + lit<";">)> {};

  struct Typedef
      : rule<create<ast::Typedef>(
            DefinitionAttrs + kw<"typedef"> +
            assign<&ast::Typedef::type>(ref<Type>) +
            assign<&ast::Typedef::name>(ref<ID>) + lit<";">)> {};

  struct IncludesStatement
      : rule<create<ast::IncludesStatement>(
            DefinitionAttrs +
            assign<&ast::IncludesStatement::target>(ref<ID>) +
            kw<"includes"> +
            assign<&ast::IncludesStatement::mixin>(ref<ID>) + lit<";">)> {};

  static constexpr auto DefinitionChoice =
      ref<Interface> | ref<Mixin> | ref<CallbackInterface> |
      ref<Callback> | ref<Namespace> | ref<Dictionary> |
      ref<Enum> | ref<Typedef> | ref<IncludesStatement>;

  struct Document
      : rule<create<ast::Document>(
            many(append<&ast::Document::definitions>(DefinitionChoice)))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace webidl
