// The javacc language: its grammar. A .jj file is options, a
// PARSER_BEGIN/PARSER_END compilation unit, then productions whose bodies
// embed a real declaration/statement/expression grammar for the generated
// parser's target language.
#pragma once

#include <cstdint>
#include <limits>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "javacc/core/ast.hpp"

namespace javacc {

inline constexpr auto stripEnds1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto decodeQuoted = [](std::string_view t, std::pmr::memory_resource* memory) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const auto room = cursiv::text_room(memory, body.size());
  std::size_t size = 0;
  auto write = [&](unsigned c) {
    if (c < 0x80) room[size++] = char(c);
    else if (c < 0x800) { room[size++] = char(0xc0 | (c >> 6)); room[size++] = char(0x80 | (c & 63)); }
    else { room[size++] = char(0xe0 | (c >> 12)); room[size++] = char(0x80 | ((c >> 6) & 63)); room[size++] = char(0x80 | (c & 63)); }
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') { room[size++] = body[i]; continue; }
    const char c = body[++i];
    if (c >= '0' && c <= '7') {
      unsigned value = c - '0';
      const auto limit = c <= '3' ? 3u : 2u;
      for (unsigned n = 1; n < limit && i + 1 < body.size() && body[i + 1] >= '0' && body[i + 1] <= '7'; ++n)
        value = value * 8 + body[++i] - '0';
      write(value);
    } else if (c == 'u') { // Literal escapes only; the global input phase remains separate.
      while (i + 1 < body.size() && body[i + 1] == 'u') ++i;
      unsigned value = 0;
      for (unsigned n = 0; n < 4; ++n) {
        const char h = body[++i];
        value = value * 16 + (h <= '9' ? h - '0' : (h | 32) - 'a' + 10);
      }
      write(value);
    } else {
      switch (c) {
        case 'b': room[size++] = '\b'; break;
        case 't': room[size++] = '\t'; break;
        case 'n': room[size++] = '\n'; break;
        case 'f': room[size++] = '\f'; break;
        case 'r': room[size++] = '\r'; break;
        default: room[size++] = c; break;
      }
    }
  }
  return std::string_view(room.data(), size);
};

inline constexpr auto parseIntLiteral = [](std::string_view t) -> cursiv::ConversionResult<std::int64_t> {
  if (!t.empty() && (t.back() == 'l' || t.back() == 'L')) t.remove_suffix(1);
  int base = 10;
  if (t.size() > 1 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
    base = 16;
    t.remove_prefix(2);
  } else if (t.size() > 1 && t[0] == '0' && (t[1] == 'b' || t[1] == 'B')) {
    base = 2;
    t.remove_prefix(2);
  } else if (t.size() > 1 && t[0] == '0') {
    base = 8;
    t.remove_prefix(1);
  }
  std::int64_t v = 0;
  for (const char c : t) {
    if (c == '_') continue;
    int d = -1;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
    if (d < 0 || d >= base)
      return cursiv::ConversionResult<std::int64_t>::err("Invalid integer digit");
    if (v > (std::numeric_limits<std::int64_t>::max() - d) / base)
      return cursiv::ConversionResult<std::int64_t>::err("Integer exceeds signed 64-bit range");
    v = v * base + d;
  }
  return cursiv::ConversionResult<std::int64_t>::ok(v);
};

inline constexpr auto countBrackets = [](std::span<const std::string_view> tokens) -> std::uint32_t {
  std::uint32_t n = 0;
  for (const auto token : tokens)
    if (token == "[") ++n;
  return n;
};

