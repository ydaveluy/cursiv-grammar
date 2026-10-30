#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <cstdint>

namespace guido::ast {

struct SequenceItem : cursiv::Extends<cursiv::AstNode> {};

struct Number : cursiv::Extends<cursiv::AstNode> {
  boolean negative = false;
  boolean positive = false;
  string_view value;
};

enum class Accidental : std::uint8_t { sharp, flat };

struct Duration : cursiv::Extends<cursiv::AstNode> {
  boolean asterisk = false;
  boolean milliseconds = false;
  Number *numerator = nullptr;
  Number *denominator = nullptr;
};

struct Note : cursiv::Extends<SequenceItem> {
  string_view name;
  string_view annotation;
  vector<Accidental> accidentals;
  Number *octave = nullptr;
  Duration *duration = nullptr;
  string_view dots;
};

struct Tablature : cursiv::Extends<SequenceItem> {
  string_view string;  // s1 through s6
  string_view frets;   // decoded colon-delimited fret text
  Duration* duration = nullptr;
  string_view dots;
};

struct ChordMember : cursiv::Extends<cursiv::AstNode> {
  vector<SequenceItem *> items;
};

struct Chord : cursiv::Extends<SequenceItem> {
  vector<ChordMember *> members;
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view text;
  string_view unit;
  boolean quoted = false;
  boolean real = false;
  boolean bare = false;
  boolean variable = false;
};

struct Tag : cursiv::Extends<SequenceItem> {
  string_view name;
  string_view id;
  boolean bar = false;
  boolean ranged = false;
  vector<Parameter *> parameters;
  vector<SequenceItem *> notes;
};

struct VariableRef : cursiv::Extends<SequenceItem> {
  string_view name;
};

struct VariableDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Parameter *value = nullptr;
};

struct Sequence : cursiv::Extends<cursiv::AstNode> {
  vector<SequenceItem *> items;
};

struct Segment : cursiv::Extends<cursiv::AstNode> {
  vector<Sequence *> sequences;
};

struct Prog : cursiv::Extends<cursiv::AstNode> {
  vector<VariableDecl *> variables;
  vector<Segment *> segments;
  vector<Sequence *> sequences;
};

} // namespace guido::ast
