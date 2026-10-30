#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pbm/core/ast.hpp"
namespace pbm {
struct Pbm : cursiv::GrammarSpec {
  struct PLAIN : terminal<lit<"P1">> {};
  struct BINARY : terminal<lit<"P4">> {};
  struct OTHER_MAGIC : terminal<lit<"P"> + cr<"0235-9">> {};
  struct NUMBER : terminal<some(cr<"0-9">)> {};
  struct SPACE : terminal<some(cr<R"( \t\r\n)">)> {};
  struct FRAME_SPACE : terminal<some(cr<R"( \t\r\n\f\v)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + cr<R"(\r\n)">> {};
  struct SEPARATOR : terminal<ref<COMMENT> | byte> {};
  struct RASTER : terminal<some(byte)> {};
  struct TRAILING : terminal<!(lit<"P"> + cr<"0-9">)+!cr<R"(01 \t\r\n\f\v)"> +
                             !ref<COMMENT> + some(byte)> {};
  struct BIT : terminal<as<[](std::string_view text) { return text == "1"; }>(
                   cr<"01">)> {};
  static constexpr auto gap = many(ref<SPACE> | ref<COMMENT>);
  static constexpr auto header = gap + assign<&ast::Image::width>(ref<NUMBER>) +
                                 ref<SEPARATOR> + gap +
                                 assign<&ast::Image::height>(ref<NUMBER>) +
                                 assign<&ast::Image::separator>(ref<SEPARATOR>);
  struct PlainImage : rule<assign<&ast::Image::magic>(ref<PLAIN>) + header +
                           many(ref<SPACE> | ref<COMMENT> |
                                append<&ast::Image::table>(ref<BIT>))> {};
  struct BinaryImage
      : rule<assign<&ast::Image::magic>(ref<BINARY> | ref<OTHER_MAGIC>) +
             header + option(assign<&ast::Image::raster>(ref<RASTER>))> {};
  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::images>(ref<PlainImage> |
                                                 ref<BinaryImage>) +
                  option(ref<FRAME_SPACE>)) +
             option(assign<&ast::Document::trailing>(ref<TRAILING>))> {};
  using entry = Document;
};
}  // namespace pbm
