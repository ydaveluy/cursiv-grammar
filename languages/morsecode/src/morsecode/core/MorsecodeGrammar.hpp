#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "morsecode/core/ast.hpp"

namespace morsecode {

struct Morsecode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"(\t\r\n)">)> {};
  struct SPACE : terminal<lit<" ">> {};

  template <cursiv::str Pattern, std::uint8_t Character>
  using Symbol = terminal<as<Character>(lit<Pattern>)>;

  static constexpr auto LetterValue =
      ref<Symbol<".-", 'a'>> | ref<Symbol<"-...", 'b'>> |
      ref<Symbol<"-.-.", 'c'>> | ref<Symbol<"-..", 'd'>> |
      ref<Symbol<".", 'e'>> | ref<Symbol<"..-.", 'f'>> |
      ref<Symbol<"--.", 'g'>> | ref<Symbol<"....", 'h'>> |
      ref<Symbol<"..", 'i'>> | ref<Symbol<".---", 'j'>> |
      ref<Symbol<"-.-", 'k'>> | ref<Symbol<".-..", 'l'>> |
      ref<Symbol<"--", 'm'>> | ref<Symbol<"-.", 'n'>> |
      ref<Symbol<"---", 'o'>> | ref<Symbol<".--.", 'p'>> |
      ref<Symbol<"--.-", 'q'>> | ref<Symbol<".-.", 'r'>> |
      ref<Symbol<"...", 's'>> | ref<Symbol<"-", 't'>> |
      ref<Symbol<"..-", 'u'>> | ref<Symbol<"...-", 'v'>> |
      ref<Symbol<".--", 'w'>> | ref<Symbol<"-..-", 'x'>> |
      ref<Symbol<"-.--", 'y'>> | ref<Symbol<"--..", 'z'>> |
      ref<Symbol<"-----", '0'>> | ref<Symbol<".----", '1'>> |
      ref<Symbol<"..---", '2'>> | ref<Symbol<"...--", '3'>> |
      ref<Symbol<"....-", '4'>> | ref<Symbol<".....", '5'>> |
      ref<Symbol<"-....", '6'>> | ref<Symbol<"--...", '7'>> |
      ref<Symbol<"---..", '8'>> | ref<Symbol<"----.", '9'>> |
      ref<Symbol<".-..-.", '"'>> | ref<Symbol<".----.", '\''>> |
      ref<Symbol<"...-..-", '$'>> | ref<Symbol<"-.--.", '('>> |
      ref<Symbol<"-.--.-", ')'>> | ref<Symbol<".-.-.", '+'>> |
      ref<Symbol<"--..--", ','>> | ref<Symbol<"-....-", '-'>> |
      ref<Symbol<".-.-.-", '.'>> | ref<Symbol<"-..-.", '/'>> |
      ref<Symbol<"---...", ':'>> | ref<Symbol<"-.-.-.", ';'>> |
      ref<Symbol<"-...-", '='>> | ref<Symbol<"..--..", '?'>> |
      ref<Symbol<"..--.-", '_'>> | ref<Symbol<".--.-.", '@'>> |
      ref<Symbol<"..--", 0xDC>> | ref<Symbol<".-.-", 0xC4>> |
      ref<Symbol<"-.-..", 0xC7>> | ref<Symbol<"---.", 0xD6>> |
      ref<Symbol<"..-..", 0xC9>> | ref<Symbol<".-..-", 0xC8>> |
      ref<Symbol<".--.-", 0xC0>> | ref<Symbol<"--.--", 0xD1>> |
      ref<Symbol<"----", 0xAA>> | ref<Symbol<"--..-", 0xAE>> |
      ref<Symbol<"...-.-", '<'>> | ref<Symbol<"-...-.-", '>'>> |
      ref<Symbol<"...-.", '!'>> | ref<Symbol<".-...", '&'>> |
      ref<Symbol<"-.-.-", '^'>> | ref<Symbol<".-.-..", '~'>> |
      // The ITU error signal is an erasure; the byte AST uses backspace.
      ref<Symbol<"........", '\b'>>;

  struct MorsecodeRule
      : rule<append<&ast::Message::letters>(LetterValue) +
             many(ref<SPACE> + append<&ast::Message::letters>(LetterValue))> {};

  using entry = MorsecodeRule;
  using ignored = WS;
};

}  // namespace morsecode