struct Javacc : cursiv::GrammarSpec {
  // JavaCC 7.0.13's own LETTER/PART_LETTER tables, including their older
  // Unicode repertoire. Adjacent raw literals form one character class.
  static constexpr auto JavaLetter = cr<
      R"(\u{24}\u{41}-\u{5a}\u{5f}\u{61}-\u{7a}\u{a2}-\u{a5}\u{aa}\u{b5}\u{ba}\u{c0}-\u{d6})"
      R"(\u{d8}-\u{f6}\u{f8}-\u{21f}\u{222}-\u{233}\u{250}-\u{2ad}\u{2b0}-\u{2b8}\u{2bb}-\u{2c1})"
      R"(\u{2d0}-\u{2d1}\u{2e0}-\u{2e4}\u{2ee}\u{37a}\u{386}\u{388}-\u{38a}\u{38c}\u{38e}-\u{3a1})"
      R"(\u{3a3}-\u{3ce}\u{3d0}-\u{3d7}\u{3da}-\u{3f3}\u{400}-\u{481}\u{48c}-\u{4c4}\u{4c7}-\u{4c8})"
      R"(\u{4cb}-\u{4cc}\u{4d0}-\u{4f5}\u{4f8}-\u{4f9}\u{531}-\u{556}\u{559}\u{561}-\u{587})"
      R"(\u{5d0}-\u{5ea}\u{5f0}-\u{5f2}\u{621}-\u{63a}\u{640}-\u{64a}\u{671}-\u{6d3}\u{6d5})"
      R"(\u{6e5}-\u{6e6}\u{6fa}-\u{6fc}\u{710}\u{712}-\u{72c}\u{780}-\u{7a5}\u{905}-\u{939}\u{93d})"
      R"(\u{950}\u{958}-\u{961}\u{985}-\u{98c}\u{98f}-\u{990}\u{993}-\u{9a8}\u{9aa}-\u{9b0}\u{9b2})"
      R"(\u{9b6}-\u{9b9}\u{9dc}-\u{9dd}\u{9df}-\u{9e1}\u{9f0}-\u{9f3}\u{a05}-\u{a0a}\u{a0f}-\u{a10})"
      R"(\u{a13}-\u{a28}\u{a2a}-\u{a30}\u{a32}-\u{a33}\u{a35}-\u{a36}\u{a38}-\u{a39}\u{a59}-\u{a5c})"
      R"(\u{a5e}\u{a72}-\u{a74}\u{a85}-\u{a8b}\u{a8d}\u{a8f}-\u{a91}\u{a93}-\u{aa8}\u{aaa}-\u{ab0})"
      R"(\u{ab2}-\u{ab3}\u{ab5}-\u{ab9}\u{abd}\u{ad0}\u{ae0}\u{b05}-\u{b0c}\u{b0f}-\u{b10})"
      R"(\u{b13}-\u{b28}\u{b2a}-\u{b30}\u{b32}-\u{b33}\u{b36}-\u{b39}\u{b3d}\u{b5c}-\u{b5d})"
      R"(\u{b5f}-\u{b61}\u{b85}-\u{b8a}\u{b8e}-\u{b90}\u{b92}-\u{b95}\u{b99}-\u{b9a}\u{b9c})"
      R"(\u{b9e}-\u{b9f}\u{ba3}-\u{ba4}\u{ba8}-\u{baa}\u{bae}-\u{bb5}\u{bb7}-\u{bb9}\u{c05}-\u{c0c})"
      R"(\u{c0e}-\u{c10}\u{c12}-\u{c28}\u{c2a}-\u{c33}\u{c35}-\u{c39}\u{c60}-\u{c61}\u{c85}-\u{c8c})"
      R"(\u{c8e}-\u{c90}\u{c92}-\u{ca8}\u{caa}-\u{cb3}\u{cb5}-\u{cb9}\u{cde}\u{ce0}-\u{ce1})"
      R"(\u{d05}-\u{d0c}\u{d0e}-\u{d10}\u{d12}-\u{d28}\u{d2a}-\u{d39}\u{d60}-\u{d61}\u{d85}-\u{d96})"
      R"(\u{d9a}-\u{db1}\u{db3}-\u{dbb}\u{dbd}\u{dc0}-\u{dc6}\u{e01}-\u{e30}\u{e32}-\u{e33})"
      R"(\u{e3f}-\u{e46}\u{e81}-\u{e82}\u{e84}\u{e87}-\u{e88}\u{e8a}\u{e8d}\u{e94}-\u{e97})"
      R"(\u{e99}-\u{e9f}\u{ea1}-\u{ea3}\u{ea5}\u{ea7}\u{eaa}-\u{eab}\u{ead}-\u{eb0}\u{eb2}-\u{eb3})"
      R"(\u{ebd}\u{ec0}-\u{ec4}\u{ec6}\u{edc}-\u{edd}\u{f00}\u{f40}-\u{f47}\u{f49}-\u{f6a})"
      R"(\u{f88}-\u{f8b}\u{1000}-\u{1021}\u{1023}-\u{1027}\u{1029}-\u{102a}\u{1050}-\u{1055})"
      R"(\u{10a0}-\u{10c5}\u{10d0}-\u{10f6}\u{1100}-\u{1159}\u{115f}-\u{11a2}\u{11a8}-\u{11f9})"
      R"(\u{1200}-\u{1206}\u{1208}-\u{1246}\u{1248}\u{124a}-\u{124d}\u{1250}-\u{1256}\u{1258})"
      R"(\u{125a}-\u{125d}\u{1260}-\u{1286}\u{1288}\u{128a}-\u{128d}\u{1290}-\u{12ae}\u{12b0})"
      R"(\u{12b2}-\u{12b5}\u{12b8}-\u{12be}\u{12c0}\u{12c2}-\u{12c5}\u{12c8}-\u{12ce})"
      R"(\u{12d0}-\u{12d6}\u{12d8}-\u{12ee}\u{12f0}-\u{130e}\u{1310}\u{1312}-\u{1315})"
      R"(\u{1318}-\u{131e}\u{1320}-\u{1346}\u{1348}-\u{135a}\u{13a0}-\u{13f4}\u{1401}-\u{166c})"
      R"(\u{166f}-\u{1676}\u{1681}-\u{169a}\u{16a0}-\u{16ea}\u{1780}-\u{17b3}\u{17db})"
      R"(\u{1820}-\u{1877}\u{1880}-\u{18a8}\u{1e00}-\u{1e9b}\u{1ea0}-\u{1ef9}\u{1f00}-\u{1f15})"
      R"(\u{1f18}-\u{1f1d}\u{1f20}-\u{1f45}\u{1f48}-\u{1f4d}\u{1f50}-\u{1f57}\u{1f59}\u{1f5b})"
      R"(\u{1f5d}\u{1f5f}-\u{1f7d}\u{1f80}-\u{1fb4}\u{1fb6}-\u{1fbc}\u{1fbe}\u{1fc2}-\u{1fc4})"
      R"(\u{1fc6}-\u{1fcc}\u{1fd0}-\u{1fd3}\u{1fd6}-\u{1fdb}\u{1fe0}-\u{1fec}\u{1ff2}-\u{1ff4})"
      R"(\u{1ff6}-\u{1ffc}\u{203f}-\u{2040}\u{207f}\u{20a0}-\u{20af}\u{2102}\u{2107})"
      R"(\u{210a}-\u{2113}\u{2115}\u{2119}-\u{211d}\u{2124}\u{2126}\u{2128}\u{212a}-\u{212d})"
      R"(\u{212f}-\u{2131}\u{2133}-\u{2139}\u{2160}-\u{2183}\u{3005}-\u{3007}\u{3021}-\u{3029})"
      R"(\u{3031}-\u{3035}\u{3038}-\u{303a}\u{3041}-\u{3094}\u{309d}-\u{309e}\u{30a1}-\u{30fe})"
      R"(\u{3105}-\u{312c}\u{3131}-\u{318e}\u{31a0}-\u{31b7}\u{3400}-\u{4db5}\u{4e00}-\u{9fa5})"
      R"(\u{a000}-\u{a48c}\u{ac00}-\u{d7a3}\u{f900}-\u{fa2d}\u{fb00}-\u{fb06}\u{fb13}-\u{fb17})"
      R"(\u{fb1d}\u{fb1f}-\u{fb28}\u{fb2a}-\u{fb36}\u{fb38}-\u{fb3c}\u{fb3e}\u{fb40}-\u{fb41})"
      R"(\u{fb43}-\u{fb44}\u{fb46}-\u{fbb1}\u{fbd3}-\u{fd3d}\u{fd50}-\u{fd8f}\u{fd92}-\u{fdc7})"
      R"(\u{fdf0}-\u{fdfb}\u{fe33}-\u{fe34}\u{fe4d}-\u{fe4f}\u{fe69}\u{fe70}-\u{fe72}\u{fe74})"
      R"(\u{fe76}-\u{fefc}\u{ff04}\u{ff21}-\u{ff3a}\u{ff3f}\u{ff41}-\u{ff5a}\u{ff65}-\u{ffbe})"
      R"(\u{ffc2}-\u{ffc7}\u{ffca}-\u{ffcf}\u{ffd2}-\u{ffd7}\u{ffda}-\u{ffdc}\u{ffe0}-\u{ffe1})"
      R"(\u{ffe5}-\u{ffe6})"
      >;
  static constexpr auto JavaLetterOrDigit = cr<
      R"(\u{0}-\u{8}\u{e}-\u{1b}\u{24}\u{30}-\u{39}\u{41}-\u{5a}\u{5f}\u{61}-\u{7a}\u{7f}-\u{9f})"
      R"(\u{a2}-\u{a5}\u{aa}\u{b5}\u{ba}\u{c0}-\u{d6}\u{d8}-\u{f6}\u{f8}-\u{21f}\u{222}-\u{233})"
      R"(\u{250}-\u{2ad}\u{2b0}-\u{2b8}\u{2bb}-\u{2c1}\u{2d0}-\u{2d1}\u{2e0}-\u{2e4}\u{2ee})"
      R"(\u{300}-\u{34e}\u{360}-\u{362}\u{37a}\u{386}\u{388}-\u{38a}\u{38c}\u{38e}-\u{3a1})"
      R"(\u{3a3}-\u{3ce}\u{3d0}-\u{3d7}\u{3da}-\u{3f3}\u{400}-\u{481}\u{483}-\u{486}\u{48c}-\u{4c4})"
      R"(\u{4c7}-\u{4c8}\u{4cb}-\u{4cc}\u{4d0}-\u{4f5}\u{4f8}-\u{4f9}\u{531}-\u{556}\u{559})"
      R"(\u{561}-\u{587}\u{591}-\u{5a1}\u{5a3}-\u{5b9}\u{5bb}-\u{5bd}\u{5bf}\u{5c1}-\u{5c2}\u{5c4})"
      R"(\u{5d0}-\u{5ea}\u{5f0}-\u{5f2}\u{621}-\u{63a}\u{640}-\u{655}\u{660}-\u{669}\u{670}-\u{6d3})"
      R"(\u{6d5}-\u{6dc}\u{6df}-\u{6e8}\u{6ea}-\u{6ed}\u{6f0}-\u{6fc}\u{70f}-\u{72c}\u{730}-\u{74a})"
      R"(\u{780}-\u{7b0}\u{901}-\u{903}\u{905}-\u{939}\u{93c}-\u{94d}\u{950}-\u{954}\u{958}-\u{963})"
      R"(\u{966}-\u{96f}\u{981}-\u{983}\u{985}-\u{98c}\u{98f}-\u{990}\u{993}-\u{9a8}\u{9aa}-\u{9b0})"
      R"(\u{9b2}\u{9b6}-\u{9b9}\u{9bc}\u{9be}-\u{9c4}\u{9c7}-\u{9c8}\u{9cb}-\u{9cd}\u{9d7})"
      R"(\u{9dc}-\u{9dd}\u{9df}-\u{9e3}\u{9e6}-\u{9f3}\u{a02}\u{a05}-\u{a0a}\u{a0f}-\u{a10})"
      R"(\u{a13}-\u{a28}\u{a2a}-\u{a30}\u{a32}-\u{a33}\u{a35}-\u{a36}\u{a38}-\u{a39}\u{a3c})"
      R"(\u{a3e}-\u{a42}\u{a47}-\u{a48}\u{a4b}-\u{a4d}\u{a59}-\u{a5c}\u{a5e}\u{a66}-\u{a74})"
      R"(\u{a81}-\u{a83}\u{a85}-\u{a8b}\u{a8d}\u{a8f}-\u{a91}\u{a93}-\u{aa8}\u{aaa}-\u{ab0})"
      R"(\u{ab2}-\u{ab3}\u{ab5}-\u{ab9}\u{abc}-\u{ac5}\u{ac7}-\u{ac9}\u{acb}-\u{acd}\u{ad0}\u{ae0})"
      R"(\u{ae6}-\u{aef}\u{b01}-\u{b03}\u{b05}-\u{b0c}\u{b0f}-\u{b10}\u{b13}-\u{b28}\u{b2a}-\u{b30})"
      R"(\u{b32}-\u{b33}\u{b36}-\u{b39}\u{b3c}-\u{b43}\u{b47}-\u{b48}\u{b4b}-\u{b4d}\u{b56}-\u{b57})"
      R"(\u{b5c}-\u{b5d}\u{b5f}-\u{b61}\u{b66}-\u{b6f}\u{b82}-\u{b83}\u{b85}-\u{b8a}\u{b8e}-\u{b90})"
      R"(\u{b92}-\u{b95}\u{b99}-\u{b9a}\u{b9c}\u{b9e}-\u{b9f}\u{ba3}-\u{ba4}\u{ba8}-\u{baa})"
      R"(\u{bae}-\u{bb5}\u{bb7}-\u{bb9}\u{bbe}-\u{bc2}\u{bc6}-\u{bc8}\u{bca}-\u{bcd}\u{bd7})"
      R"(\u{be7}-\u{bef}\u{c01}-\u{c03}\u{c05}-\u{c0c}\u{c0e}-\u{c10}\u{c12}-\u{c28}\u{c2a}-\u{c33})"
      R"(\u{c35}-\u{c39}\u{c3e}-\u{c44}\u{c46}-\u{c48}\u{c4a}-\u{c4d}\u{c55}-\u{c56}\u{c60}-\u{c61})"
      R"(\u{c66}-\u{c6f}\u{c82}-\u{c83}\u{c85}-\u{c8c}\u{c8e}-\u{c90}\u{c92}-\u{ca8}\u{caa}-\u{cb3})"
      R"(\u{cb5}-\u{cb9}\u{cbe}-\u{cc4}\u{cc6}-\u{cc8}\u{cca}-\u{ccd}\u{cd5}-\u{cd6}\u{cde})"
      R"(\u{ce0}-\u{ce1}\u{ce6}-\u{cef}\u{d02}-\u{d03}\u{d05}-\u{d0c}\u{d0e}-\u{d10}\u{d12}-\u{d28})"
      R"(\u{d2a}-\u{d39}\u{d3e}-\u{d43}\u{d46}-\u{d48}\u{d4a}-\u{d4d}\u{d57}\u{d60}-\u{d61})"
      R"(\u{d66}-\u{d6f}\u{d82}-\u{d83}\u{d85}-\u{d96}\u{d9a}-\u{db1}\u{db3}-\u{dbb}\u{dbd})"
      R"(\u{dc0}-\u{dc6}\u{dca}\u{dcf}-\u{dd4}\u{dd6}\u{dd8}-\u{ddf}\u{df2}-\u{df3}\u{e01}-\u{e3a})"
      R"(\u{e3f}-\u{e4e}\u{e50}-\u{e59}\u{e81}-\u{e82}\u{e84}\u{e87}-\u{e88}\u{e8a}\u{e8d})"
      R"(\u{e94}-\u{e97}\u{e99}-\u{e9f}\u{ea1}-\u{ea3}\u{ea5}\u{ea7}\u{eaa}-\u{eab}\u{ead}-\u{eb9})"
      R"(\u{ebb}-\u{ebd}\u{ec0}-\u{ec4}\u{ec6}\u{ec8}-\u{ecd}\u{ed0}-\u{ed9}\u{edc}-\u{edd}\u{f00})"
      R"(\u{f18}-\u{f19}\u{f20}-\u{f29}\u{f35}\u{f37}\u{f39}\u{f3e}-\u{f47}\u{f49}-\u{f6a})"
      R"(\u{f71}-\u{f84}\u{f86}-\u{f8b}\u{f90}-\u{f97}\u{f99}-\u{fbc}\u{fc6}\u{1000}-\u{1021})"
      R"(\u{1023}-\u{1027}\u{1029}-\u{102a}\u{102c}-\u{1032}\u{1036}-\u{1039}\u{1040}-\u{1049})"
      R"(\u{1050}-\u{1059}\u{10a0}-\u{10c5}\u{10d0}-\u{10f6}\u{1100}-\u{1159}\u{115f}-\u{11a2})"
      R"(\u{11a8}-\u{11f9}\u{1200}-\u{1206}\u{1208}-\u{1246}\u{1248}\u{124a}-\u{124d})"
      R"(\u{1250}-\u{1256}\u{1258}\u{125a}-\u{125d}\u{1260}-\u{1286}\u{1288}\u{128a}-\u{128d})"
      R"(\u{1290}-\u{12ae}\u{12b0}\u{12b2}-\u{12b5}\u{12b8}-\u{12be}\u{12c0}\u{12c2}-\u{12c5})"
      R"(\u{12c8}-\u{12ce}\u{12d0}-\u{12d6}\u{12d8}-\u{12ee}\u{12f0}-\u{130e}\u{1310})"
      R"(\u{1312}-\u{1315}\u{1318}-\u{131e}\u{1320}-\u{1346}\u{1348}-\u{135a}\u{1369}-\u{1371})"
      R"(\u{13a0}-\u{13f4}\u{1401}-\u{166c}\u{166f}-\u{1676}\u{1681}-\u{169a}\u{16a0}-\u{16ea})"
      R"(\u{1780}-\u{17d3}\u{17db}\u{17e0}-\u{17e9}\u{180b}-\u{180e}\u{1810}-\u{1819})"
      R"(\u{1820}-\u{1877}\u{1880}-\u{18a9}\u{1e00}-\u{1e9b}\u{1ea0}-\u{1ef9}\u{1f00}-\u{1f15})"
      R"(\u{1f18}-\u{1f1d}\u{1f20}-\u{1f45}\u{1f48}-\u{1f4d}\u{1f50}-\u{1f57}\u{1f59}\u{1f5b})"
      R"(\u{1f5d}\u{1f5f}-\u{1f7d}\u{1f80}-\u{1fb4}\u{1fb6}-\u{1fbc}\u{1fbe}\u{1fc2}-\u{1fc4})"
      R"(\u{1fc6}-\u{1fcc}\u{1fd0}-\u{1fd3}\u{1fd6}-\u{1fdb}\u{1fe0}-\u{1fec}\u{1ff2}-\u{1ff4})"
      R"(\u{1ff6}-\u{1ffc}\u{200c}-\u{200f}\u{202a}-\u{202e}\u{203f}-\u{2040}\u{206a}-\u{206f})"
      R"(\u{207f}\u{20a0}-\u{20af}\u{20d0}-\u{20dc}\u{20e1}\u{2102}\u{2107}\u{210a}-\u{2113})"
      R"(\u{2115}\u{2119}-\u{211d}\u{2124}\u{2126}\u{2128}\u{212a}-\u{212d}\u{212f}-\u{2131})"
      R"(\u{2133}-\u{2139}\u{2160}-\u{2183}\u{3005}-\u{3007}\u{3021}-\u{302f}\u{3031}-\u{3035})"
      R"(\u{3038}-\u{303a}\u{3041}-\u{3094}\u{3099}-\u{309a}\u{309d}-\u{309e}\u{30a1}-\u{30fe})"
      R"(\u{3105}-\u{312c}\u{3131}-\u{318e}\u{31a0}-\u{31b7}\u{3400}-\u{4db5}\u{4e00}-\u{9fa5})"
      R"(\u{a000}-\u{a48c}\u{ac00}-\u{d7a3}\u{f900}-\u{fa2d}\u{fb00}-\u{fb06}\u{fb13}-\u{fb17})"
      R"(\u{fb1d}-\u{fb28}\u{fb2a}-\u{fb36}\u{fb38}-\u{fb3c}\u{fb3e}\u{fb40}-\u{fb41})"
      R"(\u{fb43}-\u{fb44}\u{fb46}-\u{fbb1}\u{fbd3}-\u{fd3d}\u{fd50}-\u{fd8f}\u{fd92}-\u{fdc7})"
      R"(\u{fdf0}-\u{fdfb}\u{fe20}-\u{fe23}\u{fe33}-\u{fe34}\u{fe4d}-\u{fe4f}\u{fe69})"
      R"(\u{fe70}-\u{fe72}\u{fe74}\u{fe76}-\u{fefc}\u{feff}\u{ff04}\u{ff10}-\u{ff19})"
      R"(\u{ff21}-\u{ff3a}\u{ff3f}\u{ff41}-\u{ff5a}\u{ff65}-\u{ffbe}\u{ffc2}-\u{ffc7})"
      R"(\u{ffca}-\u{ffcf}\u{ffd2}-\u{ffd7}\u{ffda}-\u{ffdc}\u{ffe0}-\u{ffe1}\u{ffe5}-\u{ffe6})"
      R"(\u{fff9}-\u{fffb})"
      >;
  static constexpr auto word_boundary = JavaLetterOrDigit;

