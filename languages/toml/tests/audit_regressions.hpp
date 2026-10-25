#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "toml/core/ast.hpp"
#include "toml/generated/Toml/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = toml::generated::Toml::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = toml::ast;
  auto r =
      parser->parse("\"a.b\" = 1\na.b = 2\ns = \"\"\"say \"hello\"\"\"\"\n");
  check(r.accepted && !r.cst().recovered(),
        "quoted dotted keys and multiline internal quotes");
  auto *d = as<ast::Document>(r.value);
  auto *t = d && !d->tables.empty() ? d->tables[0] : nullptr;
  check(t && t->entries.size() == 3 && t->entries[0]->key->parts.size() == 1 &&
            t->entries[0]->key->parts[0] == "a.b" &&
            t->entries[1]->key->parts.size() == 2,
        "literal dots differ from key separators");
  return ok;
}
} // namespace audit_regressions
