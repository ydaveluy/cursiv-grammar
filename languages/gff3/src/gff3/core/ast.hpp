#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace gff3::ast {

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  boolean hasEquals = false;
  vector<string_view> values;
};

struct Feature : cursiv::Extends<cursiv::AstNode> {
  string_view seqid;
  string_view source;
  string_view type;
  string_view start;
  string_view end;
  string_view score;
  string_view strand;
  string_view phase;
  boolean hasAttributeColumn = false;
  boolean attributesMissing = false;
  vector<Attribute*> attributes;
  vector<string_view> extra;
};

struct CommentLine : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Sequence : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view description;
  vector<string_view> lines;
};

struct Fasta : cursiv::Extends<cursiv::AstNode> {
  boolean marked = false;
  vector<string_view> leadingComments;
  vector<Sequence*> sequences;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Feature*, CommentLine*>> lines;
  Fasta* fasta = nullptr;
};

}  // namespace gff3::ast
