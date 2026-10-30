#pragma once
#include <string_view>
#include "bibtex/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bibtex {
inline constexpr auto unquote = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};
struct Bibtex : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"%"> + many(cr<R"(^\n)">)> {};
  struct JUNK : terminal<some(cr<R"(^@ \t\r\n)">)> {};
  struct IDENTIFIER : terminal<some(cr<R"(^ \t\r\n\f\v"{}(),=#%)">)> {};
  struct MACRO_WORD
      : terminal<many(d) + cr<R"(^0-9 \t\r\n\f\v"{}(),=#%)"> +
                 many(cr<R"(^ \t\r\n\f\v"{}(),=#%)">)> {};
  struct CitationKey : terminal<some(cr<R"(^,{}()= \t\r\n\f\v%)">)> {};
  struct NUMBER : terminal<some(d)> {};
  struct BRACED : terminal<lit<"{"> + many(ref<BRACED> | cr<"^{}">) +
                            lit<"}">> {};
  struct BRACED_VALUE : terminal<as<unquote>(ref<BRACED>)> {};
  struct QUOTED
      : terminal<as<unquote>(lit<"\""> + many(ref<BRACED> | cr<R"(^{}")">) +
                             lit<"\"">)> {};
  struct PAREN_COMMENT
      : terminal<lit<"("> + many(ref<PAREN_COMMENT> | cr<"^()">) + lit<")">> {};
  struct BracedPart
      : rule<create<ast::BracedPart>(
            assign<&ast::ValuePart::text>(ref<BRACED_VALUE>))> {};
  struct QuotedPart
      : rule<create<ast::QuotedPart>(
            assign<&ast::ValuePart::text>(ref<QUOTED>))> {};
  struct NumberPart
      : rule<create<ast::NumberPart>(
            assign<&ast::ValuePart::text>(ref<NUMBER>))> {};
  struct MacroPart
      : rule<create<ast::MacroPart>(
            assign<&ast::ValuePart::text>(ref<MACRO_WORD>))> {};
  struct Value
      : rule<create<ast::Value> +
             many(append<&ast::Value::parts>(ref<BracedPart> | ref<QuotedPart> |
                                            ref<NumberPart> | ref<MacroPart>),
                  lit<"#">)> {};
  struct Field : rule<assign<&ast::Field::key>(ref<IDENTIFIER>) + lit<"="> +
                          assign<&ast::Field::value>(ref<Value>)> {};
  template <auto Open>
  static constexpr auto open = assign<Open>(lit<"{"> | lit<"(">);
  template <auto Close>
  static constexpr auto close = assign<Close>(lit<"}"> | lit<")">);
  template <auto Fields>
  static constexpr auto fields =
      many(append<Fields>(ref<Field>), lit<",">) + option(lit<",">);
  struct Entry
      : rule<assign<&ast::Entry::type>(ref<IDENTIFIER>) +
             open<&ast::Entry::open> +
             option(assign<&ast::Entry::citekey>(ref<CitationKey>)) +
             option(flag<&ast::Entry::hasKeySeparator>(lit<",">) +
                    fields<&ast::Entry::fields>) + close<&ast::Entry::close>> {};
  struct String
      : rule<ikw<"string"> + open<&ast::StringDef::open> +
             fields<&ast::StringDef::fields> + close<&ast::StringDef::close>> {};
  struct Preamble
      : rule<ikw<"preamble"> + open<&ast::Preamble::open> +
             assign<&ast::Preamble::value>(ref<Value>) + option(lit<",">) +
             close<&ast::Preamble::close>> {};
  struct Comment
      : rule<ikw<"comment"> +
             assign<&ast::Comment::raw>(ref<BRACED> | ref<PAREN_COMMENT>)> {};
  struct Declaration
      : rule<lit<"@"> + (ref<String> | ref<Preamble> | ref<Comment> |
                         ref<Entry>)> {};
  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::declarations>(ref<Declaration>) |
                  ref<JUNK>)> {};
  using entry = Document;
  using ignored = list<WS, LINE_COMMENT>;
};
}  // namespace bibtex
