#pragma once

#include <type_traits>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "prolog/core/Literals.hpp"
#include "prolog/core/ast.hpp"

namespace prolog {
struct Prolog : cursiv::GrammarSpec {
  static constexpr auto Layout = uc::Z | cr<R"( \t\r\n\v\f)">;
  // SWI uses its fixed character table through U+00FF, then its Unicode
  // identifier classes. These ranges are the native table's three classes.
  static constexpr auto Latin1 = cr<R"(\x00-\xff)">;
  static constexpr auto NameStart =
      cr<R"(a-z\xaa\xb5\xba\xdf-\xf6\xf8-\xff)"> |
      (!Latin1 + !uc::Lu + uc::XID_Start);
  static constexpr auto VariableStart =
      cr<R"(A-Z_\xc0-\xd6\xd8-\xde)"> | (!Latin1 + uc::Lu);
  static constexpr auto IdContinue =
      cr<R"(0-9A-Z_a-z\xaa\xb5\xba\xc0-\xd6\xd8-\xf6\xf8-\xff)"> |
      (!Latin1 + uc::XID_Continue);
  static constexpr auto Graphic =
      cr<R"(#$&*+./:<=>?@^~\\-)"> | (!cr<R"(\x00-\x7f)"> + (uc::S | uc::Po));
  struct WS : terminal<some(Layout)> {};
  struct LINE_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct NAME : terminal<NameStart + many(IdContinue)> {};
  struct VARIABLE : terminal<VariableStart + many(IdContinue)> {};
  struct GRAPHIC : terminal<some(Graphic)> {};
  struct FULLSTOP : terminal<lit<".", (Layout | lit<"%"> | eof)>> {};
  struct DOT_OP : terminal<lit<".", !(Layout | lit<"%"> | eof | Graphic)>> {};
  template <cursiv::str S> struct GraphicOp : terminal<lit<S, !Graphic>> {};
  template <cursiv::str S> struct WordOp : terminal<kw<S>> {};

  // An adjacent '(' makes this a compound's functor, even when its name is
  // also a prefix operator. Layout or a comment leaves the prefix reading.
  static constexpr auto NotFunctor = !(gap::none + &lit<"(">);

  static constexpr auto HexDigit = cr<"0-9A-Fa-f">;
  static constexpr auto Escape =
      lit<"\\"> + ((lit<"u"> + repeat<4>(HexDigit)) |
                   (lit<"U"> + repeat<8>(HexDigit)) |
                   (lit<"x"> + some(HexDigit) + option(lit<"\\">)) |
                   (some(cr<"0-7">) + option(lit<"\\">)) | any);
  template <cursiv::str Quote>
  static constexpr auto Quoted = lit<Quote> +
                                 many(lit<Quote> + lit<Quote> | Escape |
                                      (!lit<Quote> + !lit<"\\"> + any)) +
                                 lit<Quote>;
  struct QUOTED_ATOM : terminal<Quoted<"'">, text_conv<atomValue>> {};
  struct STRING : terminal<Quoted<"\"">, text_conv<stringValue>> {};
  struct CODES : terminal<Quoted<"`">, text_conv<stringValue>> {};
  struct NIL
      : terminal<lit<"["> +
                     many(Layout | ref<LINE_COMMENT> | ref<BLOCK_COMMENT>) +
                     lit<"]">,
                 text_conv<[](std::string_view) {
                   return std::string_view("[]");
                 }>> {};
  struct EMPTY_BRACES
      : terminal<lit<"{"> +
                     many(Layout | ref<LINE_COMMENT> | ref<BLOCK_COMMENT>) +
                     lit<"}">,
                 text_conv<[](std::string_view) {
                   return std::string_view("{}");
                 }>> {};

