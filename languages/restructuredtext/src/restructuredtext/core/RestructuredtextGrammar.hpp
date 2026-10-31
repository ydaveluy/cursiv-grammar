// The restructuredtext language: its grammar. Line breaks are tokens, and a
// line's indentation opens and closes blocks (`indent`/`dedent`), so the
// spaces between words are trivia, kept in the CST only.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "restructuredtext/core/ast.hpp"
#include "restructuredtext/core/InlineValue.hpp"

namespace restructuredtext {

inline constexpr auto trim1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto trim2 = [](std::string_view t) {
  return t.substr(2, t.size() - 4);
};

// A simple reference's trailing underscore is dropped.
inline constexpr auto drop_last = [](std::string_view t) {
  return t.substr(0, t.size() - 1);
};

// One marker byte: the bullet character, or the run of `=`/`-`/`+`/`^`.
inline constexpr auto first_byte = [](std::string_view t) { return t.substr(0, 1); };

// A backtick span's content, its opener, closer and trailing `_` dropped.
inline constexpr auto trim_hyperlink = [](std::string_view t) {
  return t.substr(1, t.size() - 3);
};

struct Restructuredtext : cursiv::GrammarSpec {
  static constexpr auto line_break = lit<"\r\n"> | lit<"\n">;
  struct EOL : terminal<line_break> {};
  // A line break and the blank lines after it: what ends a paragraph.
  struct BLANK
      : terminal<line_break + some(many(cr<R"( \t)">) + line_break)> {};
  struct SP : terminal<some(cr<R"( \t)">)> {};

  // A doctest starts with a prompt at the beginning of a fresh block and
  // holds every nonblank physical line, including output and indentation.
  static constexpr auto nonblank_line =
      many(cr<R"( \t)">) + cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">);
  struct DOCTEST_BLOCK : terminal<
      lit<">>>", &(lit<" "> | line_break | eof)> +
      many(cr<R"(^\r\n)">) + many(line_break + nonblank_line)> {};

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
  struct SectionSep : terminal<as<first_byte>(runs_of<'-', '=', '+', '^'>)> {};
  static constexpr auto adornment4 =
      runs4_of<'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
               '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\',
               ']', '^', '_', '`', '{', '|', '}', '~'>;
  struct ADORN
      : terminal<as<first_byte>((adornment + many(cr<R"( \t)">) + line_break) |
                                (adornment4 + many(cr<R"( \t)">) + eof))> {};
  template<char... Cs>
  static constexpr auto title_runs = ((lit<cursiv::str(kOne<Cs>)> +
      many(lit<cursiv::str(kOne<Cs>)>)) | ...);
  struct TITLE_ADORN : terminal<as<first_byte>(
      title_runs<'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',',
                 '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\',
                 ']', '^', '_', '`', '{', '|', '}', '~'> +
      many(cr<R"( \t)">) + (line_break | eof))> {};
  struct SepRun : terminal<repeat<1, 2>(sepchar)> {};

  static constexpr auto word_body = some(cr<"A-Za-z0-9">);
  struct WORD : terminal<word_body> {};

  // Everything else running text can hold, one run at a time: neither a
  // word byte nor a byte another terminal below owns.
  struct PUNCT : terminal<some(cr<R"(^-=+^A-Za-z0-9 \t\r\n*`_)">)> {};

  static constexpr auto reference_name = word_body + many(cr<"-._+:"> + word_body);
  struct REFERENCE
      : terminal<as<drop_last>(reference_name + lit<"_", !cr<"A-Za-z0-9">>)> {};
  static constexpr auto escaped = lit<"\\"> + cr<R"(^\r\n)">;
  struct HYPERLINK_TARGET : terminal<as<inline_text<2,1>>(
      lit<"_`"> + some(escaped | (!lit<"`"> + any)) + lit<"`">)> {};
  struct ESCAPED : terminal<as<inline_text<0,0>>(escaped)> {};
  struct BARE_TICK : terminal<lit<"`">> {};
  struct BARE_US : terminal<lit<"_">> {};
  struct BARE_STAR : terminal<lit<"*">> {};

