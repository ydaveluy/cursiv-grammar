#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory_resource>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "graphstreamdgs/core/ast.hpp"

namespace graphstreamdgs {

inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource* resource) {
  if (text.size() < 2) return std::string_view{};
  const char delimiter = text.front();
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == std::string_view::npos &&
      text.find('\r') == std::string_view::npos)
    return text;
  auto* output = static_cast<char*>(resource->allocate(text.size() + 1));
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size() && text[i + 1] == delimiter)
      c = text[++i];
    if (c == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
      c = '\n';
    }
    output[size++] = c;
  }
  return std::string_view(output, size);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto parse_real = [](std::string_view text,
                                      std::pmr::memory_resource* resource)
    -> cursiv::ConversionResult<double> {
  if (text.size() >= 2 && (text.front() == '\"' || text.front() == '\''))
    text = unquote(text, resource);
  while (!text.empty() && static_cast<unsigned char>(text.front()) <= 0x20)
    text.remove_prefix(1);
  while (!text.empty() && static_cast<unsigned char>(text.back()) <= 0x20)
    text.remove_suffix(1);
  if (text.empty())
    return cursiv::ConversionResult<double>::err("Expected a number.");
  if (text.back() == 'f' || text.back() == 'F' || text.back() == 'd' ||
      text.back() == 'D')
    text.remove_suffix(1);
  std::string_view unsignedText = text;
  if (unsignedText.starts_with('+') || unsignedText.starts_with('-'))
    unsignedText.remove_prefix(1);
  if ((unsignedText.starts_with("0x") || unsignedText.starts_with("0X")) &&
      unsignedText.find_first_of("pP") == std::string_view::npos)
    return cursiv::ConversionResult<double>::err(
        "A hexadecimal number needs a binary exponent.");
  std::string storage(text);
  char* end = nullptr;
  const double value = std::strtod(storage.c_str(), &end);
  if (end != storage.c_str() + storage.size() || end == storage.c_str() ||
      (std::isnan(value) && unsignedText != "NaN") ||
      ((!unsignedText.empty() &&
        (unsignedText.front() == 'i' || unsignedText.front() == 'I')) &&
       unsignedText != "Infinity"))
    return cursiv::ConversionResult<double>::err("Invalid number.");
  return cursiv::ConversionResult<double>::ok(value);
};
using RealConv = cursiv::text_conv<parse_real>;

inline constexpr auto parse_color = [](std::string_view t) -> std::uint32_t {
  t.remove_prefix(1);  // drop '#'
  const auto hex = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
    return static_cast<unsigned>(c - 'A' + 10);
  };
  std::uint32_t v = 0;
  for (const char c : t) v = (v << 4) | hex(c);
  if (t.size() == 6) v = (v << 8) | 0xFFu;
  return v;
};
using ColorConv = cursiv::text_conv<parse_color>;

struct GraphstreamDgs : cursiv::GrammarSpec {
  static constexpr auto id_char = uc::Lu | uc::Ll | uc::Nd | cr<"+_.-">;
  static constexpr auto word_boundary = id_char;
  struct WS : terminal<some(cr<R"( \t)">)> {};
  static constexpr auto eol_body = lit<"\r\n"> | cr<R"(\r\n)">;
  struct EOL : terminal<eol_body> {};
  struct COMMENT
      : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + (eol_body | eof)> {};
  struct MAGIC : terminal<lit<"DGS"> + repeat<3>(d)> {};
  struct HEADER : terminal<some(cr<R"(^\r\n)">)> {};
  struct HeaderValue : rule<option(ref<HEADER>)> {};

  static constexpr auto integer = option(lit<"-">) + some(d);
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto hex = cr<"0-9a-fA-F">;
  static constexpr auto real =
      option(lit<"-">) +
      ((((some(d) + lit<"."> + many(d)) | (lit<"."> + some(d))) +
           option(exponent)) |
       (lit<"0"> + cr<"xX"> +
        ((some(hex) + lit<"."> + many(hex)) | (lit<"."> + some(hex))) +
        cr<"pP"> + option(cr<"+-">) + some(d))) +
      option(cr<"fFdD">);
  static constexpr auto true_body = cr<"tT"> + cr<"rR"> + cr<"uU"> + cr<"eE">;
  static constexpr auto false_body =
      cr<"fF"> + cr<"aA"> + cr<"lL"> + cr<"sSſ"> + cr<"eE">;
  struct INT : terminal<integer + !id_char, std::int64_t> {};
  static constexpr auto real_value = !lit<"."> + real;
  struct REAL : terminal<real_value + !id_char, RealConv> {};
  struct TRUE : terminal<true_body + !id_char, value<true>> {};
  struct FALSE : terminal<false_body + !id_char, value<false>> {};
  static constexpr auto dq =
      lit<"\""> + many(cr<R"(^"\\)"> | (some(lit<"\\">) + cr<R"(^\\)">)) +
      lit<"\"">;
  static constexpr auto sq =
      lit<"'"> + many(cr<R"(^'\\)"> | (some(lit<"\\">) + cr<R"(^\\)">)) +
      lit<"'">;
  struct STRING : terminal<dq | sq, Unquoted> {};
  struct ID : terminal<some(id_char)> {};
  struct UNSIGNED_ID : terminal<!(cr<"+-">)+some(id_char)> {};
  struct WORD_VALUE
      : terminal<!((integer | real_value | true_body | false_body) + !id_char) +
                 some(id_char)> {};
  struct STEP_VALUE : terminal<dq | sq | some(id_char), RealConv> {};
  static constexpr auto hexbyte = repeat<2>(hex);
  struct COLOR
      : terminal<lit<"#"> + repeat<3>(hexbyte) + option(hexbyte), ColorConv> {};
  struct PlusOp : terminal<lit<"+">, value<ast::AttrOp::Add>> {};
  struct MinusOp : terminal<lit<"-">, value<ast::AttrOp::Remove>> {};
  struct LeftAngle : terminal<lit<"<">, value<ast::Direction::Backward>> {};
  struct RightAngle : terminal<lit<">">, value<ast::Direction::Forward>> {};
  struct AN : terminal<cr<"aA"> + cr<"nN">> {};
  struct CN : terminal<cr<"cC"> + cr<"nN">> {};
  struct DN : terminal<cr<"dD"> + cr<"nN">> {};
  struct AE : terminal<cr<"aA"> + cr<"eE">> {};
  struct CE : terminal<cr<"cC"> + cr<"eE">> {};
  struct DE : terminal<cr<"dD"> + cr<"eE">> {};
  struct CG : terminal<cr<"cC"> + cr<"gG">> {};
  struct ST : terminal<cr<"sS"> + cr<"tT">> {};
  struct CL : terminal<cr<"cC"> + cr<"lL">> {};

