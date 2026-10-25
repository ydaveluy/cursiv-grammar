// The rfc822 language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rfc822::ast {

// A Mailbox or a Group.
struct Address : cursiv::Extends<cursiv::AstNode> {};

// A DomainRef or a DomainLiteral.
struct Subdomain : cursiv::Extends<cursiv::AstNode> {};

struct DomainRef : cursiv::Extends<Subdomain> {
  string_view name;
};

struct DomainLiteral : cursiv::Extends<Subdomain> {
  string_view text;  // between the brackets
};

// '@' domain ':' -- one hop of a source route, inside a RouteAddr.
struct Route : cursiv::Extends<cursiv::AstNode> {
  vector<Subdomain*> domain;
};

struct AddrSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> localPart;  // dot-separated words
  vector<Subdomain*> domain;
};

struct RouteAddr : cursiv::Extends<cursiv::AstNode> {
  vector<Route*> routes;
  AddrSpec* addr;
};

// A bare AddrSpec, or a display phrase followed by a RouteAddr.
struct Mailbox : cursiv::Extends<Address> {};

struct BareMailbox : cursiv::Extends<Mailbox> {
  AddrSpec* addr;
};

struct NamedMailbox : cursiv::Extends<Mailbox> {
  vector<string_view> phrase;
  RouteAddr* routeAddr;
};

struct Group : cursiv::Extends<Address> {
  vector<string_view> phrase;
  vector<Mailbox*> mailboxes;
};

}  // namespace rfc822::ast
