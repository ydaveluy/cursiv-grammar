#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace swiftfin::ast {
struct KeyValue : cursiv::Extends<cursiv::AstNode> { string_view key; string_view value; };
struct TextField : cursiv::Extends<cursiv::AstNode> { string_view tag; string_view value; };
struct Block : cursiv::Extends<cursiv::AstNode> { string_view id; };
struct Header : cursiv::Extends<Block> { string_view value; };
struct TagBlock : cursiv::Extends<Block> { vector<KeyValue*> tags; };
struct TextBlock : cursiv::Extends<Block> { vector<TextField*> fields; };
struct Message : cursiv::Extends<cursiv::AstNode> { vector<Block*> blocks; };
struct Document : cursiv::Extends<cursiv::AstNode> { vector<Message*> messages; };
}  // namespace swiftfin::ast