  struct EMPHASIS
      : terminal<as<inline_text<1,1>>(lit<"*", !cr<R"( \t\r\n)">> +
          some(escaped | (!lit<"*"> + any)) + lit<"*">)> {};
  struct STRONG
      : terminal<as<inline_text<2,2>>(lit<"**", !cr<R"( \t\r\n)">> +
          some(escaped | (!lit<"**"> + any)) + lit<"**">)> {
  };
  // Docutils distinguishes double-backtick literals from interpreted text.
  // The first matching closer ends each span; a lone tick remains literal
  // content inside a double-backtick span.
  struct BACKTICK_SPAN
      : terminal<as<trim2>(lit<"``", !cr<R"( \t\r\n)">> +
                           some(!lit<"``"> + any) + lit<"``">)> {};
  struct INTERPRETED_SPAN
      : terminal<as<inline_text<1,1>>(lit<"`", !cr<R"( \t\r\n)">> + some(escaped | (!lit<"`"> + any)) +
                           lit<"`">)> {};
  // The same span with a word-bounded trailing `_`: one byte longer, so the
  // longest match always prefers it over a bare literal when it applies --
  // resolved by the lexer, not by parser lookahead past the closing tick.
  struct HYPERLINK_SPAN
      : terminal<as<trim_hyperlink>(through(lit<"`">, lit<"`">) +
                                    lit<"_", !cr<"A-Za-z0-9">>)> {};

  // A bare marker byte ties in length with the plain-text terminals that
  // also read it (SepRun, BARE_STAR): measured, the lexer's multi-candidate
  // resolution then favours the plain-text reading and no BulletItem is
  // produced at all. The trailing word-space is required IN THE TERMINAL so
  // longest match, not candidate order, settles it.
  struct BULLET_MARK : terminal<as<first_byte>(cr<"-*+"> + cr<R"( \t)">)> {};
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

  struct Emphasis
      : rule<assign<&ast::Emphasis::text>(ref<EMPHASIS>)> {};
  struct Strong : rule<assign<&ast::Strong::text>(ref<STRONG>)> {};
  struct Literal
      : rule<assign<&ast::Literal::text>(ref<BACKTICK_SPAN>)> {};
  struct Hyperlink
      : rule<assign<&ast::Hyperlink::text>(ref<HYPERLINK_SPAN>)> {};
  // Explicit roles may precede or follow interpreted text. An absent role
  // retains the default interpreted-text meaning, rather than a literal.
  static constexpr auto role_name =
      word_body + many(cr<"-._+:"> + word_body);
  struct ROLE : terminal<as<trim1>(lit<":"> + role_name + lit<":">)> {};
  // A role prefix must touch its backtick. Keeping this condition lexical
  // also lets ordinary ":name: text" remain prose outside a field list.
  struct ROLE_PREFIX : terminal<as<trim1>(
      lit<":"> + role_name + lit<":", &lit<"`">>)> {};
  struct InterpretedText
      : rule<(assign<&ast::InterpretedText::role>(ref<ROLE_PREFIX>) +
              assign<&ast::InterpretedText::text>(ref<INTERPRETED_SPAN>)) |
             (assign<&ast::InterpretedText::text>(ref<INTERPRETED_SPAN>) +
              option(assign<&ast::InterpretedText::role>(ref<ROLE>)))> {};
  struct Reference
      : rule<assign<&ast::Reference::name>(ref<REFERENCE>)> {};
  struct HyperlinkTarget
      : rule<assign<&ast::HyperlinkTarget::name>(ref<HYPERLINK_TARGET>)> {};

  static constexpr auto span =
      ref<Hyperlink> | ref<Literal> | ref<InterpretedText> |
      ref<Strong> | ref<Emphasis> | ref<Reference> |
      ref<HyperlinkTarget> | ref<WORD> | ref<PUNCT> | ref<SepRun> |
      ref<ESCAPED> | ref<BARE_US> | ref<BARE_STAR> | ref<BARE_TICK>;

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
  struct Paragraph
      : rule<text_line<&ast::Paragraph::spans> +
             many(append<&ast::Paragraph::spans>(ref<EOL>) +
                  !block_start_guard +
                  append<&ast::Paragraph::spans>(span | block_start_marker) +
                  many(append<&ast::Paragraph::spans>(span)))> {};

  struct DoctestBlock : rule<
      assign<&ast::DoctestBlock::text>(ref<DOCTEST_BLOCK>) +
      (ref<EOL> | ref<BLANK> | eof)> {};

  // Docutils' field marker permits an internal colon except before a
  // space/backtick or line end; escaping it permits those contexts too.
  static constexpr auto field_colon =
      lit<":", !cr<R"( `\t\r\n)">> + !eof;
  static constexpr auto field_piece =
      escaped | field_colon | cr<R"(^:\\\r\n)">;
  static constexpr auto field_last =
      escaped | field_colon | cr<R"(^:\\ \t\r\n)">;
  inline static constexpr auto field_name_text = [](std::string_view text) {
    std::size_t end = text.size();
    while (end && (text[end - 1] == ' ' || text[end - 1] == '\t')) --end;
    return end >= 2 ? text.substr(1, end - 2) : std::string_view{};
  };
  struct FIELD_MARK : terminal<as<field_name_text>(
      lit<":"> + !cr<R"(: \t\r\n)"> + many(field_piece) + field_last +
      lit<":"> + (some(cr<R"( \t)">) | &(line_break | eof)))> {};

