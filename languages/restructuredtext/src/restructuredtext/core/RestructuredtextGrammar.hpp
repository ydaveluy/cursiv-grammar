// The restructuredtext language: its grammar. Line breaks are tokens, and a
// line's indentation opens and closes blocks (`indent`/`dedent`), so the
// spaces between words are trivia, kept in the CST only.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "restructuredtext/core/ast.hpp"

namespace restructuredtext {

inline constexpr auto trim1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Trim1 = cursiv::text_conv<trim1>;

inline constexpr auto trim2 = [](std::string_view t) {
  return t.substr(2, t.size() - 4);
};
using Trim2 = cursiv::text_conv<trim2>;

// A reference/target's trailing or leading `_` dropped.
inline constexpr auto drop_last = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};
using DropLast = cursiv::text_conv<drop_last>;

inline constexpr auto drop_first = [](std::string_view t) {
  return t.substr(1);
};
using DropFirst = cursiv::text_conv<drop_first>;

// One marker byte: the bullet character, or the run of `=`/`-`/`+`/`^`.
inline constexpr auto first_byte = [](std::string_view t) { return t.substr(0, 1); };
using FirstByte = cursiv::text_conv<first_byte>;

// A backtick span's content, its opener, closer and trailing `_` dropped.
inline constexpr auto trim_hyperlink = [](std::string_view t) {
  return t.substr(1, t.size() - 3);
};
using TrimHyperlink = cursiv::text_conv<trim_hyperlink>;

struct Restructuredtext : cursiv::GrammarSpec {
  static constexpr auto line_break = lit<"\r\n"> | lit<"\n">;
  struct EOL : terminal<line_break> {};
  // A line break and the blank lines after it: what ends a paragraph.
  struct BLANK
      : terminal<line_break + some(many(cr<R"( \t)">) + line_break)> {};
  struct SP : terminal<some(cr<R"( \t)">)> {};

  static constexpr auto sepchar = cr<"-=+^">;
  template <char C>
  static constexpr char kThree[] = {C, C, C, '\0'};
  template <char C>
  static constexpr char kFour[] = {C, C, C, C, '\0'};
  template <char C>
  static constexpr char kOne[] = {C, '\0'};
  template <char... Cs>
  static constexpr auto runs_of =
      ((lit<cursiv::str(kThree<Cs>)> + many(lit<cursiv::str(kOne<Cs>)>)) |
       ...);
  template <char... Cs>
  static constexpr auto runs4_of =
      ((lit<cursiv::str(kFour<Cs>)> + many(lit<cursiv::str(kOne<Cs>)>)) | ...);
  static constexpr auto adornment =
      runs_of<'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
              '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\',
              ']', '^', '_', '`', '{', '|', '}', '~'>;
  // A line of three or more of one punctuation byte and its line break:
  // longer than any run of text, so a title's underline or overline is never
  // prose. Where the input ends it needs four: three alone there is text.
  // `SectionSep` is the same run of `-=+^` without the line break.
  struct SectionSep : terminal<runs_of<'-', '=', '+', '^'>, FirstByte> {};
  static constexpr auto adornment4 =
      runs4_of<'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
               '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\',
               ']', '^', '_', '`', '{', '|', '}', '~'>;
  struct ADORN
      : terminal<(adornment + many(cr<R"( \t)">) + line_break) |
                     (adornment4 + many(cr<R"( \t)">) + eof),
                 FirstByte> {};
  struct SepRun : terminal<repeat<1, 2>(sepchar)> {};

  static constexpr auto word_body = some(cr<"A-Za-z0-9">);
  struct WORD : terminal<word_body> {};

  // Everything else running text can hold, one run at a time: neither a
  // word byte nor a byte another terminal below owns.
  struct PUNCT : terminal<some(cr<R"(^-=+^A-Za-z0-9 \t\r\n*`_)">)> {};

