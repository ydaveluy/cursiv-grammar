#pragma once

#include <cstddef>
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace css3 {

// A finite ASCII keyword's CSS spellings, not an identifier scanner. The caller
// must require a token boundary after the word (the important marker also
// requires the end of the declaration).
struct KeywordSpelling : cursiv::GrammarSpec {
  static constexpr auto whitespace = lit<"\r\n"> | cr<R"( \t\r\n\f)">;
  static constexpr auto hex = cr<"0-9a-fA-F">;
  template <char C>
  static consteval auto cases() {
    if constexpr (C >= 'a' && C <= 'z') {
      const char text[] = {C, static_cast<char>(C - 'a' + 'A'), '\0'};
      return cursiv::str{text};
    } else {
      const char text[] = {C, '\0'};
      return cursiv::str{text};
    }
  }
  template <unsigned N>
  static constexpr char digit = N < 10 ? static_cast<char>('0' + N)
                                      : static_cast<char>('a' + N - 10);
  template <unsigned Code>
  static consteval auto hex_code() {
    constexpr char high = digit<(Code >> 4)>;
    constexpr char low = digit<(Code & 15)>;
    return cr<cases<high>()> + cr<cases<low>()>;
  }
  template <char C, bool Last>
  static consteval auto letter() {
    static_assert(C >= 'a' && C <= 'z');
    constexpr auto raw = cr<cases<C>()>;
    constexpr auto code = hex_code<static_cast<unsigned>(C)>() |
        hex_code<static_cast<unsigned>(C - 'a' + 'A')>();
    constexpr auto short_end = [] {
      if constexpr (Last) return option(whitespace);
      else return whitespace | !hex;
    }();
    constexpr auto encoded =
        (repeat<4>(lit<"0">) + code + option(whitespace)) |
        (repeat<0, 3>(lit<"0">) + code + short_end);
    // A-F are hex digits, so \a is U+000A rather than a simple escape for a.
    if constexpr (C >= 'a' && C <= 'f') return raw | (lit<"\\"> + encoded);
    else return raw | (lit<"\\"> + (encoded | raw));
  }
  template <cursiv::str Word, std::size_t At = 0>
  static consteval auto word() {
    static_assert(Word.size() > 0);
    if constexpr (At + 1 == Word.size()) return letter<Word[At], true>();
    else return letter<Word[At], false>() + word<Word, At + 1>();
  }
};

}  // namespace css3
