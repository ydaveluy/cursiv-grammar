#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ply/core/ast.hpp"
namespace ply {
struct Ply : cursiv::GrammarSpec {
  static constexpr auto horizontal = cr<R"( \t\v\f\x1c-\x1f)">;
  struct WS : terminal<some(horizontal)> {};
  struct EOL : terminal<lit<"\r\n"> | lit<"\n"> | lit<"\r">> {};
  struct WORD : terminal<some(cr<R"(^ \t\r\n\v\f\x1c-\x1f)">)> {};
  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct BODY : terminal<some(byte)> {};
  struct END_HEADER : terminal<many(horizontal) + lit<"end_header"> +
                               many(horizontal) + option(ref<EOL>)> {};
  struct COMMENT : terminal<kw<"comment">> {};
  struct OBJ_INFO : terminal<kw<"obj_info">> {};
  struct Format
      : rule<create<ast::Format> + lit<"format"> + ref<WS> +
             assign<&ast::Format::encoding>(ref<WORD>) + ref<WS> +
             assign<&ast::Format::version>(ref<WORD>) +
             option(assign<&ast::Format::extra>(ref<TEXT>)) + ref<EOL>> {};
  struct Element
      : rule<create<ast::Element> + lit<"element"> + ref<WS> +
             assign<&ast::Element::name>(ref<WORD>) + ref<WS> +
             assign<&ast::Element::count>(ref<WORD>) +
             option(assign<&ast::Element::extra>(ref<TEXT>)) + ref<EOL>> {};
  struct Property
      : rule<create<ast::Property> + lit<"property"> + ref<WS> +
             ((flag<&ast::Property::list>(kw<"list">) + ref<WS> +
               assign<&ast::Property::countType>(ref<WORD>) + ref<WS> +
               assign<&ast::Property::type>(ref<WORD>) + ref<WS> +
               assign<&ast::Property::name>(ref<WORD>)) |
              (assign<&ast::Property::type>(ref<WORD>) + ref<WS> +
               assign<&ast::Property::name>(ref<WORD>))) +
             option(assign<&ast::Property::extra>(ref<TEXT>)) + ref<EOL>> {};
  struct Note
      : rule<create<ast::Note> +
             assign<&ast::Note::kind>(ref<COMMENT> | ref<OBJ_INFO>) +
             option(assign<&ast::Note::text>(ref<TEXT>)) + ref<EOL>> {};
  struct Unknown
      : rule<create<ast::Unknown> + assign<&ast::Unknown::keyword>(ref<WORD>) +
             option(assign<&ast::Unknown::text>(ref<TEXT>)) + ref<EOL>> {};
  struct Blank : rule<create<ast::Unknown> + ref<EOL>> {};
  struct Header
      : rule<option(ref<WS>) + (ref<Format> | ref<Element> |
                                ref<Property> | ref<Note> |
                                ref<Unknown> | ref<Blank>)> {};
  struct Document
      : rule<create<ast::Document> + assign<&ast::Document::magic>(ref<TEXT>) +
             ref<EOL> + many(append<&ast::Document::header>(ref<Header>)) +
             assign<&ast::Document::endHeader>(ref<END_HEADER>) +
             option(assign<&ast::Document::body>(ref<BODY>)) + eof> {};
  using entry = Document;
};
} // namespace ply
