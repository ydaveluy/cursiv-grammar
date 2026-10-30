// Protobuf source declarations: an optional syntax header, then a mix of
// imports/packages/options/definitions; no lexer modes, no precedence.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "protobuf/core/ast.hpp"

namespace protobuf {

inline constexpr auto unescape_str =
    [](std::string_view t, std::pmr::memory_resource *mem) -> std::string_view {
  const std::span<char> room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto digit = [](char c) -> unsigned {
    return c <= '9' ? c - '0' : (c | 32) - 'a' + 10;
  };
  const auto utf8 = [&](std::uint32_t cp) {
    if (cp < 0x80)
      room[n++] = static_cast<char>(cp);
    else {
      if (cp >= 0x10000)
        room[n++] = static_cast<char>(0xf0 | (cp >> 18));
      else if (cp >= 0x800)
        room[n++] = static_cast<char>(0xe0 | (cp >> 12));
      else
        room[n++] = static_cast<char>(0xc0 | (cp >> 6));
      if (cp >= 0x10000)
        room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
      if (cp >= 0x800)
        room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (cp & 63));
    }
  };
  for (std::size_t i = 0; i < t.size();) {
    if (t.substr(i, 2) == "//") {
      i = t.find_first_of("\r\n", i + 2);
      if (i == std::string_view::npos)
        break;
    } else if (t.substr(i, 2) == "/*") {
      i = t.find("*/", i + 2);
      if (i == std::string_view::npos)
        break;
      i += 2;
    } else if (t[i] != '\'' && t[i] != '"') {
      ++i;
    } else {
      const char quote = t[i++];
      while (i < t.size() && t[i] != quote) {
        if (t[i] != '\\' || i + 1 >= t.size()) {
          room[n++] = t[i++];
          continue;
        }
        const char escape = t[++i];
        ++i;
        if (escape == 'x' || escape == 'X' ||
            (escape >= '0' && escape <= '7')) {
          const unsigned base = escape == 'x' || escape == 'X' ? 16 : 8;
          unsigned value = base == 8 ? escape - '0' : 0;
          unsigned count = base == 8 ? 1 : 0;
          while (i < t.size() && count < (base == 8 ? 3 : 2) &&
                 ((t[i] >= '0' && t[i] <= '7') ||
                  (base == 16 && ((t[i] >= '8' && t[i] <= '9') ||
                                  (t[i] >= 'a' && t[i] <= 'f') ||
                                  (t[i] >= 'A' && t[i] <= 'F'))))) {
            value = value * base + digit(t[i++]);
            ++count;
          }
          room[n++] = static_cast<char>(value);
        } else if (escape == 'u' || escape == 'U') {
          std::uint32_t cp = 0;
          const std::size_t count = escape == 'u' ? 4 : 8;
          for (std::size_t j = 0; j < count && i < t.size(); ++j)
            cp = cp * 16 + digit(t[i++]);
          if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i, 2) == "\\u" &&
              i + 6 <= t.size()) {
            std::uint32_t low = 0;
            for (std::size_t j = i + 2; j < i + 6; ++j)
              low = low * 16 + digit(t[j]);
            if (low >= 0xdc00 && low <= 0xdfff) {
              cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
              i += 6;
            }
          }
          utf8(cp);
        } else {
          switch (escape) {
          case 'a':
            room[n++] = '\a';
            break;
          case 'b':
            room[n++] = '\b';
            break;
          case 'f':
            room[n++] = '\f';
            break;
          case 'n':
            room[n++] = '\n';
            break;
          case 'r':
            room[n++] = '\r';
            break;
          case 't':
            room[n++] = '\t';
            break;
          case 'v':
            room[n++] = '\v';
            break;
          default:
            room[n++] = escape;
            break;
          }
        }
      }
      if (i < t.size())
        ++i;
    }
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto text_to_label = [](std::string_view t) {
  return t == "optional" ? ast::FieldLabel::optional_
       : t == "required" ? ast::FieldLabel::required_ : ast::FieldLabel::repeated_;
};

