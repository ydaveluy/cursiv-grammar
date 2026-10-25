// The creole language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace creole::ast {

// One inline item: text and the markup around it.
struct Markup : cursiv::Extends<cursiv::AstNode> {};

// A run of text, escapes included as written; a line break joining two
// lines of one paragraph is a Text of its own.
struct Text : cursiv::Extends<Markup> {
  string_view text;
};

struct Bold : cursiv::Extends<Markup> {
  vector<Markup*> content;
};

struct Italics : cursiv::Extends<Markup> {
  vector<Markup*> content;
};

// `[[target]]`, `[[target|label]]`, or a free-standing URL.
struct Href : cursiv::Extends<Markup> {
  Text* target;
  vector<Markup*> label;
};

// `{{target}}` or `{{target|alt}}`.
struct Image : cursiv::Extends<Markup> {
  Text* target;
  Text* alt = nullptr;
};

// `\\`, a forced line break.
struct LineBreak : cursiv::Extends<Markup> {};

// `{{{...}}}` within a line.
struct Nowiki : cursiv::Extends<Markup> {
  string_view text;
};

// One block of the document.
struct Block : cursiv::Extends<cursiv::AstNode> {};

struct Paragraph : cursiv::Extends<Block> {
  vector<Markup*> content;
};

// Its text unparsed, the closing `=`s dropped.
struct Title : cursiv::Extends<Block> {
  int32 level;
  Text* content = nullptr;
};

struct BulletItem : cursiv::Extends<Block> {
  int32 level;
  vector<Markup*> content;
};

struct NumberedItem : cursiv::Extends<Block> {
  int32 level;
  vector<Markup*> content;
};

struct TableCell : cursiv::Extends<cursiv::AstNode> {
  boolean header;
  vector<Markup*> content;
};

struct TableRow : cursiv::Extends<Block> {
  vector<TableCell*> cells;
};

struct Hline : cursiv::Extends<Block> {};

// A `{{{` ... `}}}` block, its lines as written.
struct Preformatted : cursiv::Extends<Block> {
  string_view text;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Block*> blocks;
};

}  // namespace creole::ast
