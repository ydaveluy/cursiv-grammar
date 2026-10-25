// The desktopentry language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "desktopentry/core/ast.hpp"

namespace desktopentry {

struct DesktopEntry : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\v\f)">)> {};
  // A real parser terminal, not trivia: a value's own char class does not
  // exclude much, and a line end swept as trivia would let an absent value,
  // or a trailing `;`, resume on whatever the next line starts with.
  struct EOL : terminal<lit<"\r\n"> | lit<"\n"> | (lit<"\r"> + !lit<"\n">)> {};
  static constexpr auto line_end = some(ref<EOL>) | eof;

  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct KEY : terminal<some(cr<R"(^=\[\]\r\n)">),
      cursiv::text_conv<[](std::string_view text) {
        return text.substr(0, text.find_last_not_of(" \t\v\f") + 1);
      }>> {};
  struct IDENT : terminal<some(cr<R"(^_.@\[\] \t\r\n)">)> {};
  struct GROUP_HEADER
      : terminal<lit<"["> + some(cr<R"(^\[\]\r\n)">) + lit<"]">,
                 cursiv::text_conv<[](std::string_view text) {
                   return text.substr(1, text.size() - 2);
                 }>> {};

  struct VALUE : terminal<some(cr<R"(^\r\n)">),
      cursiv::text_conv<[](std::string_view text) {
        const auto first = text.find_first_not_of(" \t\v\f");
        return text.substr(first == text.npos ? text.size() : first);
      }>> {};

  struct LocaleRule
      : rule<lit<"["> + assign<&ast::Locale::language>(ref<IDENT>) +
             option(lit<"_"> + assign<&ast::Locale::country>(ref<IDENT>)) +
             option(lit<"."> + assign<&ast::Locale::encoding>(ref<IDENT>)) +
             option(lit<"@"> + assign<&ast::Locale::modifier>(ref<IDENT>)) +
             lit<"]">> {};

  static constexpr auto blank_line = ref<EOL> | (ref<COMMENT> + line_end);

  struct EntryRule
      : rule<assign<&ast::Entry::key>(ref<KEY>) +
             option(assign<&ast::Entry::locale>(ref<LocaleRule>)) + lit<"="> +
             option(assign<&ast::Entry::rawValue>(ref<VALUE>)) + line_end> {};

  struct GroupRule
      : rule<assign<&ast::Group::name>(ref<GROUP_HEADER>) +
             line_end +
             many(append<&ast::Group::entries>(ref<EntryRule>) | blank_line)> {
  };

  struct DocumentRule
      : rule<create<ast::Document> + many(blank_line) +
             many(append<&ast::Document::groups>(ref<GroupRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

} // namespace desktopentry
