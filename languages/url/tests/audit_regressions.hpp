#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "url/core/ast.hpp"
#include "url/generated/Url/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = url::generated::Url::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = url::ast;
  for (auto text :
       {"mailto:john@example.org", "urn:isbn:0451450523",
        "https://[::1]/a_b;v=1?x=hello%20world#frag", "file:///etc/fstab"}) {
    auto r = parser->parse(text);
    check(r.accepted && !r.cst().recovered(), text);
  }
  auto r = parser->parse("file:///etc/fstab");
  auto *u = as<ast::Url>(r.value);
  check(u && u->hasAuthority && !u->host && u->path && u->path->absolute &&
            u->path->segments.size() == 2 && u->path->segments[0] == "etc",
        "empty file authority does not consume a path component");
  struct Case {
    std::string_view source, scheme, path;
    const char* port;
    bool authority, login;
    std::string_view user, password;
    bool hasPassword;
  };
  const Case cases[] = {
      {"http://j@ne:password@google.com", "http", "", nullptr, true, true, "j@ne", "password", true},
      {"http://jane:p@ssword@google.com", "http", "", nullptr, true, true, "jane", "p@ssword", true},
      {"postgres://host1:1,host2:2,host3:3", "postgres", "", "3", true, false, "", "", false},
      {"http://example.org/a//b///?x=&=y&&#", "http", "/a//b///", nullptr, true, false, "", "", false},
      {"http://@example.org/", "http", "/", nullptr, true, true, "", "", false},
      {"http://:pass@example.org/", "http", "/", nullptr, true, true, "", "pass", true},
      {"http://example.org:/", "http", "/", "", true, false, "", "", false},
      {"../a/b?x=1", "", "../a/b", nullptr, false, false, "", "", false},
      {"//example.org/x", "", "/x", nullptr, true, false, "", "", false},
      {"?x=1", "", "", nullptr, false, false, "", "", false},
      {"#frag", "", "", nullptr, false, false, "", "", false},
      {"", "", "", nullptr, false, false, "", "", false},
      {"http://[fe80::1%25en0]:8080/a", "http", "/a", "8080", true, false, "", "", false},
      {"mailto:john@example.org", "mailto", "john@example.org", nullptr, false, false, "", "", false},
      {"urn:isbn:0451450523", "urn", "isbn:0451450523", nullptr, false, false, "", "", false},
      {"file:///etc/fstab", "file", "/etc/fstab", nullptr, true, false, "", "", false},
      {"http://example.org:4294967296/x", "http", "/x", "4294967296", true, false, "", "", false},
      {"http://example.org:00080/x", "http", "/x", "00080", true, false, "", "", false},
      {"http://user:pass@host/x", "http", "/x", nullptr, true, true, "user", "pass", true},
      {"scheme:a//b/", "scheme", "a//b/", nullptr, false, false, "", "", false},
      {"http://x//", "http", "//", nullptr, true, false, "", "", false},
      {"http://x///", "http", "///", nullptr, true, false, "", "", false},
      {"http://x/", "http", "/", nullptr, true, false, "", "", false},
  };
  for (const auto& c : cases) {
    const auto result = parser->parse(c.source);
    const auto* value = result.value_as<ast::Url>();
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && value,
          "native URI reference must parse clean");
    if (!value) continue;
    std::string path;
    if (value->path) {
      check(value->path->parent == value, "path parent");
      if (value->path->absolute) path += '/';
      for (std::size_t i = 0; i < value->path->segments.size(); ++i) {
        if (i) path += '/';
        path += value->path->segments[i];
      }
      if (value->path->trailingSlash) path += '/';
    }
    check(value->scheme == c.scheme && path == c.path &&
              value->hasAuthority == c.authority, "URI components preserve empty segments");
    check(c.port ? value->port() && *value->port() == c.port : !value->port(),
          "port spelling, including empty and large ports");
    if (value->host) check(value->host->parent == value, "host parent");
    check(bool(value->login) == c.login, "empty userinfo remains present");
    if (value->login)
      check(value->login->parent == value && value->login->user() == c.user &&
                value->login->password().value_or("") == c.password &&
                value->login->password().has_value() == c.hasPassword, "userinfo values");
  }
  return ok;
}
} // namespace audit_regressions
