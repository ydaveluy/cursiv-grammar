// The flatbuffers language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "flatbuffers/core/ast.hpp"

namespace flatbuffers {

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == std::string_view::npos) return t;
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\' || i + 1 == t.size()) {
      room[n++] = t[i];
      continue;
    }
    const char escaped = t[++i];
    if (escaped == 'x') {
      room[n++] = static_cast<char>(hex(t.substr(i + 1, 2)));
      i += 2;
    } else if (escaped == 'u') {
      std::uint32_t cp = hex(t.substr(i + 1, 4));
      i += 4;
      if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i + 1, 2) == "\\u" &&
          i + 7 <= t.size()) {
        const auto low = hex(t.substr(i + 3, 4));
        if (low >= 0xdc00 && low <= 0xdfff) {
          cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      if (cp < 0x80) room[n++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
        else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000) room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800) room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[n++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else {
      switch (escaped) {
        case 'b': room[n++] = '\b'; break;
        case 'f': room[n++] = '\f'; break;
        case 'n': room[n++] = '\n'; break;
        case 'r': room[n++] = '\r'; break;
        case 't': room[n++] = '\t'; break;
        default: room[n++] = escaped; break;
      }
    }
  }
  return std::string_view(room.data(), n);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Flatbuffers : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  // Soft keywords: each is spelled once and referenced both where it
  // introduces a declaration and, through `Identifier` below, where a name
  // is read -- one token, and what follows decides.
  struct ATTRIBUTE : terminal<kw<"attribute">> {};
  struct ENUM : terminal<kw<"enum">> {};
  struct FILE_EXTENSION : terminal<kw<"file_extension">> {};
  struct FILE_IDENTIFIER : terminal<kw<"file_identifier">> {};
  struct INCLUDE : terminal<kw<"include">> {};
  struct NATIVE_INCLUDE : terminal<kw<"native_include">> {};
  struct NAMESPACE : terminal<kw<"namespace">> {};
  struct ROOT_TYPE : terminal<kw<"root_type">> {};
  struct RPC_SERVICE : terminal<kw<"rpc_service">> {};
  struct STRUCT : terminal<kw<"struct">> {};
  struct TABLE : terminal<kw<"table">> {};
  struct UNION : terminal<kw<"union">> {};

  struct Name : rule<ref<ID>> {};

  static constexpr auto Identifier =
      ref<Name> | ref<ATTRIBUTE> | ref<ENUM> | ref<FILE_EXTENSION> |
      ref<FILE_IDENTIFIER> | ref<INCLUDE> | ref<NATIVE_INCLUDE> |
      ref<NAMESPACE> | ref<ROOT_TYPE> | ref<RPC_SERVICE> | ref<STRUCT> |
      ref<TABLE> | ref<UNION>;

  // A base type is never a name (it is not in `Identifier` above):
  // reserved wherever a type is read.
  static constexpr auto BaseType =
      kw<"bool"> | kw<"byte"> | kw<"ubyte"> | kw<"short"> | kw<"ushort"> |
      kw<"int"> | kw<"uint"> | kw<"float"> | kw<"long"> | kw<"ulong"> |
      kw<"double"> | kw<"int8"> | kw<"uint8"> | kw<"int16"> | kw<"uint16"> |
      kw<"int32"> | kw<"uint32"> | kw<"int64"> | kw<"uint64"> |
      kw<"float32"> | kw<"float64"> | kw<"string">;

  // `true`/`false` are read here, not as names: `Identifier` never lists
  // them, so the tie between a keyword and the general word terminal
  // resolves the same way it does for a base type.
  struct INTEGER_CONSTANT
      : terminal<(option(cr<"+-">) + some(d)) | kw<"true"> | kw<"false">> {};
  struct HEX_INTEGER_CONSTANT
      : terminal<option(cr<"+-">) + lit<"0"> + cr<"xX"> +
                 some(cr<"0-9a-fA-F">)> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto HexDigits = cr<"0-9a-fA-F">;
  static constexpr auto HexFloat = lit<"0"> + cr<"xX"> +
      ((some(HexDigits) + option(lit<"."> + many(HexDigits))) |
       (lit<"."> + some(HexDigits))) +
      cr<"pP"> + option(cr<"+-">) + some(d);
  static constexpr auto FloatLit =
      HexFloat |
      (some(d) + lit<"."> + option(some(d)) + option(Exponent)) |
      (some(d) + Exponent) | (lit<"."> + some(d) + option(Exponent)) |
      kw<"inf"> | kw<"infinity"> | kw<"nan"> |
      kw<"INF"> | kw<"INFINITY"> | kw<"NAN">;
  struct FLOAT_CONSTANT : terminal<option(cr<"+-">) + FloatLit> {};

  static constexpr auto SimpleEscape = lit<"\\"> + cr<R"("'?bfnrtv\\/)">;
  static constexpr auto HexEscape = lit<"\\x"> + repeat<2>(cr<"0-9a-fA-F">);
  static constexpr auto UnicodeEscape = lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto StringEscape = SimpleEscape | HexEscape | UnicodeEscape;
  struct STRING_CONSTANT
      : terminal<(lit<"\""> + many(StringEscape | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(StringEscape | cr<R"(^'\\\r\n)">) + lit<"'">),
                 Unquoted> {};

  static constexpr auto IntegerConst =
      ref<INTEGER_CONSTANT> | ref<HEX_INTEGER_CONSTANT>;

  struct ScalarFunctionRule;
  struct NsIdentRule;
  static constexpr auto scalar =
      IntegerConst | ref<FLOAT_CONSTANT> | ref<ScalarFunctionRule> |
      ref<NsIdentRule>;

  static constexpr auto single_value = scalar | ref<STRING_CONSTANT>;

  struct ScalarFunctionRule
      : rule<option(cr<"+-">) + Identifier + lit<"("> +
             many(single_value, lit<",">) + lit<")">> {};

  struct MetadataItemRule
      : rule<assign<&ast::Metadata::name>(Identifier | ref<STRING_CONSTANT>) +
             option(lit<":"> +
                    assign<&ast::Metadata::value>(single_value))> {};

  template <auto Member>
  static constexpr auto MetadataInto =
      option(lit<"("> + some(append<Member>(ref<MetadataItemRule>), lit<",">) +
             lit<")">);

  struct TypeRule;
  struct NsIdentRule : rule<some(Identifier, lit<".">)> {};

  struct TypeRule
      : rule<(lit<"["> + assign<&ast::Type::element>(ref<TypeRule>) +
              option(lit<":"> + assign<&ast::Type::length>(IntegerConst)) +
              lit<"]">) |
             assign<&ast::Type::name>(BaseType | ref<NsIdentRule>)> {};

  struct FieldRule
      : rule<assign<&ast::Field::name>(Identifier) + lit<":"> +
             assign<&ast::Field::type>(ref<TypeRule>) +
             option(lit<"="> +
                    assign<&ast::Field::defaultValue>(single_value)) +
             MetadataInto<&ast::Field::metadata> +
             lit<";">> {};

  struct TypeDeclRule
      : rule<(flag<&ast::TypeDecl::isStruct>(ref<STRUCT>) | ref<TABLE>) +
             assign<&ast::TypeDecl::name>(Identifier) +
             MetadataInto<&ast::TypeDecl::metadata> +
             lit<"{"> + many(append<&ast::TypeDecl::fields>(ref<FieldRule>)) +
             lit<"}">> {};

  struct EnumValRule
      : rule<assign<&ast::EnumVal::name>(ref<NsIdentRule>) +
             option(lit<"="> +
                    assign<&ast::EnumVal::value>(IntegerConst)) +
             MetadataInto<&ast::EnumVal::metadata>> {};

  struct EnumDeclRule
      : rule<ref<ENUM> + assign<&ast::EnumDecl::name>(Identifier) +
             option(lit<":"> + assign<&ast::EnumDecl::baseType>(ref<TypeRule>)) +
             MetadataInto<&ast::EnumDecl::metadata> +
             lit<"{"> +
             many(append<&ast::EnumDecl::values>(ref<EnumValRule>), lit<",">) +
             option(lit<",">) + lit<"}">> {};

  struct UnionValRule
      : rule<assign<&ast::UnionVal::name>(ref<NsIdentRule>) +
             option(lit<":"> +
                    assign<&ast::UnionVal::type>(ref<NsIdentRule>)) +
             option(lit<"="> +
                    assign<&ast::UnionVal::value>(IntegerConst)) +
             MetadataInto<&ast::UnionVal::metadata>> {};

  struct UnionDeclRule
      : rule<ref<UNION> + assign<&ast::UnionDecl::name>(Identifier) +
             option(lit<":"> + assign<&ast::UnionDecl::baseType>(ref<TypeRule>)) +
             MetadataInto<&ast::UnionDecl::metadata> +
             lit<"{"> +
             many(append<&ast::UnionDecl::values>(ref<UnionValRule>),
                  lit<",">) +
             option(lit<",">) + lit<"}">> {};

  struct RpcMethodRule
      : rule<assign<&ast::RpcMethod::name>(Identifier) + lit<"("> +
             assign<&ast::RpcMethod::argType>(ref<NsIdentRule>) + lit<")"> +
             lit<":"> + assign<&ast::RpcMethod::returnType>(ref<NsIdentRule>) +
             MetadataInto<&ast::RpcMethod::metadata> +
             lit<";">> {};

  struct RpcDeclRule
      : rule<ref<RPC_SERVICE> + assign<&ast::RpcDecl::name>(Identifier) +
             MetadataInto<&ast::RpcDecl::metadata> +
             lit<"{"> +
             some(append<&ast::RpcDecl::methods>(ref<RpcMethodRule>)) +
             lit<"}">> {};

  struct NamespaceDeclRule
      : rule<ref<NAMESPACE> +
             option(assign<&ast::Namespace::path>(ref<NsIdentRule>)) + lit<";">> {};

  struct AttributeDeclRule
      : rule<ref<ATTRIBUTE> +
             assign<&ast::Attribute::name>(ref<STRING_CONSTANT> | Identifier) +
             lit<";">> {};

  struct RootDeclRule
      : rule<ref<ROOT_TYPE> + assign<&ast::Root::name>(ref<NsIdentRule>) +
             lit<";">> {};

  struct FileExtensionDeclRule
      : rule<ref<FILE_EXTENSION> +
             assign<&ast::FileExtension::ext>(ref<STRING_CONSTANT>) +
             lit<";">> {};

  struct FileIdentifierDeclRule
      : rule<ref<FILE_IDENTIFIER> +
             assign<&ast::FileIdentifier::id>(ref<STRING_CONSTANT>) +
             lit<";">> {};

  struct IncludeRule
      : rule<(flag<&ast::Include::isNative>(ref<NATIVE_INCLUDE>) |
              ref<INCLUDE>) +
             assign<&ast::Include::path>(ref<STRING_CONSTANT>) + lit<";">> {};

  struct KeyValueRule;
  struct ObjectRule;
  struct ArrayRule;
  static constexpr auto value = single_value | ref<ObjectRule> | ref<ArrayRule>;

  struct KeyValueRule
      : rule<assign<&ast::KeyValue::name>(Identifier | ref<STRING_CONSTANT>) + lit<":"> +
             assign<&ast::KeyValue::value>(value)> {};

  struct ObjectRule
      : rule<lit<"{"> +
             many(append<&ast::Object::members>(ref<KeyValueRule>), lit<",">) +
             option(lit<",">) + lit<"}">> {};

  struct ArrayRule
      : rule<lit<"["> +
             many(append<&ast::ArrayVal::items>(value), lit<",">) +
             option(lit<",">) + lit<"]">> {};

  struct SchemaRule
      : rule<create<ast::Schema> +
             many(append<&ast::Schema::includes>(ref<IncludeRule>)) +
             many(append<&ast::Schema::declarations>(
                 ref<NamespaceDeclRule> | ref<TypeDeclRule> |
                 ref<EnumDeclRule> | ref<UnionDeclRule> | ref<RootDeclRule> |
                 ref<FileExtensionDeclRule> | ref<FileIdentifierDeclRule> |
                 ref<AttributeDeclRule> | ref<RpcDeclRule> |
                 ref<ObjectRule>))> {};

  using entry = SchemaRule;
  struct BOM : terminal<lit<"\xEF\xBB\xBF">> {};
  using ignore = list<WS, BOM>;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};

}  // namespace flatbuffers
