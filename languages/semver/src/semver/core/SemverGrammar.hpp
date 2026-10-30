// SemVer core and dot-separated identifiers; numeric validity is checked at runtime.
#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "semver/core/ast.hpp"
namespace semver {
struct Semver : cursiv::GrammarSpec {
  struct NUMBER : terminal<some(d)> {};
  struct IDENTIFIER : terminal<some(cr<"a-zA-Z0-9-">)> {};
  struct VersionCore : rule<
      assign<&ast::VersionCore::major>(ref<NUMBER>) + lit<"."> +
      assign<&ast::VersionCore::minor>(ref<NUMBER>) + lit<"."> +
      assign<&ast::VersionCore::patch>(ref<NUMBER>)> {};
  struct Segment : rule<some(append<&ast::Segment::identifiers>(ref<IDENTIFIER>), lit<".">)> {};
  struct SemverRule : rule<assign<&ast::Semver::core>(ref<VersionCore>) +
      option(lit<"-"> + assign<&ast::Semver::preRelease>(ref<Segment>)) +
      option(lit<"+"> + assign<&ast::Semver::build>(ref<Segment>))> {};
  using entry = SemverRule;
};
}
