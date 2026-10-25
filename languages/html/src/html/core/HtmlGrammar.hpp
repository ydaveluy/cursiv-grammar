#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "html/core/ast.hpp"

namespace html {

inline std::string_view normalized_text(std::string_view text,
                                         std::pmr::memory_resource* memory) {
  if (text.find('\r') == text.npos) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
      room[n++] = '\n';
    } else room[n++] = text[i];
  }
  return {room.data(), n};
}
inline std::string_view without_initial_newline(std::string_view text) {
  if (text.starts_with("\r\n")) text.remove_prefix(2);
  else if (text.starts_with('\r') || text.starts_with('\n')) text.remove_prefix(1);
  return text;
}
template <cursiv::str Name>
inline constexpr auto raw_text_value = [](std::string_view text,
                                            std::pmr::memory_resource* memory) {
  if constexpr (Name.view() == "textarea") text = without_initial_newline(text);
  return normalized_text(text, memory);
};
inline constexpr auto pre_text_value = [](std::string_view text,
                                           std::pmr::memory_resource* memory) {
  return normalized_text(without_initial_newline(text), memory);
};
inline constexpr auto text_value = [](std::string_view text,
                                        std::pmr::memory_resource* memory) {
  return normalized_text(text, memory);
};

inline constexpr auto unquote_attr =
    [](std::string_view t, std::pmr::memory_resource* memory) -> std::string_view {
  if (t.size() >= 2 && (t.front() == '"' || t.front() == '\'') &&
      t.back() == t.front())
    t = t.substr(1, t.size() - 2);
  return normalized_text(t, memory);
};
using AttrValue = cursiv::text_conv<unquote_attr>;
inline constexpr auto open_name = [](std::string_view text) { return text.substr(1); };
using OpenName = cursiv::text_conv<open_name>;

struct Html : cursiv::GrammarSpec {
  struct SEA_WS
      : terminal<some(cr<R"( \t\r\n\f)">)> {
  };
  static constexpr auto spaces = many(ref<SEA_WS>);

  struct HTML_COMMENT : terminal<through(lit<"<!--">, lit<"-->">)> {};
  struct HTML_CONDITIONAL_COMMENT
      : terminal<through(lit<"<![", !lit<"CDATA[">>, lit<"]>">)> {};
  struct XML : terminal<through(lit<"<?xml">, lit<">">)> {};
  struct CDATA : terminal<through(lit<"<![CDATA[">, lit<"]]>">)> {};
  struct DTD : terminal<through(lit<"<!", !(lit<"--"> | lit<"[">)>, lit<">">)> {};
  struct SCRIPTLET : terminal<through(lit<"<?", !lit<"xml">>, lit<"?>">) |
                              through(lit<"<%">, lit<"%>">)> {};

  static constexpr auto RawTagBoundary = cr<R"( \t\r\n\f/>)">;
  static constexpr auto RawTagTail =
      many(through(lit<"\"">, lit<"\"">) | through(lit<"'">, lit<"'">) |
           cr<"^>\"'">) + lit<">">;
  static constexpr auto ScriptOpen =
      lit<"<"> + ikw<"script"> + &RawTagBoundary + RawTagTail;
  static constexpr auto StyleOpen =
      lit<"<"> + ikw<"style"> + &RawTagBoundary + RawTagTail;
  static constexpr auto ScriptClose =
      (lit<"</"> + ikw<"script"> + &RawTagBoundary + RawTagTail) | lit<"</>">;
  static constexpr auto StyleClose =
      (lit<"</"> + ikw<"style"> + &RawTagBoundary + RawTagTail) | lit<"</>">;
  static constexpr auto DoubleScriptOpen = lit<"<"> + ikw<"script"> + &RawTagBoundary;
  static constexpr auto DoubleScriptClose = lit<"</"> + ikw<"script"> + &RawTagBoundary;
  static constexpr auto ScriptText = many(!(ScriptClose | lit<"<!--">) + any);
  static constexpr auto EscapedText = many(!(ScriptClose | lit<"-->"> | DoubleScriptOpen) + any);
  static constexpr auto DoubleText = many(!(DoubleScriptClose | lit<"-->">) + any);
  static constexpr auto DoubleReturn = DoubleScriptOpen + DoubleText + DoubleScriptClose;
  static constexpr auto EscapedPrefix = many(EscapedText + DoubleReturn) + EscapedText;
  static constexpr auto DataReturn = lit<"-->"> | (DoubleScriptOpen + DoubleText + lit<"-->">);
  static constexpr auto ScriptCycle = ScriptText + lit<"<!--"> + EscapedPrefix + DataReturn;
  struct SCRIPT
      : terminal<ScriptOpen + many(ScriptCycle) + ScriptText +
                 (ScriptClose | (lit<"<!--"> + EscapedPrefix + ScriptClose))> {};
  struct STYLE
      : terminal<StyleOpen + many(!StyleClose + any) + StyleClose> {};

