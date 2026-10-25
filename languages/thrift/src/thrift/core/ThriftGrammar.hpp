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
using Unquoted = cursiv::text_conv<unquote>;

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
using Int64Hex = cursiv::text_conv<parse_hex>;

struct Thrift : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<(lit<"//"> | lit<"#">) + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9._">)> {};

  static constexpr auto escape = lit<"\\"> + cr<R"(rnt"'\\)">;
  struct LITERAL
      : terminal<(lit<"\""> + many(escape | cr<R"(^"\\)">) + lit<"\"">) |
                     (lit<"'"> + many(escape | cr<R"(^'\\)">) + lit<"'">),
                 Unquoted> {};

  struct INTEGER : terminal<option(cr<"+-">) + some(d), std::int64_t> {};
  struct HEX_INTEGER
      : terminal<option(cr<"+-">) + (lit<"0x"> | lit<"0X">) + some(cr<"0-9a-fA-F">),
                 Int64Hex> {};
  struct DOUBLE
      : terminal<option(cr<"+-">) +
                     ((some(d) + option(lit<"."> + some(d))) |
                      (lit<"."> + some(d))) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};

  struct TRUE_INTEGER : terminal<kw<"true">, value<std::int64_t{1}>> {};
  struct FALSE_INTEGER : terminal<kw<"false">, value<std::int64_t{0}>> {};
  static constexpr auto integer_value =
      ref<INTEGER> | ref<HEX_INTEGER> | ref<TRUE_INTEGER> | ref<FALSE_INTEGER>;

  struct BASE_TYPE
      : terminal<kw<"bool"> | kw<"byte"> | kw<"i8"> | kw<"uuid"> | kw<"i16"> | kw<"i32"> |
                 kw<"i64"> | kw<"double"> | kw<"string"> | kw<"binary">> {};

  struct REQUIRED_KW : terminal<kw<"required">, value<ast::FieldReq::required_>> {};
  struct OPTIONAL_KW : terminal<kw<"optional">, value<ast::FieldReq::optional_>> {};
  static constexpr auto field_req = ref<REQUIRED_KW> | ref<OPTIONAL_KW>;

  struct StructKw : terminal<kw<"struct">, value<ast::StructKind::struct_>> {};
  struct UnionKw : terminal<kw<"union">, value<ast::StructKind::union_>> {};
  struct ExceptionKw
      : terminal<kw<"exception">, value<ast::StructKind::exception>> {};

  struct TypeAnnotationRule;
  struct BaseTypeRule;
  struct NamedTypeRule;
  struct MapTypeRule;
  struct SetTypeRule;
  struct ListTypeRule;
  struct FieldRule;
  struct ConstRefRule;
  struct ConstListRule;
  struct ConstMapRule;

  static constexpr auto list_separator = lit<","> | lit<";">;

  static constexpr auto const_value =
      integer_value | ref<DOUBLE> | ref<LITERAL> | ref<ConstRefRule> |
      ref<ConstListRule> | ref<ConstMapRule>;

  static constexpr auto annotation_value = integer_value | ref<LITERAL>;

  struct TypeAnnotationRule
      : rule<assign<&ast::TypeAnnotation::name>(ref<ID>) +
                 option(lit<"="> +
                        assign<&ast::TypeAnnotation::value>(annotation_value)) +
                 option(list_separator)> {};

  template <auto Member>
  static constexpr auto AnnotationsInto =
      option(lit<"("> + many(append<Member>(ref<TypeAnnotationRule>)) + lit<")">);

  struct FieldTypeRule
      : rule<ref<BaseTypeRule> | ref<NamedTypeRule> | ref<MapTypeRule> |
             ref<SetTypeRule> | ref<ListTypeRule>> {};

  struct BaseTypeRule
      : rule<assign<&ast::BaseType::keyword>(ref<BASE_TYPE>) +
             AnnotationsInto<&ast::BaseType::annotations>> {};

  struct NamedTypeRule : rule<assign<&ast::NamedType::name>(ref<ID>)> {};

  struct MapTypeRule
      : rule<create<ast::MapType> + kw<"map"> +
             option(kw<"cpp_type"> +
                    assign<&ast::MapType::cppType>(ref<LITERAL>)) +
             lit<"<"> + assign<&ast::MapType::key>(ref<FieldTypeRule>) +
             lit<","> + assign<&ast::MapType::value>(ref<FieldTypeRule>) +
             lit<">"> + AnnotationsInto<&ast::MapType::annotations>> {};

  struct SetTypeRule
      : rule<create<ast::SetType> + kw<"set"> +
             option(kw<"cpp_type"> +
                    assign<&ast::SetType::cppType>(ref<LITERAL>)) +
             lit<"<"> + assign<&ast::SetType::element>(ref<FieldTypeRule>) +
             lit<">"> + AnnotationsInto<&ast::SetType::annotations>> {};

  struct ListTypeRule
      : rule<create<ast::ListType> + kw<"list"> +
             option(kw<"cpp_type"> +
                    append<&ast::ListType::cppTypes>(ref<LITERAL>)) + lit<"<"> +
             assign<&ast::ListType::element>(ref<FieldTypeRule>) + lit<">"> +
             option(kw<"cpp_type"> +
                    append<&ast::ListType::cppTypes>(ref<LITERAL>)) +
             AnnotationsInto<&ast::ListType::annotations>> {};

  struct VoidTypeRule : rule<create<ast::VoidType> + kw<"void">> {};
  static constexpr auto function_type = ref<FieldTypeRule> | ref<VoidTypeRule>;

  struct FieldRule
      : rule<option(assign<&ast::Field::id>(integer_value) + lit<":">) +
             option(assign<&ast::Field::req>(field_req)) +
             assign<&ast::Field::type>(ref<FieldTypeRule>) +
             option(flag<&ast::Field::isReference>(lit<"&">)) +
             assign<&ast::Field::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::Field::value>(const_value)) +
             option(flag<&ast::Field::xsdOptional>(kw<"xsd_optional">)) +
             option(flag<&ast::Field::xsdNillable>(kw<"xsd_nillable">)) +
             option(kw<"xsd_attrs"> + lit<"{"> +
                    many(append<&ast::Field::xsdAttrs>(ref<FieldRule>)) +
                    lit<"}">) +
             AnnotationsInto<&ast::Field::annotations> +
             option(list_separator)> {};

  struct ConstRefRule : rule<assign<&ast::ConstRef::name>(ref<ID>)> {};

  struct ConstListRule
      : rule<create<ast::ConstList> + lit<"["> +
             many(append<&ast::ConstList::items>(const_value) +
                  option(list_separator)) +
             lit<"]">> {};

  struct ConstMapEntryRule
      : rule<assign<&ast::ConstMapEntry::key>(const_value) + lit<":"> +
             assign<&ast::ConstMapEntry::value>(const_value) +
             option(list_separator)> {};

  struct ConstMapRule
      : rule<create<ast::ConstMap> + lit<"{"> +
             many(append<&ast::ConstMap::entries>(ref<ConstMapEntryRule>)) +
             lit<"}">> {};

  struct ConstDefRule
      : rule<kw<"const"> + assign<&ast::ConstDef::type>(ref<FieldTypeRule>) +
             assign<&ast::ConstDef::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::ConstDef::value>(const_value)) +
             option(list_separator)> {};

  struct TypedefRule
      : rule<kw<"typedef"> +
             assign<&ast::TypedefDef::type>(ref<FieldTypeRule>) +
             assign<&ast::TypedefDef::name>(ref<ID>) +
             AnnotationsInto<&ast::TypedefDef::annotations> +
             option(list_separator)> {};

  struct EnumFieldRule
      : rule<assign<&ast::EnumField::name>(ref<ID>) +
             option(lit<"="> +
                    assign<&ast::EnumField::value>(integer_value)) +
             AnnotationsInto<&ast::EnumField::annotations> +
             option(list_separator)> {};

  struct EnumDefRule
      : rule<kw<"enum"> + assign<&ast::EnumDef::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::EnumDef::fields>(ref<EnumFieldRule>)) +
             lit<"}"> + AnnotationsInto<&ast::EnumDef::annotations>> {};

  struct SenumDefRule
      : rule<kw<"senum"> + assign<&ast::SenumDef::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::SenumDef::values>(ref<LITERAL>) +
                  option(list_separator)) +
             lit<"}"> + AnnotationsInto<&ast::SenumDef::annotations>> {};

  struct StructLikeRule
      : rule<assign<&ast::StructDef::kind>(
                 ref<StructKw> | ref<UnionKw> | ref<ExceptionKw>) +
                 assign<&ast::StructDef::name>(ref<ID>) +
                 option(flag<&ast::StructDef::xsdAll>(kw<"xsd_all">)) +
                 lit<"{"> +
                 many(append<&ast::StructDef::fields>(ref<FieldRule>)) +
                 lit<"}"> + AnnotationsInto<&ast::StructDef::annotations>> {};

  struct FunctionRule
      : rule<option(flag<&ast::Function::oneway>(kw<"oneway"> |
                                                        kw<"async">)) +
                 assign<&ast::Function::returnType>(function_type) +
                 assign<&ast::Function::name>(ref<ID>) + lit<"("> +
                 many(append<&ast::Function::params>(ref<FieldRule>)) +
                 lit<")"> +
                 option(kw<"throws"> + lit<"("> +
                        many(append<&ast::Function::throws>(ref<FieldRule>)) +
                        lit<")">) +
                 AnnotationsInto<&ast::Function::annotations> +
                 option(list_separator)> {};

  struct ServiceDefRule
      : rule<kw<"service"> + assign<&ast::ServiceDef::name>(ref<ID>) +
             option(kw<"extends"> +
                    assign<&ast::ServiceDef::extends_>(ref<ID>)) +
             lit<"{"> +
             many(append<&ast::ServiceDef::functions>(ref<FunctionRule>)) +
             lit<"}"> + AnnotationsInto<&ast::ServiceDef::annotations>> {};

  struct DefinitionRule
      : rule<ref<ConstDefRule> | ref<TypedefRule> | ref<EnumDefRule> |
             ref<SenumDefRule> | ref<StructLikeRule> | ref<ServiceDefRule>> {
  };

  struct IncludeRule
      : rule<kw<"include"> + assign<&ast::Include::path>(ref<LITERAL>)> {};

  struct CppIncludeRule
      : rule<kw<"cpp_include"> +
             assign<&ast::CppInclude::path>(ref<LITERAL>)> {};

  struct STAR : terminal<lit<"*">> {};
  struct CPP_NAMESPACE_KW : terminal<kw<"cpp_namespace">> {};
  struct PHP_NAMESPACE_KW : terminal<kw<"php_namespace">> {};

  struct NamespaceRule
      : rule<(kw<"namespace"> +
                  assign<&ast::NamespaceDecl::scope>(ref<STAR> | ref<ID>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID> | ref<LITERAL>) +
                  AnnotationsInto<&ast::NamespaceDecl::annotations>) |
                 (assign<&ast::NamespaceDecl::scope>(ref<CPP_NAMESPACE_KW>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID>)) |
                 (assign<&ast::NamespaceDecl::scope>(ref<PHP_NAMESPACE_KW>) +
                  assign<&ast::NamespaceDecl::value>(ref<ID>))> {};

  struct HeaderRule
      : rule<ref<IncludeRule> | ref<NamespaceRule> | ref<CppIncludeRule>> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::headers>(ref<HeaderRule>)) +
             many(append<&ast::Document::definitions>(ref<DefinitionRule>))> {
  };

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace thrift
