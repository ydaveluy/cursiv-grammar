#pragma once
#include "bdf/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bdf {
struct Bdf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<eol> {};
  struct ARG : terminal<some(cr<R"(^ \t\r\n)">)> {};
  struct TEXT : terminal<cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">)> {};
  struct COMMENT : terminal<kw<"COMMENT"> + many(cr<R"(^\r\n)">)> {};
  struct HASH_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct Property
      : rule<assign<&ast::Property::name>(ref<ARG>) +
             option(assign<&ast::Property::value>(ref<TEXT>)) + ref<EOL>> {};
  struct Properties
      : rule<kw<"STARTPROPERTIES"> + assign<&ast::Properties::count>(ref<ARG>) + ref<EOL> +
             many(ref<EOL> | append<&ast::Properties::properties>(ref<Property>)) +
             kw<"ENDPROPERTIES"> + ref<EOL>> {};
  static constexpr auto GlyphMetricKeyword =
      kw<"ENCODING"> | kw<"SWIDTH"> | kw<"DWIDTH"> | kw<"SWIDTH1"> |
      kw<"DWIDTH1"> | kw<"VVECTOR"> | kw<"BBX">;
  static constexpr auto SharedMetricKeyword =
      kw<"SWIDTH"> | kw<"DWIDTH"> | kw<"SWIDTH1"> | kw<"DWIDTH1"> | kw<"VVECTOR">;
  struct ROW
      : terminal<!(kw<"ENDCHAR"> | GlyphMetricKeyword | kw<"BITMAP">) +
                 cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">)> {};
  struct Bitmap
      : rule<kw<"BITMAP"> + ref<EOL> +
             many(append<&ast::Bitmap::rows>(ref<ROW>) + ref<EOL>)> {};
  struct GlyphMetric
      : rule<create<ast::Directive>(
            assign<&ast::Directive::keyword>(GlyphMetricKeyword) +
            many(append<&ast::Directive::arguments>(ref<ARG>)) + ref<EOL>)> {};
  struct Char
      : rule<kw<"STARTCHAR"> + assign<&ast::Char::name>(ref<TEXT>) + ref<EOL> +
             many(ref<EOL> | append<&ast::Char::parts>(ref<GlyphMetric> | ref<Bitmap>)) +
             kw<"ENDCHAR"> + ref<EOL>> {};
  struct HeaderMetric
      : rule<create<ast::Directive>(
            assign<&ast::Directive::keyword>(kw<"SIZE"> | kw<"FONTBOUNDINGBOX"> |
                                             kw<"CHARS"> | kw<"CONTENTVERSION"> |
                                             kw<"METRICSSET"> | SharedMetricKeyword) +
            many(append<&ast::Directive::arguments>(ref<ARG>)) + ref<EOL>)> {};
  struct FontName
      : rule<create<ast::Directive>(assign<&ast::Directive::keyword>(kw<"FONT">) +
                                    option(assign<&ast::Directive::text>(ref<TEXT>)) + ref<EOL>)> {};
  struct Font
      : rule<create<ast::Font>(many(ref<EOL>) + kw<"STARTFONT"> +
            assign<&ast::Font::version>(ref<ARG>) + ref<EOL> +
            many(ref<EOL> | append<&ast::Font::parts>(ref<FontName> | ref<HeaderMetric> |
                                                    ref<Properties> | ref<Char>)) +
            kw<"ENDFONT"> + many(ref<EOL>))> {};
  static constexpr auto word_boundary = cr<R"(^ \t\r\n)">;
  using entry = Font;
  using ignored = WS;
  using hidden = list<COMMENT, HASH_COMMENT>;
};
}  // namespace bdf
