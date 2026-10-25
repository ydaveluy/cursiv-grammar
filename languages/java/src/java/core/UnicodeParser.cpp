#include "java/core/UnicodeParser.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "cursiv/core/syntax-tree/CstBuilder.hpp"

namespace java {
namespace {

using cursiv::TextOffset;
constexpr TextOffset absent = std::numeric_limits<TextOffset>::max();

struct Unit {
  std::uint32_t value;
  TextOffset begin, end;
};

// Character.digit(char, 16), used by javac's UnicodeReader, also accepts
// decimal digits outside ASCII and fullwidth A-F. These are the BMP decimal
// blocks (supplementary characters are two UTF-16 units, neither a digit).
int hex_digit(std::uint32_t c) {
  if (c >= 'a' && c <= 'f')
    return int(c - 'a' + 10);
  if (c >= 'A' && c <= 'F')
    return int(c - 'A' + 10);
  if (c >= 0xff41 && c <= 0xff46)
    return int(c - 0xff41 + 10);
  if (c >= 0xff21 && c <= 0xff26)
    return int(c - 0xff21 + 10);
  constexpr std::uint16_t zeroes[] = {
      0x30,   0x660,  0x6f0,  0x7c0,  0x966,  0x9e6,  0xa66,  0xae6,
      0xb66,  0xbe6,  0xc66,  0xce6,  0xd66,  0xde6,  0xe50,  0xed0,
      0xf20,  0x1040, 0x1090, 0x17e0, 0x1810, 0x1946, 0x19d0, 0x1a80,
      0x1a90, 0x1b50, 0x1bb0, 0x1c40, 0x1c50, 0xa620, 0xa8d0, 0xa900,
      0xa9d0, 0xa9f0, 0xaa50, 0xabf0, 0xff10};
  for (const auto zero : zeroes)
    if (c >= zero && c < std::uint32_t(zero) + 10)
      return int(c - zero);
  return -1;
}

void utf8(std::string &out, std::uint32_t c) {
  if (c <= 0x7f)
    out += char(c);
  else if (c <= 0x7ff) {
    out += char(0xc0 | (c >> 6));
    out += char(0x80 | (c & 63));
  } else if (c <= 0xffff) {
    out += char(0xe0 | (c >> 12));
    out += char(0x80 | ((c >> 6) & 63));
    out += char(0x80 | (c & 63));
  } else {
    out += char(0xf0 | (c >> 18));
    out += char(0x80 | ((c >> 12) & 63));
    out += char(0x80 | ((c >> 6) & 63));
    out += char(0x80 | (c & 63));
  }
}

struct Translated final : cursiv::Grammar {
  std::string text;
  // Two maps cover even an engine repair ending inside a translated scalar.
  std::vector<TextOffset> begins{0}, ends{0}, originalBoundary;
  cursiv::ParseResult inner;
  std::vector<cursiv::ParseDiagnostic> inputErrors;

  const cursiv::Grammar &grammar() const { return *inner.cst().grammar(); }
  bool recordWrites(const cursiv::CstNode &n,
                    std::string_view s) const noexcept override {
    return grammar().recordWrites(n, s);
  }
  bool recordFills(const cursiv::CstNode &n,
                   std::string_view s) const noexcept override {
    return grammar().recordFills(n, s);
  }
  std::string_view
  recordRule(const cursiv::CstNode &n) const noexcept override {
    return grammar().recordRule(n);
  }
  std::string_view
  recordFeature(const cursiv::CstNode &n) const noexcept override {
    return grammar().recordFeature(n);
  }
  std::string_view terminalText(std::uint16_t t) const noexcept override {
    return grammar().terminalText(t);
  }
  std::string_view terminalName(std::uint16_t t) const noexcept override {
    return grammar().terminalName(t);
  }
  std::string_view terminalDoc(std::uint16_t t) const noexcept override {
    return grammar().terminalDoc(t);
  }
  std::string_view spellingDoc(std::string_view s) const noexcept override {
    return grammar().spellingDoc(s);
  }

  void append(std::string_view spelling, TextOffset begin, TextOffset end) {
    const auto start = TextOffset(text.size());
    begins.back() = begin;
    ends.back() = begin;
    text += spelling;
    for (std::size_t i = 0; i < spelling.size(); ++i) {
      begins.push_back(i + 1 == spelling.size() ? end : begin);
      ends.push_back(end);
    }
    originalBoundary[begin] = start;
    originalBoundary[end] = TextOffset(text.size());
  }

