// The creole language: its grammar, after Creole 1.0. A document is blocks
// -- paragraphs, headings, list items, table rows, rules and preformatted
// blocks -- that a line's first bytes open; bold and italics run on across
// the lines of a paragraph or a list item.
#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include "creole/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace creole {

// The `*`, `#` or `=` of a marker counted, the blanks around them not.
inline constexpr auto count_marks = [](std::string_view t) -> std::int32_t {
  std::int32_t n = 0;
  for (const char c : t) n += (c == '*' || c == '#' || c == '=') ? 1 : 0;
  return n;
};
using CountMarks = cursiv::text_conv<count_marks>;

// A heading's text without the `=`s and blanks closing it.
inline constexpr auto heading_text = [](std::string_view t) {
  auto e = t.size();
  while (e > 0 && (t[e - 1] == '=' || t[e - 1] == ' ' || t[e - 1] == '\t'))
    --e;
  return t.substr(0, e);
};
using HeadingText = cursiv::text_conv<heading_text>;

// `{{{` and `}}}` dropped.
inline constexpr auto trim_nowiki = [](std::string_view t) {
  return t.substr(3, t.size() - 6);
};
using NowikiText = cursiv::text_conv<trim_nowiki>;

// A preformatted block's lines, its `{{{` line and `}}}` dropped.
inline constexpr auto trim_block = [](std::string_view t) {
  const auto b = t.find('\n') + 1;
  auto e = t.size() - 3;
  if (e > b) --e;
  if (e > b && t[e - 1] == '\r') --e;
  return t.substr(b, e > b ? e - b : 0);
};
using BlockText = cursiv::text_conv<trim_block>;

struct Creole : cursiv::GrammarSpec {
  static constexpr auto line_break = lit<"\r\n"> | lit<"\n">;
  static constexpr auto blanks = many(cr<R"( \t)">);
  struct EOL : terminal<line_break> {};
  // A line break and the blank lines after it: what ends a paragraph.
  struct BLANK : terminal<line_break + some(blanks + line_break)> {};
  // Blank lines from a line's start: after a block that read its line break.
  struct BLANK_LINES : terminal<some(blanks + line_break)> {};

  // What a line opens a block with. Each reads the blanks around it, so it
  // outruns the text that would read them.
  struct HEAD_MARK : terminal<blanks + some(lit<"=">) + blanks, CountMarks> {};
  struct HEAD_TEXT : terminal<some(cr<R"(^\r\n)">), HeadingText> {};
  struct ITEM_MARK
      : terminal<blanks + lit<"*", !cr<"*">> + blanks, CountMarks> {};
  // `**` opens bold, except on the line right after a list item.
  struct SUBITEM_MARK
      : terminal<blanks + lit<"**"> + many(lit<"*">) + blanks, CountMarks> {};
  struct HASH_MARK : terminal<blanks + some(lit<"#">) + blanks, CountMarks> {};
  struct ROW_PIPE : terminal<blanks + lit<"|", !cr<"=">>> {};
  struct ROW_HEAD : terminal<blanks + lit<"|=">> {};
  // The ending pipe of a row and the row's line break.
  struct PIPE_END : terminal<lit<"|"> + blanks + line_break> {};
  // Four hyphens alone on their line, read with its line break or the end of
  // the text.
  struct HLINE
      : terminal<blanks + lit<"----", !cr<"-">> + blanks +
                 (line_break | eof)> {};
  // Its closing `}}}` opens a line: one a blank precedes is the block's.
  struct PREFORMATTED
      : terminal<through(lit<"{{{"> + line_break, lit<"\n}}}">),
                 BlockText> {};

