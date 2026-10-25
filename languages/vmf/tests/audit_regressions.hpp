#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "vmf/core/CoreServices.hpp"
#include "vmf/core/ast.hpp"
#include <string>
#include <vector>

namespace audit_regressions {
inline bool run(const vmf::VmfCoreServices& services,
                const vmf::VmfSharedCoreServices& shared) {
  struct Entry { const char* key; const char* value; std::size_t children; };
  struct Case { const char* source; std::vector<Entry> entries; bool invalidKey; };
  const Case cases[] = {
    {"world\012{\012\"classname\" \"worldspawn\"\012}\012",{{"world",nullptr,1},{"classname","worldspawn",0}},false},
    {"// first\012world {\012\"classname\" \"worldspawn\" // last\012}\012",{{"world",nullptr,1},{"classname","worldspawn",0}},false},
    {"world {\012material materials/dev/dev_measuregeneric01b\012path C:/maps/test.vmf\012}\012",{{"world",nullptr,2},{"material","materials/dev/dev_measuregeneric01b",0},{"path","C:/maps/test.vmf",0}},false},
    {"world {\012\"quoted\\\"key\" \"say \\\"hello\\\"\"\012}\012",{{"world",nullptr,1},{"quoted\"key","say \"hello\"",0}},false},
    {"world {\012\"message\" \"a\\nb\\tend\"\012}\012",{{"world",nullptr,1},{"message","a\012b\011end",0}},false},
    {"world {\012\"path\" \"C:\\maps\\unknown\\z\"\012}\012",{{"world",nullptr,1},{"path","C:\\maps\\unknown\\z",0}},false},
    {"world {\012\"one\" \"\"\012\"empty\" {}\012\"one\" \"again\"\012}\012",{{"world",nullptr,3},{"one","",0},{"empty",nullptr,0},{"one","again",0}},false},
    {"世界 {\012\"标题\" \"échec 世界\"\012}\012",{{"世界",nullptr,1},{"标题","échec 世界",0}},false},
    {"﻿versioninfo\012{\012\"editorversion\" \"400\"\012}\012",{{"versioninfo",nullptr,1},{"editorversion","400",0}},false},
    {"world {\012message \"first\012second\"\012}\012",{{"world",nullptr,1},{"message","first\012second",0}},false},
    {"world {\012message \"first\\\012second\"\012}\012",{{"world",nullptr,1},{"message","firstsecond",0}},false},
    {"",{},false},
    {"world {\012\"bell\" \"\\a\\b\\f\\r\\v\\?\\/\\'\"\012}\012",{{"world",nullptr,1},{"bell","\007\010\014\015\013?/'",0}},false},
    {"world {\012\"bad\012key\" \"value\"\012}\012",{{"world",nullptr,1},{"bad\012key","value",0}},true},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<vmf::ast::Document>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->parent) return false;
    if (!root->entries.empty()) {
      if (root->entries[0]->keyQuoted || root->entries[0]->valueQuoted) return false;
      if (!root->entries[0]->children.empty()) {
        const auto* first = root->entries[0]->children[0];
        if (!first->valueQuoted && first->value && *first->value != "materials/dev/dev_measuregeneric01b") return false;
      }
    }
    std::size_t index = 0;
    bool valid = true;
    const auto visit = [&](auto&& self, const vmf::ast::KeyValue& node,
                           const cursiv::AstNode& parent) -> void {
      if (index >= test.entries.size()) { valid = false; return; }
      const auto& expected = test.entries[index++];
      valid &= node.parent == &parent && result.cst().contains(parent.cstNodeId, node.cstNodeId) &&
          node.key == expected.key && node.children.size() == expected.children &&
          (expected.value ? node.value && *node.value == expected.value : !node.value);
      for (const auto* child : node.children) self(self, *child, node);
    };
    for (const auto* entry : root->entries) visit(visit, *entry, *root);
    if (!valid || index != test.entries.size() || result.nodes.size() != index + 1) return false;
    auto document = shared.documentFactory->fromString(source, "file:///audit.vmf");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool invalidKey = false;
    for (const auto& d : diagnostics)
      invalidKey |= d.code && d.code->isString() && d.code->string() == "vmf.key-newline";
    if (invalidKey != test.invalidKey || (!test.invalidKey && !diagnostics.empty())) return false;
  }
  {
    const std::string source = "unquoted_value bare-token(ok)_$@<>[]/end\n";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<vmf::ast::Document>(result.value);
    if (!result.accepted || result.cst().recovered() || !root || root->entries.size() != 1 ||
        root->entries[0]->keyQuoted || root->entries[0]->valueQuoted ||
        root->entries[0]->key != "unquoted_value" ||
        !root->entries[0]->value || *root->entries[0]->value != "bare-token(ok)_$@<>[]/end") return false;
    auto document = shared.documentFactory->fromString(source, "file:///audit.vmf");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& d : diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == "vmf.unquoted-token";
    if (!found) return false;
  }
  return true;
}
}  // namespace audit_regressions
