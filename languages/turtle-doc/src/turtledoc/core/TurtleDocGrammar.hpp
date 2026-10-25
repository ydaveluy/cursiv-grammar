#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "turtledoc/core/ast.hpp"

namespace turtledoc {

inline std::string_view decode(std::string_view text,
                               std::pmr::memory_resource *memory,
                               bool strings = false) {
  if (text.find('\\') == std::string_view::npos)
    return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 == text.size()) {
      room[size++] = text[i];
      continue;
    }
    const char escaped = text[++i];
    if (escaped == 'u' || escaped == 'U') {
      const std::size_t count = escaped == 'u' ? 4 : 8;
      std::uint32_t cp = 0;
      std::from_chars(text.data() + i + 1, text.data() + i + 1 + count, cp, 16);
      i += count;
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        cp = 0xfffd;
      if (cp < 0x80)
        room[size++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000)
          room[size++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800)
          room[size++] = static_cast<char>(0xe0 | (cp >> 12));
        else
          room[size++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000)
          room[size++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800)
          room[size++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[size++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else if (!strings) {
      room[size++] = '\\';
      room[size++] = escaped;
    } else
      switch (escaped) {
      case 't':
        room[size++] = '\t';
        break;
      case 'b':
        room[size++] = '\b';
        break;
      case 'n':
        room[size++] = '\n';
        break;
      case 'r':
        room[size++] = '\r';
        break;
      case 'f':
        room[size++] = '\f';
        break;
      default:
        room[size++] = escaped;
        break;
      }
  }
  return {room.data(), size};
}
inline constexpr auto iri_text = [](std::string_view text,
                                    std::pmr::memory_resource *memory) {
  return decode(text.substr(1, text.size() - 2), memory);
};
inline constexpr auto string_text = [](std::string_view text,
                                       std::pmr::memory_resource *memory) {
  const std::size_t quotes =
      text.size() >= 6 && text[0] == text[1] && text[1] == text[2] ? 3 : 1;
  return decode(text.substr(quotes, text.size() - 2 * quotes), memory, true);
};
using IriText = cursiv::text_conv<iri_text>;
using StringText = cursiv::text_conv<string_text>;

// PNAME_NS text ends in ':'; the prefix is everything before it.
inline constexpr auto strip_colon = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};
using StripColon = cursiv::text_conv<strip_colon>;

// LANGTAG text opens on '@'.
inline constexpr auto strip_at = [](std::string_view t) { return t.substr(1); };
using StripAt = cursiv::text_conv<strip_at>;

inline constexpr auto blank_label = [](std::string_view t) {
  return t.size() >= 2 && t[0] == '_' ? t.substr(2) : std::string_view{};
};
using BlankLabel = cursiv::text_conv<blank_label>;

struct TurtleDoc : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct LC : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto hex = cr<"0-9A-Fa-f">;
  static constexpr auto uchar =
      (lit<"\\u"> + repeat<4>(hex)) | (lit<"\\U"> + repeat<8>(hex));
  static constexpr auto echar = lit<"\\"> + cr<R"(tbnrf"'\\)">;

  // Unicode ranges of PN_CHARS_BASE, spelled as code points.
  static constexpr auto pn_chars_base = cr<"A-Za-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-"
                                           "῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-"
                                           "﷏ﷰ-�"> |
                                        cr<"\U00010000-\U000effff">;
  static constexpr auto pn_chars_u = pn_chars_base | cr<"_">;
  static constexpr auto pn_chars = pn_chars_u | cr<"0-9·̀-ͯ‿-⁀-">;

  static constexpr auto pn_prefix =
      pn_chars_base + option(many(pn_chars | lit<".">) + pn_chars);
  static constexpr auto pname_ns_body = option(pn_prefix) + lit<":">;

  static constexpr auto percent = lit<"%"> + repeat<2>(hex);
  static constexpr auto pn_local_esc = lit<"\\"> + cr<"_~.!$&'()*+,;=/?#@%-">;
  static constexpr auto plx = percent | pn_local_esc;
  static constexpr auto pn_local =
      (pn_chars_u | lit<":"> | cr<"0-9"> | plx) +
      option(many(pn_chars | lit<"."> | lit<":"> | plx) +
             (pn_chars | lit<":"> | plx));

  struct PNAME_NS : terminal<pname_ns_body> {};
  struct PrefixedName : terminal<pname_ns_body + pn_local> {};

  static constexpr auto blank_node_label =
      lit<"_:"> +
      (pn_chars_u | cr<"0-9">)+option(many(pn_chars | lit<".">) + pn_chars);
  struct BLANK_NODE : terminal<blank_node_label, BlankLabel> {};

  struct LANGTAG : terminal<lit<"@"> + some(cr<"a-zA-Z">) +
                                many(lit<"-"> + some(cr<"a-zA-Z0-9">)),
                            StripAt> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto integer_body = option(cr<"+-">) + some(d);
  static constexpr auto decimal_body =
      option(cr<"+-">) + many(d) + lit<"."> + some(d);
  static constexpr auto double_body =
      option(cr<"+-">) + ((some(d) + lit<"."> + many(d) + exponent) |
                          (lit<"."> + some(d) + exponent) |
                          (some(d) + exponent));
  struct NUMERIC_LITERAL : terminal<double_body | decimal_body | integer_body> {
  };

  static constexpr auto dquote_short =
      lit<"\""> + many(echar | uchar | cr<R"(^"\\\r\n)">) + lit<"\"">;
  struct STRING_QUOTE : terminal<dquote_short, StringText> {};

  static constexpr auto squote_short =
      lit<"'"> + many(echar | uchar | lit<"\""> | cr<R"(^'\\\r\n)">) + lit<"'">;
  struct STRING_SINGLE_QUOTE : terminal<squote_short, StringText> {};