  struct WS : terminal<some(cr<R"( \t\n\r\f)">)> {};
  struct EGEN : terminal<lit<"/*@egen*/">> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto ReservedKeyword =
      kw<"abstract"> | kw<"assert"> | kw<"boolean"> | kw<"break"> | kw<"byte"> |
      kw<"case"> | kw<"catch"> | kw<"char"> | kw<"class"> | kw<"const"> |
      kw<"continue"> | kw<"default"> | kw<"do"> | kw<"double"> | kw<"else"> |
      kw<"enum"> | kw<"extends"> | kw<"final"> | kw<"finally"> | kw<"float"> |
      kw<"for"> | kw<"goto"> | kw<"if"> | kw<"implements"> | kw<"import"> |
      kw<"instanceof"> | kw<"int"> | kw<"interface"> | kw<"long"> | kw<"native"> |
      kw<"new"> | kw<"package"> | kw<"private"> | kw<"protected"> | kw<"public"> |
      kw<"return"> | kw<"short"> | kw<"static"> | kw<"strictfp"> | kw<"super"> |
      kw<"switch"> | kw<"synchronized"> | kw<"this"> | kw<"throw"> | kw<"throws"> |
      kw<"transient"> | kw<"try"> | kw<"void"> | kw<"volatile"> | kw<"while"> |
      kw<"true"> | kw<"false"> | kw<"null">;

  struct ID : terminal<!ReservedKeyword + JavaLetter + many(JavaLetterOrDigit)> {};