  struct TAG_OPEN : terminal<lit<"<">> {};
  struct TAG_CLOSE : terminal<lit<">">> {};
  struct TAG_SLASH_CLOSE : terminal<lit<"/>">> {};
  struct TAG_SLASH : terminal<lit<"/">> {};
  struct TAG_EQUALS : terminal<lit<"=">> {};

  static constexpr auto tag_name_start = cr<":a-zA-Z⁰-↏Ⰰ-⿯"
                                            "、-퟿豈-﷏"
                                            "ﷰ-�">;
  static constexpr auto tag_name_cont = cr<R"(^ \t\r\n\f/>)">;
  struct TAG_NAME : terminal<tag_name_start + many(tag_name_cont)> {};
  struct ATTRIBUTE_NAME : terminal<some(cr<R"(^ \t\r\n\f/>=)">)> {};

  static constexpr auto void_names =
      ikw<"area"> | ikw<"base"> | ikw<"br"> | ikw<"col"> |
      ikw<"embed"> | ikw<"hr"> | ikw<"img"> | ikw<"input"> |
      ikw<"link"> | ikw<"meta"> | ikw<"param"> | ikw<"source"> |
      ikw<"track"> | ikw<"wbr">;
  struct VOID_OPEN : terminal<lit<"<"> + void_names + &RawTagBoundary, OpenName> {};
  static constexpr auto specialized_name =
      (void_names |
       ikw<"script"> | ikw<"style"> | ikw<"textarea"> | ikw<"title"> |
       ikw<"xmp"> | ikw<"iframe"> | ikw<"noembed"> | ikw<"noframes"> | ikw<"plaintext"> |
       ikw<"pre"> | ikw<"listing"> |
       ikw<"html"> | ikw<"head"> | ikw<"body"> |
       ikw<"li"> | ikw<"p"> | ikw<"dt"> | ikw<"dd"> | ikw<"rt"> | ikw<"rp"> |
       ikw<"option"> | ikw<"optgroup"> | ikw<"thead"> | ikw<"tbody"> |
       ikw<"tfoot"> | ikw<"tr"> | ikw<"td"> | ikw<"th">) + &RawTagBoundary;
  struct NORMAL_OPEN
      : terminal<lit<"<"> + !specialized_name + tag_name_start + many(tag_name_cont), OpenName> {};

  static constexpr auto text_char = cr<"^<"> | (lit<"<"> + !(cr<"A-Za-z!?/%"> | tag_name_start));
  struct HTML_TEXT : terminal<some(text_char), text_conv<text_value>> {};

  static constexpr auto dq_string = lit<"\""> + many(cr<R"(^")">) + lit<"\"">;
  static constexpr auto sq_string = lit<"'"> + many(cr<"^'">) + lit<"'">;
  static constexpr auto unquoted_value =
      cr<R"(^ \t\r\n\f>"')"> + many(cr<R"(^ \t\r\n\f>)">);
  struct ATTVALUE_VALUE
      : terminal<dq_string | sq_string | unquoted_value,
                 AttrValue> {};

  struct AttributeRule
      : rule<assign<&ast::Attribute::name>(ref<ATTRIBUTE_NAME>) + spaces +
             option(ref<TAG_EQUALS> + spaces +
                    assign<&ast::Attribute::value>(ref<ATTVALUE_VALUE>) + spaces)> {};
  static constexpr auto attributes = spaces +
      many(append<&ast::Element::attributes>(ref<AttributeRule>));

  struct CommentRule : rule<assign<&ast::Comment::text>(
                           ref<HTML_COMMENT> | ref<HTML_CONDITIONAL_COMMENT>)> {
  };
  struct CDataRule : rule<assign<&ast::CData::text>(ref<CDATA>)> {};
  struct ScriptletRule : rule<assign<&ast::Scriptlet::text>(ref<SCRIPTLET>)> {};
  struct ScriptRule
      : rule<assign<&ast::Script::text>(ref<SCRIPT>)> {};
  struct StyleRule
      : rule<assign<&ast::Style::text>(ref<STYLE>)> {};
  struct TextRule : rule<assign<&ast::Text::text>(ref<HTML_TEXT>)> {};

