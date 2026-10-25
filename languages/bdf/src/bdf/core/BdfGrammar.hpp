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
  static constexpr auto LineEnd = ref<EOL> | eof;
  struct PropertyRule
      : rule<assign<&ast::Property::name>(ref<ARG>) +
             option(assign<&ast::Property::value>(ref<TEXT>)) + LineEnd> {};
  struct PropertiesRule
      : rule<kw<"STARTPROPERTIES"> + assign<&ast::Properties::count>(ref<ARG>) + LineEnd +
             many(ref<EOL> | append<&ast::Properties::properties>(ref<PropertyRule>)) +
             kw<"ENDPROPERTIES"> + LineEnd> {};
  struct ROW
      : terminal<!(kw<"ENDCHAR"> | kw<"ENCODING"> | kw<"SWIDTH"> | kw<"DWIDTH"> |
                   kw<"BBX"> | kw<"BITMAP">) +
                 cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">)> {};
  struct BitmapRule
      : rule<kw<"BITMAP"> + LineEnd +
             many(append<&ast::Bitmap::rows>(ref<ROW>) + LineEnd)> {};
  struct GlyphMetric
      : rule<create<ast::Directive>(
            assign<&ast::Directive::keyword>(kw<"ENCODING"> | kw<"SWIDTH"> |
                                             kw<"DWIDTH"> | kw<"BBX">) +
            many(append<&ast::Directive::arguments>(ref<ARG>)) + LineEnd)> {};
  struct CharRule
      : rule<kw<"STARTCHAR"> + assign<&ast::Char::name>(ref<TEXT>) + LineEnd +
             many(ref<EOL> | append<&ast::Char::parts>(ref<GlyphMetric> | ref<BitmapRule>)) +
             kw<"ENDCHAR"> + LineEnd> {};
  struct HeaderMetric
      : rule<create<ast::Directive>(
            assign<&ast::Directive::keyword>(kw<"SIZE"> | kw<"FONTBOUNDINGBOX"> | kw<"CHARS">) +
            many(append<&ast::Directive::arguments>(ref<ARG>)) + LineEnd)> {};
  struct FontName
      : rule<create<ast::Directive>(assign<&ast::Directive::keyword>(kw<"FONT">) +
                                    option(assign<&ast::Directive::text>(ref<TEXT>)) + LineEnd)> {};
  struct FontRule
      : rule<create<ast::Font>(many(ref<EOL>) + kw<"STARTFONT"> +
            assign<&ast::Font::version>(ref<ARG>) + LineEnd +
            many(ref<EOL> | append<&ast::Font::parts>(ref<FontName> | ref<HeaderMetric> |
                                                    ref<PropertiesRule> | ref<CharRule>)) +
            kw<"ENDFONT"> + many(ref<EOL>))> {};
  static constexpr auto word_boundary = cr<R"(^ \t\r\n)">;
  using entry = FontRule;
  using ignore = WS;
  using hidden = list<COMMENT, HASH_COMMENT>;
};
}  // namespace bdf
