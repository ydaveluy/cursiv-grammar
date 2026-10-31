// The pegen language: its grammar. pegen is CPython's PEG metagrammar (the
// `.gram` format): a sequence of `@meta` directives and rules, each rule a
// pipe-separated list of alternatives of atoms with cardinality, guards and
// a trailing C action.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pegen/core/ast.hpp"

namespace pegen {

inline constexpr auto strip_delimiters = [](std::string_view text) {
  return text.size() >= 2 ? text.substr(1, text.size() - 2) : std::string_view{};
};

struct Pegen : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\f)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct NEWLINE
      : terminal<eol + many(many(cr<R"( \t\f)">) + option(ref<COMMENT>) + eol) +
                 option(many(cr<R"( \t\f)">) + option(ref<COMMENT>) + eof)> {};
  struct LineEnd : rule<ref<NEWLINE> | eof> {};
  struct CONTINUATION : terminal<lit<"\\"> + option(lit<"\r">) + lit<"\n">> {};
  struct NAME : terminal<(uc::XID_Start | cr<"_">) + many(uc::XID_Continue)> {};
  static constexpr auto word_boundary = uc::XID_Continue;

  static constexpr auto escape = lit<"\\"> + any;
  static constexpr auto dquote3 = lit<"\"\"\""> +
      many(escape | (!lit<"\"\"\""> + cr<R"(^\\)">)) + lit<"\"\"\"">;
  static constexpr auto squote3 = lit<"'''"> +
      many(escape | (!lit<"'''"> + cr<R"(^\\)">)) + lit<"'''">;
  static constexpr auto dquote1 =
      lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">;
  static constexpr auto squote1 = lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">;
  static constexpr auto prefix = option((cr<"bB"> + cr<"rR">) |
                                        (cr<"rR"> + cr<"bB">) | cr<"rRuUbB">);
  struct STRING : terminal<prefix + (dquote3 | squote3 | dquote1 | squote1)> {};

  struct FSTRING;
  struct Interpolation : terminal<lit<"{"> +
      many(ref<STRING> | ref<FSTRING> | ref<Interpolation> | ref<COMMENT> |
           cr<R"(^{}'"#)">) + lit<"}">> {};
  static constexpr auto fpart = escape | lit<"{{"> | lit<"}}"> | ref<Interpolation>;
  struct FSTRING : terminal<((cr<"fF"> + option(cr<"rR">)) | (cr<"rR"> + cr<"fF">)) +
      ((lit<"\"\"\""> + many(fpart | (!lit<"\"\"\""> + cr<R"(^{}\\)">)) + lit<"\"\"\"">) |
       (lit<"'''"> + many(fpart | (!lit<"'''"> + cr<R"(^{}\\)">)) + lit<"'''">) |
       (lit<"\""> + many(fpart | cr<R"(^{}"\\\r\n)">) + lit<"\"">) |
       (lit<"'"> + many(fpart | cr<R"(^{}'\\\r\n)">) + lit<"'">))> {};
  struct NUMBER : terminal<cr<"0-9"> + many(cr<"a-zA-Z0-9_.">)> {};

  struct TargetAtom;
  static constexpr auto target_items =
      many(ref<TargetAtom> | ref<WS> | ref<NEWLINE> | ref<COMMENT> | ref<CONTINUATION>);
  static constexpr auto target_trivia =
      many(ref<WS> | ref<NEWLINE> | ref<COMMENT> | ref<CONTINUATION>);
  static constexpr auto target_nonempty =
      target_trivia + some(ref<TargetAtom> + target_trivia);
  struct ACTION
      : rule<as<strip_delimiters>(lit<"{"> + tight(target_nonempty + lit<"}">))> {
  };
  struct Annotation
      : rule<as<strip_delimiters>(lit<"["> + tight(target_nonempty + lit<"]">))> {
  };
  struct NestedAction : rule<lit<"{"> + tight(target_items + lit<"}">)> {};
  struct NestedAnnotation : rule<lit<"["> + tight(target_items + lit<"]">)> {};
  struct TargetAtom : rule<ref<NestedAction> | ref<NestedAnnotation> | ref<NAME> | ref<NUMBER> |
      ref<STRING> | ref<FSTRING> | lit<"!"> |
      lit<"!="> |
      lit<"%"> |
      lit<"%="> |
      lit<"&"> |
      lit<"&="> |
      lit<"("> |
      lit<")"> |
      lit<"*"> |
      lit<"**"> |
      lit<"**="> |
      lit<"*="> |
      lit<"+"> |
      lit<"+="> |
      lit<","> |
      lit<"-"> |
      lit<"-="> |
      lit<"->"> |
      lit<"."> |
      lit<"..."> |
      lit<"/"> |
      lit<"//"> |
      lit<"//="> |
      lit<"/="> |
      lit<":"> |
      lit<":="> |
      lit<";"> |
      lit<"<"> |
      lit<"<<"> |
      lit<"<<="> |
      lit<"<="> |
      lit<"="> |
      lit<"=="> |
      lit<">"> |
      lit<">="> |
      lit<">>"> |
      lit<">>="> |
      lit<"@"> |
      lit<"@="> |
      lit<"^"> |
      lit<"^="> |
      lit<"|"> |
      lit<"|="> |
      lit<"~"> |
      lit<"?">> {};

  static constexpr auto memoflag = lit<"("> + kw<"memo"> + lit<")">;

  struct OpenBracket : rule<as<ast::Card::optional>(lit<"[">)> {};

  struct Alts;
  struct Item;
  struct NamedItem;

  struct Group : rule<create<ast::Group>(lit<"("> + assign<&ast::Group::body>(ref<Alts>) +
                                         lit<")">)> {};
  struct NameRef : rule<assign<&ast::NameRef::name>(ref<NAME>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct Atom : rule<ref<Group> | ref<NameRef> | ref<StringLit>> {};

  // `'[' alts ']'` is sugar for an optional group: the same Group node, the
  // cardinality it would carry written by `OpenBracket` instead.
  struct Item
      : rule<create<ast::Item>(assign<&ast::Item::card>(ref<OpenBracket>) +
                               assign<&ast::Item::value>(
                                   create<ast::Group>(assign<&ast::Group::body>(ref<Alts>))) +
                               lit<"]">) |
             create<ast::Item>(assign<&ast::Item::sep>(ref<Atom>) + lit<"."> +
                               assign<&ast::Item::value>(ref<Atom>) +
                               assign<&ast::Item::card>(as<ast::Card::plus>(lit<"+">))) |
             create<ast::Item>(assign<&ast::Item::value>(ref<Atom>) +
                               option(assign<&ast::Item::card>(
                                   as<ast::Card::optional>(lit<"?">) | as<ast::Card::star>(lit<"*">) | as<ast::Card::plus>(lit<"+">))))> {};

  struct Named
      : rule<create<ast::Named>(
            option(assign<&ast::Named::attrName>(ref<NAME>) +
                   option(assign<&ast::Named::attrType>(ref<Annotation>)) + lit<"=">) +
            assign<&ast::Named::value>(ref<Item>))> {};

  struct Forced
      : rule<create<ast::Forced>(lit<"&"> + lit<"&"> +
                                 assign<&ast::Forced::value>(ref<Atom>))> {};

  struct Lookahead
      : rule<create<ast::Lookahead>(assign<&ast::Lookahead::kind>(as<ast::LookKind::positive>(lit<"&">) | as<ast::LookKind::negative>(lit<"!">)) +
                                    assign<&ast::Lookahead::value>(ref<Atom>)) |
             create<ast::Lookahead>(assign<&ast::Lookahead::kind>(as<ast::LookKind::cut>(lit<"~">)))> {};

  struct NamedItem : rule<ref<Named> | ref<Forced> | ref<Lookahead>> {};

  struct Alt
      : rule<create<ast::Alt>(some(append<&ast::Alt::items>(ref<NamedItem>)) +
                              option(flag<&ast::Alt::cutMark>(lit<"$">)) +
                              option(assign<&ast::Alt::action>(ref<ACTION>)))> {};

  static constexpr auto altList = some(append<&ast::Alts::alts>(ref<Alt>), lit<"|">);
  struct Alts : rule<create<ast::Alts>(altList)> {};

  static constexpr auto moreAlts = some(lit<"|"> + altList + ref<LineEnd>);
  struct RuleAlts
      : rule<create<ast::Alts>(
            (altList + ref<LineEnd> + option(indent + moreAlts + dedent)) |
            (ref<NEWLINE> + indent + moreAlts + dedent))> {};

  // Header alternatives and the optional indented continuation share one AST
  // list, while groups retain their ordinary inline alternative syntax.
  struct Rule
      : rule<create<ast::Rule>(
            assign<&ast::Rule::name>(ref<NAME>) +
            option(assign<&ast::Rule::returnType>(ref<Annotation>)) +
            option(flag<&ast::Rule::memo>(memoflag)) + lit<":"> +
            assign<&ast::Rule::body>(ref<RuleAlts>))> {};

  struct Meta
      : rule<create<ast::Meta>(
            lit<"@"> + assign<&ast::Meta::name>(ref<NAME>) +
            option(assign<&ast::Meta::valueName>(ref<NAME>) |
                   assign<&ast::Meta::valueString>(ref<STRING>)) + ref<LineEnd>)> {};

  struct Grammar
      : rule<create<ast::Grammar>(option(ref<NEWLINE>) +
                                  many(append<&ast::Grammar::metas>(ref<Meta>)) +
                                  some(append<&ast::Grammar::rules>(ref<Rule>)))> {};

  using entry = Grammar;
  using ignored = list<WS, COMMENT, CONTINUATION>;
  using indent_newline = NEWLINE;
  static constexpr auto indent_open = lit<"("> | lit<"["> | lit<"{">;
  static constexpr auto indent_close = lit<")"> | lit<"]"> | lit<"}">;
};

}  // namespace pegen
