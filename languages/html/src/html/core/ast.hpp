// The html language: its AST. A document is optional leading declarations
// (an XML prolog, a doctype) plus a mixed sequence of elements, embedded
// script/style blocks, scriptlets and comments; an element carries its
// attributes and, recursively, the same mixed content.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace html::ast {

struct Element;
struct Script;
struct Style;
struct Scriptlet;
struct CData;
struct Comment;
struct Text;

// Namespace scope: the AstNode field vocabulary is only in reach inside a
// class deriving it, so these spell std:: rather than the alias.
using ElementContent = std::variant<Element*, Script*, Style*, Scriptlet*,
                                    CData*, Comment*, Text*>;
using DocumentContent = ElementContent;

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> value;  // absent for a bare attribute (`disabled`)
};

struct Comment : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // `<!--...-->` or `<![...]>`, delimiters included
};

struct CData : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // `<![CDATA[...]]>`, delimiters included
};

struct Text : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Scriptlet : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // `<?...?>` or `<%...%>`, delimiters included
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // the open tag through the closer, verbatim
};

struct Style : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // the open tag through the closer, verbatim
};

struct Element : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Attribute*> attributes;
  vector<ElementContent> content;
  bool selfClosing = false;
  optional<string_view> closeName;  // absent when self-closing
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> xmlDeclarations;
  vector<string_view> doctypes;
  vector<DocumentContent> content;
};

}  // namespace html::ast
