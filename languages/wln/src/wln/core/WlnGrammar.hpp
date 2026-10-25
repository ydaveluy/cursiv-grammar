#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wln/core/ast.hpp"

namespace wln {

struct Wln : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct SPACE : terminal<some(lit<" ">)> {};
  struct PREFIX : terminal<lit<"WLN: ">> {};
  struct SYMBOL : terminal<lit<"UU"> | some(d) | cr<R"(^ \t\r\n)">> {};
  struct TITLE
      : terminal<lit<"\t"> + many(cr<R"(^\r\n)">),
                 text_conv<[](std::string_view s) { return s.substr(1); }>> {};

  static constexpr auto group = some(append<&ast::Group::symbols>(ref<SYMBOL>));
  struct GroupRule : rule<create<ast::Group>(group)> {};
  struct SpacedGroupRule
      : rule<create<ast::Group>(assign<&ast::Group::separator>(ref<SPACE>) +
                                group)> {};

  struct RecordRule
      : rule<create<ast::Record>(
            option(assign<&ast::Record::prefix>(ref<PREFIX>)) +
            append<&ast::Record::groups>(ref<GroupRule>) +
            many(append<&ast::Record::groups>(ref<SpacedGroupRule>)) +
            option(assign<&ast::Record::trailing>(ref<SPACE>)) +
            option(assign<&ast::Record::title>(ref<TITLE>)))> {};

  struct DocumentRule : rule<create<ast::Document>(some(
                            append<&ast::Document::records>(ref<RecordRule>) +
                            (ref<EOL> | eof)))> {};

  using entry = DocumentRule;
};

} // namespace wln
