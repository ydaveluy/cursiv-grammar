#pragma once

#include <type_traits>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "javadoc/core/ast.hpp"

namespace javadoc {

struct Javadoc : cursiv::GrammarSpec {
  static constexpr auto NameStart = uc::L | uc::Nl | cr<"_">;
  static constexpr auto NamePart =
      NameStart | uc::Mn | uc::Mc | uc::Nd | uc::Pc | uc::Cf | cr<".:-">;
  struct NAME : terminal<NameStart + many(NamePart)> {};
  struct SPACE : terminal<some(cr<R"( \t)">)> {};
  struct START : terminal<many(cr<R"( \t\r\n)">) + lit<"/**"> +
                          many(!lit<"*/"> + lit<"*">)> {};
  struct END : terminal<some(lit<"*">) + lit<"/">> {};
  struct NL : terminal<eol | lit<"\f">> {};
  struct DECORATED_NL : terminal<ref<NL> + option(many(cr<R"( \t)">) +
                                                  some(!lit<"*/"> + lit<"*">))> {
  };

  template<bool Wrapped> struct Brace;
  template<bool Wrapped>
  static constexpr auto inlineChar = [] {
    if constexpr (Wrapped)
      return cr<"^{}*"> | (!lit<"*/"> + lit<"*">);
    else
      return cr<"^{}">;
  }();
  template<bool Wrapped>
  struct RawInlineText : terminal<some(inlineChar<Wrapped>)> {};
  template<bool Wrapped>
  struct Brace
      : rule<lit<"{"> + many(ref<RawInlineText<Wrapped>> | ref<Brace<Wrapped>>) + lit<"}">> {};
  template<bool Wrapped>
  struct InlineContent : rule<many(ref<RawInlineText<Wrapped>> | ref<Brace<Wrapped>>)> {};
  struct WrappedInlineTag;
  struct BareInlineTag;
  struct WrappedInlineBrace;
  struct BareInlineBrace;
  template <bool Wrapped> using InlineTag =
      std::conditional_t<Wrapped, WrappedInlineTag, BareInlineTag>;
  template <bool Wrapped> using InlineBrace =
      std::conditional_t<Wrapped, WrappedInlineBrace, BareInlineBrace>;

  static constexpr auto FreeStructuredName =
      lit<"summary", !NamePart> | lit<"return", !NamePart>;
  static constexpr auto LinkName =
      lit<"link", !NamePart> | lit<"linkplain", !NamePart>;
  static constexpr auto IndexName = lit<"index", !NamePart>;
  static constexpr auto StructuredName = FreeStructuredName | LinkName | IndexName;
  struct FREE_NAME : terminal<FreeStructuredName> {};
  struct LINK_NAME : terminal<LinkName> {};
  struct INDEX_NAME : terminal<IndexName> {};
  struct RAW_NAME : terminal<!StructuredName + ref<NAME>> {};
  struct INLINE_OPEN : terminal<lit<"{@">> {};
  struct BRACE_OPEN : terminal<!lit<"{@"> + lit<"{">> {};
  struct INLINE_CLOSE : terminal<lit<"}">> {};

  template <bool Wrapped> static constexpr auto rawChar = [] {
    if constexpr (Wrapped) return cr<"^*"> | (!lit<"*/"> + lit<"*">);
    else return any;
  }();
  template <bool Wrapped> static constexpr auto headerSpace = [] {
    if constexpr (Wrapped) return cr<" \t"> | ref<DECORATED_NL>;
    else return cr<" \t"> | ref<NL>;
  }();
  template <bool Wrapped> struct HeaderSpace : terminal<some(headerSpace<Wrapped>)> {};