  static constexpr auto Digits = d + option(many(d | lit<"_">) + d);
  struct INTEGER_LITERAL
      : terminal<((cr<"1-9"> + many(many(lit<"_">) + d)) |                // decimal
                  (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> +
                   option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">)) |  // hex
                  (lit<"0"> + cr<"bB"> + cr<"01"> +
                   option(many(cr<"01"> | lit<"_">) + cr<"01">)) |         // binary
                  (lit<"0"> + option(many(lit<"_"> | cr<"0-7">) + cr<"0-7">))) +  // octal
                 option(cr<"lL">)> {};

  static constexpr auto DecExponent = cr<"eE"> + option(cr<"+-">) + d + option(many(d | lit<"_">) + d);
  static constexpr auto HexExponent = cr<"pP"> + option(cr<"+-">) + d + option(many(d | lit<"_">) + d);
  struct FLOATING_POINT_LITERAL
      : terminal<(Digits + lit<"."> + option(Digits) + option(DecExponent) + option(cr<"fFdD">)) |
                 (lit<"."> + Digits + option(DecExponent) + option(cr<"fFdD">)) |
                 (Digits + DecExponent + option(cr<"fFdD">)) |
                 (Digits + option(DecExponent) + cr<"fFdD">) |
                 (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> +
                  option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">) +
                  option(lit<".">) + HexExponent + option(cr<"fFdD">)) |
                 (lit<"0"> + cr<"xX"> +
                  option(cr<"0-9a-fA-F"> + option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">)) +
                  lit<"."> + cr<"0-9a-fA-F"> +
                  option(many(cr<"0-9a-fA-F"> | lit<"_">) + cr<"0-9a-fA-F">) + HexExponent +
                  option(cr<"fFdD">))> {};

  struct BOOL_LIT : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  static constexpr auto CharEscape =
      lit<"\\"> + (cr<R"(ntbrf\\'")">                              //
                   | (cr<"0-3"> + option(cr<"0-7"> + option(cr<"0-7">)))  //
                   | (cr<"4-7"> + option(cr<"0-7">))
                   | (some(lit<"u">) + repeat<4>(cr<"0-9a-fA-F">)));
  struct CHARACTER_LITERAL
      : terminal<lit<"'"> + (CharEscape | (!cr<R"('\\\n\r)"> + any)) + lit<"'">> {};

  static constexpr auto StringEscape = CharEscape;
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(StringEscape | (!cr<R"("\\\n\r)"> + any)) + lit<"\"">> {};
  struct StringValue : rule<as<decodeQuoted>(ref<STRING_LITERAL>)> {};

  // The node descriptor's optional `(...)` customization: opaque, balanced
  // text (real corpus uses a bare digit count, e.g. `#Add(2)`).
  struct NODE_DESC_ARGS
      : terminal<as<stripEnds1>(
            lit<"("> +
            many(ref<STRING_LITERAL> | ref<CHARACTER_LITERAL> |
                 ref<COMMENT> | ref<LINE_COMMENT> | ref<NODE_DESC_ARGS> |
                 (!lit<"/*"> + !lit<"//"> + cr<R"chars(^()"')chars">)) +
            lit<")">)> {};

  static constexpr auto JavaSoftKw =
      kw<"LOOKAHEAD"> | kw<"IGNORE_CASE"> | kw<"PARSER_BEGIN"> | kw<"PARSER_END"> |
      kw<"JAVACODE"> | kw<"CPPCODE"> | kw<"TOKEN"> | kw<"SPECIAL_TOKEN"> | kw<"MORE"> |
      kw<"SKIP"> | kw<"TOKEN_MGR_DECLS"> | kw<"EOF"> | kw<"template"> |
      kw<"options"> | kw<"DCL_PARSER_BEGIN"> | kw<"DCL_PARSER_END"> | kw<"INC_PARSER_BEGIN"> |
      kw<"INC_PARSER_END"> | kw<"DEF_PARSER_BEGIN"> | kw<"DEF_PARSER_END">;
  // A javaIdentifier: any of the JavaCC-reserved spellings above is also an
  // ordinary Java identifier wherever the state reads a name, spelled once.
  struct JavaIdentifier : rule<ref<ID> | JavaSoftKw> {};
  struct Name
      : rule<ref<JavaIdentifier> + many(lit<"."> + ref<JavaIdentifier>)> {};

  struct IntLitValue : rule<as<parseIntLiteral>(ref<INTEGER_LITERAL>)> {};
  struct ExtraDims : rule<as<countBrackets>(some(lit<"["> + lit<"]">))> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto AssignOp =
      lit<"="> | lit<"*="> | lit<"/="> | lit<"%="> | lit<"+="> | lit<"-="> |
      lit<"<<="> | lit<">>="> | lit<">>>="> | lit<"&="> | lit<"^="> | lit<"|=">;

  static constexpr auto PrimitiveTypeKw =
      kw<"boolean"> | kw<"char"> | kw<"byte"> | kw<"short"> | kw<"int"> | kw<"long"> |
      kw<"float"> | kw<"double">;

  static constexpr auto ModifierKw =
      kw<"public"> | kw<"static"> | kw<"protected"> | kw<"private"> | kw<"final"> |
      kw<"abstract"> | kw<"synchronized"> | kw<"native"> | kw<"transient"> |
      kw<"volatile"> | kw<"strictfp">;

  // -- forward declarations --------------------------------------------

  struct Expression;
  struct ConditionalExpression;
  struct LogicalExpression;
  struct RelationalExpression;
  struct UnaryExpression;
  struct PrimaryExpression;
  struct AllocationExpression;
  struct Block;
  struct Type;
  struct Template;
  struct ClassTypePart;
  struct TypeArgument;
  struct ResultType;
  struct Modifiers;
  struct Annotation;
  struct AnnotationArrayInit;
  struct Statement;
  struct FormalParameters;
  struct TypeParameter;
  struct ClassBody;
  struct ClassOrInterfaceDeclaration;
  struct ClassOrInterfaceBodyDeclaration;
  struct EnumDecl;
  struct AnnotationTypeDecl;
  struct RegularExpression;
  struct RegexUnit;
  struct ComplexRegexAlt;
  struct ExpansionChoices;

  // -- types ------------------------------------------------------------

  struct PrimitiveType
      : rule<create<ast::PrimitiveType>(assign<&ast::PrimitiveType::keyword>(PrimitiveTypeKw))> {
  };

  struct TemplatePack : rule<kw<"class"> | kw<"typename">> {};

  struct TemplateBase
      : rule<create<ast::TemplateBase>(
                 assign<&ast::TemplateBase::pack>(TemplatePack::body) +
                 option(flag<&ast::TemplateBase::variadic>(lit<"...">)) +
                 assign<&ast::TemplateBase::name>(ref<ID>))> {};

  struct Template
      : rule<create<ast::Template>(
                 kw<"template"> + lit<"<"> +
                 some(append<&ast::Template::bases>(ref<TemplateBase>), lit<",">) +
                 lit<">">)> {};

  struct ClassTypePart
      : rule<create<ast::ClassTypePart>(
                 assign<&ast::ClassTypePart::name>(ref<JavaIdentifier>) +
                 option(lit<"<"> +
                        option(some(append<&ast::ClassTypePart::typeArgs>(
                                        ref<TypeArgument>),
                                    lit<",">)) +
                        lit<">">))> {};

  static constexpr auto ClassOrInterfaceTypeBody =
      option(flag<&ast::ClassType::globalScope>(lit<"::">)) +
      append<&ast::ClassType::parts>(ref<ClassTypePart>) +
      many((lit<"."> | lit<"::">) + append<&ast::ClassType::parts>(ref<ClassTypePart>));

  struct ClassOrInterfaceType
      : rule<create<ast::ClassType>(ClassOrInterfaceTypeBody)> {};

  struct ReferenceType
      : rule<create<ast::ArrayType>(
                 assign<&ast::ArrayType::element>(ref<PrimitiveType>) +
                 assign<&ast::ArrayType::dims>(ref<ExtraDims>))  //
             | (create<ast::ClassType>(
                    option(assign<&ast::ClassType::tmpl>(ref<Template>)) +
                    ClassOrInterfaceTypeBody) +
                option(nest<ast::ArrayType, &ast::ArrayType::element>(
                    assign<&ast::ArrayType::dims>(ref<ExtraDims>))))> {};

  struct Type : rule<ref<ReferenceType> | ref<PrimitiveType>> {};

  // Spliced directly into TypeArgument's own create<>: a helper that
  // writes into the caller's node cannot be its own rule<> (a second head).
  static constexpr auto WildcardBoundsBody =
      assign<&ast::WildcardTypeArgument::boundKind>(kw<"extends"> | kw<"super">) +
      assign<&ast::WildcardTypeArgument::bound>(ref<ReferenceType>);

  struct TypeArgument
      : rule<create<ast::ConcreteTypeArgument>(
                 assign<&ast::ConcreteTypeArgument::type>(ref<ReferenceType>) +
                 option(flag<&ast::ConcreteTypeArgument::variadic>(lit<"...">)))  //
             | create<ast::WildcardTypeArgument>(lit<"?"> + option(WildcardBoundsBody))> {
  };

  static constexpr auto TypeBoundBody =
      kw<"extends"> + some(append<&ast::TypeParameter::bounds>(ref<ClassOrInterfaceType>),
                           lit<"&">);

  struct TypeParameter
      : rule<create<ast::TypeParameter>(assign<&ast::TypeParameter::name>(ref<JavaIdentifier>) +
                                        option(TypeBoundBody))> {};

  struct ResultType
      : rule<create<ast::ResultType>(
                 (flag<&ast::ResultType::isVoid>(kw<"void">) +
                  option(flag<&ast::ResultType::pointer>(lit<"*">)))  //
                 | (option(flag<&ast::ResultType::isConst>(kw<"const">)) +
                    assign<&ast::ResultType::type>(ref<Type>) +
                    option(flag<&ast::ResultType::pointer>(lit<"*">) |
                           flag<&ast::ResultType::reference>(lit<"&">))))> {
  };

  // -- names, parameters --------------------------------------------------

  struct VariableDeclaratorId
      : rule<create<ast::VariableDeclaratorId>(
                 assign<&ast::VariableDeclaratorId::name>(ref<JavaIdentifier>) +
                 option(assign<&ast::VariableDeclaratorId::extraDims>(ref<ExtraDims>)))> {
  };

  struct FormalParameter
      : rule<create<ast::FormalParameter>(
                 assign<&ast::FormalParameter::mods>(ref<Modifiers>) +
                 assign<&ast::FormalParameter::type>(ref<Type>) +
                 option(assign<&ast::FormalParameter::marker>(lit<"&"> | lit<"*"> | lit<"...">)) +
                 assign<&ast::FormalParameter::id>(ref<VariableDeclaratorId>))> {};

  struct FormalParameters
      : rule<create<ast::FormalParameters>(
                 lit<"("> +
                 option(some(append<&ast::FormalParameters::params>(ref<FormalParameter>),
                             lit<",">)) +
                 lit<")">)> {};

  // -- annotations, modifiers ----------------------------------------------

  struct AnnotationArrayInit
      : rule<create<ast::AnnotationArrayInit>(
                 lit<"{"> +
                 option(some(append<&ast::AnnotationArrayInit::items>(
                                 ref<Annotation> | ref<AnnotationArrayInit> |
                                 ref<ConditionalExpression>),
                             lit<",">)) +
                 option(lit<",">) + lit<"}">)> {};

  struct MemberValuePair
      : rule<create<ast::MemberValuePair>(
                 assign<&ast::MemberValuePair::name>(ref<JavaIdentifier>) + lit<"="> +
                 assign<&ast::MemberValuePair::value>(
                     ref<Annotation> | ref<AnnotationArrayInit> |
                     ref<ConditionalExpression>))> {};

  struct Annotation
      : rule<create<ast::NormalAnnotation>(
                 lit<"@"> + assign<&ast::NormalAnnotation::name>(ref<Name>) + lit<"("> +
                 option(some(append<&ast::NormalAnnotation::pairs>(ref<MemberValuePair>),
                             lit<",">)) +
                 lit<")">)  //
             | create<ast::SingleMemberAnnotation>(
                   lit<"@"> + assign<&ast::SingleMemberAnnotation::name>(ref<Name>) + lit<"("> +
                   assign<&ast::SingleMemberAnnotation::value>(
                       ref<Annotation> | ref<AnnotationArrayInit> |
                       ref<ConditionalExpression>) +
                   lit<")">)  //
             | create<ast::MarkerAnnotation>(lit<"@"> +
                                             assign<&ast::MarkerAnnotation::name>(ref<Name>))> {};

  struct Modifiers
      : rule<create<ast::Modifiers>(
                 many(append<&ast::Modifiers::keywords>(ModifierKw) |
                      append<&ast::Modifiers::annotations>(ref<Annotation>)))> {};

  // -- expressions ----------------------------------------------------

  struct ArrayInit
      : rule<create<ast::ArrayInit>(
                 lit<"{"> +
                 option(some(append<&ast::ArrayInit::items>(ref<ArrayInit> |
                                                             ref<Expression>),
                             lit<",">)) +
                 option(lit<",">) + lit<"}">)> {};

  static constexpr auto ArrayDimsAndInitsBody =
      (some(lit<"["> + append<&ast::NewArrayExpr::dims>(ref<Expression>) + lit<"]">) +
       option(assign<&ast::NewArrayExpr::extraDims>(ref<ExtraDims>)))  //
      | (assign<&ast::NewArrayExpr::extraDims>(ref<ExtraDims>) +
         assign<&ast::NewArrayExpr::init>(ref<ArrayInit>));

  struct AllocationExpression
      : rule<create<ast::NewArrayExpr>(
                 kw<"new"> + assign<&ast::NewArrayExpr::elementType>(ref<PrimitiveType>) +
                 ArrayDimsAndInitsBody)  //
             | create<ast::NewObjectExpr>(
                   kw<"new"> +
                   assign<&ast::NewObjectExpr::type>(ref<ClassOrInterfaceType>) +
                   option(lit<"<"> +
                          option(some(append<&ast::NewObjectExpr::typeArgs>(
                                          ref<TypeArgument>),
                                      lit<",">)) +
                          lit<">">) +
                   lit<"("> +
                   option(some(append<&ast::NewObjectExpr::args>(ref<Expression>),
                               lit<",">)) +
                   lit<")"> + option(assign<&ast::NewObjectExpr::body>(ref<ClassBody>)))  //
             | create<ast::NewArrayExpr>(
                   kw<"new"> +
                   assign<&ast::NewArrayExpr::elementType>(ref<ClassOrInterfaceType>) +
                   ArrayDimsAndInitsBody)> {};

  struct PrimaryPrefix
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INTEGER_LITERAL>))  //
             | create<ast::FloatLit>(assign<&ast::FloatLit::text>(ref<FLOATING_POINT_LITERAL>))  //
             | create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHARACTER_LITERAL>))  //
             | create<ast::StringLit>(assign<&ast::StringLit::text>(ref<STRING_LITERAL>))  //
             | create<ast::BoolLit>(assign<&ast::BoolLit::value>(ref<BOOL_LIT>))  //
             | create<ast::NullLit>(kw<"null">)  //
             | create<ast::ThisExpr>(kw<"this">)  //
             | create<ast::SuperFieldExpr>(kw<"super"> + lit<"."> +
                                           assign<&ast::SuperFieldExpr::field>(ref<JavaIdentifier>))  //
             | create<ast::ParenExpr>(lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                                      lit<")">)  //
             | ref<AllocationExpression>  //
             | create<ast::ClassLiteralExpr>(assign<&ast::ClassLiteralExpr::type>(
                                                  ref<ResultType>) +
                                             lit<"."> + kw<"class">)  //
             | create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<Name>))> {};

  struct PrimaryExpression
      : rule<ref<PrimaryPrefix> +
             many(nest<ast::GenericMemberExpr, &ast::GenericMemberExpr::object>(
                      lit<"."> + lit<"<"> +
                      some(append<&ast::GenericMemberExpr::typeArgs>(ref<TypeArgument>), lit<",">) +
                      lit<">"> + assign<&ast::GenericMemberExpr::field>(ref<JavaIdentifier>)) |
                  nest<ast::FieldAccessExpr, &ast::FieldAccessExpr::object>(
                      lit<"."> +
                      assign<&ast::FieldAccessExpr::field>(ref<JavaIdentifier>))  //
                  | nest<ast::ThisAccessExpr, &ast::ThisAccessExpr::object>(lit<"."> +
                                                                            kw<"this">)  //
                  | nest<ast::AllocAccessExpr, &ast::AllocAccessExpr::object>(
                        lit<"."> + assign<&ast::AllocAccessExpr::alloc>(
                                       ref<AllocationExpression>))  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                        lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
                        lit<"]">)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        option(some(append<&ast::CallExpr::args>(ref<Expression>),
                                    lit<",">)) +
                        lit<")">))> {};

  struct PostfixExpression
      : rule<ref<PrimaryExpression> +
             option(nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                 assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct CastExpression
      : rule<create<ast::CastExpr>(lit<"("> + assign<&ast::CastExpr::type>(ref<Type>) +
                                   lit<")"> +
                                   assign<&ast::CastExpr::operand>(ref<UnaryExpression>))> {
  };

  struct UnaryExpressionNotPlusMinus
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"~"> | lit<"!">) +
                                     assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | ref<CastExpression>  //
             | ref<PostfixExpression>> {};

  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"+"> | lit<"-">) +
                                     assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"++">) +
                                       assign<&ast::PrefixExpr::operand>(
                                           ref<PrimaryExpression>))  //
             | create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(lit<"--">) +
                                       assign<&ast::PrefixExpr::operand>(
                                           ref<PrimaryExpression>))  //
             | ref<UnaryExpressionNotPlusMinus>> {};

  struct RSignedShift
      : rule<as<[] { return std::string_view(">>"); }>(lit<">"> + lit<">">)> {};
  struct RUnsignedShift : rule<as<[] { return std::string_view(">>>"); }>(
                              lit<">"> + lit<">"> + lit<">">)> {};

  // GT opens both a relational `>` and the RSignedShift/RUnsignedShift
  // sequences below: one infix<> table cannot give it two precedence steps,
  // so the relational level is a hand-written wrapper instead of one shared
  // ladder.
  struct ShiftExpression
      : infix<UnaryExpression, ast::BinaryExpr,
              levels(left(OpMul),  //
                     left(OpAdd),  //
                     left(lit<"<<"> | ref<RUnsignedShift> |
                          ref<RSignedShift>))> {};

  struct RelationalExpression
      : rule<ref<ShiftExpression> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(lit<"<="> | lit<">="> | lit<"<"> |
                                              lit<">">) +
                 assign<&ast::BinaryExpr::right>(ref<ShiftExpression>)))> {};

  struct InstanceOfExpression
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::operand>(ref<RelationalExpression>) +
                 kw<"instanceof"> + assign<&ast::InstanceOfExpr::type>(ref<Type>))  //
             | ref<RelationalExpression>> {};

  struct LogicalExpression
      : infix<InstanceOfExpression, ast::BinaryExpr,
              levels(left(OpEq),                  //
                     left(lit<"&", !lit<"&">> ),  //
                     left(lit<"^">),              //
                     left(lit<"|", !lit<"|">> ),  //
                     left(lit<"&&">),             //
                     left(lit<"||">))> {};

  struct ConditionalExpression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<LogicalExpression>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) + lit<":"> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>))  //
             | ref<LogicalExpression>> {};

  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<ConditionalExpression>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Expression>))  //
             | ref<ConditionalExpression>> {};

  // -- statements, declarations ----------------------------------------

  struct VariableDeclarator
      : rule<create<ast::VariableDeclarator>(
                 assign<&ast::VariableDeclarator::id>(ref<VariableDeclaratorId>) +
                 option(lit<"="> + assign<&ast::VariableDeclarator::init>(
                                       ref<ArrayInit> | ref<Expression>)))> {};

  struct LocalVarDecl
      : rule<create<ast::VarDecl>(
                 assign<&ast::VarDecl::mods>(ref<Modifiers>) +
                 assign<&ast::VarDecl::type>(ref<Type>) +
                 some(append<&ast::VarDecl::decls>(ref<VariableDeclarator>), lit<",">))> {
  };

  struct ExprStmt
      : rule<create<ast::ExprStmt>(option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
                                   lit<";">)> {};

  struct AssertStmt
      : rule<create<ast::AssertStmt>(
                 kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<Expression>) +
                 option(lit<":"> + assign<&ast::AssertStmt::message>(ref<Expression>)) +
                 lit<";">)> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
                 kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) +
                 lit<")"> + assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(commit(kw<"else">,
                        assign<&ast::IfStmt::elseBranch>(ref<Statement>))))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
                 kw<"while"> + lit<"("> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
                 lit<")"> + assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoStmt
      : rule<create<ast::DoStmt>(
                 kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) + kw<"while"> +
                 lit<"("> + assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
                 lit<";">)> {};

  struct ExprList
      : rule<create<ast::ExprList>(
                 some(append<&ast::ExprList::exprs>(ref<Expression>), lit<",">))> {};

  struct ForStmt
      : rule<create<ast::ForEachStmt>(
                 kw<"for"> + lit<"("> + assign<&ast::ForEachStmt::mods>(ref<Modifiers>) +
                 assign<&ast::ForEachStmt::type>(ref<Type>) +
                 assign<&ast::ForEachStmt::name>(ref<JavaIdentifier>) + lit<":"> +
                 assign<&ast::ForEachStmt::iterable>(ref<Expression>) + lit<")"> +
                 assign<&ast::ForEachStmt::body>(ref<Statement>))  //
             | create<ast::ForStmt>(
                   kw<"for"> + lit<"("> +
                   option(assign<&ast::ForStmt::init>(ref<LocalVarDecl> |
                                                       ref<ExprList>)) +
                   lit<";">                                                   //
                   + option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                   lit<";">                                                   //
                   + option(assign<&ast::ForStmt::update>(ref<ExprList>)) +
                   lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct BreakStmt
      : rule<create<ast::BreakStmt>(kw<"break"> +
                                    option(assign<&ast::BreakStmt::label>(ref<JavaIdentifier>)) +
                                    lit<";">)> {};
  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(
                 kw<"continue"> + option(assign<&ast::ContinueStmt::label>(ref<JavaIdentifier>)) +
                 lit<";">)> {};
  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
                 kw<"return"> + option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                 lit<";">)> {};
  struct ThrowStmt
      : rule<create<ast::ThrowStmt>(kw<"throw"> +
                                    assign<&ast::ThrowStmt::value>(ref<Expression>) +
                                    lit<";">)> {};

  struct SwitchLabel
      : rule<create<ast::SwitchLabelStmt>(
                 ((kw<"case"> + assign<&ast::SwitchLabelStmt::value>(ref<Expression>))  //
                 | flag<&ast::SwitchLabelStmt::isDefault>(kw<"default">)) + lit<":">)> {};

  struct BlockItem;

  struct SwitchStmt
      : rule<create<ast::SwitchStmt>(
                 kw<"switch"> + lit<"("> + assign<&ast::SwitchStmt::value>(ref<Expression>) +
                 lit<")"> + lit<"{"> +
                 many(append<&ast::SwitchStmt::items>(ref<SwitchLabel>) +
                      many(append<&ast::SwitchStmt::items>(ref<BlockItem>))) +
                 lit<"}">)> {};

  struct SynchronizedStmt
      : rule<create<ast::SynchronizedStmt>(
                 kw<"synchronized"> + lit<"("> + assign<&ast::SynchronizedStmt::lock>(
                                                      ref<Expression>) +
                 lit<")"> + assign<&ast::SynchronizedStmt::body>(ref<Block>))> {};

  struct LabeledStmt
      : rule<create<ast::LabeledStmt>(assign<&ast::LabeledStmt::label>(ref<JavaIdentifier>) +
                                      lit<":"> +
                                      assign<&ast::LabeledStmt::body>(ref<Statement>))> {
  };

  struct ResourceDecl
      : rule<create<ast::ResourceDecl>(
                 assign<&ast::ResourceDecl::type>(ref<Type>) +
                 assign<&ast::ResourceDecl::id>(ref<VariableDeclaratorId>) + lit<"="> +
                 assign<&ast::ResourceDecl::init>(ref<Expression>))> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
                 kw<"catch"> + lit<"("> + assign<&ast::CatchClause::mods>(ref<Modifiers>) +
                 append<&ast::CatchClause::types>(ref<Type>) +
                 option(assign<&ast::CatchClause::marker>(lit<"&"> | lit<"*"> | lit<"...">)) +
                 many(lit<"|"> + append<&ast::CatchClause::types>(ref<Type>)) +
                 assign<&ast::CatchClause::id>(ref<VariableDeclaratorId>) + lit<")"> +
                 assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct TryStmt
      : rule<create<ast::TryStmt>(
                 kw<"try"> +
                 option(lit<"("> +
                        some(append<&ast::TryStmt::resources>(ref<ResourceDecl>),
                             lit<";">) +
                        option(lit<";">) + lit<")">) +
                 assign<&ast::TryStmt::body>(ref<Block>) +
                 many(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                 option(kw<"finally"> +
                        assign<&ast::TryStmt::finallyBlock>(ref<Block>)))> {};

  struct Statement
      : rule<ref<LabeledStmt>       //
             | ref<AssertStmt>      //
             | ref<Block>           //
             | ref<SwitchStmt>      //
             | ref<IfStmt>          //
             | ref<WhileStmt>       //
             | ref<DoStmt>          //
             | ref<ForStmt>         //
             | ref<BreakStmt>       //
             | ref<ContinueStmt>    //
             | ref<ReturnStmt>      //
             | ref<ThrowStmt>       //
             | ref<SynchronizedStmt>  //
             | ref<TryStmt>         //
             | ref<ExprStmt>> {};

  struct BlockItem
      : rule<(ref<LocalVarDecl> + lit<";">)  //
             | ref<Statement>                //
             | ref<ClassOrInterfaceDeclaration>> {};

  struct Block
      : rule<create<ast::Block>(lit<"{"> +
                                many(append<&ast::Block::items>(ref<BlockItem>)) +
                                lit<"}">)> {};

  struct ExplicitCtorInvocation
      : rule<create<ast::ExplicitCtorInvocation>(
                 kw<"this"> + lit<"("> +
                 option(some(append<&ast::ExplicitCtorInvocation::args>(ref<Expression>),
                             lit<",">)) +
                 lit<")"> + lit<";">)  //
             | create<ast::ExplicitCtorInvocation>(
                   option(assign<&ast::ExplicitCtorInvocation::qualifier>(
                              ref<PrimaryExpression>) +
                          lit<".">) +
                   flag<&ast::ExplicitCtorInvocation::isSuper>(kw<"super">) + lit<"("> +
                   option(some(append<&ast::ExplicitCtorInvocation::args>(ref<Expression>),
                               lit<",">)) +
                   lit<")"> + lit<";">)> {};

  struct ConstructorDecl
      : rule<create<ast::ConstructorDecl>(
                 assign<&ast::ConstructorDecl::mods>(ref<Modifiers>) +
                 option(lit<"<"> +
                        some(append<&ast::ConstructorDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::ConstructorDecl::name>(ref<JavaIdentifier>) +
                 assign<&ast::ConstructorDecl::params>(ref<FormalParameters>) +
                 option(kw<"throws"> +
                        some(append<&ast::ConstructorDecl::throwsList>(ref<Name>), lit<",">)) +
                 lit<"{"> +
                 option(assign<&ast::ConstructorDecl::eci>(ref<ExplicitCtorInvocation>)) +
                 many(append<&ast::ConstructorDecl::body>(ref<BlockItem>)) + lit<"}">)> {
  };

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
                 assign<&ast::MethodDecl::mods>(ref<Modifiers>) +
                 option(lit<"<"> +
                        some(append<&ast::MethodDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodDecl::resultType>(ref<ResultType>) +
                 assign<&ast::MethodDecl::name>(ref<JavaIdentifier>) +
                 assign<&ast::MethodDecl::params>(ref<FormalParameters>) +
                 option(assign<&ast::MethodDecl::extraDims>(ref<ExtraDims>)) +
                 option(kw<"throws"> +
                        some(append<&ast::MethodDecl::throwsList>(ref<Name>), lit<",">)) +
                 (assign<&ast::MethodDecl::body>(ref<Block>) | lit<";">))> {};

  struct ClassOrInterfaceDeclaration
      : rule<create<ast::ClassOrInterfaceDecl>(
                 assign<&ast::ClassOrInterfaceDecl::mods>(ref<Modifiers>) +
                 (flag<&ast::ClassOrInterfaceDecl::isInterface>(kw<"interface">) |
                  kw<"class">) +
                 assign<&ast::ClassOrInterfaceDecl::name>(ref<JavaIdentifier>) +
                 option(lit<"<"> +
                        some(append<&ast::ClassOrInterfaceDecl::typeParams>(
                                 ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 option(kw<"extends"> +
                        some(append<&ast::ClassOrInterfaceDecl::extendsList>(
                                 ref<ClassOrInterfaceType>),
                             lit<",">)) +
                 option(kw<"implements"> +
                        some(append<&ast::ClassOrInterfaceDecl::implementsList>(
                                 ref<ClassOrInterfaceType>),
                             lit<",">)) +
                 assign<&ast::ClassOrInterfaceDecl::body>(ref<ClassBody>))> {};

  struct EnumConstant
      : rule<create<ast::EnumConstant>(
                 assign<&ast::EnumConstant::mods>(ref<Modifiers>) +
                 assign<&ast::EnumConstant::name>(ref<JavaIdentifier>) +
                 option(lit<"("> +
                        option(some(append<&ast::EnumConstant::args>(ref<Expression>),
                                    lit<",">)) +
                        lit<")">) +
                 option(assign<&ast::EnumConstant::body>(ref<ClassBody>)))> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
                 assign<&ast::EnumDecl::mods>(ref<Modifiers>) + kw<"enum"> +
                 assign<&ast::EnumDecl::name>(ref<JavaIdentifier>) +
                 option(kw<"implements"> +
                        some(append<&ast::EnumDecl::implementsList>(
                                 ref<ClassOrInterfaceType>),
                             lit<",">)) +
                 lit<"{"> +
                 option(some(append<&ast::EnumDecl::constants>(ref<EnumConstant>),
                             lit<",">)) +
                 option(lit<",">) +
                 option(lit<";"> + many(append<&ast::EnumDecl::bodyDecls>(
                                            ref<ClassOrInterfaceBodyDeclaration>))) +
                 lit<"}">)> {};

  struct AnnotationElement
      : rule<create<ast::AnnotationElement>(
                 assign<&ast::AnnotationElement::mods>(ref<Modifiers>) +
                 assign<&ast::AnnotationElement::type>(ref<Type>) +
                 assign<&ast::AnnotationElement::name>(ref<JavaIdentifier>) + lit<"("> + lit<")"> +
                 option(kw<"default"> +
                        assign<&ast::AnnotationElement::defaultVal>(
                            ref<Annotation> | ref<AnnotationArrayInit> |
                            ref<ConditionalExpression>)) +
                 lit<";">)> {};

  struct AnnotationTypeMemberDeclaration
      : rule<ref<AnnotationElement>             //
             | ref<ClassOrInterfaceDeclaration>  //
             | ref<EnumDecl>                     //
             | ref<AnnotationTypeDecl>           //
             | (ref<LocalVarDecl> + lit<";">)     //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct AnnotationTypeDecl
      : rule<create<ast::AnnotationTypeDecl>(
                 assign<&ast::AnnotationTypeDecl::mods>(ref<Modifiers>) + lit<"@"> +
                 kw<"interface"> + assign<&ast::AnnotationTypeDecl::name>(ref<JavaIdentifier>) +
                 lit<"{"> +
                 many(append<&ast::AnnotationTypeDecl::members>(
                          ref<AnnotationTypeMemberDeclaration>)) +
                 lit<"}">)> {};

  struct ClassOrInterfaceBodyDeclaration
      : rule<create<ast::InitializerBlock>(
                 option(flag<&ast::InitializerBlock::isStatic>(kw<"static">)) +
                 assign<&ast::InitializerBlock::body>(ref<Block>))  //
             | ref<ClassOrInterfaceDeclaration>                    //
             | ref<EnumDecl>                                       //
             | ref<ConstructorDecl>                                //
             | (ref<LocalVarDecl> + lit<";">)                       //
             | ref<MethodDecl>                                     //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct ClassBody
      : rule<create<ast::ClassBody>(
                 lit<"{"> +
                 many(append<&ast::ClassBody::members>(ref<ClassOrInterfaceBodyDeclaration>)) +
                 lit<"}">)> {};

  struct TypeDeclaration
      : rule<ref<ClassOrInterfaceDeclaration>  //
             | ref<EnumDecl>                    //
             | ref<AnnotationTypeDecl>          //
             | create<ast::EmptyDecl>(lit<";">)> {};

  struct PackageDecl
      : rule<create<ast::PackageDecl>(
                 assign<&ast::PackageDecl::mods>(ref<Modifiers>) + kw<"package"> +
                 assign<&ast::PackageDecl::name>(ref<Name>) + lit<";">)> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
                 kw<"import"> + option(flag<&ast::ImportDecl::isStatic>(kw<"static">)) +
                 assign<&ast::ImportDecl::name>(ref<Name>) +
                 option(lit<"."> + flag<&ast::ImportDecl::onDemand>(lit<"*">)) +
                 lit<";">)> {};

  struct CompilationUnit
      : rule<create<ast::CompilationUnit>(
                 option(assign<&ast::CompilationUnit::pkg>(ref<PackageDecl>)) +
                 many(append<&ast::CompilationUnit::imports>(ref<ImportDecl>)) +
                 many(append<&ast::CompilationUnit::types>(ref<TypeDeclaration>)))> {};

  // -- javacc: node descriptors, regular expressions -----------------------

  struct NodeDescriptor
      : rule<create<ast::NodeDescriptor>(
                 lit<"#"> + assign<&ast::NodeDescriptor::name>(ref<ID> | kw<"void">) +
                 option(assign<&ast::NodeDescriptor::args>(ref<NODE_DESC_ARGS>)))> {};

  struct CharDescriptor
      : rule<create<ast::CharDescriptor>(
                 assign<&ast::CharDescriptor::from>(ref<StringValue>) +
                 option(flag<&ast::CharDescriptor::range>(lit<"-">) +
                        assign<&ast::CharDescriptor::to>(ref<StringValue>)))> {
  };

  struct ComplexRegexAlt
      : rule<create<ast::ComplexRegexAlt>(
                 some(append<&ast::ComplexRegexAlt::units>(ref<RegexUnit>)))> {};

  struct RegexUnit
      : rule<create<ast::RegexStringUnit>(
                 assign<&ast::RegexStringUnit::text>(ref<STRING_LITERAL>))  //
             | create<ast::RegexRefUnit>(lit<"<"> +
                                         assign<&ast::RegexRefUnit::name>(ref<ID>) + lit<">">)  //
             | create<ast::CharListUnit>(
                   option(flag<&ast::CharListUnit::negated>(lit<"~">)) + lit<"["> +
                   option(some(append<&ast::CharListUnit::items>(ref<CharDescriptor>),
                               lit<",">)) +
                   lit<"]">)  //
             | create<ast::RegexGroupUnit>(
                   lit<"("> +
                   some(append<&ast::RegexGroupUnit::choices>(ref<ComplexRegexAlt>),
                        lit<"|">) +
                   lit<")"> +
                   option(assign<&ast::RegexGroupUnit::simpleSuffix>(lit<"+"> | lit<"*"> |
                                                                     lit<"?">)  //
                          | (flag<&ast::RegexGroupUnit::hasCountSuffix>(lit<"{">) +
                             assign<&ast::RegexGroupUnit::repeatMin>(ref<IntLitValue>) +
                             option(flag<&ast::RegexGroupUnit::repeatHasComma>(lit<",">) +
                                    option(assign<&ast::RegexGroupUnit::repeatMax>(
                                        ref<IntLitValue>))) +
                             lit<"}">)))> {};

  struct RegularExpression
      : rule<create<ast::StringLiteralRegex>(
                 assign<&ast::StringLiteralRegex::text>(ref<STRING_LITERAL>))  //
             | create<ast::ComplexRegex>(
                   lit<"<"> +
                   option(option(flag<&ast::ComplexRegex::isPrivate>(lit<"#">)) +
                          assign<&ast::ComplexRegex::label>(ref<ID>) + lit<":">) +
                   some(append<&ast::ComplexRegex::choices>(ref<ComplexRegexAlt>),
                        lit<"|">) +
                   lit<">">)  //
             | create<ast::TokenRefRegex>(lit<"<"> +
                                          assign<&ast::TokenRefRegex::name>(ref<ID>) +
                                          lit<">">)  //
             | create<ast::EofRegex>(lit<"<"> + kw<"EOF"> + lit<">">)> {};

  struct RegexprSpec
      : rule<create<ast::RegexprSpec>(
                 assign<&ast::RegexprSpec::regex>(ref<RegularExpression>) +
                 option(assign<&ast::RegexprSpec::action>(ref<Block>)) +
                 option(lit<":"> +
                        assign<&ast::RegexprSpec::lexicalStateAfter>(ref<ID>)))> {};

  struct RegularExprProduction
      : rule<create<ast::RegularExprProduction>(
                 option((lit<"<"> +
                         flag<&ast::RegularExprProduction::allStates>(lit<"*">) +
                         lit<">">)  //
                        | (lit<"<"> +
                           some(append<&ast::RegularExprProduction::states>(ref<ID>),
                                lit<",">) +
                           lit<">">)) +
                 assign<&ast::RegularExprProduction::kind>(kw<"TOKEN"> | kw<"SPECIAL_TOKEN"> |
                                                           kw<"SKIP"> | kw<"MORE">) +
                 option(lit<"["> +
                        flag<&ast::RegularExprProduction::ignoreCase>(kw<"IGNORE_CASE">) +
                        lit<"]">) +
                 lit<":"> + lit<"{"> +
                 some(append<&ast::RegularExprProduction::specs>(ref<RegexprSpec>),
                      lit<"|">) +
                 lit<"}">)> {};

  struct TokenManagerDecls
      : rule<create<ast::TokenManagerDecls>(
                 kw<"TOKEN_MGR_DECLS"> + lit<":"> +
                 option(assign<&ast::TokenManagerDecls::body>(ref<ClassBody>)))> {};

  struct JavaCodeProduction
      : rule<create<ast::JavaCodeProduction>(
                 kw<"JAVACODE"> +
                 option(assign<&ast::JavaCodeProduction::accessModifier>(
                     kw<"public"> | kw<"protected"> | kw<"private">)) +
                 assign<&ast::JavaCodeProduction::resultType>(ref<ResultType>) +
                 assign<&ast::JavaCodeProduction::name>(ref<ID>) +
                 assign<&ast::JavaCodeProduction::params>(ref<FormalParameters>) +
                 option(kw<"throws"> +
                        some(append<&ast::JavaCodeProduction::throwsList>(ref<Name>), lit<",">)) +
                 option(assign<&ast::JavaCodeProduction::nodeDesc>(ref<NodeDescriptor>)) +
                 assign<&ast::JavaCodeProduction::body>(ref<Block>))> {};

  struct CppCodeProduction
      : rule<create<ast::CppCodeProduction>(
                 kw<"CPPCODE"> +
                 option(assign<&ast::CppCodeProduction::accessModifier>(
                     kw<"public"> | kw<"protected"> | kw<"private">)) +
                 assign<&ast::CppCodeProduction::resultType>(ref<ResultType>) +
                 assign<&ast::CppCodeProduction::name>(ref<ID>) +
                 assign<&ast::CppCodeProduction::params>(ref<FormalParameters>) +
                 option(kw<"throws"> +
                        some(append<&ast::CppCodeProduction::throwsList>(ref<Name>), lit<",">)) +
                 assign<&ast::CppCodeProduction::body>(ref<Block>))> {};

  struct BnfProduction
      : rule<create<ast::BnfProduction>(
                 option(assign<&ast::BnfProduction::accessModifier>(
                     kw<"public"> | kw<"protected"> | kw<"private">)) +
                 assign<&ast::BnfProduction::resultType>(ref<ResultType>) +
                 assign<&ast::BnfProduction::name>(ref<ID>) +
                 assign<&ast::BnfProduction::params>(ref<FormalParameters>) +
                 option(kw<"throws"> +
                        some(append<&ast::BnfProduction::throwsList>(ref<Name>), lit<",">)) +
                 option(assign<&ast::BnfProduction::nodeDesc>(ref<NodeDescriptor>)) +
                 lit<":"> + assign<&ast::BnfProduction::localDecls>(ref<Block>) + lit<"{"> +
                 assign<&ast::BnfProduction::body>(ref<ExpansionChoices>) + lit<"}">)> {
  };

  struct Production
      : rule<ref<JavaCodeProduction>       //
             | ref<CppCodeProduction>      //
             | ref<RegularExprProduction>  //
             | ref<TokenManagerDecls>      //
             | ref<BnfProduction>> {};

  // -- javacc: expansions -----------------------------------------------

  struct LocalLookahead
      : rule<create<ast::LocalLookahead>(
                 option(assign<&ast::LocalLookahead::amount>(ref<IntLitValue>)) +
                 option(lit<",">) +
                 option(assign<&ast::LocalLookahead::choices>(ref<ExpansionChoices>)) +
                 option(lit<",">) +
                 option(lit<"{"> +
                        option(assign<&ast::LocalLookahead::semanticPredicate>(
                            ref<Expression>)) +
                        lit<"}">))> {};

  struct ExpansionCatchClause
      : rule<create<ast::ExpansionCatchClause>(
                 kw<"catch"> + lit<"("> +
                 option(assign<&ast::ExpansionCatchClause::typeName>(ref<Name>) +
                        assign<&ast::ExpansionCatchClause::varName>(ref<ID>)) +
                 lit<")"> + assign<&ast::ExpansionCatchClause::body>(ref<Block>))> {};

  struct ExpansionUnit
      : rule<create<ast::LookaheadUnit>(
                 kw<"LOOKAHEAD"> + lit<"("> +
                 assign<&ast::LookaheadUnit::lookahead>(ref<LocalLookahead>) + lit<")">)  //
             | create<ast::ActionUnit>(assign<&ast::ActionUnit::action>(ref<Block>))  //
             | create<ast::OptionalUnit>(lit<"["> +
                                         assign<&ast::OptionalUnit::choices>(
                                             ref<ExpansionChoices>) +
                                         lit<"]">)  //
             | create<ast::TryExpansionUnit>(
                   kw<"try"> + lit<"{"> +
                   assign<&ast::TryExpansionUnit::body>(ref<ExpansionChoices>) + lit<"}"> +
                   many(append<&ast::TryExpansionUnit::catches>(ref<ExpansionCatchClause>)) +
                   option(kw<"finally"> +
                          assign<&ast::TryExpansionUnit::finallyBlock>(ref<Block>)))  //
             | create<ast::ExpansionCallUnit>(
                   option(assign<&ast::ExpansionCallUnit::assignTarget>(
                              ref<PrimaryExpression>) +
                          lit<"=">) +
                   assign<&ast::ExpansionCallUnit::name>(ref<ID>) +
                   option(lit<"<"> +
                          option(some(append<&ast::ExpansionCallUnit::typeArgs>(
                                          ref<TypeArgument>),
                                      lit<",">)) +
                          lit<">">) +
                   lit<"("> +
                   option(some(append<&ast::ExpansionCallUnit::args>(ref<Expression>),
                               lit<",">)) +
                   lit<")">)  //
             | create<ast::ExpansionRegexUnit>(
                   option(assign<&ast::ExpansionRegexUnit::assignTarget>(
                              ref<PrimaryExpression>) +
                          lit<"=">) +
                   assign<&ast::ExpansionRegexUnit::regex>(ref<RegularExpression>) +
                   option(lit<"."> +
                          assign<&ast::ExpansionRegexUnit::lexicalStateAfter>(ref<ID>)))  //
             | create<ast::GroupUnit>(
                   lit<"("> + assign<&ast::GroupUnit::choices>(ref<ExpansionChoices>) +
                   lit<")"> +
                   option(assign<&ast::GroupUnit::repeat>(lit<"+"> | lit<"*"> | lit<"?">)))> {
      };

  struct ExpansionItem
      : rule<create<ast::ExpansionItem>(
                 assign<&ast::ExpansionItem::unit>(ref<ExpansionUnit>) +
                 option(assign<&ast::ExpansionItem::nodeDesc>(ref<NodeDescriptor>)))> {};

  struct Expansion
      : rule<create<ast::Expansion>(
                 option(kw<"LOOKAHEAD"> + lit<"("> +
                        assign<&ast::Expansion::lookahead>(ref<LocalLookahead>) +
                        lit<")">) +
                 many(append<&ast::Expansion::items>(ref<ExpansionItem>)))> {};

  struct ExpansionChoices
      : rule<create<ast::ExpansionChoices>(
                 some(append<&ast::ExpansionChoices::alts>(ref<Expansion>), lit<"|">))> {};

  struct StringList
      : rule<create<ast::StringList>(
                 lit<"("> + some(append<&ast::StringList::items>(ref<StringValue>), lit<",">) +
                 lit<")">)> {};

  struct OptionBinding
      : rule<create<ast::OptionBinding>(
                 assign<&ast::OptionBinding::name>(ref<ID> | kw<"LOOKAHEAD"> |
                                                   kw<"IGNORE_CASE"> | kw<"static"> |
                                                   kw<"PARSER_BEGIN">) +
                 lit<"="> +
                 assign<&ast::OptionBinding::value>(ref<IntLitValue> | ref<BOOL_LIT> |
                                                    ref<StringValue> | ref<StringList>) +
                 lit<";">)> {};

  struct JavaccInput
      : rule<option(kw<"options"> + lit<"{"> +
                    many(append<&ast::JavaccInput::options>(ref<OptionBinding>)) +
                    lit<"}">) +
             kw<"PARSER_BEGIN"> + lit<"("> +
             assign<&ast::JavaccInput::parserBeginName>(ref<ID>) + lit<")"> +
             assign<&ast::JavaccInput::unit>(ref<CompilationUnit>) + kw<"PARSER_END"> +
             lit<"("> + assign<&ast::JavaccInput::parserEndName>(ref<ID>) + lit<")"> +
             some(append<&ast::JavaccInput::productions>(ref<Production>))> {};

  using entry = JavaccInput;
  using ignored = list<WS, EGEN>;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace javacc
