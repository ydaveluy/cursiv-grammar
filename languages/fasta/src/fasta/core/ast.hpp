#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fasta::ast {

struct Section : cursiv::Extends<cursiv::AstNode> {};

struct DescriptionLine : cursiv::Extends<Section> {
  string_view text;
};

struct SequenceLine : cursiv::Extends<Section> {
  string_view text;
};

struct CommentLine : cursiv::Extends<Section> {
  string_view text;
};

struct BlankLine : cursiv::Extends<Section> {};

struct Record : cursiv::Extends<cursiv::AstNode> {
  DescriptionLine* description;
  vector<Section*> body;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Section*> preamble;
  vector<Record*> records;
};

} // namespace fasta::ast
