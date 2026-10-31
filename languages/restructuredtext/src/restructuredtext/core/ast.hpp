// The restructuredtext language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace restructuredtext::ast {

struct Emphasis : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Strong : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// Double backticks delimit an inline literal.
struct Literal : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// `word_` -- a reference to a target defined elsewhere in the document.
struct Reference : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// _`target name` -- an inline internal target.
struct HyperlinkTarget : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// `` `label <url>`_ ``, content kept whole: the label/url split is a
// consumer's job, not the tree's.
struct Hyperlink : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// Single backticks delimit interpreted text. A role may precede or follow
// the text; an empty role denotes the default title-reference role.
struct InterpretedText : cursiv::Extends<cursiv::AstNode> {
  string_view role;
  string_view text;
};

// Namespace scope, so the AstNode vocabulary (`string_view`...) is out of
// reach here and the std spelling is the only one.
using Span = std::variant<std::string_view, Emphasis*, Strong*, Literal*,
                           Reference*, HyperlinkTarget*, Hyperlink*,
                           InterpretedText*>;

struct Paragraph : cursiv::Extends<cursiv::AstNode> {
  vector<Span> spans;
};

// An interactive session is source text, without inline interpretation.
struct DoctestBlock : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct DirectiveBody : cursiv::Extends<cursiv::AstNode> {
  vector<variant<string_view, DirectiveBody*>> entries;
};

struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view header;  // name:: arguments.
  DirectiveBody*
      body;  // Indented options and content, including deeper blocks.
};

// The target header is distinct from a comment. Destination is raw URI or
// indirect reference text; indented continuation lines retain their hierarchy.
struct ExplicitTarget : cursiv::Extends<cursiv::AstNode> {
  boolean anonymous;
  string_view name;
  string_view destination;
  DirectiveBody* continuation;
};

struct Comment : cursiv::Extends<cursiv::AstNode> {
  string_view firstLine;  // the rest of the marker's own line, may be empty
  vector<string_view> bodyLines;
};

struct BulletItem;
struct EnumItem;
struct LineBlock;
struct BlockQuote;
struct DefinitionItem;
struct FieldList;

// A line of four or more of one punctuation byte between blank lines.
struct Transition : cursiv::Extends<cursiv::AstNode> {};

using SectionElement =
    std::variant<ExplicitTarget*, Directive*, Comment*, BulletItem*, EnumItem*, LineBlock*,
                 Paragraph*, Transition*, BlockQuote*, DefinitionItem*,
                 DoctestBlock*, FieldList*>;

// Keep the field name's source spelling, including escapes/inline markup.
// Bibliographic transforms and name interpretation belong to consumers.
struct FieldItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<SectionElement> body;
};

struct FieldList : cursiv::Extends<cursiv::AstNode> {
  vector<FieldItem*> fields;
};

// An item's body: its text, then whatever its indented lines hold. Empty
// for an item with no text.
struct BulletItem : cursiv::Extends<cursiv::AstNode> {
  string_view marker;  // "*", "-" or "+"
  vector<SectionElement> body;
};

struct EnumItem : cursiv::Extends<cursiv::AstNode> {
  string_view number;
  vector<SectionElement> body;
};

struct LineBlockLine : cursiv::Extends<cursiv::AstNode> {
  vector<Span> spans;
};

struct LineBlock : cursiv::Extends<cursiv::AstNode> {
  vector<LineBlockLine*> lines;
};

// Lines indented past the text around them.
struct BlockQuote : cursiv::Extends<cursiv::AstNode> {
  vector<SectionElement> body;
};

// A one-line term, its definition indented on the lines right below.
struct DefinitionItem : cursiv::Extends<cursiv::AstNode> {
  vector<Span> term;
  vector<SectionElement> body;
};

// A heading: title and underline only. Its own trailing content is not a
// grammar member -- see `section_body` below.
struct Section : cursiv::Extends<cursiv::AstNode> {
  vector<Span> title;
  string_view underline;
};

using Block = std::variant<Section*, ExplicitTarget*, Directive*, Comment*, BulletItem*,
                           EnumItem*, LineBlock*, Paragraph*, Transition*,
                           BlockQuote*, DefinitionItem*, DoctestBlock*, FieldList*>;

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Block> blocks;
};

// The Blocks between `section` and the next Section (or the end of
// `blocks`), the way docutils itself groups a flat parse by heading in a
// separate pass. Grouping never recurses: a further heading found while
// scanning ends it, and belongs to the next call, for its own title.
inline std::vector<SectionElement> section_body(const Document& doc,
                                                 const Section* section) {
  std::vector<SectionElement> body;
  bool inside = false;
  for (const Block& block : doc.blocks) {
    if (const auto* s = std::get_if<Section*>(&block)) {
      if (inside) break;
      inside = (*s == section);
      continue;
    }
    if (!inside) continue;
    std::visit(
        [&](auto* node) {
          if constexpr (!std::is_same_v<decltype(node), Section*>)
            body.push_back(node);
        },
        block);
  }
  return body;
}

}  // namespace restructuredtext::ast
