// The smalltalk language: its grammar. Every send shape (unary, binary,
// keyword) folds left onto ast::MessageSend; a cascade keeps its base
// unpeeled, the way a consumer must already destructure it.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "smalltalk/core/ast.hpp"

namespace smalltalk {

inline constexpr auto unquote = [](std::string_view t, std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '\'' && i + 1 < body.size() && body[i+1] == '\'') ++i;
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto hex_to_double = [](std::string_view t) {
  bool neg = false;
  std::size_t i = 0;
  if (!t.empty() && t[0] == '-') {
    neg = true;
    i = 1;
  }
  i += 3;  // "16r"
  double v = 0;
  for (; i < t.size(); ++i) {
    const char c = t[i];
    const int d = (c >= '0' && c <= '9')   ? c - '0'
                  : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                                           : c - 'A' + 10;
    v = v * 16 + d;
  }
  return neg ? -v : v;
};

inline constexpr auto drop_first = [](std::string_view t) {
  return t.substr(1);
};

struct Smalltalk : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"\"">, lit<"\"">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};

  struct STRING : terminal<as<unquote>(lit<"'"> + many(lit<"''"> | cr<"^'">) +
                                       lit<"'">)> {};

  struct NUMBER : terminal<as<double>(option(lit<"-">) + some(d) +
                                      option(lit<"."> + some(d)) +
                                      option(lit<"e"> + option(lit<"-">) + some(d)))> {};

  // 16rFF -- the fixed "16r" radix prefix, not a computed base.
  struct HEX_NUMBER : terminal<as<hex_to_double>(option(lit<"-">) + lit<"16r"> +
                                                 some(cr<"0-9a-fA-F">))> {};

  static constexpr auto AnyBinChar = cr<R"(\\+*/=><,@%~|&?-)">;
  struct BINARY_SELECTOR : terminal<some(AnyBinChar)> {};

  struct KEYWORD : terminal<cr<"a-zA-Z"> + many(w) + lit<":">> {};

  struct BLOCK_PARAM
      : terminal<as<drop_first>(lit<":"> + cr<"a-zA-Z"> + many(w))> {};

  struct CHARACTER_CONSTANT : terminal<as<drop_first>(lit<"$"> + any)> {};

  // nil/true/false/self/super: a pseudo-variable where a state reads one.
  static constexpr auto ReservedWord =
      kw<"nil"> | kw<"true"> | kw<"false"> | kw<"self"> | kw<"super">;
  struct Name : rule<ref<ID>> {};

  struct Expression;
  struct BareLiteralArray;
  struct Sequence;

  // A run of keyword tokens with no gap between them, the multi-keyword
  // form of a bare symbol (#at:put:); a data rule, so it yields its own
  // matched text.
  struct KeywordSelectorText
      : rule<ref<KEYWORD> + many(gap::none + ref<KEYWORD>)> {};

  static constexpr auto SymbolBody =
      assign<&ast::SymbolLiteral::name>(ref<Name> | ref<BINARY_SELECTOR> |
                                        ref<KeywordSelectorText> | ref<STRING>);

  // #foo -- no gap between the '#' and the symbol text.
  struct Symbol
      : rule<lit<"#"> + gap::none + create<ast::SymbolLiteral>(SymbolBody)> {};

  // The same shapes, unprefixed: a literal array's bare items are implicit
  // symbols (#(foo bar: + 1)).
  struct BareSymbol : rule<create<ast::SymbolLiteral>(SymbolBody)> {};

  struct PseudoVariable
      : rule<create<ast::PseudoVariable>(
            assign<&ast::PseudoVariable::name>(ReservedWord))> {};

  struct NumberLiteral
      : rule<create<ast::NumberLiteral>(assign<&ast::NumberLiteral::value>(
            ref<NUMBER> | ref<HEX_NUMBER>))> {};

  struct CharLiteral
      : rule<create<ast::CharLiteral>(
            assign<&ast::CharLiteral::value>(ref<CHARACTER_CONSTANT>))> {};

  struct StringLiteral
      : rule<create<ast::StringLiteral>(
            assign<&ast::StringLiteral::value>(ref<STRING>))> {};

  struct LiteralArray;
  static constexpr auto ScalarLiteral =
      ref<NumberLiteral> | ref<CharLiteral> | ref<LiteralArray> |
      ref<StringLiteral> | ref<Symbol>;
  static constexpr auto ParsetimeLiteral = ref<PseudoVariable> | ScalarLiteral;

  // Only the three literal constants retain their values in a literal array.
  // Bare self/super are symbols there, not receiver-dependent pseudo-variables.
  struct ArrayConstant
      : rule<create<ast::PseudoVariable>(assign<&ast::PseudoVariable::name>(
            kw<"nil"> | kw<"true"> | kw<"false">))> {};
  static constexpr auto ArrayItem = ref<ArrayConstant> | ScalarLiteral;

  struct LiteralArray
      : rule<create<ast::LiteralArray> + lit<"#("> +
             many(append<&ast::LiteralArray::items>(ArrayItem |
                                                    ref<BareLiteralArray> |
                                                    ref<BareSymbol>)) +
             lit<")">> {};

  struct BareLiteralArray
      : rule<create<ast::LiteralArray> + lit<"("> +
             many(append<&ast::LiteralArray::items>(ArrayItem |
                                                    ref<BareLiteralArray> |
                                                    ref<BareSymbol>)) +
             lit<")">> {};

  // Amber closes a nonempty parameter list with its own mandatory pipe.
  struct Block
      : rule<create<ast::Block> + lit<"["> +
             ((some(append<&ast::Block::params>(ref<BLOCK_PARAM>)) + lit<"|"> +
               option(assign<&ast::Block::body>(ref<Sequence>))) |
              option(assign<&ast::Block::body>(ref<Sequence>))) +
             lit<"]">> {};

  struct DynamicArray
      : rule<create<ast::DynamicArray> + lit<"{"> +
             option(some(append<&ast::DynamicArray::items>(ref<Expression>),
                         lit<".">)) + option(lit<".">) +
             lit<"}">> {};

  struct DynamicDictionary
      : rule<create<ast::DynamicDictionary> + lit<"#{"> +
             option(
                 some(append<&ast::DynamicDictionary::items>(ref<Expression>),
                      lit<".">)) + option(lit<".">) +
             lit<"}">> {};

  struct VarRef
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<Name>))> {};

  struct Operand
      : rule<ref<Block> | ref<DynamicDictionary> |
             ref<DynamicArray> | ParsetimeLiteral | ref<VarRef> |
             (lit<"("> + ref<Expression> + lit<")">)> {};

  // unarySend, folded left: each further unary message wraps the previous
  // send as its own receiver.
  struct UnarySend
      : rule<ref<Operand> |
             create<ast::MessageSend>(
                 assign<&ast::MessageSend::receiver>(ref<UnarySend>) +
                 append<&ast::MessageSend::selectorParts>(ref<Name>))> {};

  // binarySend, folded left the same way; the right operand is its own
  // unary chain, never the outer fold.
  struct BinarySend
      : rule<ref<UnarySend> |
             create<ast::MessageSend>(
                 assign<&ast::MessageSend::receiver>(ref<BinarySend>) +
                 append<&ast::MessageSend::selectorParts>(
                     ref<BINARY_SELECTOR>) +
                 append<&ast::MessageSend::args>(ref<UnarySend>))> {};

  // keywordSend: one message, one or more keyword:arg pairs -- not a fold.
  struct KeywordSend
      : rule<create<ast::MessageSend>(
            assign<&ast::MessageSend::receiver>(ref<BinarySend>) +
            some(append<&ast::MessageSend::selectorParts>(ref<KEYWORD>) +
                 append<&ast::MessageSend::args>(ref<BinarySend>)))> {};

  struct CascadedMessage
      : rule<create<ast::CascadedMessage>(
            (append<&ast::CascadedMessage::selectorParts>(
                 ref<BINARY_SELECTOR>) +
             append<&ast::CascadedMessage::args>(ref<UnarySend>)) |
            append<&ast::CascadedMessage::selectorParts>(ref<Name>) |
            some(append<&ast::CascadedMessage::selectorParts>(ref<KEYWORD>) +
                 append<&ast::CascadedMessage::args>(ref<BinarySend>)))> {};

  struct Cascade
      : rule<create<ast::Cascade>(
            assign<&ast::Cascade::base>(ref<KeywordSend> | ref<BinarySend>) +
            some(lit<";"> +
                 append<&ast::Cascade::messages>(ref<CascadedMessage>)))> {
  };

  struct Assignment
      : rule<create<ast::Assignment>(
            assign<&ast::Assignment::name>(ref<Name>) + lit<":="> +
            assign<&ast::Assignment::value>(ref<Expression>))> {};

  struct Expression : rule<ref<Assignment> | ref<Cascade> |
                           ref<KeywordSend> | ref<BinarySend>> {};

  // ^expr, with its optional trailing period.
  static constexpr auto Answer =
      lit<"^"> + assign<&ast::Sequence::answer>(ref<Expression>) +
      option(lit<".">);

  static constexpr auto Temps =
      lit<"|"> + many(append<&ast::Sequence::temps>(ref<Name>)) + lit<"|">;

  static constexpr auto Statements =
      (some(append<&ast::Sequence::statements>(ref<Expression>), lit<".">) +
       option(lit<"."> + option(Answer))) |
      Answer;

  struct Sequence : rule<create<ast::Sequence> +
                             ((Temps + option(Statements)) | Statements)> {};

  using entry = Sequence;
  using ignored = list<WS, COMMENT>;
};

}  // namespace smalltalk
