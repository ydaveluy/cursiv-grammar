#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace url::ast {

struct Login : cursiv::Extends<cursiv::AstNode> {
  string_view text;

  string_view user() const { return text.substr(0, text.find(':')); }
  optional<string_view> password() const {
    const auto colon = text.find(':');
    if (colon == string_view::npos) return {};
    return text.substr(colon + 1);
  }
};

struct Host : cursiv::Extends<cursiv::AstNode> {
  string_view text;

  optional<string_view> port() const {
    const auto colon = text.rfind(':');
    const auto bracket = text.rfind(']');
    if (colon == string_view::npos ||
        (bracket != string_view::npos && colon < bracket)) return {};
    return text.substr(colon + 1);
  }
  string_view name() const {
    auto name = text;
    if (const auto p = port()) name.remove_suffix(p->size() + 1);
    if (name.starts_with('[') && name.ends_with(']'))
      name = name.substr(1, name.size() - 2);
    return name;
  }
};

struct SearchParameter : cursiv::Extends<cursiv::AstNode> {
  boolean hasEquals;
  string_view key;
  string_view value;
};

struct Path : cursiv::Extends<cursiv::AstNode> {
  boolean absolute;
  vector<string_view> segments;
  boolean trailingSlash;
};

struct Url : cursiv::Extends<cursiv::AstNode> {
  boolean hasAuthority;
  boolean hasQuery;
  boolean hasFragment;
  string_view scheme;
  Login* login;
  Host* host;
  Path* path;
  vector<SearchParameter*> query;
  string_view frag;

  optional<string_view> port() const { return host ? host->port() : optional<string_view>{}; }
};

}  // namespace url::ast