  struct REFERENCE
      : terminal<word_body + lit<"_", !cr<"A-Za-z0-9">>, DropLast> {};
  struct HYPERLINK_TARGET
      : terminal<lit<"_"> + word_body, DropFirst> {};
  struct BARE_US : terminal<lit<"_">> {};
  struct BARE_STAR : terminal<lit<"*">> {};

  struct EMPHASIS
      : terminal<through(lit<"*", !cr<R"( \t\r\n)">>, lit<"*">), Trim1> {};
  struct STRONG
      : terminal<through(lit<"**", !cr<R"( \t\r\n)">>, lit<"**">), Trim2> {};
  // Docutils distinguishes double-backtick literals from interpreted text.
  // The first matching closer ends each span; a lone tick remains literal
  // content inside a double-backtick span.
  struct BACKTICK_SPAN
      : terminal<lit<"``", !cr<R"( \t\r\n)">> +
                     some(!lit<"``"> + any) + lit<"``">,
                 Trim2> {};
  struct INTERPRETED_SPAN
      : terminal<lit<"`", !cr<R"( \t\r\n)">> +
                     some(!lit<"`"> + any) + lit<"`">,
                 Trim1> {};
  // The same span with a word-bounded trailing `_`: one byte longer, so the
  // longest match always prefers it over a bare literal when it applies --
  // resolved by the lexer, not by parser lookahead past the closing tick.
  struct HYPERLINK_SPAN
      : terminal<through(lit<"`">, lit<"`">) + lit<"_", !cr<"A-Za-z0-9">>,
                 TrimHyperlink> {};

  // A bare marker byte ties in length with the plain-text terminals that
  // also read it (SepRun, BARE_STAR): measured, the lexer's multi-candidate
  // resolution then favours the plain-text reading and no BulletItem is
  // produced at all. The trailing word-space is required IN THE TERMINAL so
  // longest match, not candidate order, settles it.
  struct BULLET_MARK : terminal<cr<"-*+"> + cr<R"( \t)">, FirstByte> {};
  // An enumerator is followed by whitespace: `1.5` is text.
  struct ENUM_MARK : terminal<some(d) + lit<".", !cr<R"(^ \t\r\n)">>> {};
  struct BLOCK_MARK : terminal<lit<"|"> + cr<R"( \t)">> {};
  struct COMMENT_MARK : terminal<lit<".. ">> {};
  // `..` alone on its line, its line break read with it.
  struct BARE_COMMENT
      : terminal<lit<".."> + many(cr<R"( \t)">) + line_break> {};
  // Opens on a visible byte: the blanks before it are trivia.
  struct REST_OF_LINE
      : terminal<cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">)> {};
  struct INDENT : terminal<indent> {};
  struct DEDENT : terminal<dedent> {};

  struct EmphasisRule
      : rule<assign<&ast::Emphasis::text>(ref<EMPHASIS>)> {};
  struct StrongRule : rule<assign<&ast::Strong::text>(ref<STRONG>)> {};
  struct LiteralRule
      : rule<assign<&ast::Literal::text>(ref<BACKTICK_SPAN>)> {};
  struct HyperlinkRule
      : rule<assign<&ast::Hyperlink::text>(ref<HYPERLINK_SPAN>)> {};
  // Explicit roles may precede or follow interpreted text. An absent role
  // retains the default interpreted-text meaning, rather than a literal.
  static constexpr auto role_name =
      word_body + many(cr<"-._+:"> + word_body);
  struct ROLE : terminal<lit<":"> + role_name + lit<":">, Trim1> {};
  struct InterpretedTextRule
      : rule<(assign<&ast::InterpretedText::role>(ref<ROLE>) +
              assign<&ast::InterpretedText::text>(ref<INTERPRETED_SPAN>)) |
             (assign<&ast::InterpretedText::text>(ref<INTERPRETED_SPAN>) +
              option(assign<&ast::InterpretedText::role>(ref<ROLE>)))> {};
  struct ReferenceRule
      : rule<assign<&ast::Reference::name>(ref<REFERENCE>)> {};
  struct HyperlinkTargetRule
      : rule<assign<&ast::HyperlinkTarget::name>(ref<HYPERLINK_TARGET>)> {};

