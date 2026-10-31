#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "teal/core/CoreServices.hpp"
#include "teal/core/ast.hpp"

namespace teal_interfaces {
inline bool run() {
  namespace a = teal::ast;
  const auto services = cursiv::create_language<teal::TealCoreServices,
      teal::TealSharedCoreServices, teal::install_core_module>();
  const auto& language = services.language;
  bool ok = true;
  auto check = [&](bool value, const char* reason, std::string_view name) {
    if (!value) {
      std::fprintf(stderr, "teal interface: %s; case=%.*s\n", reason,
                   int(name.size()), name.data());
      ok = false;
    }
  };
  auto base = [](a::Type* type) -> a::Type* {
    const auto* u = cursiv::ast_cast<a::UnionType>(type);
    return u && u->alternatives.size() == 1 ? u->alternatives[0] : type;
  };
  struct Case { const char* name; const char* source; bool clean, diagnostic;
                unsigned interfaces, records; };
  const Case cases[] = {
      {"local", "local interface I\n x: number\nend\n", true, false, 1, 0},
      {"global", "global interface I\n x: number\nend\n", true, false, 1, 0},
      {"empty", "local interface I end", true, false, 1, 0},
      {"generic", "local interface I<T> value: T end", true, false, 1, 0},
      {"array", "local interface I {number} end", true, false, 1, 0},
      {"nested-record-parent", "local record R interface I x:number end end", true, false, 1, 1},
      {"nested-interface-parent", "local interface I interface J x:number end end", true, false, 2, 0},
      {"nested-enum-alias", "local interface I enum E \"a\" \"b\" end type N = number field:N end", true, false, 1, 0},
      {"context-local", "local interface=1", true, false, 0, 0},
      {"context-field", "local record R interface:number end", true, false, 0, 1},
      {"context-global", "global interface:number", true, false, 0, 0},
      {"mixed", "local record R x:number end\nlocal interface I x:number end", true, false, 1, 1},
      {"missing-name", "local interface end", false, false, 0, 0},
      {"missing-end", "local interface I x:number", false, false, 0, 0},
      {"invalid-name", "global interface 1 end", false, false, 0, 0},
      {"missing-field-type", "local interface I x: end", false, false, 0, 0},
      {"nested-record-forbidden", "local interface I record R x:number end end", true, true, 1, 1},
      {"nested-interface-record-forbidden", "local record R interface I record Bad x:number end end end", true, true, 1, 2},
      {"field-initializer", "local interface I x:number=1 end", false, false, 0, 0},
      {"unclosed-generic", "local interface I<T end", false, false, 0, 0},
  };
  for (const auto& c : cases) {
    const std::string text(c.source);
    const auto result = language.parser->parse(text);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    check(clean == c.clean, "native syntax boundary", c.name);
    if (!c.clean || !clean) continue;
    unsigned interfaces = 0, records = 0;
    const a::RecordNewType* firstInterface = nullptr;
    for (const auto* node : result.nodes) {
      if (const auto* body = cursiv::ast_cast<a::RecordNewType>(node)) {
        if (body->isInterface) { ++interfaces; if (!firstInterface) firstInterface = body; }
        else ++records;
        check(!body->legacy, "named bodies keep the modern syntax marker", c.name);
      }
    }
    check(interfaces == c.interfaces && records == c.records,
          "record/interface body markers", c.name);
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = result.value && !result.value->parent;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (result.value) walk(walk, *result.value);
    check(owned && seen.size() == result.nodes.size(),
          "complete unique AST/CST ownership", c.name);
    const auto validation = cursiv::test::validationHelper(language, text);
    if (c.diagnostic) {
      check(validation.diagnostics.size() == 1 && validation.diagnostics[0].code &&
            validation.diagnostics[0].code->isString() &&
            validation.diagnostics[0].code->string() == "teal.interface-record",
            "native nested-record restriction is a local diagnostic", c.name);
      continue;
    }
    check(validation.diagnostics.empty(), "valid interface has no validation errors", c.name);
    const auto* block = cursiv::ast_cast<a::Block>(result.value);
    check(block && !block->statements.empty(), "top-level declaration retained", c.name);
    if (!block || block->statements.empty()) continue;
    const std::string_view name(c.name);
    if (name == "local" || name == "empty" || name == "generic" || name == "array") {
      const auto* decl = cursiv::ast_cast<a::LocalNewTypeStat>(block->statements[0]);
      check(decl && decl->name == "I" && decl->value == firstInterface,
            "local name and interface value retained", c.name);
    }
    if (name == "global") {
      const auto* decl = cursiv::ast_cast<a::GlobalAssignStat>(block->statements[0]);
      check(decl && decl->name == "I" && decl->value == firstInterface,
            "global name and interface value retained", c.name);
    }
    if (name == "local" || name == "global") {
      const auto* field = firstInterface && firstInterface->fields.size() == 1
          ? firstInterface->fields[0] : nullptr;
      const auto* type = field ? cursiv::ast_cast<a::PrimitiveType>(base(field->type)) : nullptr;
      check(field && field->name == "x" && type && type->kind == "number",
            "shared record fields preserve names and types", c.name);
    }
    if (name == "generic") {
      const auto* field = firstInterface && firstInterface->fields.size() == 1
          ? firstInterface->fields[0] : nullptr;
      const auto* type = field ? cursiv::ast_cast<a::NamedType>(base(field->type)) : nullptr;
      check(firstInterface && firstInterface->typeParams &&
            firstInterface->typeParams->names.size() == 1 &&
            firstInterface->typeParams->names[0] == "T" && field &&
            field->name == "value" && type && type->name == "T",
            "generic parameters and field reference retained", c.name);
    }
    if (name == "array") {
      const auto* type = firstInterface
          ? cursiv::ast_cast<a::PrimitiveType>(base(firstInterface->arrayPart)) : nullptr;
      check(type && type->kind == "number", "array interface type retained", c.name);
    }
    if (name == "nested-record-parent" || name == "nested-interface-parent") {
      const auto* decl = cursiv::ast_cast<a::LocalNewTypeStat>(block->statements[0]);
      const auto* body = decl ? cursiv::ast_cast<a::RecordNewType>(decl->value) : nullptr;
      const auto* nested = body && body->nestedTypes.size() == 1 ? body->nestedTypes[0] : nullptr;
      const auto* inner = nested ? cursiv::ast_cast<a::RecordNewType>(nested->value) : nullptr;
      check(body && body->isInterface == (name == "nested-interface-parent") && nested &&
            nested->name == (name == "nested-interface-parent" ? "J" : "I") &&
            inner && inner->isInterface && inner->fields.size() == 1 &&
            inner->fields[0]->name == "x", "nested names and bodies retained", c.name);
    }
    if (name == "nested-enum-alias") {
      const auto* body = firstInterface;
      const auto* en = body && body->nestedTypes.size() == 2
          ? cursiv::ast_cast<a::EnumNewType>(body->nestedTypes[0]->value) : nullptr;
      const auto* alias = body && body->nestedTypes.size() == 2
          ? cursiv::ast_cast<a::AliasNewType>(body->nestedTypes[1]->value) : nullptr;
      check(body && body->nestedTypes.size() == 2 && body->nestedTypes[0]->name == "E" &&
            body->nestedTypes[1]->name == "N" && en && en->values.size() == 2 &&
            en->values[0] == "a" && en->values[1] == "b" && alias && alias->type &&
            body->fields.size() == 1 && body->fields[0]->name == "field",
            "nested enum, alias and fields retained in order", c.name);
    }
    if (name == "context-local") {
      const auto* decl = cursiv::ast_cast<a::LocalAttrAssignStat>(block->statements[0]);
      check(decl && decl->vars.size() == 1 && decl->vars[0]->name == "interface",
            "interface remains a contextual variable name", c.name);
    }
    if (name == "context-global") {
      const auto* decl = cursiv::ast_cast<a::GlobalAttrStat>(block->statements[0]);
      check(decl && decl->vars.size() == 1 && decl->vars[0]->name == "interface",
            "interface remains a contextual global name", c.name);
    }
    if (name == "context-field") {
      const auto* decl = cursiv::ast_cast<a::LocalNewTypeStat>(block->statements[0]);
      const auto* body = decl ? cursiv::ast_cast<a::RecordNewType>(decl->value) : nullptr;
      check(body && body->fields.size() == 1 && body->fields[0]->name == "interface",
            "interface remains a contextual field name", c.name);
    }
  }
  std::printf("teal interface: 12 valid, 6 syntax errors, 2 local restrictions; AST/CST ownership\n");
  return ok;
}
} // namespace teal_interfaces
