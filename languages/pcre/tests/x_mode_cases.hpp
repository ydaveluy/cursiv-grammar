#pragma once

#include <string_view>

namespace pcre_x_modes {
using namespace std::string_view_literals;
struct Case {
  std::string_view name, source, literals;
  unsigned options, rootBranches, groups, classes, quantified;
};
inline constexpr Case cases[] = {
  {"plain-space-hash"sv, "a b#c"sv, "a b#c"sv, 0, 1, 0, 0, 0},
  {"x-space-comment"sv, "(?x)a b # comment\n c"sv, "abc"sv, 1, 1, 0, 0, 0},
  {"x-quoted-space-hash"sv, "(?x)\\Q # \\E"sv, " # "sv, 1, 1, 0, 0, 0},
  {"x-escaped-space-hash"sv, "(?x)a\\ b\\#c"sv, "a\\ b\\#c"sv, 1, 1, 0, 0, 0},
  {"x-class-preserves-space"sv, "(?x)[ a# ]"sv, " a# "sv, 1, 1, 0, 1, 0},
  {"xx-class-ignores-space"sv, "(?xx)[ a\t# ]"sv, "a#"sv, 1, 1, 0, 1, 0},
  {"xx-class-quoted-space"sv, "(?xx)[\\Q \t#\\E]"sv, " \t#"sv, 1, 1, 0, 1, 0},
  {"xx-to-x"sv, "(?xx)(?x)[ a ]"sv, " a "sv, 2, 1, 0, 1, 0},
  {"separated-x-not-xx"sv, "(?xix)[ a ]"sv, " a "sv, 1, 1, 0, 1, 0},
  {"unset-x"sv, "(?xx)a(?-x) b"sv, "a b"sv, 2, 1, 0, 0, 0},
  {"unset-xx"sv, "(?x)a(?-xx) b"sv, "a b"sv, 2, 1, 0, 0, 0},
  {"reset"sv, "(?x)a(?^) b"sv, "a b"sv, 2, 1, 0, 0, 0},
  {"reset-then-x"sv, "(?xx)a(?^x)[ b ]"sv, "a b "sv, 2, 1, 0, 1, 0},
  {"scoped-on"sv, "a(?x: b # c\n c) d"sv, "abc d"sv, 0, 1, 1, 0, 0},
  {"scoped-off"sv, "(?x)a(?-x: b ) c"sv, "a b c"sv, 1, 1, 1, 0, 0},
  {"nested-restore"sv, "(?x)a((?-x)b c)d e"sv, "ab cde"sv, 2, 1, 1, 0, 0},
  {"top-alternative-carry"sv, "a(?x) b | c d"sv, "abcd"sv, 1, 2, 0, 0, 0},
  {"group-alternative-carry"sv, "(a(?x)b | c d) e"sv, "abcd e"sv, 1, 1, 1, 0, 0},
  {"scoped-xx-restores-class-mode"sv, "(?xx:[ a ]) [ b ]"sv, "a  b "sv, 0, 1, 1, 2, 0},
  {"hash-after-disable"sv, "(?x)a(?-x)# b"sv, "a# b"sv, 2, 1, 0, 0, 0},
  {"hash-hides-close-pipe"sv, "(?x:a# ) | invalid\nb)c"sv, "abc"sv, 0, 1, 1, 0, 0},
  {"hash-until-EOF"sv, "(?x)a# comment"sv, "a"sv, 1, 1, 0, 0, 0},
  {"LF-not-CR"sv, "(*LF)(?x)a# c\rb"sv, "a"sv, 2, 1, 0, 0, 0},
  {"CR-newline"sv, "(*CR)(?x)a# c\rb"sv, "ab"sv, 2, 1, 0, 0, 0},
  {"CRLF-not-CR"sv, "(*CRLF)(?x)a# c\rb"sv, "a"sv, 2, 1, 0, 0, 0},
  {"CRLF-newline"sv, "(*CRLF)(?x)a# c\r\nb"sv, "ab"sv, 2, 1, 0, 0, 0},
  {"ANYCRLF-newline"sv, "(*ANYCRLF)(?x)a# c\nb"sv, "ab"sv, 2, 1, 0, 0, 0},
  {"ANY-vertical-tab"sv, "(*ANY)(?x)a# c\vb"sv, "ab"sv, 2, 1, 0, 0, 0},
  {"NUL-not-LF"sv, "(*NUL)(?x)a# c\nb"sv, "a"sv, 2, 1, 0, 0, 0},
  {"NUL-newline"sv, "(*NUL)(?x)a# c\000b"sv, "ab"sv, 2, 1, 0, 0, 0},
  {"leading-last-CR"sv, "(*LF)(*CR)(?x)a# c\rb"sv, "ab"sv, 3, 1, 0, 0, 0},
  {"leading-last-LF"sv, "(*CR)(*LF)(?x)a# c\rb"sv, "a"sv, 3, 1, 0, 0, 0},
  {"leading-other-options"sv, "(*CR)(*UTF)(*LF)(?x)a# c\rb"sv, "a"sv, 4, 1, 0, 0, 0},
  {"pattern-white-space"sv, "(*UTF)(?x)a‎ b c"sv, "abc"sv, 2, 1, 0, 0, 0},
  {"NBSP-stays-literal"sv, "(*UTF)(?x)a b"sv, "a b"sv, 2, 1, 0, 0, 0},
  {"quantifier-padding"sv, "(?x)a # c\n + # d\n ? b"sv, "ab"sv, 1, 1, 0, 0, 1},
  {"quantifier-after-restored-normal"sv, "(?x:a) +"sv, "a "sv, 0, 1, 1, 0, 1},
  {"quantifier-after-restored-x"sv, "(?x)(?-x:a) # c\n +"sv, "a"sv, 1, 1, 1, 0, 1},
  {"conditional-alternative-carry"sv, "(a)?(?(1)(?x) b | c d)"sv, "abcd"sv, 1, 1, 1, 0, 1},
  {"empty-option"sv, "(?)a"sv, "a"sv, 1, 1, 0, 0, 0},
  {"ascii-pair-and-xx"sv, "(?aTxx)[ a ]"sv, "a"sv, 1, 1, 0, 1, 0},
  {"mixed-enable-disable"sv, "(?xx)(?i-x)a b"sv, "a b"sv, 2, 1, 0, 0, 0},
};
struct Invalid { std::string_view name, source; };
inline constexpr Invalid invalid[] = {
  {"double-hyphen"sv, "(?x-x-x)a"sv},
  {"reset-hyphen"sv, "(?^x-x)a"sv},
  {"comment-hides-group-close"sv, "(?x:a# trailing)"sv},
  {"xx-empty-class"sv, "(?xx)[ \t ]"sv},
  {"invalid-standalone-flag"sv, "(?D)a"sv},
  {"spaces-in-option-header"sv, "(? x)a"sv},
};
}  // namespace pcre_x_modes
