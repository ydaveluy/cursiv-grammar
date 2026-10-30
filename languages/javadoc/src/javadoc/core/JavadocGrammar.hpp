#pragma once

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
                          many(lit<"*", !lit<"/">>)> {};
  struct END : terminal<some(lit<"*">) + lit<"/">> {};
  struct NL : terminal<eol | lit<"\f">> {};
  struct DECORATED_NL : terminal<ref<NL> + option(many(cr<R"( \t)">) +
                                                  some(lit<"*", !lit<"/">>))> {
  };

  template<bool Wrapped> struct Brace;
  template<bool Wrapped>
  static constexpr auto inlineChar = [] {
    if constexpr (Wrapped)
      return cr<"^{}*"> | lit<"*", !lit<"/">>;
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
  template<bool Wrapped>
  struct InlineTag
      : rule<lit<"{@"> + assign<&ast::InlineTag::name>(ref<NAME>) +
             assign<&ast::InlineTag::content>(ref<InlineContent<Wrapped>>) + lit<"}">> {
  };

  template <bool Wrapped>
  static constexpr auto ordinary = [] {
    if constexpr (Wrapped)
      return cr<R"(^{*\r\n\f)"> | lit<"{", !lit<"@">> |
             lit<"*", !lit<"/">> | lit<"{@@">;
    else
      return cr<R"(^{\r\n\f)"> | lit<"{", !lit<"@">> | lit<"{@@">;
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
