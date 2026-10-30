// The jam language: its AST. A chord chart -- sections of bar-separated
// measures, each measure a run of chords and the shorthand marks between them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace jam::ast {

enum class Accidental : std::uint8_t { none, flat, sharp };
// Written quality spellings stay distinct; this is not harmonic normalization.
enum class Quality : std::uint8_t { none, m, M, P, d, maj, min, dim, aug, sus, aug_plus };
enum class Sign : std::uint8_t { none, plus, minus };

struct Note : cursiv::Extends<cursiv::AstNode> {
  string_view key;  // one of A-G
  Accidental accidental;
};

struct Bass : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Accidental accidental;
};

// The five shapes a measure repeats. Empty by design: every alternative of
// Item extends it, so a measure's items are one list of one base.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Chord : cursiv::Extends<Item> {
  Note* note;
  Quality quality;
  // 0 is a spellable extension (digits with no floor), so absence needs
  // optional<>, not a sentinel.
  optional<std::int64_t> extension;
  Sign alterationSign;
  optional<std::int64_t> alteration;
  Bass* bass;
};

struct RepeatChord : cursiv::Extends<Item> {};
struct Rest : cursiv::Extends<Item> {};
struct ExtendChord : cursiv::Extends<Item> {};
struct RepeatMeasure : cursiv::Extends<Item> {
  std::int64_t count; // %% repeats the preceding two measures, not one twice.
};

struct Meter : cursiv::Extends<cursiv::AstNode> {
  std::int64_t numerator;
  std::int64_t denominator;
};

struct Measure : cursiv::Extends<cursiv::AstNode> {
  Meter* meter;  // null when this measure states no meter of its own
  vector<Item*> items;
};

struct Section : cursiv::Extends<cursiv::AstNode> {
  vector<Measure*> measures;
};

struct Jam : cursiv::Extends<cursiv::AstNode> {
  vector<Section*> sections;
};

}  // namespace jam::ast
