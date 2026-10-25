#pragma once
// Frozen projections from rfc3986 2.0.0, 2026-10-25; no runtime Python dependency.
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "url/core/CoreServices.hpp"
#include <unordered_set>
namespace native_conformance {
using J = cursiv::JsonValue;
inline J project(const url::ast::Url *n) {
  if (!n)
    return nullptr;
  std::string authority, path, query;
  if (n->login) {
    authority += n->login->text;
    authority += '@';
  }
  if (n->host)
    authority += n->host->text;
  if (n->path) {
    if (n->path->absolute)
      path += '/';
    for (std::size_t i = 0; i < n->path->segments.size(); ++i) {
      if (i)
        path += '/';
      path += n->path->segments[i];
    }
    if (n->path->trailingSlash)
      path += '/';
  }
  for (std::size_t i = 0; i < n->query.size(); ++i) {
    if (i)
      query += '&';
    query += n->query[i]->key;
    if (n->query[i]->hasEquals) {
      query += '=';
      query += n->query[i]->value;
    }
  }
  return J::Object{{"scheme", n->scheme.empty() ? J(nullptr) : J(n->scheme)},
                   {"authority", authority.empty() ? J(nullptr) : J(authority)},
                   {"path", path.empty() ? J(nullptr) : J(path)},
                   {"query", n->hasQuery ? J(query) : J(nullptr)},
                   {"fragment", n->hasFragment ? J(n->frag) : J(nullptr)}};
}
inline bool run() {
  auto language = cursiv::create_language<url::UrlCoreServices,
                                         url::UrlSharedCoreServices,
                                         url::install_core_module>();
  auto factory = url::generated::Url::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) std::fprintf(stderr, "native URI: %s\n", message);
    ok &= value;
  };
  struct Case { std::string_view text, expected; };
  const Case cases[] = {
    {R"uri()uri", R"json({"authority":null,"fragment":null,"path":null,"query":null,"scheme":null})json"},
    {R"uri(http://example.com)uri", R"json({"authority":"example.com","fragment":null,"path":null,"query":null,"scheme":"http"})json"},
    {R"uri(HTTP://EXAMPLE.COM:080/a)uri", R"json({"authority":"EXAMPLE.COM:080","fragment":null,"path":"/a","query":null,"scheme":"HTTP"})json"},
    {R"uri(mailto:John.Doe@example.com)uri", R"json({"authority":null,"fragment":null,"path":"John.Doe@example.com","query":null,"scheme":"mailto"})json"},
    {R"uri(g?y#s)uri", R"json({"authority":null,"fragment":"s","path":"g","query":"y","scheme":null})json"},
    {R"uri(//)uri", R"json({"authority":null,"fragment":null,"path":null,"query":null,"scheme":null})json"},
    {R"uri(///)uri", R"json({"authority":null,"fragment":null,"path":"/","query":null,"scheme":null})json"},
    {R"uri(a:?#)uri", R"json({"authority":null,"fragment":"","path":null,"query":"","scheme":"a"})json"},
    {R"uri(?)uri", R"json({"authority":null,"fragment":null,"path":null,"query":"","scheme":null})json"},
    {R"uri(#)uri", R"json({"authority":null,"fragment":"","path":null,"query":null,"scheme":null})json"},
    {R"uri(?#)uri", R"json({"authority":null,"fragment":"","path":null,"query":"","scheme":null})json"},
    {R"uri(///a//b///)uri", R"json({"authority":null,"fragment":null,"path":"/a//b///","query":null,"scheme":null})json"},
    {R"uri(https://user:password@example.org:443/a)uri", R"json({"authority":"user:password@example.org:443","fragment":null,"path":"/a","query":null,"scheme":"https"})json"},
    {R"uri(https://:pass@example.org/)uri", R"json({"authority":":pass@example.org","fragment":null,"path":"/","query":null,"scheme":"https"})json"},
    {R"uri(https://user:@example.org/)uri", R"json({"authority":"user:@example.org","fragment":null,"path":"/","query":null,"scheme":"https"})json"},
    {R"uri(http://[::1]:80/)uri", R"json({"authority":"[::1]:80","fragment":null,"path":"/","query":null,"scheme":"http"})json"},
    {R"uri(http://[v1.a:b]/)uri", R"json({"authority":"[v1.a:b]","fragment":null,"path":"/","query":null,"scheme":"http"})json"},
    {R"uri(http://exa%6Dple.org/%2f/%41?x=%26%3d#%23)uri", R"json({"authority":"exa%6Dple.org","fragment":"%23","path":"/%2f/%41","query":"x=%26%3d","scheme":"http"})json"},
    {R"uri(http://host/?a=1&a=2)uri", R"json({"authority":"host","fragment":null,"path":"/","query":"a=1&a=2","scheme":"http"})json"},
    {R"uri(http://host/?&&a=&=v&&)uri", R"json({"authority":"host","fragment":null,"path":"/","query":"&&a=&=v&&","scheme":"http"})json"},
    {R"uri(http://host/?a=b=c&x=y/z?q#f?g)uri", R"json({"authority":"host","fragment":"f?g","path":"/","query":"a=b=c&x=y/z?q","scheme":"http"})json"},
    {R"uri(http://host/?#)uri", R"json({"authority":"host","fragment":"","path":"/","query":"","scheme":"http"})json"},
  };
  for (const auto &c : cases) {
    const auto r = language.language.parser->parse(c.text);
    const auto f = factory->parse(c.text);
    const auto *n = r.value_as<url::ast::Url>();
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && n, "native positive parses");
    if (!n) continue;
    const auto expected = c.expected;
    check(project(n).toJsonString({.pretty = false}) == expected, "native five-component projection");
    check(project(f.value_as<url::ast::Url>()).toJsonString({.pretty = false}) == expected &&
          f.accepted && !f.cst().recovered(), "generated factory agrees");
    const auto components = project(n);
    std::string raw;
    if (!n->scheme.empty()) { raw += n->scheme; raw += ':'; }
    if (n->hasAuthority) { raw += "//"; if (n->login) { raw += n->login->text; raw += '@'; } if (n->host) raw += n->host->text; }
    if (const auto &p = components.object().at("path"); !p.isNull()) raw += p.string();
    if (n->hasQuery) { raw += '?'; raw += components.object().at("query").string(); }
    if (n->hasFragment) { raw += '#'; raw += n->frag; }
    check(raw == c.text && r.cst().text() == c.text, "raw delimiters and empty components roundtrip");
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      check(seen.insert(&node).second, "unique AST ownership");
      language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
          check(child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId), "parent/CST containment");
          self(self, child);
        });
    };
    walk(walk, *n);
    check(seen.size() == r.nodes.size(), "all nodes reachable");
    check(cursiv::test::validationHelper(language.language, std::string(c.text)).diagnostics.empty(),
          "valid repeated and empty query keys have no warnings");
  }
  struct Invalid { std::string_view text, code; std::size_t begin, end; };
  for (const auto &c : {Invalid{"http://a b/", "url.host", 7, 10},
       Invalid{"http://u@@h/", "url.userinfo", 7, 10},
       Invalid{"http://[]/", "url.host", 7, 9},
       Invalid{"http://[::1]:80]/", "url.host", 7, 16},
       Invalid{"http://[v1.a]:port]/", "url.host", 7, 19},
       Invalid{"http://[::1]:80:90/", "url.host", 7, 18},
       Invalid{"http://[::1]:garbage:80/", "url.host", 7, 23},
       Invalid{"http://[::g]/", "url.host", 7, 12},
       Invalid{"http://[1::2::3]/", "url.host", 7, 16},
       Invalid{"http://[v.a]/", "url.host", 7, 12},
       Invalid{"http://[::ffff:256.0.0.1]/", "url.host", 7, 25},
       Invalid{"http://[::1/", "url.host", 7, 11},
       Invalid{"http://ho%GGst/", "url.host", 7, 14},
       Invalid{"http://host:abc/", "url.port-digits", 7, 15},
       Invalid{"http://host/#a#b", "url.fragment", 13, 16}}) {
    const auto v = cursiv::test::validationHelper(language.language, std::string(c.text));
    check(v.diagnostics.size() == 1 && v.diagnostics[0].code &&
          v.diagnostics[0].code->isString() && v.diagnostics[0].code->string() == c.code &&
          v.diagnostics[0].severity == cursiv::DiagnosticSeverity::Error &&
          v.diagnostics[0].range.begin == c.begin && v.diagnostics[0].range.end == c.end,
          "native rejection has precise runtime diagnostic");
  }
  return ok;
}
} // namespace native_conformance