  struct ElementRule;
  template <cursiv::str Name>
  struct RawOpen : terminal<lit<"<"> + ikw<Name> + &RawTagBoundary, OpenName> {};
  template <cursiv::str Name>
  struct RawBody : terminal<some(!(lit<"</"> + ikw<Name> + &RawTagBoundary) + any),
                            text_conv<raw_text_value<Name>>> {};
  template <cursiv::str Name>
  struct RawTextRule : rule<assign<&ast::Text::text>(ref<RawBody<Name>>)> {};
  template <cursiv::str Name> struct RawElementRule;
  template <cursiv::str Name> struct OptionalEndRule;
  template <cursiv::str Name> struct PreElementRule;
  struct PlaintextRule;

  static constexpr auto void_open_tag =
      assign<&ast::Element::name>(ref<VOID_OPEN>) +
      attributes;

  struct VoidElementRule
      : rule<void_open_tag +
             (ref<TAG_CLOSE> |
              flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>))> {};

  static constexpr auto content_item = ref<VoidElementRule> | ref<RawElementRule<"textarea">> |
                                       ref<RawElementRule<"title">> | ref<RawElementRule<"xmp">> |
                                       ref<RawElementRule<"iframe">> | ref<RawElementRule<"noembed">> |
                                       ref<RawElementRule<"noframes">> | ref<PlaintextRule> |
                                       ref<PreElementRule<"pre">> | ref<PreElementRule<"listing">> |
                                       ref<OptionalEndRule<"html">> | ref<OptionalEndRule<"head">> |
                                       ref<OptionalEndRule<"body">> |
                                       ref<OptionalEndRule<"li">> | ref<OptionalEndRule<"p">> |
                                       ref<OptionalEndRule<"dt">> | ref<OptionalEndRule<"dd">> |
                                       ref<OptionalEndRule<"rt">> | ref<OptionalEndRule<"rp">> |
                                       ref<OptionalEndRule<"option">> | ref<OptionalEndRule<"optgroup">> |
                                       ref<OptionalEndRule<"thead">> | ref<OptionalEndRule<"tbody">> |
                                       ref<OptionalEndRule<"tfoot">> | ref<OptionalEndRule<"tr">> |
                                       ref<OptionalEndRule<"td">> | ref<OptionalEndRule<"th">> | ref<ElementRule> |
                                       ref<ScriptRule> | ref<StyleRule> |
                                       ref<ScriptletRule> | ref<CDataRule> |
                                       ref<CommentRule> | ref<TextRule>;

  static constexpr auto open_tag_prefix =
      assign<&ast::Element::name>(ref<NORMAL_OPEN>) +
      attributes;

  static constexpr auto close_tag =
      ref<TAG_OPEN> + ref<TAG_SLASH> +
      assign<&ast::Element::closeName>(ref<TAG_NAME>) +
      spaces + ref<TAG_CLOSE>;
  static constexpr auto close_start = ref<TAG_OPEN> + ref<TAG_SLASH>;
  template <cursiv::str Name>
  static constexpr auto matching_close = close_start + ikw<Name> + &RawTagBoundary;
  struct PreFirstTextValue : rule<ref<HTML_TEXT>, text_conv<pre_text_value>> {};
  struct PreFirstTextRule : rule<assign<&ast::Text::text>(ref<PreFirstTextValue>)> {};

  template <cursiv::str Name>
  struct RawElementRule
      : rule<assign<&ast::Element::name>(ref<RawOpen<Name>>) +
             attributes +
             (flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>) |
              (ref<TAG_CLOSE> +
               option(append<&ast::Element::content>(ref<RawTextRule<Name>>)) +
               ((&matching_close<Name> + close_tag) | !matching_close<Name>)))> {};

  template <cursiv::str Name>
  struct PreElementRule
      : rule<assign<&ast::Element::name>(ref<RawOpen<Name>>) + attributes +
             (flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>) |
              (ref<TAG_CLOSE> +
               option(append<&ast::Element::content>(ref<PreFirstTextRule>)) +
               many(append<&ast::Element::content>(content_item)) + close_tag))> {};

  struct PLAIN_TEXT : terminal<some(any), text_conv<text_value>> {};
  struct PlainTextRule : rule<assign<&ast::Text::text>(ref<PLAIN_TEXT>)> {};
  struct PlaintextRule
      : rule<assign<&ast::Element::name>(ref<RawOpen<"plaintext">>) +
             attributes +
             (flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>) |
              (ref<TAG_CLOSE> + option(append<&ast::Element::content>(ref<PlainTextRule>))))> {};

  template <cursiv::str Name>
  static constexpr auto implied_end_text = [] {
    if constexpr (Name.view() == "head")
      return (lit<"<"> + !(lit<"!--"> |
          ((ikw<"base"> | ikw<"basefont"> | ikw<"bgsound"> | ikw<"link"> |
            ikw<"meta"> | ikw<"noframes"> | ikw<"script"> | ikw<"style"> |
            ikw<"template"> | ikw<"title">) + &RawTagBoundary)) +
          tag_name_start + many(tag_name_cont)) |
          (lit<"<"> + &lit<"/">) |
          (lit<"<"> + !(cr<"A-Za-z!?/%"> | tag_name_start) + many(text_char)) |
          (&cr<R"(^ \t\r\n\f<)"> + some(text_char));
    else if constexpr (Name.view() == "p")
      return lit<"<"> + (ikw<"address"> | ikw<"article"> | ikw<"aside"> | ikw<"blockquote"> |
          ikw<"center"> | ikw<"details"> | ikw<"dialog"> | ikw<"dir"> | ikw<"div"> |
          ikw<"dl"> | ikw<"fieldset"> | ikw<"figcaption"> |
          ikw<"figure"> | ikw<"footer"> | ikw<"form"> | ikw<"h1"> | ikw<"h2"> |
          ikw<"h3"> | ikw<"h4"> | ikw<"h5"> | ikw<"h6"> | ikw<"header"> |
          ikw<"hgroup"> | ikw<"hr"> | ikw<"main"> | ikw<"menu"> | ikw<"nav"> |
          ikw<"ol"> | ikw<"p"> | ikw<"pre"> | ikw<"listing"> | ikw<"search"> | ikw<"section"> |
          ikw<"summary"> | ikw<"table"> | ikw<"ul"> | ikw<"li"> | ikw<"dt"> | ikw<"dd"> |
          ikw<"xmp"> | ikw<"plaintext">) + &RawTagBoundary;
    else if constexpr (Name.view() == "dt" || Name.view() == "dd")
      return lit<"<"> + (ikw<"dt"> | ikw<"dd">) + &RawTagBoundary;
    else if constexpr (Name.view() == "rt" || Name.view() == "rp")
      return lit<"<"> + (ikw<"rt"> | ikw<"rp">) + &RawTagBoundary;
    else if constexpr (Name.view() == "option")
      return lit<"<"> + (ikw<"option"> | ikw<"optgroup">) + &RawTagBoundary;
    else if constexpr (Name.view() == "td" || Name.view() == "th")
      return lit<"<"> + (ikw<"td"> | ikw<"th"> | ikw<"tr"> | ikw<"thead"> |
                           ikw<"tbody"> | ikw<"tfoot">) + &RawTagBoundary;
    else if constexpr (Name.view() == "tr")
      return lit<"<"> + (ikw<"tr"> | ikw<"thead"> | ikw<"tbody"> | ikw<"tfoot">) + &RawTagBoundary;
    else if constexpr (Name.view() == "thead" || Name.view() == "tbody" || Name.view() == "tfoot")
      return lit<"<"> + (ikw<"thead"> | ikw<"tbody"> | ikw<"tfoot">) + &RawTagBoundary;
    else return lit<"<"> + ikw<Name> + &RawTagBoundary;
  }();
  template <cursiv::str Name>
  struct IMPLIED_END : terminal<implied_end_text<Name>> {};
  template <cursiv::str Name>
  static constexpr auto implied_end = ref<IMPLIED_END<Name>>;

  template <cursiv::str Name>
  struct OptionalEndRule
      : rule<assign<&ast::Element::name>(ref<RawOpen<Name>>) +
             attributes +
             (flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>) |
              (ref<TAG_CLOSE> +
               many(!implied_end<Name> + append<&ast::Element::content>(content_item)) +
               &(implied_end<Name> | close_start | eof) +
               ((&matching_close<Name> + close_tag) | !matching_close<Name>)))> {};

  struct ElementRule
      : rule<(open_tag_prefix + ref<TAG_CLOSE> +
              many(append<&ast::Element::content>(content_item)) + close_tag) |
             (open_tag_prefix +
              flag<&ast::Element::selfClosing>(ref<TAG_SLASH_CLOSE>))> {};

  struct DocRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::xmlDeclarations>(ref<XML>) |
                  append<&ast::Document::doctypes>(ref<DTD>) |
                  append<&ast::Document::content>(content_item))> {};

  using entry = DocRule;
  using ignore = list<>;
};

} // namespace html