  static constexpr auto identifier_body = ref<STRING> | ref<ID>;
  static constexpr auto direction_body = ref<LeftAngle> | ref<RightAngle>;
  struct ArrayRule;
  struct MapRule;
  static constexpr auto value_body =
      ref<INT> | ref<REAL> | ref<TRUE> | ref<FALSE> | ref<COLOR> |
      ref<ArrayRule> | ref<MapRule> | ref<STRING> | ref<WORD_VALUE>;
  struct ArrayRule
      : rule<lit<"{"> + many(append<&ast::Array::items>(value_body), lit<",">) +
             lit<"}">> {};
  struct MappingRule
      : rule<assign<&ast::Mapping::key>(identifier_body) +
             option((lit<"="> |
                     lit<":">)+assign<&ast::Mapping::value>(value_body))> {};
  struct MapRule
      : rule<lit<"["> +
             many(append<&ast::Map::entries>(ref<MappingRule>), lit<",">) +
             option(lit<",">) + lit<"]">> {};
  struct AttributeRule
      : rule<((assign<&ast::Attribute::op>(ref<PlusOp> | ref<MinusOp>) +
               assign<&ast::Attribute::name>(identifier_body)) |
              assign<&ast::Attribute::name>(ref<STRING> | ref<UNSIGNED_ID>)) +
             option((lit<"="> | lit<":">)+some(
                 append<&ast::Attribute::values>(value_body), lit<",">))> {};

  struct AddNodeRule
      : rule<ref<AN> + assign<&ast::AddNode::id>(identifier_body) +
             many(append<&ast::AddNode::attributes>(ref<AttributeRule>))> {};

  struct ChangeNodeRule
      : rule<ref<CN> + assign<&ast::ChangeNode::id>(identifier_body) +
             many(append<&ast::ChangeNode::attributes>(ref<AttributeRule>))> {};

  struct DelNodeRule
      : rule<ref<DN> + assign<&ast::DelNode::id>(identifier_body)> {};

  struct AddEdgeRule
      : rule<ref<AE> + assign<&ast::AddEdge::id>(identifier_body) +
             assign<&ast::AddEdge::source>(identifier_body) +
             option(assign<&ast::AddEdge::direction>(direction_body)) +
             assign<&ast::AddEdge::target>(identifier_body) +
             many(append<&ast::AddEdge::attributes>(ref<AttributeRule>))> {};

  struct ChangeEdgeRule
      : rule<ref<CE> + assign<&ast::ChangeEdge::id>(identifier_body) +
             many(append<&ast::ChangeEdge::attributes>(ref<AttributeRule>))> {};

  struct DelEdgeRule
      : rule<ref<DE> + assign<&ast::DelEdge::id>(identifier_body)> {};

  struct ChangeGraphRule
      : rule<ref<CG> +
             many(append<&ast::ChangeGraph::attributes>(ref<AttributeRule>))> {
  };

  struct StepRule : rule<ref<ST> + assign<&ast::Step::time>(ref<STEP_VALUE>)> {
  };

  struct ClearRule : rule<create<ast::Clear> + ref<CL>> {};

  struct EventRule
      : rule<(ref<AddNodeRule> | ref<ChangeNodeRule> | ref<DelNodeRule> |
              ref<AddEdgeRule> | ref<ChangeEdgeRule> | ref<DelEdgeRule> |
              ref<ChangeGraphRule> | ref<StepRule> |
              ref<ClearRule>)+(ref<COMMENT> | ref<EOL> | eof)> {};
  struct DocumentRule
      : rule<assign<&ast::Document::magic>(ref<MAGIC>) + ref<EOL> +
             tight(assign<&ast::Document::header>(ref<HeaderValue>) +
                      (ref<EOL> | eof)) +
             many(append<&ast::Document::events>(ref<EventRule>) |
                  ref<COMMENT> | ref<EOL>)> {};
  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace graphstreamdgs
