// The clif language: its grammar -- Common Logic Interchange Format
// (ISO/IEC 24707 Annex A.2), an s-expression syntax for first-order logic.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "clif/core/ast.hpp"

namespace clif {

inline constexpr auto unquote1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote1>;

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
  struct NAMECHARSEQUENCE
      : terminal<!lit<"..."> +
                 ((NonDigitChar + many(NameContinue)) |
                  (some(d) + NonDigitNameContinue + many(NameContinue)))> {};
  struct SEQMARK : terminal<lit<"..."> + many(CHAR)> {};
  // Closes on the first matching quote, not the last: an inner quote is
  // itself a valid body byte, so a name with more than one could close
  // either way -- a quirk no document in this port's corpus relies on.
  struct QUOTEDSTRING : terminal<through(lit<"'">, lit<"'">), Unquoted> {};
  struct ENCLOSEDNAME : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  struct WHITE : terminal<some(cr<R"( \t\n\r\v)">)> {};
  struct BLOCKCOMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCKCOMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\n\r)">)> {};

  struct BareName : rule<ref<NAMECHARSEQUENCE>> {};

  struct AND_KW : terminal<kw<"and">, value<ast::BoolOp::And>> {};
  struct OR_KW : terminal<kw<"or">, value<ast::BoolOp::Or>> {};
  struct IF_KW : terminal<kw<"if">, value<ast::BoolOp::If>> {};
  struct IFF_KW : terminal<kw<"iff">, value<ast::BoolOp::Iff>> {};
  struct NOT_KW : terminal<kw<"not">, value<ast::BoolOp::Not>> {};

  struct TermRule;
  struct ApplicationRule;
  struct CommentedTermRule;
  struct SentenceRule;
  struct PhraseRule;
  struct ModuleRule;
  struct NamedTextRule;
  struct TextRule;

  // A.2.3.2 name: interpretedname (NUMERAL | QUOTEDSTRING) distributes over
  // two node classes; interpretablename (NAMECHARSEQUENCE | ENCLOSEDNAME)
  // is always a NameTerm.
  struct InterpretedNameTerm
      : rule<assign<&ast::NumeralTerm::value>(ref<NUMERAL>) |
             assign<&ast::StringTerm::value>(ref<QUOTEDSTRING>)> {};

  struct InterpretableNameTerm
      : rule<assign<&ast::NameTerm::value>(ref<BareName> |
                                           ref<ENCLOSEDNAME>)> {};

  struct SeqMarkTermRule
      : rule<assign<&ast::SeqMarkTerm::value>(ref<SEQMARK>)> {};

  struct NameAny
      : rule<ref<InterpretedNameTerm> | ref<InterpretableNameTerm>> {};

  // A.2.3.1 termseq: a term, or a sequence marker.
  static constexpr auto TermSeqElem = ref<TermRule> | ref<SeqMarkTermRule>;

  // A.2.3.3 term: a name, an application, or a commented term.
  struct ApplicationRule
      : rule<OPEN + assign<&ast::Application::op>(ref<TermRule>) +
             many(append<&ast::Application::args>(TermSeqElem)) + CLOSE> {};

  struct CommentedTermRule
      : rule<OPEN + kw<"cl-comment"> +
             assign<&ast::CommentedTerm::comment>(ref<QUOTEDSTRING>) +
             assign<&ast::CommentedTerm::body>(ref<TermRule>) + CLOSE> {};

  struct TermRule
      : rule<ref<NameAny> | ref<ApplicationRule> | ref<CommentedTermRule>> {
  };

  // A.2.3.4 equation
  struct EquationRule
      : rule<OPEN + kw<"="> + assign<&ast::Equation::left>(ref<TermRule>) +
             assign<&ast::Equation::right>(ref<TermRule>) + CLOSE> {};

  // A.2.3.6 atomic sentence: a predication, or the role-based form.
  struct AtomRule
      : rule<OPEN + assign<&ast::Atom::predicate>(ref<TermRule>) +
             many(append<&ast::Atom::args>(TermSeqElem)) + CLOSE> {};

  struct RolesetAtomRule
      : rule<OPEN + assign<&ast::RolesetAtom::subject>(ref<TermRule>) +
             OPEN + kw<"cl-roleset"> + OPEN +
             assign<&ast::RolesetAtom::role>(ref<NameAny>) +
             assign<&ast::RolesetAtom::filler>(ref<TermRule>) + CLOSE +
             CLOSE + CLOSE> {};

  // A.2.3.7 boolean sentence
  struct BoolSentenceRule
      : rule<OPEN +
             ((assign<&ast::BoolSentence::op>(ref<AND_KW> | ref<OR_KW>) +
               many(append<&ast::BoolSentence::operands>(ref<SentenceRule>))) |
              (assign<&ast::BoolSentence::op>(ref<IF_KW> | ref<IFF_KW>) +
               append<&ast::BoolSentence::operands>(ref<SentenceRule>) +
               append<&ast::BoolSentence::operands>(ref<SentenceRule>)) |
              (assign<&ast::BoolSentence::op>(ref<NOT_KW>) +
               append<&ast::BoolSentence::operands>(ref<SentenceRule>))) +
             CLOSE> {};

  // A.2.3.8 quantified sentence
  static constexpr auto BoundName =
      ref<InterpretableNameTerm> | ref<SeqMarkTermRule>;

  struct BoundItemRule
      : rule<assign<&ast::BoundItem::name>(BoundName) |
             (OPEN + assign<&ast::BoundItem::name>(BoundName) +
              assign<&ast::BoundItem::sort>(ref<TermRule>) + CLOSE)> {};

  struct BoundListRule
      : rule<create<ast::BoundList> + OPEN +
             many(append<&ast::BoundList::items>(ref<BoundItemRule>)) +
             CLOSE> {};

  struct QuantSentenceRule
      : rule<OPEN +
             (flag<&ast::QuantSentence::isExists>(kw<"exists">) |
              kw<"forall">) +
             option(assign<&ast::QuantSentence::boundName>(
                 ref<InterpretableNameTerm>)) +
             assign<&ast::QuantSentence::bounds>(ref<BoundListRule>) +
             assign<&ast::QuantSentence::body>(ref<SentenceRule>) + CLOSE> {
  };

  // A.2.3.9 commented sentence
  struct CommentedSentenceRule
      : rule<OPEN + kw<"cl-comment"> +
             assign<&ast::CommentedSentence::comment>(ref<ENCLOSEDNAME>) +
             assign<&ast::CommentedSentence::body>(ref<SentenceRule>) +
             CLOSE> {};

  // A.2.3.5 sentence
  struct SentenceRule
      : rule<ref<EquationRule> | ref<AtomRule> | ref<RolesetAtomRule> |
             ref<BoolSentenceRule> | ref<QuantSentenceRule> |
             ref<CommentedSentenceRule>> {};

  // A.2.3.10/11 module, text, named text, phrase -- mutually recursive, so
  // one alternative set (`CltextAlt`) is shared by name. A bare phrase
  // stands directly for a text's body here: a text is a run of phrases, and
  // grouping consecutive phrases into a nested text would let the same run
  // parse two ways.
  static constexpr auto CltextAlt =
      ref<ModuleRule> | ref<NamedTextRule> | ref<PhraseRule>;

  struct ImportsRule
      : rule<OPEN + kw<"cl-imports"> +
             assign<&ast::Imports::name>(ref<InterpretableNameTerm>) +
             CLOSE> {};

  struct CommentedPhraseRule
      : rule<OPEN + kw<"cl-comment"> +
             assign<&ast::CommentedPhrase::comment>(ref<ENCLOSEDNAME>) +
             option(assign<&ast::CommentedPhrase::body>(CltextAlt)) +
             CLOSE> {};

  struct PhraseRule
      : rule<ref<SentenceRule> | ref<ModuleRule> | ref<ImportsRule> |
             ref<CommentedPhraseRule>> {};

  struct TextRule : rule<some(append<&ast::Text::phrases>(ref<PhraseRule>))> {
  };

  struct NamedTextRule
      : rule<OPEN + kw<"cl-text"> +
             assign<&ast::NamedText::name>(ref<InterpretableNameTerm>) +
             option(assign<&ast::NamedText::body>(ref<TextRule>)) + CLOSE> {
  };

  struct ModuleRule
      : rule<OPEN + kw<"cl-module"> +
             assign<&ast::Module::name>(ref<InterpretableNameTerm>) +
             option(OPEN + kw<"cl-excludes"> +
                    many(append<&ast::Module::excludes>(ref<NameAny>)) +
                    CLOSE) +
             many(append<&ast::Module::body>(CltextAlt)) + CLOSE> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::items>(CltextAlt))> {};

  using entry = DocumentRule;
  using ignore = list<WHITE, BLOCKCOMMENT, LineComment>;
};

}  // namespace clif
