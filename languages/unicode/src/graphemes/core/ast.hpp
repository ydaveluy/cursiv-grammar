// The graphemes language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace graphemes::ast {

// One user-perceived character (a Unicode extended grapheme cluster): an
// emoji sequence, a Hangul syllable, a CRLF pair, or any other code point,
// each with its trailing combining marks.
struct GraphemeCluster : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<GraphemeCluster*> clusters;
};

}  // namespace graphemes::ast