  static constexpr auto span =
      ref<HyperlinkRule> | ref<LiteralRule> | ref<InterpretedTextRule> |
      ref<StrongRule> | ref<EmphasisRule> | ref<ReferenceRule> |
      ref<HyperlinkTargetRule> | ref<WORD> | ref<PUNCT> | ref<SepRun> |
      ref<BARE_US> | ref<BARE_STAR>;

  // Named so a guard can ask of the token in hand; also spliced into the
  // first read of a continuation line, so the marker is a live candidate
  // there for the guard to see -- the guard already refuses it, so that
  // branch is never actually taken.
  static constexpr auto block_start_marker =
      ref<BULLET_MARK> | ref<ENUM_MARK> | ref<BLOCK_MARK> |
      ref<COMMENT_MARK> | ref<BARE_COMMENT> | ref<SectionSep>;
  // What the guard below asks: only a prefix, so the runs are bounded. A
  // guard that may read an unbounded run makes every completion in the
  // document wait on it. Each run is a terminal: a condition reads a
  // sequence past the skipper.
  static constexpr auto block_start_guard =
      ref<BULLET_MARK> |
      ref<terminal<repeat<1, 9>(d) + lit<".", !cr<R"(^ \t\r\n)">>>> |
      ref<BLOCK_MARK> | ref<COMMENT_MARK> | lit<"..", !cr<R"(^ \t\r\n)">> |
      ref<terminal<sepchar + sepchar + sepchar>>;

  template <auto Spans>
  static constexpr auto text_line = some(append<Spans>(span));

  // A paragraph joins physical lines at its own indentation until one of
  // them opens a block; the EOLs between joined lines are kept as spans.
  struct ParagraphRule
      : rule<text_line<&ast::Paragraph::spans> +
             many(append<&ast::Paragraph::spans>(ref<EOL>) +
                  !block_start_guard +
                  append<&ast::Paragraph::spans>(span | block_start_marker) +
                  many(append<&ast::Paragraph::spans>(span)))> {};

  struct LineBlockLineRule
      : rule<ref<BLOCK_MARK> +
             text_line<&ast::LineBlockLine::spans>> {};
  struct LineBlockRule
      : rule<some(append<&ast::LineBlock::lines>(ref<LineBlockLineRule>),
                  ref<EOL>)> {};

  // A comment's body is the indented block that follows. Its lines at the
  // block's own indentation are kept; deeper ones stay in the CST only.
  struct RawNest
      : rule<ref<INDENT> +
             many(ref<REST_OF_LINE> | ref<EOL> | ref<BLANK> | ref<RawNest>) +
             ref<DEDENT>> {};
  static constexpr auto raw_block =
      ref<INDENT> +
      some(append<&ast::Comment::bodyLines>(ref<REST_OF_LINE>) | ref<EOL> |
           ref<BLANK> | ref<RawNest>) +
      ref<DEDENT>;
  // A comment's text and the line break after it, then its body: indented
  // right below, or past a blank line. An empty comment takes only the lines
  // right below it. A comment ending the input has no line break.
  struct DIRECTIVE_HEADER
      : terminal<some(cr<"a-zA-Z0-9_-">) + lit<"::"> + many(cr<R"(^\r\n)">)> {};
  struct DirectiveBodyRule : rule<create<ast::DirectiveBody>(
                                 ref<INDENT> +
                                 many(append<&ast::DirectiveBody::entries>(
                                     ref<REST_OF_LINE> | ref<EOL> | ref<BLANK> |
                                     ref<DirectiveBodyRule>)) +
                                 ref<DEDENT>)> {};
  struct DirectiveRule
      : rule<create<ast::Directive>(
            ref<COMMENT_MARK> +
            assign<&ast::Directive::header>(ref<DIRECTIVE_HEADER>) +
            (ref<EOL> | ref<BLANK>)+option(
                assign<&ast::Directive::body>(ref<DirectiveBodyRule>)))> {};
  struct LastDirectiveRule
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Directive::header>(ref<DIRECTIVE_HEADER>)> {};
  struct CommentRule
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Comment::firstLine>(ref<REST_OF_LINE>) +
             (ref<EOL> | ref<BLANK>) + option(raw_block)> {};
  struct LastCommentRule
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Comment::firstLine>(ref<REST_OF_LINE>)> {};
  struct MarkCommentRule : rule<create<ast::Comment> + ref<COMMENT_MARK>> {};
  struct EmptyCommentRule
      : rule<create<ast::Comment> + ref<COMMENT_MARK> + ref<EOL> +
             option(raw_block)> {};
  struct BareCommentRule
      : rule<create<ast::Comment> + ref<BARE_COMMENT> + option(raw_block)> {};

