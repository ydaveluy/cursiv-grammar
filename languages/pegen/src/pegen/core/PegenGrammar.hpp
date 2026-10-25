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
using TargetText = cursiv::text_conv<strip_delimiters>;

struct Pegen : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
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
  static constexpr auto prefix = option(cr<"bB"> + cr<"rR"> |
                                        cr<"rR"> + cr<"bB"> | cr<"rRuUbB">);
  struct STRING : terminal<prefix + (dquote3 | squote3 | dquote1 | squote1)> {};

  struct FSTRING;
  struct Interpolation : terminal<lit<"{"> +
      many(ref<STRING> | ref<FSTRING> | ref<Interpolation> | ref<COMMENT> |
           cr<R"(^{}'"#)">) + lit<"}">> {};
  static constexpr auto fpart = escape | lit<"{{"> | lit<"}}"> | ref<Interpolation>;
  struct FSTRING : terminal<(cr<"fF"> + option(cr<"rR">) | cr<"rR"> + cr<"fF">) +
      (lit<"\"\"\""> + many(fpart | (!lit<"\"\"\""> + cr<R"(^{}\\)">)) + lit<"\"\"\""> |
       lit<"'''"> + many(fpart | (!lit<"'''"> + cr<R"(^{}\\)">)) + lit<"'''"> |
       lit<"\""> + many(fpart | cr<R"(^{}"\\\r\n)">) + lit<"\""> |
       lit<"'"> + many(fpart | cr<R"(^{}'\\\r\n)">) + lit<"'">)> {};
  struct NUMBER : terminal<cr<"0-9"> + many(cr<"a-zA-Z0-9_.">)> {};

  struct TargetAtom;
  static constexpr auto target_items =
      many(ref<TargetAtom> | ref<WS> | ref<COMMENT> | ref<CONTINUATION>);
  struct ACTION : rule<lit<"{"> + tight(target_items + lit<"}">), TargetText> {};
  struct Annotation
      : rule<lit<"["> + tight(target_items + lit<"]">), TargetText> {};
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

  struct CardOpt : rule<lit<"?">, value<ast::Card::optional>> {};
  struct CardStar : rule<lit<"*">, value<ast::Card::star>> {};
  struct CardPlus : rule<lit<"+">, value<ast::Card::plus>> {};
  struct OpenBracket : rule<lit<"[">, value<ast::Card::optional>> {};

  struct KindPos : rule<lit<"&">, value<ast::LookKind::positive>> {};
  struct KindNeg : rule<lit<"!">, value<ast::LookKind::negative>> {};
  struct KindCut : rule<lit<"~">, value<ast::LookKind::cut>> {};

  struct Alts;
  struct Item;
  struct NamedItem;

  struct Group : rule<create<ast::Group>(lit<"("> + assign<&ast::Group::body>(ref<Alts>) +
                                         lit<")">)> {};
  struct NameRef : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<NAME>))> {};
  struct StringLit
      : rule<create<ast::StringLit>(assign<&ast::StringLit::value>(ref<STRING>))> {};
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
                               assign<&ast::Item::card>(ref<CardPlus>)) |
             create<ast::Item>(assign<&ast::Item::value>(ref<Atom>) +
                               option(assign<&ast::Item::card>(
                                   ref<CardOpt> | ref<CardStar> | ref<CardPlus>)))> {};

  struct Named
      : rule<create<ast::Named>(
            option(assign<&ast::Named::attrName>(ref<NAME>) +
                   option(assign<&ast::Named::attrType>(ref<Annotation>)) + lit<"=">) +
            assign<&ast::Named::value>(ref<Item>))> {};

  struct Forced
      : rule<create<ast::Forced>(lit<"&"> + lit<"&"> +
                                 assign<&ast::Forced::value>(ref<Atom>))> {};

  struct Lookahead
      : rule<create<ast::Lookahead>(assign<&ast::Lookahead::kind>(ref<KindPos> | ref<KindNeg>) +
                                    assign<&ast::Lookahead::value>(ref<Atom>)) |
             create<ast::Lookahead>(assign<&ast::Lookahead::kind>(ref<KindCut>))> {};

  struct NamedItem : rule<ref<Named> | ref<Forced> | ref<Lookahead>> {};

  struct Alt
      : rule<create<ast::Alt>(some(append<&ast::Alt::items>(ref<NamedItem>)) +
                              option(flag<&ast::Alt::cutMark>(lit<"$">)) +
                              option(assign<&ast::Alt::action>(ref<ACTION>)))> {};

  struct Alts : rule<create<ast::Alts>(some(append<&ast::Alts::alts>(ref<Alt>), lit<"|">))> {};

  // Layout is not significant here, so a rule's several shapes (alts on
  // the header line, or led by an indented block of `|` alts) collapse to
  // one alternative list with an optional leading `|`.
  struct Rule
      : rule<create<ast::Rule>(
            assign<&ast::Rule::name>(ref<NAME>) +
            option(assign<&ast::Rule::returnType>(ref<Annotation>)) +
            option(flag<&ast::Rule::memo>(memoflag)) + lit<":"> + option(lit<"|">) +
            assign<&ast::Rule::body>(ref<Alts>))> {};

  struct Meta
      : rule<create<ast::Meta>(
            lit<"@"> + assign<&ast::Meta::name>(ref<NAME>) +
            option(assign<&ast::Meta::valueName>(ref<NAME>) |
                   assign<&ast::Meta::valueString>(ref<STRING>)))> {};

  struct Grammar
      : rule<create<ast::Grammar>(many(append<&ast::Grammar::metas>(ref<Meta>)) +
                                  some(append<&ast::Grammar::rules>(ref<Rule>)))> {};

  using entry = Grammar;
  using ignore = list<WS, COMMENT, CONTINUATION>;
};

}  // namespace pegen
