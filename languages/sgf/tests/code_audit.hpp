#pragma once

#include <cstdio>
#include <functional>
#include <string>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "sgf/core/CoreServices.hpp"

namespace code_audit {
namespace ast = sgf::ast;
inline bool run() {
  auto language =
      cursiv::create_language<sgf::SgfCoreServices, sgf::SgfSharedCoreServices,
                              sgf::install_core_module>();
  bool ok = true;
  auto check = [&](bool b, const char* message) {
    if (!b) {
      std::fprintf(stderr, "sgf code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    const auto* root = r.value_as<ast::Collection>();
    check(root != nullptr, "collection root");
    std::function<void(const ast::GameTree*)> walk = [&](const auto* t) {
      for (const auto* n : t->nodes) {
        check(n->parent == t, "node parent");
        for (const auto* p : n->properties)
          check(p->parent == n, "property parent");
      }
      for (const auto* c : t->children) {
        check(c->parent == t, "variation parent");
        walk(c);
      }
    };
    if (root)
      for (const auto* t : root->trees) {
        check(t->parent == root, "game parent");
        walk(t);
      }
    return r;
  };
  {
    const auto r = parse("(;B[]KO[]DD[]FG[]TB[]TW[]XX[][x][];W[])");
    const auto* root = r.value_as<ast::Collection>();
    check(root && root->trees.size() == 1 && root->trees[0]->nodes.size() == 2,
          "empty values keep game sequence");
    if (root && root->trees.size() == 1 && root->trees[0]->nodes.size() == 2) {
      const auto& ps = root->trees[0]->nodes[0]->properties;
      check(ps.size() == 7, "all empty-valued properties remain present");
      if (ps.size() == 7) {
        for (unsigned i = 0; i < 6; ++i)
          check(ps[i]->values.size() == 1 && ps[i]->values[0].empty(),
                "empty brackets are one empty value");
        check(ps[6]->values.size() == 3 && ps[6]->values[0].empty() &&
                  ps[6]->values[1] == "x" && ps[6]->values[2].empty(),
              "unknown properties preserve every value");
      }
    }
  }
  for (unsigned count = 1; count <= 8; ++count) {
    const std::string value =
        "a" + std::string(count, '\\') + (count % 2 ? "]b" : "");
    const std::string source = "(;C[" + value + "]N[next])";
    const auto r = parse(source);
    const auto* root = r.value_as<ast::Collection>();
    if (root && !root->trees.empty() && !root->trees[0]->nodes.empty()) {
      const auto& ps = root->trees[0]->nodes[0]->properties;
      check(ps.size() == 2 && ps[0]->values.size() == 1 &&
                ps[0]->values[0] == value && ps[1]->id == "N" &&
                ps[1]->values.size() == 1 && ps[1]->values[0] == "next",
            "escaped delimiters respect backslash parity");
    }
  }
  {
    const auto r = parse("(;FileFormat[4]AddBlack[aa][bb]Comment[legacy])");
    const auto* root = r.value_as<ast::Collection>();
    if (root && !root->trees.empty() && !root->trees[0]->nodes.empty()) {
      const auto& ps = root->trees[0]->nodes[0]->properties;
      check(ps.size() == 3 && ps[0]->id == "FF" && ps[1]->id == "AB" &&
                ps[1]->values.size() == 2 && ps[2]->id == "C",
            "legacy long identifiers retain uppercase letters");
    }
  }
  {
    const auto r = parse(
        "(;GM[1];B[aa](;W[bb];B[])(;W[cc](;B[dd])(;B[ee])))"
        "(;GM[2]SZ[8];B[a1])");
    const auto* root = r.value_as<ast::Collection>();
    check(root && root->trees.size() == 2 &&
              root->trees[0]->nodes.size() == 2 &&
              root->trees[0]->children.size() == 2 &&
              root->trees[0]->children[0]->nodes.size() == 2 &&
              root->trees[0]->children[1]->children.size() == 2 &&
              root->trees[1]->nodes.size() == 2,
          "collections and nested variations preserve structure");
  }
  {
    const std::string bytes = std::string("caf\xe9\0", 5) + "\\\r\ntext\t";
    const std::string source = "(\f;\vCA[ISO-8859-1]C[" + bytes + "])";
    const auto r = parse(source);
    const auto* root = r.value_as<ast::Collection>();
    if (root && !root->trees.empty() && !root->trees[0]->nodes.empty()) {
      const auto& ps = root->trees[0]->nodes[0]->properties;
      check(ps.size() == 2 && ps[1]->values.size() == 1 &&
                ps[1]->values[0] == bytes,
            "values preserve encoding, escapes, line endings and NUL");
    }
  }
  {
    const std::string id(80, 'X');
    const std::string source = "(;" + id + "[v])";
    const auto r = parse(source);
    const auto* root = r.value_as<ast::Collection>();
    check(root && !root->trees.empty() && !root->trees[0]->nodes.empty() &&
              root->trees[0]->nodes[0]->properties.size() == 1 &&
              root->trees[0]->nodes[0]->properties[0]->id == id,
          "identifiers have no arbitrary length limit");
  }
  {
    const std::string source = "(;FF[4][3]KO[x]XX XX[v]lower[v])";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:sgf", "sgf", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    const auto* root = doc.parseResult.value_as<ast::Collection>();
    if (root && !root->trees.empty() && !root->trees[0]->nodes.empty()) {
      const auto* n = root->trees[0]->nodes[0];
      language.language.validator->checkNode(*n, accept);
      for (const auto* p : n->properties)
        language.language.validator->checkProperty(*p, accept);
    }
    check(errors == 5,
          "cardinality, missing values and duplicates validate at runtime");
  }
  return ok;
}
}  // namespace code_audit