inline constexpr auto numberSource =
    [](cursiv::Parts parts) -> std::string_view {
  if (parts.empty())
    return {};
  const auto first = parts.front(), last = parts.back();
  return {first.data(),
          static_cast<std::size_t>(last.data() + last.size() - first.data())};
};

struct Protobuf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)> {};

  // Every keyword also names the general identifier position ("message
  // message { ... }" -- see keywordasid.proto): spelled once, referenced
  // both as a keyword and as an alternative of Ident.
  struct SYNTAX_KW : terminal<kw<"syntax">> {};
  struct IMPORT_KW : terminal<kw<"import">> {};
  struct WEAK_KW : terminal<kw<"weak">> {};
  struct PUBLIC_KW : terminal<kw<"public">> {};
  struct PACKAGE_KW : terminal<kw<"package">> {};
  struct OPTION_KW : terminal<kw<"option">> {};
  struct OPTIONAL_KW : terminal<kw<"optional">> {};
  struct REQUIRED_KW : terminal<kw<"required">> {};
  struct REPEATED_KW : terminal<kw<"repeated">> {};
  struct ONEOF_KW : terminal<kw<"oneof">> {};
  struct MAP_KW : terminal<kw<"map">> {};
  struct INT32_KW : terminal<kw<"int32">> {};
  struct INT64_KW : terminal<kw<"int64">> {};
  struct UINT32_KW : terminal<kw<"uint32">> {};
  struct UINT64_KW : terminal<kw<"uint64">> {};
  struct SINT32_KW : terminal<kw<"sint32">> {};
  struct SINT64_KW : terminal<kw<"sint64">> {};
  struct FIXED32_KW : terminal<kw<"fixed32">> {};
  struct FIXED64_KW : terminal<kw<"fixed64">> {};
  struct SFIXED32_KW : terminal<kw<"sfixed32">> {};
  struct SFIXED64_KW : terminal<kw<"sfixed64">> {};
  struct BOOL_KW : terminal<kw<"bool">> {};
  struct STRING_KW : terminal<kw<"string">> {};
  struct DOUBLE_KW : terminal<kw<"double">> {};
  struct FLOAT_KW : terminal<kw<"float">> {};
  struct BYTES_KW : terminal<kw<"bytes">> {};
  struct GROUP_KW : terminal<kw<"group">> {};
  struct EXTENSIONS_KW : terminal<kw<"extensions">> {};
  struct RESERVED_KW : terminal<kw<"reserved">> {};
  struct TO_KW : terminal<kw<"to">> {};
  struct MAX_KW : terminal<kw<"max">> {};
  struct ENUM_KW : terminal<kw<"enum">> {};
  struct MESSAGE_KW : terminal<kw<"message">> {};
  struct SERVICE_KW : terminal<kw<"service">> {};
  struct EXTEND_KW : terminal<kw<"extend">> {};
  struct RPC_KW : terminal<kw<"rpc">> {};
  struct STREAM_KW : terminal<kw<"stream">> {};
  struct RETURNS_KW : terminal<kw<"returns">> {};
  struct TRUE_KW : terminal<kw<"true">> {};
  struct FALSE_KW : terminal<kw<"false">> {};

  static constexpr auto scalar_kw =
      ref<DOUBLE_KW> | ref<FLOAT_KW> | ref<INT32_KW> | ref<INT64_KW> |
      ref<UINT32_KW> | ref<UINT64_KW> | ref<SINT32_KW> | ref<SINT64_KW> |
      ref<FIXED32_KW> | ref<FIXED64_KW> | ref<SFIXED32_KW> | ref<SFIXED64_KW> |
      ref<BOOL_KW> | ref<STRING_KW> | ref<BYTES_KW>;
  static constexpr auto bool_kw = ref<TRUE_KW> | ref<FALSE_KW>;
  static constexpr auto other_kw =
      ref<SYNTAX_KW> | ref<IMPORT_KW> | ref<WEAK_KW> | ref<PUBLIC_KW> |
      ref<PACKAGE_KW> | ref<OPTION_KW> | ref<OPTIONAL_KW> | ref<REQUIRED_KW> | ref<REPEATED_KW> |
      ref<ONEOF_KW> | ref<MAP_KW> | ref<GROUP_KW> | ref<EXTENSIONS_KW> | ref<RESERVED_KW> | ref<TO_KW> |
      ref<MAX_KW> | ref<ENUM_KW> | ref<MESSAGE_KW> | ref<SERVICE_KW> |
      ref<EXTEND_KW> | ref<RPC_KW> | ref<STREAM_KW> | ref<RETURNS_KW>;
  static constexpr auto ident_kw = scalar_kw | bool_kw | other_kw;

  struct Ident : rule<ref<ID> | ident_kw> {};
  struct FullIdent : rule<some(ref<Ident>, lit<".">)> {};
  static constexpr auto key_type_kw =
      ref<INT32_KW> | ref<INT64_KW> | ref<UINT32_KW> | ref<UINT64_KW> |
      ref<SINT32_KW> | ref<SINT64_KW> | ref<FIXED32_KW> | ref<FIXED64_KW> |
      ref<SFIXED32_KW> | ref<SFIXED64_KW> | ref<BOOL_KW> | ref<STRING_KW>;

  // A bare identifier that names a message or an enum type is one shape in
  // the grammar either way: its head
  // leaves the scalar keywords out, so the choice in type_expr does not fork.
  struct TypeName
      : rule<(lit<"."> + some(ref<Ident>, lit<".">)) |
             ((ref<ID> | bool_kw | other_kw) + many(lit<"."> + ref<Ident>))> {};
  struct QualifiedTypeName
      : rule<option(lit<".">) + some(ref<Ident>, lit<".">)> {};

  static constexpr auto type_expr =
      scalar_kw | (!ref<GROUP_KW> + ref<TypeName>);

  static constexpr auto str_escape =
      (lit<"\\"> + cr<"xX"> + cr<"0-9a-fA-F"> + option(cr<"0-9a-fA-F">)) |
      (lit<"\\"> + cr<"0-7"> + repeat<0, 2>(cr<"0-7">)) |
      (lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">)) |
      (lit<"\\U"> + repeat<8>(cr<"0-9a-fA-F">)) |
      (lit<"\\"> + cr<R"(abfnrtv\\'")">);
  struct STR_LIT
      : terminal<(lit<"'"> + many(str_escape | cr<R"(^'\\\n)">) + lit<"'">) |
                 (lit<"\""> + many(str_escape | cr<R"(^"\\\n)">) + lit<"\"">)> {
  };
  struct StringLit : rule<as<unescape_str>(some(ref<STR_LIT>))> {};

  struct INT_LIT_TXT
      : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) |
                 (lit<"0"> + many(cr<"0-7">)) | (cr<"1-9"> + many(d))> {};
  struct IntLit : rule<as<numberSource>(ref<INT_LIT_TXT>)> {};
  struct SignedIntLit
      : rule<as<numberSource>(option(cr<"+-">) + ref<INT_LIT_TXT>)> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT_LIT_TXT
      : terminal<(some(d) + lit<"."> + many(d) + option(exponent)) |
                 (some(d) + exponent) |
                 (lit<"."> + some(d) + option(exponent)) | kw<"inf"> |
                 kw<"nan">> {};
  struct SignedFloatLit
      : rule<as<numberSource>(option(cr<"+-">) + ref<FLOAT_LIT_TXT>)> {};

  static constexpr auto bool_lit = as<true>(kw<"true">) | as<false>(kw<"false">);

  // A constant reads `true` and `false` as the boolean literal; the head of an
  // identifier leaves them out.
  struct ConstIdent
      : rule<(ref<ID> | scalar_kw | other_kw) + many(lit<"."> + ref<Ident>)> {};
  struct IdentRef : rule<create<ast::IdentRef> +
                             assign<&ast::IdentRef::name>(ref<ConstIdent>)> {};

  struct MessageFieldValue;
  struct MessageListValue;
  struct MessageValue : rule<create<ast::MessageValue> + lit<"{"> +
                                 many(append<&ast::MessageValue::fields>(
                                          ref<MessageFieldValue>) +
                                      option(cr<",;">)) +
                                 lit<"}">> {};
  struct MessageAngleValue : rule<create<ast::MessageValue> + lit<"<"> +
                                      many(append<&ast::MessageValue::fields>(
                                               ref<MessageFieldValue>) +
                                           option(cr<",;">)) +
                                      lit<">">> {};

  struct IntegerConstant
      : rule<create<ast::NumberLiteral> +
             assign<&ast::NumberLiteral::text>(ref<SignedIntLit>)> {};
  struct FloatConstant
      : rule<create<ast::NumberLiteral> +
             flag<&ast::NumberLiteral::floating>(
                 assign<&ast::NumberLiteral::text>(ref<SignedFloatLit>))> {
  };
  static constexpr auto constant =
      ref<IntegerConstant> | ref<FloatConstant> | bool_lit |
      ref<StringLit> | ref<IdentRef> | ref<MessageValue> |
      ref<MessageAngleValue> | ref<MessageListValue>;

  struct MessageListValue
      : rule<create<ast::MessageListValue> + lit<"["> +
             many(append<&ast::MessageListValue::items>(constant), lit<",">) +
             lit<"]">> {};
  struct MessageFieldName
      : rule<ref<FullIdent> |
             (lit<"["> + ref<QualifiedTypeName> + lit<"]">)> {};
  struct MessageFieldValue
      : rule<assign<&ast::MessageFieldValue::name>(ref<MessageFieldName>) +
             option(lit<":">) +
             assign<&ast::MessageFieldValue::value>(constant)> {};

  struct OptionName
      : rule<assign<&ast::OptionName::name>(ref<FullIdent>) |
             (flag<&ast::OptionName::isExtension>(lit<"(">) +
              assign<&ast::OptionName::name>(ref<FullIdent>) + lit<")"> +
              option(lit<"."> +
                     assign<&ast::OptionName::suffix>(ref<FullIdent>)))> {};

  struct OptionDecl
      : rule<kw<"option"> +
             assign<&ast::OptionDecl::name>(ref<OptionName>) + lit<"="> +
             assign<&ast::OptionDecl::value>(constant)> {};

  struct FieldOption
      : rule<assign<&ast::FieldOption::name>(ref<OptionName>) + lit<"="> +
             assign<&ast::FieldOption::value>(constant)> {};

  template <auto Member>
  static constexpr auto OptionsInto =
      lit<"["> + some(append<Member>(ref<FieldOption>), lit<",">) +
      lit<"]">;
  static constexpr auto field_options = OptionsInto<&ast::Field::options>;
  static constexpr auto oneof_field_options =
      OptionsInto<&ast::OneofField::options>;
  static constexpr auto map_field_options =
      OptionsInto<&ast::MapField::options>;
  static constexpr auto enum_field_options =
      OptionsInto<&ast::EnumField::options>;

  struct FieldLabel
      : rule<as<text_to_label>(ref<OPTIONAL_KW> | ref<REQUIRED_KW> | ref<REPEATED_KW>)> {};

  static constexpr auto field_label = assign<&ast::Field::label>(ref<FieldLabel>);
  static constexpr auto field_body =
      assign<&ast::Field::type>(type_expr) +
      assign<&ast::Field::name>(ref<Ident>) + lit<"="> +
      assign<&ast::Field::number>(ref<IntLit>) + option(field_options) + lit<";">;
  // A label word at the head is consumed as a label, never retried as a type.
  struct Field : rule<(field_label | !ref<FieldLabel>) + field_body> {};

  static constexpr auto message_statement_kw =
      ref<MESSAGE_KW> | ref<ENUM_KW> | ref<EXTENSIONS_KW> | ref<RESERVED_KW> |
      ref<EXTEND_KW> | ref<OPTION_KW> | ref<ONEOF_KW>;
  // These words dispatch message statements only before an unlabelled field.
  struct MessageField
      : rule<(field_label | !(ref<FieldLabel> | message_statement_kw)) + field_body> {};

  struct OneofField
      : rule<!(ref<OPTION_KW> | ref<FieldLabel>) +
             assign<&ast::OneofField::type>(type_expr) +
             assign<&ast::OneofField::name>(ref<Ident>) + lit<"="> +
             assign<&ast::OneofField::number>(ref<IntLit>) +
             option(oneof_field_options) + lit<";">> {};

  struct Group;
  struct OneofGroup;
  struct MessageDef;

  struct Oneof
      : rule<create<ast::Oneof> + kw<"oneof"> +
             assign<&ast::Oneof::name>(ref<Ident>) + lit<"{"> +
             many((append<&ast::Oneof::options>(ref<OptionDecl>) + lit<";">) |
                  append<&ast::Oneof::fields>(ref<OneofField>) |
                  append<&ast::Oneof::groups>(ref<OneofGroup>) | lit<";">) +
             lit<"}">> {};

  struct MapField
      : rule<kw<"map"> + lit<"<"> +
             assign<&ast::MapField::keyType>(key_type_kw) + lit<","> +
             assign<&ast::MapField::valueType>(type_expr) + lit<">"> +
             assign<&ast::MapField::name>(ref<Ident>) + lit<"="> +
             assign<&ast::MapField::number>(ref<IntLit>) +
             option(map_field_options) + lit<";">> {};

  template <auto Integer>
  static constexpr auto RangeBody =
      assign<&ast::ReservedRange::from>(Integer) +
      option(kw<"to"> +
             (flag<&ast::ReservedRange::toMax>(kw<"max">) |
              assign<&ast::ReservedRange::to>(Integer)));

  struct ReservedRange : rule<RangeBody<ref<SignedIntLit>>> {};
  struct ExtensionRange : rule<RangeBody<ref<IntLit>>> {};
  struct Extensions
      : rule<kw<"extensions"> +
             some(append<&ast::Extensions::ranges>(ref<ExtensionRange>), lit<",">) +
             option(OptionsInto<&ast::Extensions::options>) + lit<";">> {};

  struct Reserved
      : rule<create<ast::Reserved> + kw<"reserved"> +
             (some(append<&ast::Reserved::ranges>(ref<ReservedRange>),
                   lit<",">) |
              some(append<&ast::Reserved::fieldNames>(ref<StringLit>),
                   lit<",">)) +
             lit<";">> {};

  struct EnumField
      : rule<assign<&ast::EnumField::name>(ref<Ident>) + lit<"="> +
             assign<&ast::EnumField::value>(ref<SignedIntLit>) +
             option(enum_field_options) + lit<";">> {};

  struct EnumDef
      : rule<create<ast::EnumDef> + kw<"enum"> +
             assign<&ast::EnumDef::name>(ref<Ident>) + lit<"{"> +
             many((append<&ast::EnumDef::options>(ref<OptionDecl>) +
                      lit<";">) |
                  append<&ast::EnumDef::fields>(ref<EnumField>) |
                  append<&ast::EnumDef::reserved>(ref<Reserved>) |
                  lit<";">) +
             lit<"}">> {};

  struct ExtendDef
      : rule<create<ast::ExtendDef> + kw<"extend"> +
             assign<&ast::ExtendDef::target>(ref<QualifiedTypeName>) +
             lit<"{"> +
             many(append<&ast::ExtendDef::fields>(ref<Field>) |
                  append<&ast::ExtendDef::groups>(ref<Group>) | lit<";">) +
             lit<"}">> {};

  static constexpr auto message_body =
      lit<"{"> +
      many(append<&ast::MessageDef::fields>(ref<MessageField>) |
           append<&ast::MessageDef::groups>(ref<Group>) |
           append<&ast::MessageDef::enums>(ref<EnumDef>) |
           append<&ast::MessageDef::messages>(ref<MessageDef>) |
           append<&ast::MessageDef::extends_>(ref<ExtendDef>) |
           (append<&ast::MessageDef::options>(ref<OptionDecl>) + lit<";">) |
           append<&ast::MessageDef::oneofs>(ref<Oneof>) |
           append<&ast::MessageDef::mapFields>(ref<MapField>) |
           append<&ast::MessageDef::reserved>(ref<Reserved>) |
           append<&ast::MessageDef::extensions>(ref<Extensions>) | lit<";">) +
      lit<"}">;

  struct MessageDef
      : rule<create<ast::MessageDef> + kw<"message"> +
             assign<&ast::MessageDef::name>(ref<Ident>) + message_body> {};

  static constexpr auto group_tail =
      kw<"group"> + assign<&ast::MessageDef::name>(ref<Ident>) + lit<"="> +
      assign<&ast::Group::number>(ref<IntLit>) +
      option(OptionsInto<&ast::Group::fieldOptions>) + message_body;
  struct Group
      : rule<create<ast::Group> +
             option(assign<&ast::Group::label>(ref<FieldLabel>)) + group_tail> {};
  struct OneofGroup : rule<create<ast::Group> + group_tail> {};

  struct Rpc
      : rule<create<ast::Rpc> + kw<"rpc"> +
             assign<&ast::Rpc::name>(ref<Ident>) + lit<"("> +
             option(flag<&ast::Rpc::requestStream>(kw<"stream">)) +
             assign<&ast::Rpc::requestType>(ref<QualifiedTypeName>) +
             lit<")"> + kw<"returns"> + lit<"("> +
             option(flag<&ast::Rpc::responseStream>(kw<"stream">)) +
             assign<&ast::Rpc::responseType>(ref<QualifiedTypeName>) +
             lit<")"> +
             (lit<";"> |
              (lit<"{"> +
               many((append<&ast::Rpc::options>(ref<OptionDecl>) + lit<";">) |
                    lit<";">) +
               lit<"}">))> {};

  struct ServiceDef
      : rule<create<ast::ServiceDef> + kw<"service"> +
             assign<&ast::ServiceDef::name>(ref<Ident>) + lit<"{"> +
             many((append<&ast::ServiceDef::options>(ref<OptionDecl>) +
                      lit<";">) |
                  append<&ast::ServiceDef::rpcs>(ref<Rpc>) | lit<";">) +
             lit<"}">> {};

  struct Import
      : rule<kw<"import"> +
             option(flag<&ast::Import::isWeak>(kw<"weak">) |
                    flag<&ast::Import::isPublic>(kw<"public">)) +
             assign<&ast::Import::path>(ref<StringLit>) + lit<";">> {};

  struct Package
      : rule<kw<"package"> + assign<&ast::Package::name>(ref<FullIdent>) +
             lit<";">> {};

  static constexpr auto syntax_stmt =
      kw<"syntax"> + lit<"="> +
      assign<&ast::Document::syntax>(ref<StringLit>) + lit<";">;

  struct Document
      : rule<create<ast::Document> + option(syntax_stmt) +
             many(append<&ast::Document::imports>(ref<Import>) |
                  append<&ast::Document::packages>(ref<Package>) |
                  (append<&ast::Document::options>(ref<OptionDecl>) +
                      lit<";">) |
                  append<&ast::Document::messages>(ref<MessageDef>) |
                  append<&ast::Document::enums>(ref<EnumDef>) |
                  append<&ast::Document::extends_>(ref<ExtendDef>) |
                  append<&ast::Document::services>(ref<ServiceDef>) |
                  lit<";">)> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

} // namespace protobuf
