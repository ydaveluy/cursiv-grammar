#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ini/core/ast.hpp"

namespace ini {
struct INI : cursiv::GrammarSpec {
  static constexpr auto space =
      uc::Z | cr<R"(\t\v\f\x1c-\x20)"> | lit<"\u0085">;
  struct INDENT : terminal<some(space)> {};
  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct EOL : terminal<eol> {};
  struct HEADER : terminal<lit<"["> + some(cr<R"(^\r\n)">) + lit<"]"> +
                           many(cr<R"(^\r\n)">)> {};
  struct HEADER_NAME : terminal<some(cr<R"(^\r\n)">) + &lit<"]">> {};
  struct KEY : terminal<!ref<HEADER> + !cr<"#;"> + !space +
                        cr<R"(^\r\n=:)"> + many(cr<R"(^\r\n=:)">)> {};
  struct COMMENT : terminal<cr<"#;"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto indentation =
      option(assign<&ast::Line::indentation>(ref<INDENT>));
  struct Header : rule<create<ast::SectionHeader>(indentation +
      assign<&ast::SectionHeader::open>(lit<"[">) +
      assign<&ast::SectionHeader::name>(ref<HEADER_NAME>) +
      assign<&ast::SectionHeader::close>(lit<"]">) +
      option(assign<&ast::SectionHeader::suffix>(ref<TEXT>)))> {};
  struct Option : rule<create<ast::Option>(indentation +
      ((assign<&ast::Option::key>(ref<KEY>) +
        option(assign<&ast::Option::separator>(cr<"=:">) +
               option(assign<&ast::Option::value>(ref<TEXT>)))) |
       (assign<&ast::Option::separator>(cr<"=:">) +
        option(assign<&ast::Option::value>(ref<TEXT>)))))> {};
  struct Comment : rule<create<ast::Comment>(indentation +
      assign<&ast::Comment::text>(ref<COMMENT>))> {};
  struct Content : rule<ref<Header> | ref<Comment> | ref<Option>> {};
  struct Line : rule<ref<Content> +
      (assign<&ast::Line::ending>(ref<EOL>) | eof)> {};
  struct BlankLine : rule<create<ast::Blank>(indentation +
      assign<&ast::Line::ending>(ref<EOL>))> {};
  struct FinalBlank : rule<create<ast::Blank>(
      assign<&ast::Line::indentation>(ref<INDENT>) + eof)> {};
  struct Document : rule<create<ast::Document>(
      many(append<&ast::Document::lines>(ref<Line> | ref<BlankLine>)) +
      option(append<&ast::Document::lines>(ref<FinalBlank>)))> {};
  using entry = Document;
};
} // namespace ini