  static constexpr auto Decimals = some(uc::Nd);
  static constexpr auto Grouped =
      Decimals + many(((lit<"_"> + many(Layout)) | lit<" ">)+Decimals);
  static constexpr auto BaseDigits =
      some(cr<"0-9A-Za-z">) +
      many(lit<"_"> + many(Layout) + some(cr<"0-9A-Za-z">));
  static constexpr auto Character = lit<"0'"> + (Escape | lit<"''"> | any);
  static constexpr auto UnsignedInteger =
      Character | (lit<"0"> + cr<"box"> + BaseDigits) |
      (some(d) + lit<"'"> + BaseDigits) | Grouped;
  static constexpr auto SignedInteger =
      (lit<"-"> + &d + UnsignedInteger) | UnsignedInteger;
  struct INTEGER : terminal<SignedInteger, text_conv<integerValue>> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + Decimals;
  static constexpr auto FloatBody =
      (Decimals + lit<"."> + Decimals + option(Exponent)) |
      (Decimals + Exponent) |
      (Decimals + lit<"."> + Decimals + (lit<"Inf"> | lit<"NaN">));
  struct FLOAT : terminal<((lit<"-"> + &d + FloatBody) | FloatBody),
                          text_conv<floatValue>> {};
  struct RATIONAL
      : terminal<((lit<"-"> + &d + Grouped) | Grouped) + lit<"r"> + Grouped> {};
  struct QUASI_CONTENT
      : terminal<lit<"||"> + many(!lit<"|}"> + any) + lit<"|}">,
                 text_conv<[](std::string_view t) {
                   return t.substr(2, t.size() - 4);
                 }>> {};

