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
using Unquoted = cursiv::text_conv<unquote>;

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
using HexValue = cursiv::text_conv<hex_to_double>;

inline constexpr auto drop_first = [](std::string_view t) {
  return t.substr(1);
};
using DropFirst = cursiv::text_conv<drop_first>;

struct Smalltalk : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"\"">, lit<"\"">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};

  struct STRING : terminal<lit<"'"> + many(lit<"''"> | cr<"^'">) + lit<"'">, Unquoted> {};

  struct NUMBER
      : terminal<option(lit<"-">) + some(d) + option(lit<"."> + some(d)),
                 double> {};

  // 16rFF -- the fixed "16r" radix prefix, not a computed base.
  struct HEX_NUMBER
      : terminal<option(lit<"-">) + lit<"16r"> + some(cr<"0-9a-fA-F">),
                 HexValue> {};

  // A lone '|' is the temps delimiter, never a one-character selector: the
  // pipe-starting branch needs a second character.
  static constexpr auto NoPipeChar = cr<R"(\\\\+*/=><,@%~&?-)">;
  static constexpr auto AnyBinChar = cr<R"(\\\\+*/=><,@%~|&?-)">;
  struct BINARY_SELECTOR : terminal<(NoPipeChar + many(AnyBinChar)) |
                                    (lit<"|"> + some(AnyBinChar))> {};

  struct KEYWORD : terminal<cr<"a-zA-Z"> + many(w) + lit<":">> {};

  struct BLOCK_PARAM : terminal<lit<":"> + cr<"a-zA-Z"> + many(w), DropFirst> {
  };

  struct CHARACTER_CONSTANT
      : terminal<lit<"$"> + any, DropFirst> {};

  // nil/true/false/self/super: a pseudo-variable where a state reads one.
  static constexpr auto ReservedWord =
      kw<"nil"> | kw<"true"> | kw<"false"> | kw<"self"> | kw<"super">;
  struct NameRule : rule<ref<ID>> {};

  struct Expression;
  struct BareLiteralArrayRule;
  struct SequenceRule;

  // A run of keyword tokens with no gap between them, the multi-keyword
  // form of a bare symbol (#at:put:); a data rule, so it yields its own
  // matched text.
  struct KeywordSelectorText
      : rule<ref<KEYWORD> + many(gap::none + ref<KEYWORD>)> {};

  static constexpr auto SymbolBody =
      assign<&ast::SymbolLiteral::name>(ref<NameRule> | ref<BINARY_SELECTOR> |
                                        ref<KeywordSelectorText> | ref<STRING>);

  // #foo -- no gap between the '#' and the symbol text.
  struct SymbolRule
      : rule<lit<"#"> + gap::none + create<ast::SymbolLiteral>(SymbolBody)> {};

  // The same shapes, unprefixed: a literal array's bare items are implicit
  // symbols (#(foo bar: + 1)).
  struct BareSymbolRule : rule<create<ast::SymbolLiteral>(SymbolBody)> {};

  struct PseudoVariableRule
      : rule<create<ast::PseudoVariable>(
            assign<&ast::PseudoVariable::name>(ReservedWord))> {};

  struct NumberLiteralRule
      : rule<create<ast::NumberLiteral>(assign<&ast::NumberLiteral::value>(
            ref<NUMBER> | ref<HEX_NUMBER>))> {};

  struct CharLiteralRule
      : rule<create<ast::CharLiteral>(
            assign<&ast::CharLiteral::value>(ref<CHARACTER_CONSTANT>))> {};

  struct StringLiteralRule
      : rule<create<ast::StringLiteral>(
            assign<&ast::StringLiteral::value>(ref<STRING>))> {};

  struct LiteralArrayRule;
  static constexpr auto ParsetimeLiteral =
      ref<PseudoVariableRule> | ref<NumberLiteralRule> | ref<CharLiteralRule> |
      ref<LiteralArrayRule> | ref<StringLiteralRule> | ref<SymbolRule>;

  struct LiteralArrayRule
      : rule<create<ast::LiteralArray> + lit<"#("> +
             many(append<&ast::LiteralArray::items>(ParsetimeLiteral |
                                                    ref<BareLiteralArrayRule> |
                                                    ref<BareSymbolRule>)) +
             lit<")">> {};

  struct BareLiteralArrayRule
      : rule<create<ast::LiteralArray> + lit<"("> +
             many(append<&ast::LiteralArray::items>(ParsetimeLiteral |
                                                    ref<BareLiteralArrayRule> |
                                                    ref<BareSymbolRule>)) +
             lit<")">> {};

  // A param list's own closing pipe: mandatory once
  // matched and reachable only right after at least one param, so it never
  // overlaps with a temps declaration's leading pipe; `[:x]` (params, no
  // pipe, no body) still parses.
  struct BlockRule
      : rule<create<ast::Block> + lit<"["> +
             ((some(append<&ast::Block::params>(ref<BLOCK_PARAM>)) +
               option(lit<"|"> +
                      option(assign<&ast::Block::body>(ref<SequenceRule>)))) |
              option(assign<&ast::Block::body>(ref<SequenceRule>))) +
             lit<"]">> {};

  struct DynamicArrayRule
      : rule<create<ast::DynamicArray> + lit<"{"> +
             option(some(append<&ast::DynamicArray::items>(ref<Expression>),
                         lit<".">)) +
             lit<"}">> {};

  struct DynamicDictionaryRule
      : rule<create<ast::DynamicDictionary> + lit<"#{"> +
             option(
                 some(append<&ast::DynamicDictionary::items>(ref<Expression>),
                      lit<".">)) +
             lit<"}">> {};

  struct VarRefRule
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<NameRule>))> {};

  struct Operand
      : rule<ref<BlockRule> | ref<DynamicDictionaryRule> |
             ref<DynamicArrayRule> | ParsetimeLiteral | ref<VarRefRule> |
             (lit<"("> + ref<Expression> + lit<")">)> {};

  // unarySend, folded left: each further unary message wraps the previous
  // send as its own receiver.
  struct UnarySend
      : rule<ref<Operand> |
             create<ast::MessageSend>(
                 assign<&ast::MessageSend::receiver>(ref<UnarySend>) +
                 append<&ast::MessageSend::selectorParts>(ref<NameRule>))> {};

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

  struct CascadedMessageRule
      : rule<create<ast::CascadedMessage>(
            (append<&ast::CascadedMessage::selectorParts>(
                 ref<BINARY_SELECTOR>) +
             append<&ast::CascadedMessage::args>(ref<UnarySend>)) |
            append<&ast::CascadedMessage::selectorParts>(ref<NameRule>) |
            some(append<&ast::CascadedMessage::selectorParts>(ref<KEYWORD>) +
                 append<&ast::CascadedMessage::args>(ref<BinarySend>)))> {};

  struct CascadeRule
      : rule<create<ast::Cascade>(
            assign<&ast::Cascade::base>(ref<KeywordSend> | ref<BinarySend>) +
            some(lit<";"> +
                 append<&ast::Cascade::messages>(ref<CascadedMessageRule>)))> {
  };

  struct AssignmentRule
      : rule<create<ast::Assignment>(
            assign<&ast::Assignment::name>(ref<NameRule>) + lit<":="> +
            assign<&ast::Assignment::value>(ref<Expression>))> {};

  struct Expression : rule<ref<AssignmentRule> | ref<CascadeRule> |
                           ref<KeywordSend> | ref<BinarySend>> {};

  // ^expr, with its optional trailing period.
  static constexpr auto Answer =
      lit<"^"> + assign<&ast::Sequence::answer>(ref<Expression>) +
      option(lit<".">);

  static constexpr auto Temps =
      lit<"|"> + some(append<&ast::Sequence::temps>(ref<NameRule>)) + lit<"|">;

  static constexpr auto Statements =
      (some(append<&ast::Sequence::statements>(ref<Expression>), lit<".">) +
       option(lit<"."> + option(Answer))) |
      Answer;

  struct SequenceRule : rule<create<ast::Sequence> +
                             ((Temps + option(Statements)) | Statements)> {};

  using entry = SequenceRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace smalltalk
