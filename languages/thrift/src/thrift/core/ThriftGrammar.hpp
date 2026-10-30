// The thrift language: its grammar. Apache Thrift IDL -- headers, then
// definitions (const, typedef, enum, senum, struct/union/exception,
// service); no lexer modes, no precedence.
#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <limits>
#include <system_error>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "thrift/core/ast.hpp"

namespace thrift {

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource* mem) {
      const std::string_view body = t.substr(1, t.size() - 2);
      if (body.find('\\') == std::string_view::npos) return body;
      const std::span<char> room = cursiv::text_room(mem, body.size());
      std::size_t n = 0;
      for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) {
          ++i;
          switch (body[i]) {
            case 'r': room[n++] = '\r'; break;
            case 'n': room[n++] = '\n'; break;
            case 't': room[n++] = '\t'; break;
            default: room[n++] = body[i]; break;  // '"' '\'' '\\'
          }
        } else {
          room[n++] = body[i];
        }
      }
      return std::string_view(room.data(), n);
    };

inline constexpr auto parse_hex = [](std::string_view t) -> cursiv::ConversionResult<std::int64_t> {
  const bool neg = !t.empty() && t.front() == '-';
  if (!t.empty() && (t.front() == '-' || t.front() == '+')) t.remove_prefix(1);
  t.remove_prefix(2);
  std::uint64_t v = 0;
  const auto [end, ec] = std::from_chars(t.data(), t.data() + t.size(), v, 16);
  constexpr std::uint64_t sign = std::uint64_t{1} << 63;
  if (ec != std::errc{} || end != t.data() + t.size() || v > (neg ? sign : sign - 1))
    return cursiv::ConversionResult<std::int64_t>::err("hexadecimal integer is outside the signed 64-bit range");
  const auto n = neg && v == sign ? std::numeric_limits<std::int64_t>::min() : neg ? -static_cast<std::int64_t>(v) : static_cast<std::int64_t>(v);
  return cursiv::ConversionResult<std::int64_t>::ok(n);
};