  struct BlockQuoteRule;
  struct DefinitionItemRule;
  struct BulletItemRule;
  struct LastBulletRule;
  struct EnumItemRule;
  struct LastEnumRule;
  struct SectionRule;

  struct TRANSITION_ADORN
      : terminal<adornment4 + many(cr<R"( \t)">) + (line_break | eof)> {};
  struct TransitionRule
      : rule<create<ast::Transition> + &ref<TRANSITION_ADORN> + ref<ADORN>> {};

  // A block quote or a transition opens only after a blank line, and a
  // blank line or the end follows a transition.
  template <auto Body>
  static constexpr auto transition =
      append<Body>(ref<TransitionRule>) +
      (eof | ((ref<EOL> | ref<BLANK>) +
              option(append<Body>(ref<BlockQuoteRule>))));
  template <auto Body>
  static constexpr auto after_blank =
      option(append<Body>(ref<BlockQuoteRule>) | transition<Body>);
  template <auto Body>
  static constexpr auto line_end = ref<EOL> | (ref<BLANK> + after_blank<Body>);
  // ...and the blank lines after a block whose last token read its own line
  // break.
  template <auto Body>
  static constexpr auto after_line =
      option((ref<EOL> | ref<BLANK>) + after_blank<Body>);

  // One body element and what ends it. An element ending mid-line needs a
  // line end, unless it is the last one of its body; one that reads its
  // own line break may be followed by a transition.
  template <auto Body>
  static constexpr auto unit =
      ((append<Body>(ref<BulletItemRule>) | append<Body>(ref<EnumItemRule>) |
        append<Body>(ref<DirectiveRule> | ref<CommentRule>)) +
       option(transition<Body>)) |
      (append<Body>(ref<MarkCommentRule>) + ref<BLANK> + after_blank<Body>) |
      (append<Body>(ref<EmptyCommentRule>) + after_line<Body>) |
      (append<Body>(ref<BareCommentRule>) + after_line<Body>) |
      (append<Body>(ref<LineBlockRule>) + line_end<Body>) |
      append<Body>(ref<DefinitionItemRule>) |
      (append<Body>(ref<ParagraphRule>) + line_end<Body>);
  template <auto Body>
  static constexpr auto last =
      append<Body>(ref<LastBulletRule>) | append<Body>(ref<LastEnumRule>) |
      append<Body>(ref<LastDirectiveRule> | ref<LastCommentRule>) |
      append<Body>(ref<MarkCommentRule>) | append<Body>(ref<LineBlockRule>) |
      append<Body>(ref<ParagraphRule>);
  template <auto Body>
  static constexpr auto suite =
      ref<INDENT> + ((many(unit<Body>) + last<Body>) | some(unit<Body>)) +
      ref<DEDENT>;

  struct BlockQuoteRule : rule<suite<&ast::BlockQuote::body>> {};
  struct DefinitionItemRule
      : rule<text_line<&ast::DefinitionItem::term> + ref<EOL> +
             suite<&ast::DefinitionItem::body>> {};

