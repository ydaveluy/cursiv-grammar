// A sources.list document preserves records; comments remain CST trivia.
#pragma once
#include "apt/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace apt {
struct Apt : cursiv::GrammarSpec {
  struct WSS : terminal<some(cr<R"( \t\v\f)">)> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct TYPE
      : terminal<kw<"deb-src"> | kw<"deb">, text_conv<[](std::string_view t) {
                   return t == "deb-src" ? ast::Type::DebSrc : ast::Type::Deb;
                 }>> {};
  static constexpr auto Quoted = lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">;
  static constexpr auto Bracketed = lit<"["> + many(cr<R"(^]\r\n)">) + lit<"]">;
  struct OPTION_NAME : terminal<some(cr<R"(^ \t\v\f\r\n=])">)> {};
  struct OPTION_VALUE
      : terminal<some(Quoted | cr<R"(^ \t\v\f\r\n]")">)> {};
  static constexpr auto Word =
      some(Quoted | Bracketed | cr<R"(^ \t\v\f\r\n#"[)">);
  struct WORD : terminal<Word> {};
  struct URI_WORD : terminal<!lit<"["> + Word> {};
  struct FieldWord
      : rule<ref<WORD> | ref<TYPE>, text_conv<[](std::string_view text) {
               return text;
             }>> {};
  struct OptionRule
      : rule<assign<&ast::Option::name>(ref<OPTION_NAME>) +
             lit<"="> +
             assign<&ast::Option::value>(ref<OPTION_VALUE>)> {};
  struct RecordRule
      : rule<assign<&ast::Record::type>(ref<TYPE>) + ref<WSS> +
             option(lit<"["> + option(ref<WSS>) +
                    many(append<&ast::Record::options>(ref<OptionRule>),
                         ref<WSS>) +
                    option(ref<WSS>) + lit<"]"> + option(ref<WSS>)) +
             assign<&ast::Record::uri>(ref<URI_WORD>) + ref<WSS> +
             assign<&ast::Record::distribution>(ref<FieldWord>) +
             many(ref<WSS> + append<&ast::Record::components>(ref<FieldWord>)) +
             option(ref<WSS>) + &(ref<COMMENT> | ref<EOL> | eof)> {};
  struct DocumentRule
      : rule<create<ast::Document> +
             many(ref<WSS> | ref<EOL> | ref<COMMENT> |
                  append<&ast::Document::records>(ref<RecordRule>))> {};
  using entry = DocumentRule;
};
} // namespace apt