  static constexpr auto long_single_inner =
      option(lit<"''"> | lit<"'">) +
      (echar | uchar | lit<"\""> | cr<R"(^'\\)">);
  struct STRING_LONG_SINGLE_QUOTE
      : terminal<lit<"'''"> + many(long_single_inner) + lit<"'''">,
                 StringText> {};

  static constexpr auto long_double_inner =
      option(lit<"\"\""> | lit<"\"">) +
      (echar | uchar | lit<"'"> | cr<R"(^"\\)">);
  struct STRING_LONG_QUOTE
      : terminal<lit<"\"\"\""> + many(long_double_inner) + lit<"\"\"\"">,
                 StringText> {};

  static constexpr auto iriref_body =
      lit<"<"> + many(uchar | cr<R"(^\x00- <>"{}|`\\^)">) + lit<">">;
  struct IRIREF : terminal<iriref_body, IriText> {};

  // Only PN_CHARS/underscore/digits may abut a keyword: '.', ':' and '#'
  // stay outside so a term like `ex:a` or `a.` still lexes as intended.
  static constexpr auto word_boundary = pn_chars;

  struct IsARule;
  struct IriRefRule;
  struct PrefixedTermRule;
  struct BlankNodeRule;
  struct CollectionRule;
  struct BlankNodePropertyListRule;
  struct RdfLiteralRule;
  struct NumberLitRule;
  struct BoolLitRule;

  static constexpr auto IriTerm = ref<IriRefRule> | ref<PrefixedTermRule>;
  static constexpr auto SubjectTerm =
      IriTerm | ref<BlankNodeRule> | ref<CollectionRule>;
  static constexpr auto VerbTerm = IriTerm | ref<IsARule>;
  static constexpr auto ObjectTerm =
      IriTerm | ref<BlankNodeRule> | ref<CollectionRule> |
      ref<BlankNodePropertyListRule> | ref<RdfLiteralRule> |
      ref<NumberLitRule> | ref<BoolLitRule>;

  struct PrefixNameRule : rule<ref<PNAME_NS>, StripColon> {};
  struct IriRefRule : rule<assign<&ast::IriRef::value>(ref<IRIREF>)> {};
  struct PrefixedTermRule : rule<assign<&ast::PrefixedTerm::value>(
                                ref<PrefixedName> | ref<PNAME_NS>)> {};
  struct BlankNodeRule : rule<assign<&ast::BlankNode::label>(ref<BLANK_NODE>) |
                              (create<ast::BlankNode> + lit<"["> + lit<"]">)> {
  };
  struct IsARule : rule<create<ast::RdfType> + kw<"a">> {};

  struct CollectionRule
      : rule<lit<"("> + many(append<&ast::Collection::items>(ObjectTerm)) +
             lit<")">> {};

  struct RdfLiteralRule
      : rule<assign<&ast::RdfLiteral::value>(
                 ref<STRING_LONG_SINGLE_QUOTE> | ref<STRING_LONG_QUOTE> |
                 ref<STRING_SINGLE_QUOTE> | ref<STRING_QUOTE>) +
             option(
                 assign<&ast::RdfLiteral::lang>(ref<LANGTAG>) |
                 (lit<"^^"> + assign<&ast::RdfLiteral::datatype>(IriTerm)))> {};
  struct NumberLitRule
      : rule<assign<&ast::NumberLit::text>(ref<NUMERIC_LITERAL>)> {};
  struct BoolLitRule
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"true">) | ikw<"false">)> {};

  struct PredicateObjectRule
      : rule<assign<&ast::PredicateObject::predicate>(VerbTerm) +
             some(append<&ast::PredicateObject::objects>(ObjectTerm),
                  lit<",">)> {};

  struct PredicateObjectListRule
      : rule<append<&ast::PredicateObjectList::items>(
                 ref<PredicateObjectRule>) +
             many(lit<";"> + option(append<&ast::PredicateObjectList::items>(
                                 ref<PredicateObjectRule>)))> {};

  struct BlankNodePropertyListRule
      : rule<lit<"["> +
             assign<&ast::BlankNodePropertyList::properties>(
                 ref<PredicateObjectListRule>) +
             lit<"]">> {};

  struct TriplesRule : rule<assign<&ast::Triples::subject>(
                                SubjectTerm | ref<BlankNodePropertyListRule>) +
                            option(assign<&ast::Triples::properties>(
                                ref<PredicateObjectListRule>))> {};

  struct PrefixIDRule
      : rule<ikw<"@prefix"> +
             assign<&ast::PrefixDecl::prefix>(ref<PrefixNameRule>) +
             assign<&ast::PrefixDecl::iri>(ref<IRIREF>) + lit<".">> {};
  struct BaseRule : rule<ikw<"@base"> +
                         assign<&ast::BaseDecl::iri>(ref<IRIREF>) + lit<".">> {
  };
  struct SparqlPrefixRule
      : rule<ikw<"PREFIX"> +
             assign<&ast::SparqlPrefixDecl::prefix>(ref<PrefixNameRule>) +
             assign<&ast::SparqlPrefixDecl::iri>(ref<IRIREF>)> {};
  struct SparqlBaseRule
      : rule<ikw<"BASE"> + assign<&ast::SparqlBaseDecl::iri>(ref<IRIREF>)> {};

  struct DirectiveRule : rule<ref<PrefixIDRule> | ref<BaseRule> |
                              ref<SparqlPrefixRule> | ref<SparqlBaseRule>> {};

  struct StatementRule
      : rule<ref<DirectiveRule> | (ref<TriplesRule> + lit<".">)> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::statements>(ref<StatementRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<LC>;
};

} // namespace turtledoc