  void translate(std::string_view source,
                 const cursiv::CancellationToken &cancel) {
    cursiv::throw_if_cancelled(cancel);
    originalBoundary.assign(source.size() + 1, absent);
    originalBoundary[0] = 0;
    std::vector<Unit> units;
    for (std::size_t i = 0; i < source.size();) {
      if ((i & 1023) == 0)
        cursiv::throw_if_cancelled(cancel);
      const auto begin = TextOffset(i);
      auto c = std::uint32_t(static_cast<unsigned char>(source[i++]));
      const unsigned bytes = c < 0x80                 ? 1
                             : c >= 0xc2 && c <= 0xdf ? 2
                             : c >= 0xe0 && c <= 0xef ? 3
                             : c >= 0xf0 && c <= 0xf4 ? 4
                                                      : 0;
      bool valid = bytes != 0 && begin + bytes <= source.size();
      std::uint32_t scalar = bytes > 1 ? c & ((1u << (7 - bytes)) - 1) : c;
      for (unsigned j = 1; valid && j < bytes; ++j) {
        const auto b = static_cast<unsigned char>(source[begin + j]);
        valid = (b & 0xc0) == 0x80;
        scalar = (scalar << 6) | (b & 63);
      }
      valid = valid &&
              (bytes < 2 || scalar >= (bytes == 2   ? 0x80u
                                       : bytes == 3 ? 0x800u
                                                    : 0x10000u)) &&
              scalar <= 0x10ffff && !(scalar >= 0xd800 && scalar <= 0xdfff);
      if (!valid) {
        // Preserve malformed UTF-8 bytes for the lexical parser to diagnose.
        units.push_back({0x110000 + c, begin, TextOffset(i)});
      } else {
        i = begin + bytes;
        if (scalar > 0xffff) {
          units.push_back(
              {0xd800 + ((scalar - 0x10000) >> 10), begin, TextOffset(i)});
          units.push_back(
              {0xdc00 + ((scalar - 0x10000) & 1023), begin, TextOffset(i)});
        } else
          units.push_back({scalar, begin, TextOffset(i)});
      }
    }

    std::size_t pos = 0;
    bool wasBackslash = false, wasUnicodeEscape = false;
    // This follows UnicodeReader.nextUnicodeInputCharacter, including parity
    // after an escape producing a backslash and recovery of a malformed one.
    const auto next = [&]() -> Unit {
      for (;;) {
        if ((pos & 1023) == 0)
          cursiv::throw_if_cancelled(cancel);
        if (pos == units.size())
          return {0xffffffff, TextOffset(source.size()),
                  TextOffset(source.size())};
        auto u = units[pos++];
        if (u.value == '\\' && (!wasBackslash || wasUnicodeEscape)) {
          auto p = pos;
          while (p < units.size() && units[p].value == 'u') {
            if ((p & 1023) == 0)
              cursiv::throw_if_cancelled(cancel);
            ++p;
          }
          if (p == pos) {
            wasUnicodeEscape = false;
            wasBackslash = !wasBackslash;
          } else {
            std::uint32_t value = 0;
            unsigned digits = 0;
            for (; digits < 4 && p < units.size(); ++digits, ++p) {
              const auto d = hex_digit(units[p].value);
              if (d < 0)
                break;
              value = (value << 4) | unsigned(d);
            }
            if (digits != 4) {
              const auto at = p == units.size() ? TextOffset(source.size())
                                                : units[p].begin;
              inputErrors.push_back({cursiv::DiagnosticKind::conversion_error,
                                     cursiv::kNoElement,
                                     {},
                                     u.begin,
                                     at,
                                     "Malformed Java Unicode escape"});
              pos = p;
              continue;
            }
            u.value = value;
            u.end = units[p - 1].end;
            pos = p;
            wasUnicodeEscape = true;
            wasBackslash = value == '\\' && !wasBackslash;
          }
        } else
          wasUnicodeEscape = wasBackslash = false;
        return u;
      }
    };
    for (;;) {
      auto u = next();
      if (u.value == 0xffffffff)
        break;
      if (u.value >= 0xd800 && u.value <= 0xdbff) {
        const auto saved = pos;
        const auto low = next();
        if (low.value >= 0xdc00 && low.value <= 0xdfff) {
          u.value = 0x10000 + ((u.value - 0xd800) << 10) + low.value - 0xdc00;
          u.end = low.end;
        } else {
          // javac restores the input position, but retains the lookahead's
          // backslash state for a high surrogate without a low partner.
          pos = saved;
        }
      }
      std::string spelling;
      if (u.value >= 0xd800 && u.value <= 0xdfff) {
        // Cursiv's Unicode lexer accepts scalars, not WTF-8. Keep this UTF-16
        // unit losslessly as a literal escape; paired units use normal UTF-8.
        constexpr char hex[] = "0123456789ABCDEF";
        spelling = "\\u";
        for (int shift = 12; shift >= 0; shift -= 4)
          spelling += hex[(u.value >> shift) & 15];
      } else if (u.value > 0x10ffff)
        spelling += char(u.value - 0x110000);
      else
        utf8(spelling, u.value);
      append(spelling, u.begin, u.end);
    }
    begins.back() = ends.back() = TextOffset(source.size());
    originalBoundary[source.size()] = TextOffset(text.size());
  }
};

class UnicodeParser final : public cursiv::Parser {
public:
  explicit UnicodeParser(std::unique_ptr<const cursiv::Parser> delegate)
      : _delegate(std::move(delegate)) {}
  const cursiv::Grammar &getGrammar() const noexcept override {
    return _delegate->getGrammar();
  }
  cursiv::ParseResult
  parse(std::string_view source,
        const cursiv::CancellationToken &cancel) const override {
    cursiv::throw_if_cancelled(cancel);
    if (source.find("\\u") == std::string_view::npos)
      return _delegate->parse(source, cancel);
    // Allocate before parsing: moving a small string afterwards invalidates
    // the AST's borrowed views even when the moved-to string is still alive.
    auto owner = std::make_shared<Translated>();
    cursiv::Cst tree(source); // validates the original document's size
    owner->translate(source, cancel);
    owner->inner = _delegate->parse(owner->text, cancel);
    const auto &old = owner->inner.cst();
    cursiv::CstBuilder builder(tree);
    builder.reserve_stream(old.size());
    for (cursiv::NodeId id = 0; id < old.size(); ++id) {
      if ((id & 1023) == 0)
        cursiv::throw_if_cancelled(cancel);
      auto n = old[id];
      n.begin = owner->begins[n.begin];
      n.end = owner->ends[n.end];
      builder.leaf(n.begin, n.end, n.engine);
      builder.stream().at(id) = n; // identical ids, stamps and subtree sizes
    }
    if (old.engine_stamped())
      builder.bind_stream();
    builder.bind_entry(old.entry_begin(), old.entry_end(), old.entry_root());
    builder.bind_accepted(old.accepted());
    for (const auto &d : old.diagnostics())
      builder.note_repair(d.kind, d.element, owner->begins[d.begin],
                          owner->ends[d.end], d.also);
    cursiv::ParseResult result(std::move(tree));
    result.arena = std::move(owner->inner.arena);
    result.value = owner->inner.value;
    result.error = owner->inner.error;
    result.accepted = owner->inner.accepted;
    result.nodes.assign(owner->inner.nodes.begin(), owner->inner.nodes.end());
    // Reference handles retain the address of this object, not of a result.
    result.references = std::move(owner->inner.references);
    for (auto d : owner->inner.diagnostics) {
      d.begin = owner->begins[d.begin];
      d.end = owner->ends[d.end];
      result.diagnostics.push_back(std::move(d));
    }
    result.diagnostics.insert(result.diagnostics.end(),
                              owner->inputErrors.begin(),
                              owner->inputErrors.end());
    result.bind_grammar(std::move(owner));
    return result;
  }
  void nextFeaturesAt(const cursiv::Cst &cst, TextOffset at, TextOffset resume,
                      std::vector<cursiv::NextFeature> &out) const override {
    if (const auto *translated =
            dynamic_cast<const Translated *>(cst.grammar())) {
      if (at >= translated->originalBoundary.size() ||
          resume >= translated->originalBoundary.size())
        return;
      const auto a = translated->originalBoundary[at],
                 r = translated->originalBoundary[resume];
      // A replacement strictly inside an escape would corrupt its spelling.
      if (a != absent && r != absent)
        _delegate->nextFeaturesAt(translated->inner.cst(), a, r, out);
    } else
      _delegate->nextFeaturesAt(cst, at, resume, out);
  }

private:
  std::unique_ptr<const cursiv::Parser> _delegate;
};

} // namespace

std::unique_ptr<cursiv::Parser>
unicode_parser(std::unique_ptr<const cursiv::Parser> delegate) {
  return std::make_unique<UnicodeParser>(std::move(delegate));
}

} // namespace java
