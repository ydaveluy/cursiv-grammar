#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "srt/core/ast.hpp"

namespace srt {

struct Srt : cursiv::GrammarSpec {
  struct BOM : terminal<lit<"\xEF\xBB\xBF">> {};
  struct NEWLINE : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  static constexpr auto space = many(cr<R"( \t\v\f)">);
  static constexpr auto integer = space + option(cr<"+-">) + some(d);
  static constexpr auto clock = integer + lit<":"> + integer + lit<":"> +
                                integer + cr<",."> + integer;
  static constexpr auto arrow = space + lit<"-->">;
  struct TIME : terminal<clock> {};
  struct ARROW : terminal<arrow> {};
  static constexpr auto timing_line = clock + arrow + clock + many(cr<R"(^\r\n)">);
  static constexpr auto number_prefix =
      space + ((option(lit<"+">) + some(d)) |
                (lit<"-"> + some(lit<"0">) + !d));
  struct NUMBER_LINE
      : terminal<!timing_line + number_prefix + many(cr<R"(^\r\n)">)> {};
  struct STANDALONE_NUMBER : terminal<number_prefix + &eol> {};
  static constexpr auto text_line = !timing_line + some(cr<R"(^\r\n)">);
  struct TEXT : terminal<text_line> {};
  struct FIRST_TEXT : terminal<!ref<BOM> + text_line> {};
  struct SETTINGS : terminal<some(cr<R"(^\r\n)">)> {};

  static constexpr auto timing_start = ref<TIME> + ref<ARROW> + ref<TIME>;
  static constexpr auto start =
      timing_start | (ref<NUMBER_LINE> + some(ref<NEWLINE>) + timing_start);
  static constexpr auto first_caption_end =
      timing_start | (ref<STANDALONE_NUMBER> + some(ref<NEWLINE>) + timing_start);

  struct SubtitleRule
      : rule<option(assign<&ast::Subtitle::number>(ref<NUMBER_LINE>) +
                    some(ref<NEWLINE>)) +
             assign<&ast::Subtitle::start>(ref<TIME>) + ref<ARROW> +
             assign<&ast::Subtitle::end>(ref<TIME>) +
             option(assign<&ast::Subtitle::settings>(ref<SETTINGS>)) +
             option(ref<NEWLINE>) +
             many(ref<NEWLINE>) +
             option(!first_caption_end +
                    append<&ast::Subtitle::lines>(ref<TEXT>) + option(ref<NEWLINE>) +
                    many(!start + ((append<&ast::Subtitle::lines>(ref<TEXT>) +
                                     option(ref<NEWLINE>)) | ref<NEWLINE>)))> {};

  static constexpr auto preamble_line =
      !start + ((append<&ast::Document::preamble>(ref<TEXT>) + option(ref<NEWLINE>)) |
                 ref<NEWLINE>);
  static constexpr auto subtitles = append<&ast::Document::subtitles>(ref<SubtitleRule>);
  template <class FirstText>
  static constexpr auto content =
      ((!start + ((append<&ast::Document::preamble>(ref<FirstText>) +
                    option(ref<NEWLINE>)) | ref<NEWLINE>)) +
         many(preamble_line) + many(subtitles)) | some(subtitles) | eof;
  struct DocumentRule
      : rule<create<ast::Document> +
             ((ref<BOM> + content<TEXT>) | content<FIRST_TEXT>)> {};

  using entry = DocumentRule;
};

}  // namespace srt