  // Only the label of a link is inlineContent in DocTrees. The reference is
  // retained as one opaque source token, including whitespace inside ()/<>.
  struct WrappedReferenceParens;
  struct BareReferenceParens;
  struct WrappedReferenceAngles;
  struct BareReferenceAngles;
  template <bool Wrapped> using ReferenceParens =
      std::conditional_t<Wrapped, WrappedReferenceParens, BareReferenceParens>;
  template <bool Wrapped> using ReferenceAngles =
      std::conditional_t<Wrapped, WrappedReferenceAngles, BareReferenceAngles>;
  template <bool Wrapped> static constexpr auto referenceNested =
      (!cr<"(){}<>"> + rawChar<Wrapped>) |
      ref<ReferenceParens<Wrapped>> | ref<ReferenceAngles<Wrapped>>;
  struct WrappedReferenceParens
      : terminal<lit<"("> + many(referenceNested<true>) + lit<")">> {};
  struct BareReferenceParens
      : terminal<lit<"("> + many(referenceNested<false>) + lit<")">> {};
  struct WrappedReferenceAngles
      : terminal<lit<"<"> + many(referenceNested<true>) + lit<">">> {};
  struct BareReferenceAngles
      : terminal<lit<"<"> + many(referenceNested<false>) + lit<">">> {};
  template <bool Wrapped> struct ReferenceHead
      : terminal<many(headerSpace<Wrapped>) +
                 some((!cr<R"( \t\r\n\f(){}<>)"> + rawChar<Wrapped>) |
                      ref<ReferenceParens<Wrapped>> | ref<ReferenceAngles<Wrapped>>)> {};
  template <bool Wrapped> struct QuotedIndexHead
      : terminal<lit<"\""> + many(!lit<"\""> + rawChar<Wrapped>) + lit<"\"">> {};
  template <bool Wrapped> struct IndexHead
      : terminal<many(headerSpace<Wrapped>) +
                 (ref<QuotedIndexHead<Wrapped>> |
                  (!lit<"\""> + some(!cr<R"( \t\r\n\f{})"> + rawChar<Wrapped>)))> {};

  template <auto Member, bool Wrapped> static constexpr auto parts =
      many(append<Member>(ref<InlineTag<Wrapped>> | ref<InlineBrace<Wrapped>> |
                          ref<RawInlineText<Wrapped>>));
  template <bool Wrapped> static constexpr auto linkParts =
      ((append<&ast::InlineTag::parts>(ref<ReferenceHead<Wrapped>>) +
        parts<&ast::InlineTag::parts, Wrapped>) |
       option(append<&ast::InlineTag::parts>(ref<HeaderSpace<Wrapped>>)));
  template <bool Wrapped> static constexpr auto indexParts =
      ((append<&ast::InlineTag::parts>(ref<IndexHead<Wrapped>>) +
        parts<&ast::InlineTag::parts, Wrapped>) |
       option(append<&ast::InlineTag::parts>(ref<HeaderSpace<Wrapped>>)));
  template <bool Wrapped> static constexpr auto inlineTagBody =
      assign<&ast::InlineTag::open>(ref<INLINE_OPEN>) +
      ((flag<&ast::InlineTag::structured>(assign<&ast::InlineTag::name>(ref<FREE_NAME>)) +
        parts<&ast::InlineTag::parts, Wrapped>) |
       (flag<&ast::InlineTag::structured>(assign<&ast::InlineTag::name>(ref<LINK_NAME>)) +
        linkParts<Wrapped>) |
       (flag<&ast::InlineTag::structured>(assign<&ast::InlineTag::name>(ref<INDEX_NAME>)) +
        indexParts<Wrapped>) |
       (assign<&ast::InlineTag::name>(ref<RAW_NAME>) +
        assign<&ast::InlineTag::content>(ref<InlineContent<Wrapped>>))) +
      assign<&ast::InlineTag::close>(ref<INLINE_CLOSE>);
  struct WrappedInlineTag : rule<inlineTagBody<true>> {};
  struct BareInlineTag : rule<inlineTagBody<false>> {};
  struct WrappedInlineBrace
      : rule<assign<&ast::InlineBrace::open>(ref<BRACE_OPEN>) +
             parts<&ast::InlineBrace::parts, true> + assign<&ast::InlineBrace::close>(ref<INLINE_CLOSE>)> {};
  struct BareInlineBrace
      : rule<assign<&ast::InlineBrace::open>(ref<BRACE_OPEN>) +
             parts<&ast::InlineBrace::parts, false> + assign<&ast::InlineBrace::close>(ref<INLINE_CLOSE>)> {};