  struct Term;
  struct ArgTerm;
  struct ListArgTerm;
  struct Primary;
  struct PowerTerm;
  struct DictEntryRule;
  static constexpr auto PrefixGraphic =
      ref<GraphicOp<"-">> | ref<GraphicOp<"+">> | ref<GraphicOp<"\\">> |
      ref<GraphicOp<"\\+">> | ref<GraphicOp<":-">> | ref<GraphicOp<"?-">> |
      ref<GraphicOp<"$">>;
  static constexpr auto PrefixWord =
      ref<WordOp<R"(discontiguous)">> | ref<WordOp<R"(dynamic)">> |
      ref<WordOp<R"(volatile)">> | ref<WordOp<R"(thread_local)">> |
      ref<WordOp<R"(initialization)">> |
      ref<WordOp<R"(thread_initialization)">> |
      ref<WordOp<R"(module_transparent)">> | ref<WordOp<R"(multifile)">> |
      ref<WordOp<R"(meta_predicate)">> | ref<WordOp<R"(public)">> |
      ref<WordOp<R"(table)">>;
  static constexpr auto Atom = ref<NAME> | ref<GRAPHIC> | ref<QUOTED_ATOM> |
                               ref<EMPTY_BRACES> | lit<"!"> | lit<";"> |
                               lit<"|"> | PrefixGraphic | PrefixWord;
  struct AtomTermRule : rule<assign<&ast::AtomTerm::value>(Atom)> {};
  struct VariableRule : rule<assign<&ast::VariableTerm::name>(ref<VARIABLE>)> {
  };
  struct DictRule
      : rule<assign<&ast::DictTerm::tag>(ref<AtomTermRule> |
                                         ref<VariableRule>) +
             lit<"{"> +
             many(append<&ast::DictTerm::entries>(ref<DictEntryRule>),
                  lit<",">) +
             lit<"}">> {};
  struct Primary
      : rule<
            ref<DictRule> | ref<VariableRule> |
            create<ast::BracedTerm>(lit<"("> +
                                    assign<&ast::BracedTerm::inner>(ref<Term>) +
                                    lit<")">) |
            create<ast::FloatTerm>(assign<&ast::FloatTerm::value>(ref<FLOAT>)) |
            create<ast::RationalTerm>(
                assign<&ast::RationalTerm::text>(ref<RATIONAL>)) |
            create<ast::IntegerTerm>(
                assign<&ast::IntegerTerm::value>(ref<INTEGER>)) |
            create<ast::StringTerm>(
                assign<&ast::StringTerm::value>(ref<STRING>)) |
            create<ast::CodeListTerm>(
                assign<&ast::CodeListTerm::value>(ref<CODES>)) |
            create<ast::QuasiTerm>(
                lit<"{|"> + assign<&ast::QuasiTerm::syntax>(ref<Term>) +
                assign<&ast::QuasiTerm::content>(ref<QUASI_CONTENT>)) |
            create<ast::CompoundTerm>(
                (assign<&ast::CompoundTerm::functor>(Atom) |
                 flag<&ast::CompoundTerm::nilFunctor>(
                     assign<&ast::CompoundTerm::functor>(ref<NIL>))) +
                lit<"("> +
                many(append<&ast::CompoundTerm::args>(ref<ArgTerm>), lit<",">) +
                lit<")">) |
            create<ast::ListTerm>(
                lit<"["> +
                some(append<&ast::ListTerm::items>(ref<ListArgTerm>), lit<",">) +
                option(lit<"|"> + assign<&ast::ListTerm::tail>(ref<Term>)) +
                lit<"]">) |
            create<ast::NilTerm>(ref<NIL>) |
            create<ast::CurlyTerm>(lit<"{"> +
                                   assign<&ast::CurlyTerm::inner>(ref<Term>) +
                                   lit<"}">) |
            ref<AtomTermRule>> {};
  // SWI's priority-1 fx operator binds before dot and power.
  struct DollarTerm
      : rule<create<ast::UnaryTerm>(
                 assign<&ast::UnaryTerm::op>(ref<GraphicOp<"$">>) + NotFunctor +
                 assign<&ast::UnaryTerm::operand>(ref<Primary>)) |
             ref<Primary>> {};
  struct DotTerm : infix<DollarTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
                         &ast::BinaryTerm::right, left(ref<DOT_OP>)> {};
  struct PowerTerm
      : rule<create<ast::UnaryTerm>(
                 assign<&ast::UnaryTerm::op>(ref<GraphicOp<"-">> |
                                             ref<GraphicOp<"+">> |
                                             ref<GraphicOp<"\\">>) +
                 NotFunctor +
                 assign<&ast::UnaryTerm::operand>(ref<PowerTerm>)) |
             create<ast::BinaryTerm>(
                 assign<&ast::BinaryTerm::left>(ref<DotTerm>) +
                 assign<&ast::BinaryTerm::op>(ref<GraphicOp<"**">> |
                                              ref<GraphicOp<"^">>) +
                 assign<&ast::BinaryTerm::right>(ref<PowerTerm>)) |
             ref<DotTerm>> {};
  struct MulTerm
      : infix<PowerTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right,
              left(ref<GraphicOp<R"(*)">> | ref<GraphicOp<R"(/)">> |
                   ref<GraphicOp<R"(//)">> | ref<WordOp<R"(div)">> |
                   ref<WordOp<R"(rdiv)">> | ref<WordOp<R"(rem)">> |
                   ref<WordOp<R"(mod)">> | ref<GraphicOp<R"(<<)">> |
                   ref<GraphicOp<R"(>>)">> | ref<WordOp<R"(xor)">>)> {};
  struct AddTerm
      : infix<MulTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right,
              left(ref<GraphicOp<R"(+)">> | ref<GraphicOp<R"(-)">> |
                   ref<GraphicOp<R"(/\)">> | ref<GraphicOp<R"(\/)">>)> {};
  struct ColonTerm
      : infix<AddTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right, right(ref<GraphicOp<R"(:)">>)> {};
  struct CompareTerm
      : infix<ColonTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right,
              left(ref<GraphicOp<R"(=)">> | ref<GraphicOp<R"(\=)">> |
                   ref<GraphicOp<R"(==)">> | ref<GraphicOp<R"(\==)">> |
                   ref<GraphicOp<R"(@<)">> | ref<GraphicOp<R"(@=<)">> |
                   ref<GraphicOp<R"(@>)">> | ref<GraphicOp<R"(@>=)">> |
                   ref<GraphicOp<R"(=..)">> | ref<WordOp<R"(is)">> |
                   ref<GraphicOp<R"(=:=)">> | ref<GraphicOp<R"(=\=)">> |
                   ref<GraphicOp<R"(<)">> | ref<GraphicOp<R"(=<)">> |
                   ref<GraphicOp<R"(>)">> | ref<GraphicOp<R"(>=)">> |
                   ref<GraphicOp<R"(=@=)">> | ref<GraphicOp<R"(\=@=)">> |
                   ref<GraphicOp<R"(>:<)">> | ref<GraphicOp<R"(:<)">> |
                   ref<WordOp<R"(as)">>)> {};
  struct AssignTerm
      : infix<CompareTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right, left(ref<GraphicOp<R"(:=)">>)> {};
  struct NegatedTerm : rule<create<ast::UnaryTerm>(
                            assign<&ast::UnaryTerm::op>(ref<GraphicOp<"\\+">>) +
                            NotFunctor +
                            assign<&ast::UnaryTerm::operand>(ref<NegatedTerm>)) |
                        ref<AssignTerm>> {};
  struct DictEntryRule
      : rule<assign<&ast::DictEntry::key>(ref<Primary>) + lit<":"> +
             assign<&ast::DictEntry::value>(ref<ArgTerm>)> {};
  struct CommaTerm
      : infix<NegatedTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
              &ast::BinaryTerm::right, right(lit<",">)> {};
  // Inside a compound or dict, comma separates arguments rather than acting
  // as an operator. A list item also leaves bar to the list's tail syntax.
  template <bool UseComma, bool UseBar>
  struct Expression {
    using Commas = std::conditional_t<UseComma, CommaTerm, NegatedTerm>;
    struct ArrowTerm
        : infix<Commas, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
                &ast::BinaryTerm::right,
                right(ref<GraphicOp<R"(->)">> | ref<GraphicOp<R"(*->)">>)> {};
    struct SemicolonTerm
        : infix<ArrowTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
                &ast::BinaryTerm::right, right(lit<";">)> {};
    struct BarTerm
        : infix<SemicolonTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
                &ast::BinaryTerm::right, right(lit<"|">)> {};
    using Bars = std::conditional_t<UseBar, BarTerm, SemicolonTerm>;
    struct DirectiveTerm
        : rule<create<ast::UnaryTerm>(
                   assign<&ast::UnaryTerm::op>(PrefixWord) +
                   NotFunctor +
                   assign<&ast::UnaryTerm::operand>(ref<DirectiveTerm>)) |
               ref<Bars>> {};
    struct RuleTerm
        : infix<DirectiveTerm, &ast::BinaryTerm::left, &ast::BinaryTerm::op,
                &ast::BinaryTerm::right,
                left(ref<GraphicOp<R"(:-)">> | ref<GraphicOp<R"(-->)">> |
                     ref<GraphicOp<R"(=>)">>)> {};
    struct Term : rule<create<ast::UnaryTerm>(
                           assign<&ast::UnaryTerm::op>(ref<GraphicOp<":-">> |
                                                       ref<GraphicOp<"?-">>) +
                           NotFunctor +
                           assign<&ast::UnaryTerm::operand>(ref<Term>)) |
                       ref<RuleTerm>> {};
  };
  struct Term : Expression<true, true>::Term {};
  struct ArgTerm : Expression<false, true>::Term {};
  struct ListArgTerm : Expression<false, false>::Term {};
  struct Item : rule<assign<&ast::Item::term>(ref<Term>) + ref<FULLSTOP>> {};
  struct PText
      : rule<create<ast::PText> + many(append<&ast::PText::items>(ref<Item>))> {
  };
  using entry = PText;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};
} // namespace prolog
