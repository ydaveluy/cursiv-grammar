#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace semver::ast {
struct VersionCore : cursiv::Extends<cursiv::AstNode> {
  string_view major;
  string_view minor;
  string_view patch;
};
struct Segment : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> identifiers;
};
struct Semver : cursiv::Extends<cursiv::AstNode> {
  VersionCore* core;
  Segment* preRelease;
  Segment* build;
};
}
