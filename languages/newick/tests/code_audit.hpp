#pragma once
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "newick/core/CoreServices.hpp"
#include "newick/core/LengthValue.hpp"
#include "newick/generated/Newick/Module.hpp"
namespace code_audit {
namespace ast = newick::ast;
inline bool run() {
  auto language = cursiv::create_language<newick::NewickCoreServices,
                                          newick::NewickSharedCoreServices,
                                          newick::install_core_module>();
  bool ok = true;
  auto check = [&](bool b, const char* s) {
    if (!b) {
      std::fprintf(stderr, "newick code audit: %s\n", s);
      ok = false;
    }
  };
  const auto visit = [&](const ast::Clade* root, auto&& callback) {
    std::function<void(const ast::Clade*)> walk = [&](const auto* n) {
      if (!n) return;
      callback(*n);
      for (const auto* c : n->children) {
        check(c->parent == n, "parent ownership");
        walk(c);
      }
    };
    walk(root);
  };
  const auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    visit(r.value_as<ast::Clade>(), [&](const auto& n) {
      if (n.length)
        check(newick::length_value(*n.length).has_value(),
              "native valid branch length");
    });
    return r;
  };
  const char* const cases[] = {
      R"nwk((,,(,));
)nwk",
      R"nwk((O'Brien,'x''y');
)nwk",
      R"nwk(('',A)'';
)nwk",
      R"nwk(();
)nwk",
      R"nwk((A[discard]B,C);
)nwk",
      R"nwk((A:1[x].2e[foo]+3,B:2);
)nwk",
      R"nwk((:1.5 A,:2 B):3 root;
)nwk",
      R"nwk((A[a[b]c]B,C);
)nwk",
      R"nwk((A:Inf,B:-Infinity,C:NaN);
)nwk",
      R"nwk((A:+1.,B:+.5e+2);
)nwk",
      R"nwk((A:'1.5',B:'1_000');
)nwk",
      R"nwk((A:1e1000,B:1e-1000);
)nwk",
      R"nwk([&R](A[&rate=1]:.1,B)[root];
)nwk",
      R"nwk(('[]_''':.1,名字:+2.5e+5);
)nwk",
  };
  for (const char* source : cases) {
    const auto r = parse(source);
    check(r.value_as<ast::Clade>(), "single tree root");
  }
  {
    const auto r = parse("(A[a[b]c]B:1[x].2e[y]+3,'x''y':+2)root;");
    const auto* n = r.value_as<ast::Clade>();
    check(n && n->children.size() == 2, "two clades");
    if (n && n->children.size() == 2)
      check(n->hasName && n->name == "root" && n->children[0]->name == "AB" &&
                n->children[0]->length == "1.2e+3" &&
                n->children[1]->name == "x'y" && n->children[1]->length == "+2",
            "comments inside tokens and exact values");
  }
  {
    const auto r = parse("('',)'';");
    const auto* n = r.value_as<ast::Clade>();
    check(n && n->hasName && n->name.empty() && n->children.size() == 2,
          "explicit empty label");
    if (n && n->children.size() == 2)
      check(n->children[0]->hasName && n->children[0]->name.empty() &&
                !n->children[1]->hasName,
            "empty and absent labels differ");
  }
  {
    const auto r = parse("(A:1e1000,B:1e-1000,C:NaN,D:'1_000');");
    const auto* n = r.value_as<ast::Clade>();
    check(n && n->children.size() == 4, "four lengths");
    if (n && n->children.size() == 4)
      check(n->children[0]->length == "1e1000" &&
                std::isinf(*newick::length_value(*n->children[0]->length)) &&
                *newick::length_value(*n->children[1]->length) == 0 &&
                std::isnan(*newick::length_value(*n->children[2]->length)) &&
                *newick::length_value(*n->children[3]->length) == 1000,
            "native numeric conversion and raw preservation");
  }
  const std::string invalid =
      "(A:abc,B:1e+,C:'',D:'nan(payload)',E:0x10,F:1__0);";
  cursiv::Document doc(
      cursiv::TextDocument::create("memory:newick", "newick", 0, invalid));
  doc.parseResult = language.language.parser->parse(invalid);
  check(doc.parseResult.accepted && !doc.parseResult.cst().recovered() &&
            doc.parseResult.diagnostics.empty(),
        "invalid lengths parse clean");
  unsigned errors = 0;
  auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
  cursiv::ValidationAcceptor accept(sink, doc, invalid);
  visit(doc.parseResult.value_as<ast::Clade>(), [&](const auto& n) {
    language.language.validator->checkClade(n, accept);
  });
  check(errors == 6, "precise runtime branch-length errors");
  return ok;
}
}  // namespace code_audit
