// The clif language: its grammar -- Common Logic Interchange Format
// (ISO/IEC 24707 Annex A.2), an s-expression syntax for first-order logic.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "clif/core/ast.hpp"

namespace clif {

inline constexpr auto unquote1 = [](std::string_view t,
                                    std::pmr::memory_resource* memory) {
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == t.npos) return t;
  const auto room = cursiv::text_room(memory, t.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') { room[n++] = t[i]; continue; }
    const char c = t[++i];
    if (c != 'u' && c != 'U') { room[n++] = c; continue; }
    const auto digits = c == 'u' ? 4u : 6u;
    std::uint32_t cp = 0;
    std::from_chars(t.data() + i + 1, t.data() + i + 1 + digits, cp, 16);
    i += digits;
    if (cp < 0x80) room[n++] = static_cast<char>(cp);
    else {
      if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
      else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
      else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
      if (cp >= 0x10000) room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
      if (cp >= 0x800) room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (cp & 63));
    }
  }
  return std::string_view(room.data(), n);
};

struct Clif : cursiv::GrammarSpec {
  // A.2.2.3 CHAR: every byte a bare name may hold, `-` last to read literally.
  static constexpr auto CHAR =
      cr<"0-9~!#$%^&*_+{}|:<>?`=[];,./A-Za-z-">;
  static constexpr auto NonDigitChar =
      cr<"~!#$%^&*_+{}|:<>?`=[];,./A-Za-z-">;
  // NAMECHARSEQUENCE also carries a quote or a backslash past its first
  // byte; word_boundary must be one class, so the union is spelled once.
  static constexpr auto NameContinue =
      cr<R"(0-9~!#$%^&*_+{}|:<>?`=[];,./A-Za-z-'"\\)">;
  static constexpr auto NonDigitNameContinue =
      cr<R"(~!#$%^&*_+{}|:<>?`=[];,./A-Za-z-'"\\)">;
  static constexpr auto word_boundary = NameContinue;

  static constexpr auto OPEN = lit<"(">;
  static constexpr auto CLOSE = lit<")">;

  struct NUMERAL : terminal<some(d)> {};
  // Excludes a SEQMARK's "..." prefix (the two tie in length and SEQMARK
  // always wins it), and a run that is digits alone: that ties NUMERAL in
  // length too, and a bare digit run reads as a NUMERAL.
  static constexpr auto Reserved =
      kw<"="> | kw<"and"> | kw<"or"> | kw<"iff"> | kw<"if"> | kw<"forall"> |
      kw<"exists"> | kw<"not"> | kw<"roleset:"> | kw<"cl:text"> |
      kw<"cl:imports"> | kw<"cl:excludes"> | kw<"cl:module"> | kw<"cl:comment"> |
      kw<"cl-roleset"> | kw<"cl-text"> | kw<"cl-imports"> | kw<"cl-excludes"> |
      kw<"cl-module"> | kw<"cl-comment">;
  struct NAMECHARSEQUENCE
      : terminal<!Reserved + !lit<"..."> +
                 ((NonDigitChar + many(NameContinue)) |
                  (some(d) + NonDigitNameContinue + many(NameContinue)))> {};
  struct SEQMARK : terminal<lit<"..."> + many(CHAR)> {};
  static constexpr auto Hex = cr<"0-9A-Fa-f">;
  static constexpr auto Escape = lit<"\\"> +
      (cr<R"('"\\)"> | (lit<"u"> + repeat<4>(Hex)) |
       (lit<"U"> + repeat<6>(Hex)));
  struct QUOTEDSTRING : terminal<as<unquote1>(
      lit<"'"> + many(Escape | cr<R"(^'\\)">) + lit<"'">)> {};
  struct ENCLOSEDNAME : terminal<as<unquote1>(
      lit<"\""> + many(Escape | cr<R"(^"\\)">) + lit<"\"">)> {};
  static constexpr auto CommentText = ref<QUOTEDSTRING> | ref<ENCLOSEDNAME>;

  struct WHITE : terminal<some(cr<R"( \t\n\r\v\f)">)> {};
  struct BLOCKCOMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCKCOMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\n\r)">)> {};

  struct BareName : rule<ref<NAMECHARSEQUENCE>> {};


  struct Term;
  struct Application;
  struct CommentedTerm;
  struct Sentence;
  struct Phrase;
  struct Module;
  struct NamedText;
  struct Text;

  // A.2.3.2 name: interpretedname (NUMERAL | QUOTEDSTRING) distributes over
  // two node classes; interpretablename (NAMECHARSEQUENCE | ENCLOSEDNAME)
  // is always a NameTerm.
  struct InterpretedNameTerm
      : rule<assign<&ast::NumeralTerm::value>(ref<NUMERAL>) |
             assign<&ast::StringTerm::value>(ref<QUOTEDSTRING>)> {};

  struct InterpretableNameTerm
      : rule<assign<&ast::NameTerm::value>(ref<BareName> |
                                           ref<ENCLOSEDNAME>)> {};

  struct SeqMarkTerm
      : rule<assign<&ast::SeqMarkTerm::value>(ref<SEQMARK>)> {};

  struct NameAny
      : rule<ref<InterpretedNameTerm> | ref<InterpretableNameTerm>> {};

  // A.2.3.1 termseq: a term, or a sequence marker.
  static constexpr auto TermSeqElem = ref<Term> | ref<SeqMarkTerm>;

  // A.2.3.3 term: a name, an application, or a commented term.
  struct Application
      : rule<OPEN + assign<&ast::Application::op>(ref<Term>) +
             many(append<&ast::Application::args>(TermSeqElem)) + CLOSE> {};

  struct CommentedTerm
      : rule<OPEN + (kw<"cl:comment"> | kw<"cl-comment">) +
             assign<&ast::CommentedTerm::comment>(CommentText) +
             assign<&ast::CommentedTerm::body>(ref<Term>) + CLOSE> {};

  struct Term
      : rule<ref<NameAny> | ref<Application> | ref<CommentedTerm>> {
  };

  // A.2.3.4 equation
  struct Equation
      : rule<OPEN + kw<"="> + assign<&ast::Equation::left>(ref<Term>) +
             assign<&ast::Equation::right>(ref<Term>) + CLOSE> {};

  // A.2.3.6 atomic sentence: a predication, or the role-based form.
  struct Atom
      : rule<OPEN + assign<&ast::Atom::predicate>(ref<Term>) +
             many(append<&ast::Atom::args>(TermSeqElem)) + CLOSE> {};

  struct RoleBinding
      : rule<OPEN + assign<&ast::RoleBinding::role>(ref<NameAny>) +
             assign<&ast::RoleBinding::filler>(ref<Term>) + CLOSE> {};

  struct RolesetAtom
      : rule<OPEN + assign<&ast::RolesetAtom::subject>(ref<Term>) +
             OPEN + (kw<"roleset:"> | kw<"cl-roleset">) +
             many(append<&ast::RolesetAtom::roles>(ref<RoleBinding>)) +
             CLOSE + CLOSE> {};

  // A.2.3.7 boolean sentence
  struct BoolSentence
      : rule<OPEN +
             ((assign<&ast::BoolSentence::op>(as<ast::BoolOp::And>(kw<"and">) | as<ast::BoolOp::Or>(kw<"or">)) +
               many(append<&ast::BoolSentence::operands>(ref<Sentence>))) |
              (assign<&ast::BoolSentence::op>(as<ast::BoolOp::If>(kw<"if">) | as<ast::BoolOp::Iff>(kw<"iff">)) +
               append<&ast::BoolSentence::operands>(ref<Sentence>) +
               append<&ast::BoolSentence::operands>(ref<Sentence>)) |
              (assign<&ast::BoolSentence::op>(as<ast::BoolOp::Not>(kw<"not">)) +
               append<&ast::BoolSentence::operands>(ref<Sentence>))) +
             CLOSE> {};

  // A.2.3.8 quantified sentence
  static constexpr auto BoundName =
      ref<InterpretableNameTerm> | ref<SeqMarkTerm>;

  struct BoundItem
      : rule<assign<&ast::BoundItem::name>(BoundName) |
             (OPEN + assign<&ast::BoundItem::name>(BoundName) +
              assign<&ast::BoundItem::sort>(ref<Term>) + CLOSE)> {};

  struct BoundList
      : rule<create<ast::BoundList> + OPEN +
             many(append<&ast::BoundList::items>(ref<BoundItem>)) +
             CLOSE> {};

  struct QuantSentence
      : rule<OPEN +
             (flag<&ast::QuantSentence::isExists>(kw<"exists">) |
              kw<"forall">) +
             option(assign<&ast::QuantSentence::boundName>(
                 ref<InterpretableNameTerm>)) +
             assign<&ast::QuantSentence::bounds>(ref<BoundList>) +
             assign<&ast::QuantSentence::body>(ref<Sentence>) + CLOSE> {
  };

  // A.2.3.9 commented sentence
  struct CommentedSentence
      : rule<OPEN + (kw<"cl:comment"> | kw<"cl-comment">) +
             assign<&ast::CommentedSentence::comment>(CommentText) +
             assign<&ast::CommentedSentence::body>(ref<Sentence>) +
             CLOSE> {};

  // A.2.3.5 sentence
  struct Sentence
      : rule<ref<Equation> | ref<Atom> | ref<RolesetAtom> |
             ref<BoolSentence> | ref<QuantSentence> |
             ref<CommentedSentence>> {};

  // A.2.3.10/11 module, text, named text, phrase -- mutually recursive, so
  // one alternative set (`CltextAlt`) is shared by name. A bare phrase
  // stands directly for a text's body here: a text is a run of phrases, and
  // grouping consecutive phrases into a nested text would let the same run
  // parse two ways.
  static constexpr auto CltextAlt =
      ref<Module> | ref<NamedText> | ref<Phrase>;

  struct Imports
      : rule<OPEN + (kw<"cl:imports"> | kw<"cl-imports">) +
             assign<&ast::Imports::name>(ref<InterpretableNameTerm>) +
             CLOSE> {};

  struct CommentedPhrase
      : rule<OPEN + (kw<"cl:comment"> | kw<"cl-comment">) +
             assign<&ast::CommentedPhrase::comment>(CommentText) +
             many(append<&ast::CommentedPhrase::items>(CltextAlt)) +
             CLOSE> {};

  struct Phrase
      : rule<ref<Sentence> | ref<Module> | ref<Imports> |
             ref<CommentedPhrase>> {};

  struct Text : rule<some(append<&ast::Text::phrases>(ref<Phrase>))> {
  };

  struct NamedText
      : rule<OPEN + (kw<"cl:text"> | kw<"cl-text">) +
             assign<&ast::NamedText::name>(ref<InterpretableNameTerm>) +
             option(assign<&ast::NamedText::body>(ref<Text>)) + CLOSE> {
  };

  struct Module
      : rule<OPEN + (kw<"cl:module"> | kw<"cl-module">) +
             assign<&ast::Module::name>(ref<InterpretableNameTerm>) +
             option(OPEN + (kw<"cl:excludes"> | kw<"cl-excludes">) +
                    many(append<&ast::Module::excludes>(ref<NameAny>)) +
                    CLOSE) +
             many(append<&ast::Module::body>(CltextAlt)) + CLOSE> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::items>(CltextAlt))> {};

  using entry = Document;
  using ignored = list<WHITE, BLOCKCOMMENT, LineComment>;
};

}  // namespace clif
