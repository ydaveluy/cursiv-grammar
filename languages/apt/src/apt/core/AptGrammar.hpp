// A sources.list document preserves records; comments remain CST trivia.
#pragma once
#include "apt/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace apt {
struct Apt : cursiv::GrammarSpec {
  struct WSS : terminal<some(cr<R"( \t\r\v\f)">)> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct TYPE : terminal<as<[](std::string_view t) {
    return t == "deb-src" ? ast::Type::DebSrc : ast::Type::Deb;
  }>((kw<"deb-src"> | kw<"deb">) + &cr<R"( \t\v)">)> {};
  static constexpr auto Quoted = lit<"\""> + many(cr<R"(^"\n)">) + lit<"\"">;
  static constexpr auto Bracketed = lit<"["> + many(cr<R"(^]\n)">) + lit<"]">;
  struct OPTION_NAME
      : terminal<some(Quoted | cr<R"(^ \t\v\f\r\n=]")">)> {};
  struct OPTION_VALUE
      : terminal<some(Quoted | cr<R"(^ \t\v\f\r\n]")">)> {};
  static constexpr auto Word =
      some(Quoted | Bracketed | cr<R"(^ \t\v\f\r\n#"[)">);
  struct WORD : terminal<Word> {};
  struct URI_WORD : terminal<!lit<"["> + Word> {};
  struct FieldWord : rule<as<[](std::string_view text) { return text; }>(
                         ref<WORD> | ref<TYPE>)> {};
  struct Option
      : rule<assign<&ast::Option::name>(ref<OPTION_NAME>) +
             lit<"="> +
             assign<&ast::Option::value>(ref<OPTION_VALUE>)> {};
  struct Record
      : rule<assign<&ast::Record::type>(ref<TYPE>) + ref<WSS> +
             option(lit<"["> + option(ref<WSS>) +
                    many(append<&ast::Record::options>(ref<Option>),
                         ref<WSS>) +
                    option(ref<WSS>) + lit<"]"> + option(ref<WSS>)) +
             assign<&ast::Record::uri>(ref<URI_WORD>) + ref<WSS> +
             assign<&ast::Record::distribution>(ref<FieldWord>) +
             many(ref<WSS> + append<&ast::Record::components>(ref<FieldWord>)) +
             option(ref<WSS>) + &(ref<COMMENT> | ref<EOL> | eof)> {};
  struct Document
      : rule<create<ast::Document> +
             many(ref<WSS> | ref<EOL> | ref<COMMENT> |
                  append<&ast::Document::records>(ref<Record>))> {};
  using entry = Document;
};
} // namespace apt
