// The iri language: its grammar, RFC 3987. One document, one IRI: a scheme,
// an optional "//"-introduced authority, a path, an optional query and
// fragment. No whitespace is legal inside an IRI, so no trivia is declared.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "iri/core/ast.hpp"

namespace iri {

struct Iri : cursiv::GrammarSpec {
  static constexpr auto Alpha = cr<"a-zA-Z">;
  static constexpr auto Digit = cr<"0-9">;
  static constexpr auto HexDig = cr<"0-9a-fA-F">;

  // ucschar (RFC 3987 §2.2) and iprivate (§2.2): ranges `uc::` has no name
  // for, spelled directly as code points.
  static constexpr auto UcsChar =
      cr<R"( -퟿豈-﷏ﷰ-￯𐀀-🿽)"
         "\U00020000-\U0002FFFD\U00030000-\U0003FFFD\U00040000-\U0004FFFD"
         "\U00050000-\U0005FFFD\U00060000-\U0006FFFD\U00070000-\U0007FFFD"
         "\U00080000-\U0008FFFD\U00090000-\U0009FFFD\U000A0000-\U000AFFFD"
         "\U000B0000-\U000BFFFD\U000C0000-\U000CFFFD\U000D0000-\U000DFFFD"
         "\U000E1000-\U000EFFFD">;
  static constexpr auto IPrivate = cr<R"(-󰀀-󿿽􀀀-􏿽)">;

  static constexpr auto SubDelims = cr<"!$&'()*+,;=">;
  static constexpr auto Unreserved = Alpha | Digit | cr<"-._~">;
  static constexpr auto IUnreserved = Unreserved | UcsChar;
  static constexpr auto PctEncoded = lit<"%"> + HexDig + HexDig;

  struct SCHEME : terminal<Alpha + many(Alpha | Digit | cr<"+.-">)> {};

  // ipchar: the alphabet a path segment, a query and a fragment share.
  static constexpr auto IChar = IUnreserved | PctEncoded | SubDelims | cr<":@">;

  static constexpr auto UserinfoChar =
      IUnreserved | PctEncoded | SubDelims | lit<":">;
  static constexpr auto RegNameChar = IUnreserved | PctEncoded | SubDelims;

  static constexpr auto DecOctet = repeat<1, 3>(Digit);
  static constexpr auto Ipv4Address = DecOctet + lit<"."> + DecOctet +
                                      lit<"."> + DecOctet + lit<"."> + DecOctet;
  static constexpr auto H16 = repeat<1, 4>(HexDig);
  static constexpr auto Ls32 = (H16 + lit<":"> + H16) | Ipv4Address;

  static constexpr auto Ipv6Address =
      (repeat<6>(H16 + lit<":">) + Ls32) |
      (lit<"::"> + repeat<5>(H16 + lit<":">) + Ls32) |
      (option(H16) + lit<"::"> + repeat<4>(H16 + lit<":">) + Ls32) |
      (option(repeat<0, 1>(H16 + lit<":">) + H16) + lit<"::"> +
       repeat<3>(H16 + lit<":">) + Ls32) |
      (option(repeat<0, 2>(H16 + lit<":">) + H16) + lit<"::"> +
       repeat<2>(H16 + lit<":">) + Ls32) |
      (option(repeat<0, 3>(H16 + lit<":">) + H16) + lit<"::"> + H16 + lit<":"> +
       Ls32) |
      (option(repeat<0, 4>(H16 + lit<":">) + H16) + lit<"::"> + Ls32) |
      (option(repeat<0, 5>(H16 + lit<":">) + H16) + lit<"::"> + H16) |
      (option(repeat<0, 6>(H16 + lit<":">) + H16) + lit<"::">);

  static constexpr auto IpVFuture = cr<"vV"> + some(HexDig) + lit<"."> +
                                    some(Unreserved | SubDelims | lit<":">);
  static constexpr auto IpLiteral =
      lit<"["> + (Ipv6Address | IpVFuture) + lit<"]">;

  // ihost may be empty (a reg-name of zero characters); each branch below
  // guarantees at least one byte some other way, so AUTHORITY as a whole
  // never matches nothing and an entirely empty authority ("scheme:///x")
  // is simply not offered, leaving `Authority::raw` its default "".
  static constexpr auto HostAlone = IpLiteral | some(RegNameChar);
  static constexpr auto HostAfterAt = IpLiteral | many(RegNameChar);
  static constexpr auto PortPart = lit<":"> + many(Digit);
  struct AUTHORITY : terminal<(many(UserinfoChar) + lit<"@"> + HostAfterAt +
                               option(PortPart)) |
                              (HostAlone + option(PortPart)) | PortPart> {};

  struct AuthorityRule
      : rule<create<ast::Authority> +
             option(assign<&ast::Authority::raw>(ref<AUTHORITY>))> {};

  // A no-authority path may not open on "//" -- that spelling is always the
  // authority marker (RFC 3987 itself resolves the same clash by trying the
  // "//" branch of ihier-part first); a leading slash followed by another
  // one is refused here so the two terminals never tie on the same bytes.
  static constexpr auto MoreSegments = many(lit<"/"> + many(IChar));
  struct PATH_NO_AUTHORITY
      : terminal<(lit<"/", !lit<"/">> + many(IChar) + MoreSegments) |
                 (some(IChar) + MoreSegments)> {};
  struct PATH_ABEMPTY : terminal<some(lit<"/"> + many(IChar))> {};

  static constexpr auto QueryChar = IChar | IPrivate | cr<"/?">;
  struct QUERY : terminal<some(QueryChar)> {};

  static constexpr auto FragmentChar = IChar | cr<"/?">;
  struct FRAGMENT : terminal<some(FragmentChar)> {};

  struct IriRule
      : rule<assign<&ast::Iri::scheme>(ref<SCHEME>) + lit<":"> +
             ((lit<"//"> + assign<&ast::Iri::authority>(ref<AuthorityRule>) +
               option(assign<&ast::Iri::path>(ref<PATH_ABEMPTY>))) |
              option(assign<&ast::Iri::path>(ref<PATH_NO_AUTHORITY>))) +
             option(flag<&ast::Iri::has_query>(lit<"?">) +
                    option(assign<&ast::Iri::query>(ref<QUERY>))) +
             option(flag<&ast::Iri::has_fragment>(lit<"#">) +
                    option(assign<&ast::Iri::fragment>(ref<FRAGMENT>)))> {};

  using entry = IriRule;
};

}  // namespace iri