  // An item's first line and its line break, alone or continued by the
  // lines indented below it, which join it whatever they open; the line
  // break ending them is theirs too.
  struct ItemLineRule
      : rule<text_line<&ast::Paragraph::spans> + ref<EOL>> {};
  static constexpr auto item_more_line =
      append<&ast::Paragraph::spans>(span | block_start_marker) +
      many(append<&ast::Paragraph::spans>(span));
  struct ItemTextRule
      : rule<text_line<&ast::Paragraph::spans> +
             append<&ast::Paragraph::spans>(ref<EOL>) + ref<INDENT> +
             item_more_line +
             many(append<&ast::Paragraph::spans>(ref<EOL>) + item_more_line) +
             option(ref<EOL> | ref<BLANK>)> {};
  struct ItemOnlyRule : rule<text_line<&ast::Paragraph::spans>> {};
  // An item's body goes on indented: in the block its continued text opens,
  // past a blank line, or right below a marker alone on its line.
  template <auto Body>
  static constexpr auto item_rest =
      (append<Body>(ref<ItemTextRule>) + many(unit<Body>) +
       option(last<Body>) + ref<DEDENT>) |
      append<Body>(ref<ItemLineRule>) |
      (append<Body>(ref<ItemOnlyRule>) + ref<BLANK> + option(suite<Body>)) |
      ((ref<EOL> | ref<BLANK>) + option(suite<Body>));

  // A bullet alone on its line lexes as the plain run a paragraph would open
  // on: the item wins because it precedes the paragraph.
  static constexpr auto lone_bullet = lit<"-", !cr<R"(^\r\n)">> |
                                      lit<"+", !cr<R"(^\r\n)">> |
                                      lit<"*", !cr<R"(^\r\n)">>;
  static constexpr auto bullet =
      assign<&ast::BulletItem::marker>(ref<BULLET_MARK>) |
      (&lone_bullet +
       assign<&ast::BulletItem::marker>(ref<SepRun> | ref<BARE_STAR>));
  struct BulletItemRule
      : rule<bullet + item_rest<&ast::BulletItem::body>> {};
  struct LastBulletRule
      : rule<bullet + option(append<&ast::BulletItem::body>(
                          ref<ItemOnlyRule>))> {};
  struct EnumItemRule
      : rule<assign<&ast::EnumItem::number>(ref<ENUM_MARK>) +
             item_rest<&ast::EnumItem::body>> {};
  struct LastEnumRule
      : rule<assign<&ast::EnumItem::number>(ref<ENUM_MARK>) +
             option(append<&ast::EnumItem::body>(ref<ItemOnlyRule>))> {};

  // Title and underline only: see ast.hpp for how a section's own trailing
  // content is grouped from the flat list this produces. An overlined title
  // may be inset.
  struct SectionRule
      : rule<((ref<ADORN> + ref<INDENT> + text_line<&ast::Section::title> +
               ref<EOL> + ref<DEDENT>) |
              (option(ref<ADORN>) + text_line<&ast::Section::title> +
               ref<EOL>)) +
             assign<&ast::Section::underline>(ref<ADORN> | ref<SectionSep>)> {};

  // The document may open on a block quote or a transition. The section comes
  // before the unit: text over an adornment line that a paragraph could take
  // as prose is a title, whatever the adornment byte.
  struct DocRule
      : rule<create<ast::Document> + option(ref<EOL> | ref<BLANK>) +
             option(append<&ast::Document::blocks>(ref<BlockQuoteRule>) |
                    transition<&ast::Document::blocks>) +
             many((append<&ast::Document::blocks>(ref<SectionRule>) +
                   after_line<&ast::Document::blocks>) |
                  unit<&ast::Document::blocks>) +
             option(last<&ast::Document::blocks>)> {};

  using entry = DocRule;
  using hidden = list<SP>;
};

}  // namespace restructuredtext