  template <bool Wrapped>
  static constexpr auto ordinary = [] {
    if constexpr (Wrapped)
      return cr<R"(^{*\r\n\f)"> | (!lit<"{@"> + lit<"{">) |
             (!lit<"*/"> + lit<"*">) | lit<"{@@">;
    else
      return cr<R"(^{\r\n\f)"> | (!lit<"{@"> + lit<"{">) | lit<"{@@">;
  }();

  template <bool Wrapped> struct Text : terminal<some(ordinary<Wrapped>)> {};
  template <bool Wrapped>
  struct FirstText
      : terminal<((cr<"@"> + cr<"@*">) | (!cr<R"( @\t)"> + ordinary<Wrapped>)) +
                 many(ordinary<Wrapped>)> {};
  struct BareFirstText : terminal<!lit<"/**"> + ref<FirstText<false>>> {};

  template <bool Wrapped>
  static constexpr auto newline = [] {
    if constexpr (Wrapped)
      return ref<DECORATED_NL>;
    else
      return ref<NL>;
  }();
  template <bool Wrapped>
  static constexpr auto lineEnd = [] {
    if constexpr (Wrapped)
      return &(ref<DECORATED_NL> | ref<END> | eof);
    else
      return &(ref<NL> | eof);
  }();

  template <auto Member, bool Wrapped, bool First = false>
  static constexpr auto line = [] {
    constexpr auto first = [] {
      if constexpr (First)
        return ref<BareFirstText>;
      else
        return ref<FirstText<Wrapped>>;
    }();
    return (option(append<Member>(ref<SPACE>)) +
            append<Member>(ref<InlineTag<Wrapped>> | first) +
            many(append<Member>(ref<InlineTag<Wrapped>> | ref<Text<Wrapped>>)) +
            lineEnd<Wrapped> + option(append<Member>(newline<Wrapped>))) |
           (option(append<Member>(ref<SPACE>)) +
            append<Member>(newline<Wrapped>)) |
           (append<Member>(ref<SPACE>) + lineEnd<Wrapped>);
  }();

  template <bool Wrapped>
  struct BlockTag
      : rule<option(ref<SPACE>) + lit<"@"> +
             assign<&ast::BlockTag::name>(ref<NAME>) +
             many(append<&ast::BlockTag::content>(ref<InlineTag<Wrapped>> |
                                                  ref<Text<Wrapped>>)) +
             lineEnd<Wrapped> +
             option(append<&ast::BlockTag::content>(newline<Wrapped>)) +
             many(line<&ast::BlockTag::content, Wrapped>)> {};

  struct WrappedDocument
      : rule<create<ast::Documentation> +
             flag<&ast::Documentation::wrapped>(ref<START>) +
             many(line<&ast::Documentation::description, true>) +
             many(append<&ast::Documentation::tags>(ref<BlockTag<true>>)) +
             ref<END>> {};
  struct BareDocument
      : rule<create<ast::Documentation> +
             option(line<&ast::Documentation::description, false, true> +
                    many(line<&ast::Documentation::description, false>)) +
             many(
                 append<&ast::Documentation::tags>(ref<BlockTag<false>>))> {
  };
  struct EmptyDocument : rule<create<ast::Documentation> +
                              flag<&ast::Documentation::wrapped>(lit<"/**/">)> {
  };
  struct Documentation
      : rule<ref<EmptyDocument> | ref<WrappedDocument> | ref<BareDocument>> {};
  using entry = Documentation;
};

} // namespace javadoc
