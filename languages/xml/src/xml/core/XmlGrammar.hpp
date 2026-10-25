// The xml language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xml/core/ast.hpp"

namespace xml {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto uncdata = [](std::string_view t) {
  return t.substr(9, t.size() - 12);  // strip "<![CDATA[" and "]]>"
};
using Uncdata = cursiv::text_conv<uncdata>;

struct Xml : cursiv::GrammarSpec {
  // XML's own NameStartChar and NameChar code-point ranges.
  static constexpr auto NameStart = cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-󯿿:">;
  static constexpr auto NameCont = NameStart | cr<"0-9.·̀-ͯ‿-⁀-">;
  static constexpr auto NameBody = NameStart + many(NameCont);

  struct Name : terminal<NameBody> {};

  // "<!" also opens a comment ("<!--") or a CDATA section ("<!["); the guard
  // leaves those two to their own terminals.
  static constexpr auto DtdQuoted = through(lit<"\"">, lit<"\"">) | through(lit<"'">, lit<"'">);
  static constexpr auto DtdSubset =
      lit<"["> + many(DtdQuoted | through(lit<"<!--">, lit<"-->">) |
                      through(lit<"<?">, lit<"?>">) | cr<"^]\"'">) + lit<"]">;
  struct DTD : terminal<lit<"<!", !(lit<"--"> | lit<"[CDATA[">)> + many(DtdQuoted | DtdSubset | cr<"^>[\"'">) + lit<">">> {};

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"<!--">, lit<"-->">)> {};
  struct CDATA : terminal<through(lit<"<![CDATA[">, lit<"]]>">), Uncdata> {};

  // "<?xml" followed by whitespace opens the declaration (below), not a
  // generic PI -- a choice of words, one per byte `S` may be in the source.
  struct PI
      : terminal<through(lit<"<?", !(lit<"xml "> | lit<"xml\t"> | lit<"xml\r"> |
                             lit<"xml\n">)>, lit<"?>">)> {};

  struct EntityRef : terminal<lit<"&"> + NameBody + lit<";">> {};
  struct CharRef : terminal<(lit<"&#"> + some(d) + lit<";">) |
                            (lit<"&#x"> + some(cr<"a-fA-F0-9">) + lit<";">)> {
  };

  struct TEXT : terminal<some(cr<"^<&">)> {};

  struct STRING : terminal<(lit<"\""> + many(cr<R"(^<")">) + lit<"\"">) |
                                (lit<"'"> + many(cr<"^<'">) + lit<"'">),
                           Unquoted> {};

  struct AttributeRule
      : rule<assign<&ast::Attribute::name>(ref<Name>) + lit<"="> +
             assign<&ast::Attribute::value>(ref<STRING>)> {};

  struct DeclRule
      : rule<lit<"<?xml"> + !gap::none +
             many(append<&ast::Declaration::attributes>(ref<AttributeRule>)) +
             lit<"?>">> {};

  struct ElementRule;

  static constexpr auto ChildValue = ref<ElementRule> | ref<TEXT> |
                                     ref<CDATA> | ref<EntityRef> |
                                     ref<CharRef>;
  static constexpr auto ChildItem =
      append<&ast::Element::children>(ChildValue) | ref<COMMENT> | ref<PI>;

  static constexpr auto TagHead =
      lit<"<"> + assign<&ast::Element::name>(ref<Name>) +
      many(append<&ast::Element::attributes>(ref<AttributeRule>));

  struct ElementRule
      : rule<(TagHead + lit<"/>">) |
             (TagHead + lit<">"> + many(ChildItem) + lit<"</"> +
              assign<&ast::Element::closeName>(ref<Name>) + lit<">">)> {};

  static constexpr auto Misc = ref<COMMENT> | ref<PI>;

  struct DocRule
      : rule<option(lit<"\xEF\xBB\xBF">) +
             option(assign<&ast::Document::declaration>(ref<DeclRule>)) +
             many(Misc | append<&ast::Document::doctypes>(ref<DTD>)) +
             assign<&ast::Document::root>(ref<ElementRule>) +
             many(Misc)> {};

  using entry = DocRule;
  using ignore = WS;
};

}  // namespace xml