struct Thrift : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"#">) + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9._">)> {};

  static constexpr auto escape = lit<"\\"> + cr<R"(rnt"'\\)">;
  struct LITERAL : terminal<as<unquote>(
                       (lit<"\""> + many(escape | cr<R"(^"\\)">) + lit<"\"">) |
                       (lit<"'"> + many(escape | cr<R"(^'\\)">) + lit<"'">))> {
  };

  struct INTEGER : terminal<as<std::int64_t>(option(cr<"+-">) + some(d))> {};
  struct HEX_INTEGER
      : terminal<as<parse_hex>(option(cr<"+-">) +
                               (lit<"0x"> | lit<"0X">)+some(cr<"0-9a-fA-F">))> {
  };
  struct DOUBLE
      : terminal<as<double>(
            option(cr<"+-">) +
            ((some(d) + option(lit<"."> + some(d))) | (lit<"."> + some(d))) +
            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  static constexpr auto integer_value =
      ref<INTEGER> | ref<HEX_INTEGER> | as<std::int64_t{1}>(kw<"true">) | as<std::int64_t{0}>(kw<"false">);

  struct BASE_TYPE
      : terminal<kw<"bool"> | kw<"byte"> | kw<"i8"> | kw<"uuid"> | kw<"i16"> | kw<"i32"> |
                 kw<"i64"> | kw<"double"> | kw<"string"> | kw<"binary">> {};

  static constexpr auto field_req = as<ast::FieldReq::required_>(kw<"required">) | as<ast::FieldReq::optional_>(kw<"optional">);


  struct TypeAnnotation;
  struct BaseType;
  struct NamedType;
  struct MapType;
  struct SetType;
  struct ListType;
  struct Field;
  struct ConstRef;
  struct ConstList;
  struct ConstMap;

  static constexpr auto list_separator = lit<","> | lit<";">;

  static constexpr auto const_value =
      integer_value | ref<DOUBLE> | ref<LITERAL> | ref<ConstRef> |
      ref<ConstList> | ref<ConstMap>;

  static constexpr auto annotation_value = integer_value | ref<LITERAL>;

  struct TypeAnnotation
      : rule<assign<&ast::TypeAnnotation::name>(ref<ID>) +
                 option(lit<"="> +
                        assign<&ast::TypeAnnotation::value>(annotation_value)) +
                 option(list_separator)> {};

  template <auto Member>
  static constexpr auto AnnotationsInto =
      option(lit<"("> + many(append<Member>(ref<TypeAnnotation>)) + lit<")">);

  struct FieldType
      : rule<ref<BaseType> | ref<NamedType> | ref<MapType> |
             ref<SetType> | ref<ListType>> {};

  struct BaseType
      : rule<assign<&ast::BaseType::keyword>(ref<BASE_TYPE>) +
             AnnotationsInto<&ast::BaseType::annotations>> {};

  struct NamedType : rule<assign<&ast::NamedType::name>(ref<ID>)> {};

  struct MapType
      : rule<create<ast::MapType> + kw<"map"> +
             option(kw<"cpp_type"> +
                    assign<&ast::MapType::cppType>(ref<LITERAL>)) +
             lit<"<"> + assign<&ast::MapType::key>(ref<FieldType>) +
             lit<","> + assign<&ast::MapType::value>(ref<FieldType>) +
             lit<">"> + AnnotationsInto<&ast::MapType::annotations>> {};

  struct SetType
      : rule<create<ast::SetType> + kw<"set"> +
             option(kw<"cpp_type"> +
                    assign<&ast::SetType::cppType>(ref<LITERAL>)) +
             lit<"<"> + assign<&ast::SetType::element>(ref<FieldType>) +
             lit<">"> + AnnotationsInto<&ast::SetType::annotations>> {};

  struct ListType
      : rule<create<ast::ListType> + kw<"list"> +
             option(kw<"cpp_type"> +
                    append<&ast::ListType::cppTypes>(ref<LITERAL>)) + lit<"<"> +
             assign<&ast::ListType::element>(ref<FieldType>) + lit<">"> +
             option(kw<"cpp_type"> +
                    append<&ast::ListType::cppTypes>(ref<LITERAL>)) +
             AnnotationsInto<&ast::ListType::annotations>> {};

  struct VoidType : rule<create<ast::VoidType> + kw<"void">> {};
  static constexpr auto function_type = ref<FieldType> | ref<VoidType>;

  struct Field
      : rule<option(assign<&ast::Field::id>(integer_value) + lit<":">) +
             option(assign<&ast::Field::req>(field_req)) +
             assign<&ast::Field::type>(ref<FieldType>) +
             option(flag<&ast::Field::isReference>(lit<"&">)) +
             assign<&ast::Field::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::Field::value>(const_value)) +
             option(flag<&ast::Field::xsdOptional>(kw<"xsd_optional">)) +
             option(flag<&ast::Field::xsdNillable>(kw<"xsd_nillable">)) +
             option(kw<"xsd_attrs"> + lit<"{"> +
                    many(append<&ast::Field::xsdAttrs>(ref<Field>)) +
                    lit<"}">) +
             AnnotationsInto<&ast::Field::annotations> +
             option(list_separator)> {};

  struct ConstRef : rule<assign<&ast::ConstRef::name>(ref<ID>)> {};

  struct ConstList
      : rule<create<ast::ConstList> + lit<"["> +
             many(append<&ast::ConstList::items>(const_value) +
                  option(list_separator)) +
             lit<"]">> {};

  struct ConstMapEntry
      : rule<assign<&ast::ConstMapEntry::key>(const_value) + lit<":"> +
             assign<&ast::ConstMapEntry::value>(const_value) +
             option(list_separator)> {};

  struct ConstMap
      : rule<create<ast::ConstMap> + lit<"{"> +
             many(append<&ast::ConstMap::entries>(ref<ConstMapEntry>)) +
             lit<"}">> {};

  struct ConstDef
      : rule<kw<"const"> + assign<&ast::ConstDef::type>(ref<FieldType>) +
             assign<&ast::ConstDef::name>(ref<ID>) +
             lit<"="> + assign<&ast::ConstDef::value>(const_value) +
             option(list_separator)> {};

  struct Typedef
      : rule<kw<"typedef"> +
             assign<&ast::TypedefDef::type>(ref<FieldType>) +
             assign<&ast::TypedefDef::name>(ref<ID>) +
             AnnotationsInto<&ast::TypedefDef::annotations> +
             option(list_separator)> {};

  struct EnumField
      : rule<assign<&ast::EnumField::name>(ref<ID>) +
             option(lit<"="> +
                    assign<&ast::EnumField::value>(integer_value)) +
             AnnotationsInto<&ast::EnumField::annotations> +
             option(list_separator)> {};

  struct EnumDef
      : rule<kw<"enum"> + assign<&ast::EnumDef::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::EnumDef::fields>(ref<EnumField>)) +
             lit<"}"> + AnnotationsInto<&ast::EnumDef::annotations>> {};

  struct SenumDef
      : rule<kw<"senum"> + assign<&ast::SenumDef::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::SenumDef::values>(ref<LITERAL>) +
                  option(list_separator)) +
             lit<"}"> + AnnotationsInto<&ast::SenumDef::annotations>> {};

  struct StructLike
      : rule<assign<&ast::StructDef::kind>(
                 as<ast::StructKind::struct_>(kw<"struct">) | as<ast::StructKind::union_>(kw<"union">) | as<ast::StructKind::exception>(kw<"exception">)) +
                 assign<&ast::StructDef::name>(ref<ID>) +
                 option(flag<&ast::StructDef::xsdAll>(kw<"xsd_all">)) +
                 lit<"{"> +
                 many(append<&ast::StructDef::fields>(ref<Field>)) +
                 lit<"}"> + AnnotationsInto<&ast::StructDef::annotations>> {};

  struct Function
      : rule<option(flag<&ast::Function::oneway>(kw<"oneway"> |
                                                        kw<"async">)) +
                 assign<&ast::Function::returnType>(function_type) +
                 assign<&ast::Function::name>(ref<ID>) + lit<"("> +
                 many(append<&ast::Function::params>(ref<Field>)) +
                 lit<")"> +
                 option(kw<"throws"> + lit<"("> +
                        many(append<&ast::Function::throws>(ref<Field>)) +
                        lit<")">) +
                 AnnotationsInto<&ast::Function::annotations> +
                 option(list_separator)> {};

  struct ServiceDef
      : rule<kw<"service"> + assign<&ast::ServiceDef::name>(ref<ID>) +
             option(kw<"extends"> +
                    assign<&ast::ServiceDef::extends_>(ref<ID>)) +
             lit<"{"> +
             many(append<&ast::ServiceDef::functions>(ref<Function>)) +
             lit<"}"> + AnnotationsInto<&ast::ServiceDef::annotations>> {};

  struct Definition
      : rule<ref<ConstDef> | ref<Typedef> | ref<EnumDef> |
             ref<SenumDef> | ref<StructLike> | ref<ServiceDef>> {
  };

  struct Include
      : rule<kw<"include"> + assign<&ast::Include::path>(ref<LITERAL>)> {};

  struct CppInclude
      : rule<kw<"cpp_include"> +
             assign<&ast::CppInclude::path>(ref<LITERAL>)> {};

  struct STAR : terminal<lit<"*">> {};
  struct CPP_NAMESPACE_KW : terminal<kw<"cpp_namespace">> {};
  struct PHP_NAMESPACE_KW : terminal<kw<"php_namespace">> {};

  struct Namespace
      : rule<(kw<"namespace"> +
                  assign<&ast::NamespaceDecl::scope>(ref<STAR> | ref<ID>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID> | ref<LITERAL>) +
                  AnnotationsInto<&ast::NamespaceDecl::annotations>) |
                 (assign<&ast::NamespaceDecl::scope>(ref<CPP_NAMESPACE_KW>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID>)) |
                 (assign<&ast::NamespaceDecl::scope>(ref<PHP_NAMESPACE_KW>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID>))> {};

  struct Header
      : rule<ref<Include> | ref<Namespace> | ref<CppInclude>> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::headers>(ref<Header>)) +
             many(append<&ast::Document::definitions>(ref<Definition>))> {
  };

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace thrift
