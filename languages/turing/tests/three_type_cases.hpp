#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "turing/core/ast.hpp"

namespace turing_three_types {
struct Case { const char* id; std::string_view source; bool valid; };
inline constexpr Case cases[] = {
    {"enum", "type Color : enum(red, green, blue)", true},
    {"enum-singleton", "type Bit : enum(zero)", true},
    {"enum-variable", "var c : enum(red, green)", true},
    {"enum-array-index", "var a : array enum(left,right) of int", true},
    {"set-range", "type Small : set of 0..2", true},
    {"set-named", "var s : set of Color", true},
    {"set-enum", "type Flags : set of enum(off,on)", true},
    {"composed-array", "var a : array enum(left,right), 1..2 of set of Color", true},
    {"pointer", "type Link : pointer to list", true},
    {"pointer-record", "type Node : record next : pointer to list end record", true},
    {"qualified-index", "type Colors : set of M.Color", true},
    {"parameter-types", "proc visit(p : pointer to list, s : set of Color) end visit", true},
    {"result-type", "fcn head : pointer to list result nil(list) end head", true},
    {"enum-empty", "type E : enum()", false},
    {"enum-trailing-comma", "type E : enum(a,)", false},
    {"enum-missing-comma", "type E : enum(a b)", false},
    {"enum-keyword", "type E : enum(int)", false},
    {"enum-duplicate", "type E : enum(red, red)", false},
    {"set-missing-of", "type S : set Color", false},
    {"set-missing-base", "type S : set of", false},
    {"set-record", "type S : set of record x : int end record", false},
    {"pointer-caret-alias", "type P : ^List", false},
    {"pointer-array", "type P : pointer to array 1..2 of int", false},
    {"pointer-missing-to", "type P : pointer List", false},
    {"pointer-qualified-collection", "type P : pointer to M.List", false},
};

inline bool run(const auto& services) {
  namespace ast = turing::ast;
  for (const auto& test : cases) {
    const auto fail = [&](const char* reason) {
      std::fprintf(stderr, "turing types: %s: %s; source=%.*s\n",
                   test.id, reason, int(test.source.size()), test.source.data());
      return false;
    };
    const auto v = cursiv::test::validationHelper(services, std::string(test.source));
    const auto& r = v.document->parseResult;
    const bool syntax = r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    const bool clean = syntax && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "turing types: %s expected=%d accepted=%d recovered=%d parseDiagnostics=%zu validationDiagnostics=%zu\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(), r.diagnostics.size(), v.diagnostics.size());
      for (const auto& d : v.diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return fail("formal source/validation expectation");
    }
    if (!syntax) continue;
    const auto* root = r.template value_as<ast::Program>();
    if (!root || root->parent != v.document.get() || root->items.size() != 1)
      return fail("Program/Document/one item");
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto describe = [&](const char* role, const cursiv::AstNode& node) {
      const auto type = services.astReflection->getTypeMetaData(node.type).name;
      std::fprintf(stderr, "turing types: %s %s type=%.*s cst=%u",
                   test.id, role, int(type.size()), type.data(), unsigned(node.cstNodeId));
      if (node.cstNodeId < r.cst().size()) {
        const auto& cst = r.cst()[node.cstNodeId];
        const auto text = r.cst().text(node.cstNodeId);
        std::fprintf(stderr, " bytes=%u..%u text=%.*s", cst.begin, cst.end,
                     int(text.size() > 140 ? 140 : text.size()), text.data());
      }
      std::fputc('\n', stderr);
    };
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; describe("duplicate", node); return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contains = node.cstNodeId < r.cst().size() && child.cstNodeId < r.cst().size() &&
                r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contains) {
              std::fprintf(stderr, "turing types: %s member=%.*s index=%u parent=%d contains=%d\n",
                           test.id, int(member.size()), member.data(), index, parent, contains);
              describe("owner", node); describe("child", child);
            }
            owned &= parent && contains;
            self(self, child);
          });
    };
    walk(walk, *root);
    for (const auto* node : r.nodes)
      if (!seen.contains(node)) describe("unreachable", *node);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "turing types: %s ownership=%d visited=%zu arena=%zu\n",
                   test.id, owned, seen.size(), r.nodes.size());
      return fail("complete parent/CST/arena ownership");
    }
    const auto* declaration = cursiv::ast_cast<ast::TypeDecl>(root->items[0]);
    const auto* variable = cursiv::ast_cast<ast::VarDecl>(root->items[0]);
    const auto* subprogram = cursiv::ast_cast<ast::SubprogramDecl>(root->items[0]);
    const std::string_view id = test.id;
    if (id == "enum-duplicate") {
      const auto* enumeration = declaration ? cursiv::ast_cast<ast::EnumType>(declaration->typeSpec) : nullptr;
      const auto offset = test.source.rfind("red");
      if (!enumeration || enumeration->names.size() != 2 || enumeration->names[0] != "red" ||
          enumeration->names[1] != "red" || v.diagnostics.size() != 1 ||
          !v.diagnostics[0].code || !v.diagnostics[0].code->isString() ||
          v.diagnostics[0].code->string() != "turing.enum-duplicate" ||
          v.diagnostics[0].range.begin != offset || v.diagnostics[0].range.end != offset + 3)
        return fail("duplicate enum retains spelling and diagnoses second identifier precisely");
      continue;
    }
    if (!clean) return fail("unexpected clean-syntax negative");
    const auto* type = declaration ? declaration->typeSpec : variable ? variable->typeSpec :
        subprogram ? subprogram->returnType : nullptr;
    if (id == "parameter-types") {
      if (!subprogram || subprogram->isFunction || subprogram->name != "visit" ||
          subprogram->endName != "visit" || subprogram->params.size() != 2) return fail("procedure parameters");
      const auto* first = cursiv::ast_cast<ast::SimpleParamType>(subprogram->params[0]->type);
      const auto* second = cursiv::ast_cast<ast::SimpleParamType>(subprogram->params[1]->type);
      const auto* pointer = first ? cursiv::ast_cast<ast::PointerType>(first->type) : nullptr;
      const auto* set = second ? cursiv::ast_cast<ast::SetType>(second->type) : nullptr;
      const auto* named = set ? cursiv::ast_cast<ast::NamedType>(set->base) : nullptr;
      if (!pointer || pointer->collection != "list" || !named || named->name != "Color")
        return fail("parameter TypeSpec variants");
    } else if (id == "enum" || id == "enum-singleton" || id == "enum-variable") {
      const auto* enumeration = cursiv::ast_cast<ast::EnumType>(type);
      const std::size_t expected = id == "enum" ? 3 : id == "enum-singleton" ? 1 : 2;
      if (!enumeration || enumeration->names.size() != expected ||
          enumeration->names[0] != (id == "enum-singleton" ? "zero" : "red") ||
          (expected > 1 && enumeration->names[1] != "green") ||
          (expected > 2 && enumeration->names[2] != "blue")) return fail("ordered enum values");
    } else if (id == "set-range" || id == "set-named" || id == "set-enum" || id == "qualified-index") {
      const auto* set = cursiv::ast_cast<ast::SetType>(type);
      if (!set) return fail("set TypeSpec");
      if (id == "set-range") {
        const auto* range = cursiv::ast_cast<ast::SubrangeType>(set->base);
        const auto* low = range ? cursiv::ast_cast<ast::IntLit>(range->low) : nullptr;
        const auto* high = range ? cursiv::ast_cast<ast::IntLit>(range->high) : nullptr;
        if (!low || low->value != 0 || !high || high->value != 2) return fail("set range operands");
      } else if (id == "set-enum") {
        const auto* enumeration = cursiv::ast_cast<ast::EnumType>(set->base);
        if (!enumeration || enumeration->names.size() != 2 || enumeration->names[0] != "off" ||
            enumeration->names[1] != "on") return fail("inline set enum");
      } else {
        const auto* named = cursiv::ast_cast<ast::NamedType>(set->base);
        if (!named || named->name != (id == "qualified-index" ? "M.Color" : "Color"))
          return fail("named index type source spelling");
      }
    } else if (id == "enum-array-index" || id == "composed-array") {
      const auto* array = cursiv::ast_cast<ast::ArrayType>(type);
      const auto* enumeration = array && !array->indices.empty()
          ? cursiv::ast_cast<ast::EnumType>(array->indices[0]) : nullptr;
      if (!array || array->indices.size() != (id == "composed-array" ? 2u : 1u) ||
          !enumeration || enumeration->names.size() != 2 || enumeration->names[0] != "left" ||
          enumeration->names[1] != "right") return fail("array shares enum index type");
      if (id == "composed-array") {
        const auto* set = cursiv::ast_cast<ast::SetType>(array->element);
        const auto* named = set ? cursiv::ast_cast<ast::NamedType>(set->base) : nullptr;
        if (!cursiv::ast_cast<ast::SubrangeType>(array->indices[1]) || !named || named->name != "Color")
          return fail("composed array/set index types");
      }
    } else if (id == "pointer-record") {
      const auto* record = cursiv::ast_cast<ast::RecordType>(type);
      const auto* pointer = record && record->fields.size() == 1
          ? cursiv::ast_cast<ast::PointerType>(record->fields[0]->type) : nullptr;
      if (!pointer || pointer->collection != "list" || record->fields[0]->names.size() != 1 ||
          record->fields[0]->names[0] != "next") return fail("record pointer field");
    } else {
      const auto* pointer = cursiv::ast_cast<ast::PointerType>(type);
      if (!pointer || pointer->collection != "list") return fail("pointer collection identifier");
      if (id == "result-type" && (!subprogram || !subprogram->isFunction || subprogram->body.size() != 1 ||
                                   !cursiv::ast_cast<ast::ResultStmt>(subprogram->body[0])))
        return fail("function pointer result");
    }
  }
  std::fputs("turing types:25 primary-derived controls, structured values and ownership\n", stdout);
  return true;
}
}  // namespace turing_three_types
