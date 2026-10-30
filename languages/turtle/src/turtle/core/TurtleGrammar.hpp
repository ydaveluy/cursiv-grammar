// The turtle language: its grammar. RDF 1.1 Turtle: directives, triples,
// blank nodes, collections and the four literal kinds.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "turtle/core/ast.hpp"

namespace turtle {

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

// PNAME_NS text ends in ':'; the prefix is everything before it.
inline constexpr auto strip_colon = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};

// LANGTAG text opens on '@'.
inline constexpr auto strip_at = [](std::string_view t) { return t.substr(1); };

inline constexpr auto blank_label = [](std::string_view t) {
  return t.size() >= 2 && t[0] == '_' ? t.substr(2) : std::string_view{};
};

struct Turtle : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
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
  struct BLANK_NODE : terminal<as<blank_label>(blank_node_label)> {};

  struct LANGTAG
      : terminal<as<strip_at>(lit<"@"> + some(cr<"a-zA-Z">) +
                              many(lit<"-"> + some(cr<"a-zA-Z0-9">)))> {};

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
  struct STRING_QUOTE : terminal<as<string_text>(dquote_short)> {};

  static constexpr auto squote_short =
      lit<"'"> + many(echar | uchar | cr<R"(^'\\\r\n)">) + lit<"'">;
  struct STRING_SINGLE_QUOTE : terminal<as<string_text>(squote_short)> {};

  static constexpr auto long_single_inner =
      option(lit<"''"> | lit<"'">) +
      (echar | uchar | cr<R"(^'\\)">);
  struct STRING_LONG_SINGLE_QUOTE
      : terminal<as<string_text>(lit<"'''"> + many(long_single_inner) +
                                 lit<"'''">)> {};

  static constexpr auto long_double_inner =
      option(lit<"\"\""> | lit<"\"">) +
      (echar | uchar | cr<R"(^"\\)">);
  struct STRING_LONG_QUOTE
      : terminal<as<string_text>(lit<"\"\"\""> + many(long_double_inner) +
                                 lit<"\"\"\"">)> {};

  static constexpr auto iriref_body =
      lit<"<"> + many(uchar | cr<R"(^\x00- <>"{}|`\\^)">) + lit<">">;
  struct IRIREF : terminal<as<iri_text>(iriref_body)> {};

  // Only PN_CHARS/underscore/digits may abut a keyword: '.', ':' and '#'
  // stay outside so a term like `ex:a` or `a.` still lexes as intended.
  static constexpr auto word_boundary = pn_chars;

  struct IsA;
  struct IriRef;
  struct PrefixedTerm;
  struct BlankNode;
  struct Collection;
  struct BlankNodePropertyList;
  struct RdfLiteral;
  struct NumberLit;
  struct BoolLit;

  static constexpr auto IriTerm = ref<IriRef> | ref<PrefixedTerm>;
  static constexpr auto SubjectTerm =
      IriTerm | ref<BlankNode> | ref<Collection>;
  static constexpr auto VerbTerm = IriTerm | ref<IsA>;
  static constexpr auto ObjectTerm =
      IriTerm | ref<BlankNode> | ref<Collection> |
      ref<BlankNodePropertyList> | ref<RdfLiteral> |
      ref<NumberLit> | ref<BoolLit>;

  struct PrefixName : rule<as<strip_colon>(ref<PNAME_NS>)> {};
  struct IriRef : rule<assign<&ast::IriRef::value>(ref<IRIREF>)> {};
  struct PrefixedTerm : rule<assign<&ast::PrefixedTerm::value>(
                                ref<PrefixedName> | ref<PNAME_NS>)> {};
  struct BlankNode : rule<assign<&ast::BlankNode::label>(ref<BLANK_NODE>) |
                              (create<ast::BlankNode> + lit<"["> + lit<"]">)> {
  };
  struct IsA : rule<create<ast::RdfType> + kw<"a">> {};

  struct Collection
      : rule<lit<"("> + many(append<&ast::Collection::items>(ObjectTerm)) +
             lit<")">> {};

  struct RdfLiteral
      : rule<assign<&ast::RdfLiteral::value>(
                 ref<STRING_LONG_SINGLE_QUOTE> | ref<STRING_LONG_QUOTE> |
                 ref<STRING_SINGLE_QUOTE> | ref<STRING_QUOTE>) +
             option(
                 assign<&ast::RdfLiteral::lang>(ref<LANGTAG>) |
                 (lit<"^^"> + assign<&ast::RdfLiteral::datatype>(IriTerm)))> {};
  struct NumberLit
      : rule<assign<&ast::NumberLit::text>(ref<NUMERIC_LITERAL>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit> +
             (flag<&ast::BoolLit::value>(ikw<"true">) | ikw<"false">)> {};

  struct PredicateObject
      : rule<assign<&ast::PredicateObject::predicate>(VerbTerm) +
             some(append<&ast::PredicateObject::objects>(ObjectTerm),
                  lit<",">)> {};

  struct PredicateObjectList
      : rule<append<&ast::PredicateObjectList::items>(
                 ref<PredicateObject>) +
             many(lit<";"> + option(append<&ast::PredicateObjectList::items>(
                                 ref<PredicateObject>)))> {};

  struct BlankNodePropertyList
      : rule<lit<"["> +
             assign<&ast::BlankNodePropertyList::properties>(
                 ref<PredicateObjectList>) +
             lit<"]">> {};

  struct Triples : rule<assign<&ast::Triples::subject>(
                                SubjectTerm | ref<BlankNodePropertyList>) +
                            option(assign<&ast::Triples::properties>(
                                ref<PredicateObjectList>))> {};

  struct PrefixID
      : rule<ikw<"@prefix"> +
             assign<&ast::PrefixDecl::prefix>(ref<PrefixName>) +
             assign<&ast::PrefixDecl::iri>(ref<IRIREF>) + lit<".">> {};
  struct Base : rule<ikw<"@base"> +
                         assign<&ast::BaseDecl::iri>(ref<IRIREF>) + lit<".">> {
  };
  struct SparqlPrefix
      : rule<ikw<"PREFIX"> +
             assign<&ast::SparqlPrefixDecl::prefix>(ref<PrefixName>) +
             assign<&ast::SparqlPrefixDecl::iri>(ref<IRIREF>)> {};
  struct SparqlBase
      : rule<ikw<"BASE"> + assign<&ast::SparqlBaseDecl::iri>(ref<IRIREF>)> {};

  struct Directive : rule<ref<PrefixID> | ref<Base> |
                              ref<SparqlPrefix> | ref<SparqlBase>> {};

  struct Statement
      : rule<ref<Directive> | (ref<Triples> + lit<".">)> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::statements>(ref<Statement>))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<LC>;
};

} // namespace turtle
