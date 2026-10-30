// The xml language: its grammar.
#pragma once

#include <string_view>
#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xml/core/ast.hpp"

namespace xml {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto uncdata = [](std::string_view t) {
  return t.substr(9, t.size() - 12);  // strip "<![CDATA[" and "]]>"
};

inline constexpr bool valid_reference(std::string_view text) {
  const bool hex = text.starts_with("&#x");
  const unsigned base = hex ? 16 : 10;
  std::uint32_t value = 0;
  for (char c : text.substr(hex ? 3 : 2, text.size() - (hex ? 4 : 3))) {
    const unsigned digit = c >= '0' && c <= '9' ? c - '0'
                           : c >= 'a' && c <= 'f' ? c - 'a' + 10
                           : c >= 'A' && c <= 'F' ? c - 'A' + 10 : base;
    if (digit >= base || value > (0x10ffffu - digit) / base) return false;
    value = value * base + digit;
  }
  return value == 9 || value == 10 || value == 13 ||
         (value >= 0x20 && value <= 0xd7ff) ||
         (value >= 0xe000 && value <= 0xfffd) ||
         (value >= 0x10000 && value <= 0x10ffff);
}

inline constexpr auto reference_value = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  return valid_reference(text) ? Result::ok(text)
                              : Result::err("Invalid XML character reference.");
};

inline constexpr auto attribute_value = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto body = unquote(text);
  for (auto at = body.find("&#"); at != body.npos; at = body.find("&#", at + 1)) {
    const auto end = body.find(';', at);
    if (end == body.npos || !valid_reference(body.substr(at, end - at + 1)))
      return Result::err("Invalid XML character reference in attribute.");
  }
  return Result::ok(body);
};

struct Xml : cursiv::GrammarSpec {
  static constexpr auto Char =
      cr<R"(\t\n\r\u{20}-\u{D7FF}\u{E000}-\u{FFFD}\u{10000}-\u{10FFFF})">;
  static constexpr auto Space = cr<R"( \t\r\n)">;
  template<cursiv::str End>
  static constexpr auto until_end = many(!lit<End> + Char) + lit<End>;
  // XML's own NameStartChar and NameChar code-point ranges.
  static constexpr auto NameStart = cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-󯿿:">;
  static constexpr auto NameCont = NameStart | cr<"0-9.·̀-ͯ‿-⁀-">;
  static constexpr auto NameBody = NameStart + many(NameCont);

  struct Name : terminal<NameBody> {};

  static constexpr auto DtdQuoted =
      (lit<"\""> + until_end<"\"">) | (lit<"'"> + until_end<"'">);
  static constexpr auto DtdSubset =
      lit<"["> + many(DtdQuoted | (lit<"<!--"> + until_end<"-->">) |
                      (lit<"<?"> + until_end<"?>">) |
                      (!cr<"]\"'"> + Char)) + lit<"]">;
  static constexpr auto PubidChar = cr<R"( \r\na-zA-Z0-9'()+,./:=?;!*#@$_%-)">;
  static constexpr auto PubidLiteral =
      (lit<"\""> + many(PubidChar) + lit<"\"">) |
      (lit<"'"> + many(!lit<"'"> + PubidChar) + lit<"'">);
  static constexpr auto ExternalId =
      (lit<"SYSTEM"> + some(Space) + DtdQuoted) |
      (lit<"PUBLIC"> + some(Space) + PubidLiteral + some(Space) + DtdQuoted);
  struct DTD : terminal<lit<"<!DOCTYPE"> + some(Space) + NameBody +
                        option(some(Space) + ExternalId) + many(Space) +
                        option(DtdSubset + many(Space)) + lit<">">> {};

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT
      : terminal<lit<"<!--"> + many(!lit<"--"> + Char) + lit<"-->">> {};
  struct CDATA : terminal<as<uncdata>(lit<"<![CDATA["> + until_end<"]]>">)> {
  };

  // The target is an XML Name other than "xml", in any letter case.
  static constexpr auto xml_target =
      cr<"xX"> + cr<"mM"> + cr<"lL"> + !NameCont;
  struct PI
      : terminal<lit<"<?"> + !xml_target + NameBody +
                 (lit<"?>"> | (some(Space) + until_end<"?>">))> {};

  struct EntityRef : terminal<lit<"&"> + NameBody + lit<";">> {};
  static constexpr auto char_reference =
      (lit<"&#"> + some(d) + lit<";">) |
      (lit<"&#x"> + some(cr<"a-fA-F0-9">) + lit<";">);
  struct CharRef : terminal<as<reference_value>(char_reference)> {};

  struct TEXT : terminal<some(!lit<"]]>"> + !cr<"<&"> + Char)> {};

  struct STRING
      : terminal<as<attribute_value>(
            (lit<"\""> + many(ref<EntityRef> | char_reference |
                              (!cr<R"(<&")"> + Char)) + lit<"\"">) |
            (lit<"'"> + many(ref<EntityRef> | char_reference |
                             (!cr<"<&'"> + Char)) + lit<"'">))> {};

  struct Attribute
      : rule<assign<&ast::Attribute::name>(ref<Name>) + lit<"="> +
             assign<&ast::Attribute::value>(ref<STRING>)> {};

  struct Decl
      : rule<lit<"<?xml"> +
             some(!gap::none + append<&ast::Declaration::attributes>(ref<Attribute>)) +
             lit<"?>">> {};

  struct Element;

  static constexpr auto ChildValue = ref<Element> | ref<TEXT> |
                                     ref<CDATA> | ref<EntityRef> |
                                     ref<CharRef>;
  static constexpr auto ChildItem =
      append<&ast::Element::children>(ChildValue) | ref<COMMENT> | ref<PI>;

  static constexpr auto TagHead =
      lit<"<"> + gap::none + assign<&ast::Element::name>(ref<Name>) +
      many(!gap::none + append<&ast::Element::attributes>(ref<Attribute>));

  struct Element
      : rule<TagHead +
             (lit<"/>"> | (lit<">"> + many(ChildItem) + lit<"</"> + gap::none +
              assign<&ast::Element::closeName>(ref<Name>) + lit<">">))> {};

  static constexpr auto Misc = ref<COMMENT> | ref<PI>;

  struct Doc
      : rule<option(lit<"\xEF\xBB\xBF">) +
             option(gap::none + assign<&ast::Document::declaration>(ref<Decl>)) +
             many(Misc) +
             option(append<&ast::Document::doctypes>(ref<DTD>) + many(Misc)) +
             assign<&ast::Document::root>(ref<Element>) +
             many(Misc)> {};

  using entry = Doc;
  using ignored = WS;
};

}  // namespace xml
