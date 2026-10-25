// The iri language: its AST. RFC 3987 has one construct, the IRI itself --
// scheme, an optional authority, a path, and an optional query/fragment.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace iri::ast {

// One span, not three fields: userinfo and host share the same character
// class, so splitting them needs the '@' lookahead AUTHORITY already
// resolves by trying the "userinfo@host" alternative first.
struct Authority : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};

struct Iri : cursiv::Extends<cursiv::AstNode> {
  string_view scheme;
  Authority* authority;  // null when the IRI carries no "//" at all
  string_view path;
  boolean has_query;
  string_view query;
  boolean has_fragment;
  string_view fragment;
};

}  // namespace iri::ast