  // A `//` right after `http:`, `https:` or `ftp:` is a free URL's, never
  // italics; punctuation ending one is the sentence's.
  static constexpr auto url =
      (lit<"http://"> | lit<"https://"> | lit<"ftp://">) +
      option(many(cr<R"(^ \t\r\n|[]{}*"'<>)">) +
             cr<R"(^ \t\r\n|[]{}*"'<>,.?!:;)">);
  struct URL : terminal<url> {};
  // The tilde escapes the byte after it, or the URL it stands in front of.
  struct ESC : terminal<lit<"~"> + (url | cr<R"(^ \t\r\n)">)> {};
  // Inline nowiki ends on the last three of a run of closing braces.
  struct NOWIKI
      : terminal<through(lit<"{{{", !cr<R"(\r\n)">>, lit<"}}}">) + many(lit<"}">),
                 NowikiText> {};

  struct WORD : terminal<some(cr<R"(A-Za-z0-9\u{80}-\u{10FFFF})">)> {};
  struct OTHER
      : terminal<some(cr<R"(^A-Za-z0-9\u{80}-\u{10FFFF}*/[]{}|\\~=#\r\n)">)> {};
  // A markup byte that opens nothing on its own.
  struct LONE
      : terminal<lit<"/", !cr<"/">> | lit<"[", !cr<"[">> |
                 lit<"]", !cr<"]">> | lit<"{", !cr<"{">> |
                 lit<"}", !cr<"}">> | lit<"\\", !cr<R"(\\)">> |
                 lit<"~", !cr<R"(^ \t\r\n)">>> {};
  // ...and one a line opens a block with: text past a line's start only.
  struct MID : terminal<lit<"*", !cr<"*">> | lit<"="> | lit<"#">> {};

  // Where text is read: a paragraph, a list item, a table cell, a link's
  // label. A cell's `|` ends it; only paragraphs and items go on past a
  // line break.
  static constexpr int kPara = 0, kItem = 1, kCell = 2, kLabel = 3;

  static constexpr auto line_piece =
      ref<WORD> | ref<OTHER> | ref<LONE> | ref<ESC>;
  static constexpr auto cell_piece = line_piece | ref<MID>;
  static constexpr auto any_piece =
      cell_piece | lit<"|", !cr<"=">> | lit<"|=">;
  template <int X>
  static constexpr auto piece =
      std::conditional_t<X == kCell, decltype(cell_piece),
                         decltype(any_piece)>{};

  template <int X>
  struct TextBody : rule<some(piece<X>)> {};
  template <int X>
  struct LineTextBody : rule<line_piece + many(piece<X>)> {};
  // Text in the middle of a line, and text opening one.
  template <int X>
  struct TextRule : rule<assign<&ast::Text::text>(ref<TextBody<X>>)> {};
  template <int X>
  struct LineTextRule
      : rule<assign<&ast::Text::text>(ref<LineTextBody<X>>)> {};

  // A line break goes on within a paragraph or an item unless the next line
  // opens a block: a list item, a heading, a table row, a rule, a
  // preformatted block -- or, after an item, a deeper item. The markers are
  // read there only for the guard to refuse.
  static constexpr auto leadingBlanks = many(cr<R"( \t)">);
  static constexpr auto opens_other_block =
      (leadingBlanks + (lit<"="> | lit<"|">)) |
      (leadingBlanks + lit<"----", !cr<R"(^ \t\r\n)">>) |
      lit<"{{{", !cr<R"(^ \t\r\n)">>;
  static constexpr auto opens_block =
      opens_other_block | (leadingBlanks + (lit<"*", !cr<"*">> | lit<"#">));
  static constexpr auto block_mark =
      ref<HEAD_MARK> | ref<ITEM_MARK> | ref<HASH_MARK> | ref<ROW_PIPE> |
      ref<ROW_HEAD> | ref<HLINE> | ref<PREFORMATTED>;
  struct ParaBreakRule
      : rule<assign<&ast::Text::text>(ref<EOL>) + !(opens_block | eof) +
             option(block_mark)> {};
  struct ItemBreakRule
      : rule<assign<&ast::Text::text>(ref<EOL>) +
             !(opens_block | (leadingBlanks + lit<"**">) | eof) +
             option(block_mark | ref<SUBITEM_MARK>)> {};

  struct HrefRule;
  struct ImageRule;
  struct NowikiRule;
  struct LineBreakRule;
  struct UrlRule;
  template <int X>
  struct BoldRule;
  template <int X>
  struct ItalicsRule;
  template <int X>
  struct OpenBoldRule;
  template <int X>
  struct OpenItalicsRule;
  template <int X>
  struct InnerOpenBoldRule;
  template <int X>
  struct InnerOpenItalicsRule;

  static constexpr auto plain_marks = ref<HrefRule> | ref<ImageRule> |
                                      ref<NowikiRule> | ref<LineBreakRule> |
                                      ref<UrlRule>;
  template <int X>
  static constexpr auto closed_marks =
      ref<BoldRule<X>> | ref<ItalicsRule<X>> | plain_marks;
  template <int X>
  static constexpr auto open_marks =
      ref<OpenBoldRule<X>> | ref<OpenItalicsRule<X>>;

  // Inline content, two texts never side by side: markup, then the text
  // after it; a line break, then the text opening the next line.
  template <auto M, int X, class Closed>
  static constexpr auto flow_body = std::conditional_t<
      X == kPara || X == kItem,
      decltype(many((append<M>(Closed{}) +
                     option(append<M>(ref<TextRule<X>>))) |
                    (append<M>(std::conditional_t<X == kItem,
                                                  decltype(ref<ItemBreakRule>),
                                                  decltype(ref<ParaBreakRule>)>{}) +
                     option(append<M>(ref<LineTextRule<X>>))))),
      decltype(many(append<M>(Closed{}) +
                    option(append<M>(ref<TextRule<X>>))))>{};
  // A bold or an italics left open runs to the end of the content holding
  // it, so it comes last; one opened inside another holds no other.
  template <auto M, int X, class Closed, class Open>
  static constexpr auto flow = option(append<M>(ref<TextRule<X>>)) +
                               flow_body<M, X, Closed> +
                               option(append<M>(Open{}));
  template <auto M, int X, class Closed>
  static constexpr auto inner_flow =
      option(append<M>(ref<TextRule<X>>)) + flow_body<M, X, Closed>;

  // Neither opens directly inside itself: a `**` among a bold's own content
  // closes it.
  template <int X>
  static constexpr auto bold_marks = ref<ItalicsRule<X>> | plain_marks;
  template <int X>
  static constexpr auto italics_marks = ref<BoldRule<X>> | plain_marks;
  template <int X>
  struct BoldRule
      : rule<lit<"**"> +
             flow<&ast::Bold::content, X, decltype(bold_marks<X>),
                  decltype(ref<InnerOpenItalicsRule<X>>)> +
             lit<"**">> {};
  template <int X>
  struct OpenBoldRule
      : rule<lit<"**"> + flow<&ast::Bold::content, X, decltype(bold_marks<X>),
                              decltype(ref<InnerOpenItalicsRule<X>>)>> {};
  template <int X>
  struct InnerOpenBoldRule
      : rule<lit<"**"> +
             inner_flow<&ast::Bold::content, X, decltype(plain_marks)>> {};
  template <int X>
  struct ItalicsRule
      : rule<lit<"//"> +
             flow<&ast::Italics::content, X, decltype(italics_marks<X>),
                  decltype(ref<InnerOpenBoldRule<X>>)> +
             lit<"//">> {};
  template <int X>
  struct OpenItalicsRule
      : rule<lit<"//"> +
             flow<&ast::Italics::content, X, decltype(italics_marks<X>),
                  decltype(ref<InnerOpenBoldRule<X>>)>> {};
  template <int X>
  struct InnerOpenItalicsRule
      : rule<lit<"//"> +
             inner_flow<&ast::Italics::content, X, decltype(plain_marks)>> {};

  // A link's or an image's target is not markup: its `//` is its own.
  struct TargetBody
      : rule<some(ref<WORD> | ref<OTHER> | ref<LONE> | ref<MID> | ref<ESC> |
                  ref<URL> | lit<"//"> | lit<"**"> | lit<"\\\\">)> {};
  struct TargetRule : rule<assign<&ast::Text::text>(ref<TargetBody>)> {};
  struct AltBody
      : rule<some(ref<WORD> | ref<OTHER> | ref<LONE> | ref<MID> | ref<ESC> |
                  ref<URL> | lit<"//"> | lit<"**"> | lit<"\\\\"> |
                  lit<"|", !cr<"=">> | lit<"|=">)> {};
  struct AltRule : rule<assign<&ast::Text::text>(ref<AltBody>)> {};

  struct HrefRule
      : rule<lit<"[["> + assign<&ast::Href::target>(ref<TargetRule>) +
             option(lit<"|"> +
                    flow<&ast::Href::label, kLabel,
                         decltype(closed_marks<kLabel>),
                         decltype(open_marks<kLabel>)>) +
             lit<"]]">> {};
  struct ImageRule
      : rule<lit<"{{"> + assign<&ast::Image::target>(ref<TargetRule>) +
             option(lit<"|"> + assign<&ast::Image::alt>(ref<AltRule>)) +
             lit<"}}">> {};
  struct UrlTextRule : rule<assign<&ast::Text::text>(ref<URL>)> {};
  struct UrlRule : rule<assign<&ast::Href::target>(ref<UrlTextRule>)> {};
  struct NowikiRule : rule<assign<&ast::Nowiki::text>(ref<NOWIKI>)> {};
  struct LineBreakRule : rule<create<ast::LineBreak> + lit<"\\\\">> {};

  // A paragraph opens on text that may open a line, or on markup.
  template <auto M, int X>
  static constexpr auto line_flow =
      (append<M>(ref<LineTextRule<X>>) +
       flow_body<M, X, decltype(closed_marks<X>)> +
       option(append<M>(open_marks<X>))) |
      (append<M>(closed_marks<X>) + option(append<M>(ref<TextRule<X>>)) +
       flow_body<M, X, decltype(closed_marks<X>)> +
       option(append<M>(open_marks<X>))) |
      append<M>(open_marks<X>);
  struct ParagraphRule
      : rule<line_flow<&ast::Paragraph::content, kPara>> {};

  // Its text unparsed.
  struct HeadingTextRule : rule<assign<&ast::Text::text>(ref<HEAD_TEXT>)> {};
  struct TitleRule
      : rule<assign<&ast::Title::level>(ref<HEAD_MARK>) +
             option(assign<&ast::Title::content>(ref<HeadingTextRule>))> {};

  template <auto M>
  static constexpr auto item_text =
      flow<M, kItem, decltype(closed_marks<kItem>),
           decltype(open_marks<kItem>)>;
  struct BulletItemRule
      : rule<assign<&ast::BulletItem::level>(ref<ITEM_MARK>) +
             item_text<&ast::BulletItem::content>> {};
  struct SubItemRule
      : rule<assign<&ast::BulletItem::level>(ref<SUBITEM_MARK>) +
             item_text<&ast::BulletItem::content>> {};
  struct NumberedItemRule
      : rule<assign<&ast::NumberedItem::level>(ref<HASH_MARK>) +
             item_text<&ast::NumberedItem::content>> {};

  static constexpr auto cell_text =
      flow<&ast::TableCell::content, kCell, decltype(closed_marks<kCell>),
           decltype(open_marks<kCell>)>;
  struct FirstCellRule
      : rule<(ref<ROW_PIPE> | flag<&ast::TableCell::header>(
                                  ref<ROW_HEAD>)) +
             cell_text> {};
  struct TableCellRule
      : rule<((lit<"|", !cr<"=">> + !eof) |
              flag<&ast::TableCell::header>(lit<"|=">)) +
             cell_text> {};
  // The ending pipe is optional, and with it the row reads its line break --
  // or ends the text, where a pipe opens no cell.
  struct TableRowRule
      : rule<append<&ast::TableRow::cells>(ref<FirstCellRule>) +
             many(append<&ast::TableRow::cells>(ref<TableCellRule>))> {};
  struct EndedRowRule
      : rule<append<&ast::TableRow::cells>(ref<FirstCellRule>) +
             many(append<&ast::TableRow::cells>(ref<TableCellRule>)) +
             (ref<PIPE_END> | (lit<"|", !cr<"=">> + eof))> {};

  struct HlineRule : rule<create<ast::Hline> + ref<HLINE>> {};
  struct PreformattedRule
      : rule<assign<&ast::Preformatted::text>(ref<PREFORMATTED>)> {};

  // A list: its items one per line, a deeper `**` item only after another.
  static constexpr auto list =
      append<&ast::Document::blocks>(ref<BulletItemRule> |
                                     ref<NumberedItemRule>) +
      many(ref<EOL> +
           append<&ast::Document::blocks>(ref<BulletItemRule> |
                                          ref<SubItemRule> |
                                          ref<NumberedItemRule>));

  // A paragraph or a list ends on blank lines, or on a line opening a block;
  // a block whose last token read its line break needs nothing more. A
  // block ending mid-line without either is the document's last.
  static constexpr auto runs_on =
      ref<BLANK> | (ref<EOL> + (&opens_block | eof));
  static constexpr auto line_end = ref<EOL> | ref<BLANK>;
  static constexpr auto unit =
      (list + (ref<BLANK> | (ref<EOL> + (&opens_other_block | eof)))) |
      (append<&ast::Document::blocks>(ref<ParagraphRule>) + runs_on) |
      (append<&ast::Document::blocks>(ref<TitleRule> | ref<TableRowRule> |
                                      ref<PreformattedRule>) +
       line_end) |
      (append<&ast::Document::blocks>(ref<EndedRowRule> | ref<HlineRule>) +
       option(ref<BLANK_LINES>));
  static constexpr auto last =
      list | append<&ast::Document::blocks>(
                 ref<ParagraphRule> | ref<TitleRule> | ref<TableRowRule> |
                 ref<PreformattedRule>);

  struct DocRule
      : rule<create<ast::Document> + option(ref<BLANK_LINES>) + many(unit) +
             option(last)> {};

  using entry = DocRule;
};

}  // namespace creole