  struct LineBlockLine
      : rule<ref<BLOCK_MARK> +
             text_line<&ast::LineBlockLine::spans>> {};
  struct LineBlock
      : rule<some(append<&ast::LineBlock::lines>(ref<LineBlockLine>),
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
  struct DirectiveBody : rule<create<ast::DirectiveBody>(
                                 ref<INDENT> +
                                 many(append<&ast::DirectiveBody::entries>(
                                     ref<REST_OF_LINE> | ref<EOL> | ref<BLANK> |
                                     ref<DirectiveBody>)) +
                                 ref<DEDENT>)> {};
  struct Directive
      : rule<create<ast::Directive>(
            ref<COMMENT_MARK> +
            assign<&ast::Directive::header>(ref<DIRECTIVE_HEADER>) +
            (ref<EOL> | ref<BLANK>)+option(
                assign<&ast::Directive::body>(ref<DirectiveBody>)))> {};
  struct LastDirective
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Directive::header>(ref<DIRECTIVE_HEADER>)> {};
  static constexpr auto target_name_body = lit<"_"> +
      ((lit<"`"> + some(escaped | (!lit<"`"> + any)) + lit<"`">) |
       some(escaped | cr<R"(^:\r\n)">)) + lit<":">;
  struct TARGET_NAME : terminal<as<target_name>(target_name_body)> {};
  // Exclude target headers in the comment token itself. A parser guard
  // cannot make a shorter target-label token beat a full comment line.
  struct COMMENT_TEXT : terminal<!target_name_body +
      cr<R"(^ \t\r\n)"> + many(cr<R"(^\r\n)">)> {};
  struct ANONYMOUS_TARGET : terminal<lit<"__:">> {};
  static constexpr auto target_header = ref<COMMENT_MARK> +
      (flag<&ast::ExplicitTarget::anonymous>(ref<ANONYMOUS_TARGET>) |
       assign<&ast::ExplicitTarget::name>(ref<TARGET_NAME>)) +
      option(assign<&ast::ExplicitTarget::destination>(ref<REST_OF_LINE>));
  struct ExplicitTarget : rule<create<ast::ExplicitTarget>(target_header +
      (ref<EOL> | ref<BLANK>) +
      option(assign<&ast::ExplicitTarget::continuation>(ref<DirectiveBody>)))> {};
  struct LastExplicitTarget : rule<create<ast::ExplicitTarget>(target_header)> {};
  struct Comment
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Comment::firstLine>(ref<COMMENT_TEXT>) +
             (ref<EOL> | ref<BLANK>) + option(raw_block)> {};
  struct LastComment
      : rule<ref<COMMENT_MARK> +
             assign<&ast::Comment::firstLine>(ref<COMMENT_TEXT>)> {};
  struct MarkComment : rule<create<ast::Comment> + ref<COMMENT_MARK>> {};
  struct EmptyComment
      : rule<create<ast::Comment> + ref<COMMENT_MARK> + ref<EOL> +
             option(raw_block)> {};
  struct BareComment
      : rule<create<ast::Comment> + ref<BARE_COMMENT> + option(raw_block)> {};

  struct BlockQuote;
  struct DefinitionItem;
  struct BulletItemRule;
  struct LastBullet;
  struct EnumItem;
  struct LastEnum;
  struct Section;
  struct FieldList;

  struct TRANSITION_ADORN
      : terminal<adornment4 + many(cr<R"( \t)">) + (line_break | eof)> {};
  struct Transition
      : rule<create<ast::Transition> + &ref<TRANSITION_ADORN> + ref<ADORN>> {};

  // A block quote or a transition opens only after a blank line, and a
  // blank line or the end follows a transition.
  template <auto Body>
  static constexpr auto transition =
      append<Body>(ref<Transition>) +
      (eof | ((ref<EOL> | ref<BLANK>) +
              option(append<Body>(ref<BlockQuote>))));
  template <auto Body>
  static constexpr auto fresh_block =
      append<Body>(ref<BlockQuote> | ref<DoctestBlock> | ref<FieldList>) |
      transition<Body>;
  template <auto Body>
  static constexpr auto after_blank = many(fresh_block<Body>);
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
      ((append<Body>(ref<BulletItemRule>) | append<Body>(ref<EnumItem>) |
        append<Body>(ref<ExplicitTarget> | ref<Directive> | ref<Comment>)) +
       after_blank<Body>) |
      (append<Body>(ref<MarkComment>) + ref<BLANK> + after_blank<Body>) |
      (append<Body>(ref<EmptyComment>) + after_line<Body>) |
      (append<Body>(ref<BareComment>) + after_line<Body>) |
      (append<Body>(ref<LineBlock>) + line_end<Body>) |
      (append<Body>(ref<DefinitionItem>) + after_blank<Body>) |
      (append<Body>(ref<Paragraph>) + line_end<Body>);
  template <auto Body>
  static constexpr auto last =
      append<Body>(ref<LastBullet>) | append<Body>(ref<LastEnum>) |
      append<Body>(ref<LastExplicitTarget> | ref<LastDirective> | ref<LastComment>) |
      append<Body>(ref<MarkComment>) | append<Body>(ref<LineBlock>) |
      append<Body>(ref<Paragraph>);
  template <auto Body>
  static constexpr auto suite =
      ref<INDENT> +
      ((some(fresh_block<Body>) + many(unit<Body>) + option(last<Body>)) |
       (many(unit<Body>) + last<Body>) | some(unit<Body>)) +
      ref<DEDENT>;

  struct BlockQuote : rule<suite<&ast::BlockQuote::body>> {};
  struct DefinitionItem
      : rule<text_line<&ast::DefinitionItem::term> + ref<EOL> +
             suite<&ast::DefinitionItem::body>> {};

  // An item's first line and its line break, alone or continued by the
  // lines indented below it, which join it whatever they open; the line
  // break ending them is theirs too.
  struct ItemLine
      : rule<text_line<&ast::Paragraph::spans> + ref<EOL>> {};
  static constexpr auto item_more_line =
      append<&ast::Paragraph::spans>(span | block_start_marker) +
      many(append<&ast::Paragraph::spans>(span));
  struct ItemText
      : rule<text_line<&ast::Paragraph::spans> +
             append<&ast::Paragraph::spans>(ref<EOL>) + ref<INDENT> +
             item_more_line +
             many(append<&ast::Paragraph::spans>(ref<EOL>) + item_more_line) +
             option(ref<EOL> | ref<BLANK>)> {};
  struct ItemOnly : rule<text_line<&ast::Paragraph::spans>> {};
  // An item's body goes on indented: in the block its continued text opens,
  // past a blank line, or right below a marker alone on its line.
  template <auto Body>
  static constexpr auto item_rest =
      (append<Body>(ref<ItemText>) + many(unit<Body>) +
       option(last<Body>) + ref<DEDENT>) |
      append<Body>(ref<ItemLine>) |
      (append<Body>(ref<ItemOnly>) + ref<BLANK> + option(suite<Body>)) |
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
  struct LastBullet
      : rule<bullet + option(append<&ast::BulletItem::body>(
                          ref<ItemOnly>))> {};
  struct EnumItem
      : rule<assign<&ast::EnumItem::number>(ref<ENUM_MARK>) +
             item_rest<&ast::EnumItem::body>> {};
  struct LastEnum
      : rule<assign<&ast::EnumItem::number>(ref<ENUM_MARK>) +
             option(append<&ast::EnumItem::body>(ref<ItemOnly>))> {};

  struct FieldItem : rule<assign<&ast::FieldItem::name>(ref<FIELD_MARK>) +
                         item_rest<&ast::FieldItem::body>> {};
  struct LastField : rule<assign<&ast::FieldItem::name>(ref<FIELD_MARK>) +
      option(append<&ast::FieldItem::body>(ref<ItemOnly>)) + eof> {};
  struct FieldList : rule<
      (some(append<&ast::FieldList::fields>(ref<FieldItem>)) +
       option(append<&ast::FieldList::fields>(ref<LastField>))) |
      append<&ast::FieldList::fields>(ref<LastField>)> {};

  // Title and underline only: see ast.hpp for how a section's own trailing
  // content is grouped from the flat list this produces. An overlined title
  // may be inset.
  struct Section
      : rule<((ref<ADORN> + ref<INDENT> + text_line<&ast::Section::title> +
               ref<EOL> + ref<DEDENT>) |
              (option(ref<ADORN>) + text_line<&ast::Section::title> +
               ref<EOL>)) +
             assign<&ast::Section::underline>(ref<TITLE_ADORN>)> {};

  // The document may open on a block quote or a transition. The section comes
  // before the unit: text over an adornment line that a paragraph could take
  // as prose is a title, whatever the adornment byte.
  struct Doc
      : rule<create<ast::Document> + option(ref<EOL> | ref<BLANK>) +
             after_blank<&ast::Document::blocks> +
             many((append<&ast::Document::blocks>(ref<Section>) +
                   after_line<&ast::Document::blocks>) |
                  unit<&ast::Document::blocks>) +
             option(last<&ast::Document::blocks>)> {};

  using entry = Doc;
  using hidden = list<SP>;
};

}  // namespace restructuredtext
