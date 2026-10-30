#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wln/core/ast.hpp"

namespace wln {

struct Wln : cursiv::GrammarSpec {
  struct EOL : terminal<eol> {};
  struct SPACE : terminal<some(lit<" ">)> {};
  struct PREFIX : terminal<lit<"WLN: ">> {};
  struct SYMBOL : terminal<lit<"UU"> | some(d) | cr<R"(^ \t\r\n)">> {};
  struct TITLE : terminal<as<[](std::string_view s) { return s.substr(1); }>(
                     lit<"\t"> + many(cr<R"(^\r\n)">))> {};

  static constexpr auto group = some(append<&ast::Group::symbols>(ref<SYMBOL>));
  struct Group : rule<create<ast::Group>(group)> {};
  struct SpacedGroup
      : rule<create<ast::Group>(assign<&ast::Group::separator>(ref<SPACE>) +
                                group)> {};

  struct Record
      : rule<create<ast::Record>(
            option(assign<&ast::Record::prefix>(ref<PREFIX>)) +
            append<&ast::Record::groups>(ref<Group>) +
            many(append<&ast::Record::groups>(ref<SpacedGroup>)) +
            option(assign<&ast::Record::trailing>(ref<SPACE>)) +
            option(assign<&ast::Record::title>(ref<TITLE>)))> {};

  struct Document : rule<create<ast::Document>(some(
                            append<&ast::Document::records>(ref<Record>) +
                            (ref<EOL> | eof)))> {};

  using entry = Document;
};

} // namespace wln
