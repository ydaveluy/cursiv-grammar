#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "tinyosnesc/core/ast.hpp"
namespace tinyosnesc_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = tinyosnesc::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "nesC native: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"nc(generic module M(typedef T) @safe() @custom(1) {
  provides command int value();
  uses { async event void done(); interface I as I0; }
} implementation {
  task void run() {
    char *s="a\"{b";
    char c='}';
    if ((data.field >> 1) & ~1) call I0.run(s);
  }
})nc";
  const auto r = language.language.parser->parse(source);
  const auto* root = r.template value_as<a::Document>();
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && root, "native syntax shell/literals/conditions parse clean");
  if (!r.diagnostics.empty()) for (const auto& d : r.diagnostics)
    std::fprintf(stderr, "nesC kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  const auto* module = root && root->components.size() == 1 ? cursiv::ast_cast<a::Module>(root->components[0]) : nullptr;
  check(module && module->hasParameters && module->isGeneric && module->parameters.find("T") != std::string_view::npos &&
        module->attribute.find("@safe()") != std::string_view::npos && module->attribute.find("@custom(1)") != std::string_view::npos,
        "parameters precede complete attribute sequence");
  bool bare = false, async = false, character = false, string = false, shift = false, bitwise = false, negate = false, member = false;
  for (const auto* node : r.nodes) {
    if (const auto* p = cursiv::ast_cast<a::UsesProvides>(node)) bare |= p->isProvides && p->members.size() == 1 && p->interfaces.empty();
    if (const auto* p = cursiv::ast_cast<a::InterfaceMember>(node)) async |= p->isAsync && p->isEvent;
    if (const auto* p = cursiv::ast_cast<a::FuncStat>(node)) {
      character |= p->signature.find("'}'") != std::string_view::npos;
      string |= p->signature.find(R"nc("a\"{b")nc") != std::string_view::npos;
    }
    if (const auto* p = cursiv::ast_cast<a::BinaryExpr>(node)) { shift |= p->op == ">>"; bitwise |= p->op == "&"; }
    if (const auto* p = cursiv::ast_cast<a::UnaryExpr>(node)) negate |= p->op == "~";
    if (const auto* p = cursiv::ast_cast<a::AtomExpr>(node)) member |= p->text == "data.field";
  }
  check(bare && async && character && string && shift && bitwise && negate && member, "native captures/literal boundaries and precedence retained");
  if (root && !r.cst().recovered()) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *root);
    check(owned && seen.size() == r.nodes.size(), "complete AST ownership");
  }
  const auto q = language.language.parser->parse("interface Queue<t> { async command void push(t x); async event t done(); }");
  const auto* d = q.template value_as<a::Document>();
  const auto* i = d && d->components.size() == 1 ? cursiv::ast_cast<a::InterfaceDecl>(d->components[0]) : nullptr;
  check(q.accepted && !q.cst().recovered() && q.diagnostics.empty() && i && i->typeParameters.size() == 1 &&
        i->typeParameters[0] == "t" && i->members.size() == 2 && i->members[0]->isAsync && i->members[1]->isAsync && i->members[1]->isEvent,
        "manual generic interface and async declarations retained");
  for (const auto bad : {"module M {uses interface;} implementation{}", "generic module M @safe()(typedef T){} implementation{}", "module M @bare {} implementation{}"}) {
    const auto p = language.language.parser->parse(bad);
    check(!p.accepted || p.cst().recovered() || !p.diagnostics.empty(), bad);
  }
  for (const auto source : {"generic module M{} implementation{}", "module M(int n){} implementation{}",
                            "generic configuration C{} implementation{}", "configuration C(int n){} implementation{}"}) {
    const auto v = cursiv::test::validationHelper(language.language, source);
    bool code = false;
    for (const auto& d : v.diagnostics) code |= d.code == "tinyosnesc.generic-parameters";
    check(v.document->parseResult.accepted && !v.document->parseResult.cst().recovered() && code, "generic parameter relation diagnostic");
  }
  const auto multiple = cursiv::test::validationHelper(language.language, "module A{} implementation{} module B{} implementation{}");
  bool definition = false;
  for (const auto& d : multiple.diagnostics) definition |= d.code == "tinyosnesc.source-definition";
  check(definition, "one native source definition per file");
  return ok;
}
} // namespace tinyosnesc_native_followup
