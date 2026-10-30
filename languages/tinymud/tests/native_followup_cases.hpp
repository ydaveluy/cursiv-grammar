#pragma once
#include <cstdio>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "tinymud/core/ast.hpp"
namespace tinymud_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = tinymud::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "tinymud native: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"mud(
@DESC #42=9   lives! {a}; b=c?
@desc me=
@lock #42=(me|#7)&!#8
@set me=!dark
@set me=!!d
@force Bob=say Hi, #42! x=y;
@link exit=#123
@open n;north=home
@pcreate Alice=Case!42
@stats
look
kill Bob
page Bob
@newpassword Bob=
WHO
QUIT
)mud";
  const auto v = cursiv::test::validationHelper(language.language, std::string(source));
  const auto& r = v.document->parseResult;
  const auto* root = r.template value_as<a::Session>();
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && v.diagnostics.empty() && root && root->lines.size() == 16,
        "native punctuation/refs/flags/defaults and blank lines parse and validate clean");
  if (!r.diagnostics.empty()) for (const auto& d : r.diagnostics)
    std::fprintf(stderr, "tinymud kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
  bool description = false, clearing = false, lock = false, force = false, reference = false, open = false;
  bool player = false, stats = false, look = false, kill = false, page = false, password = false;
  unsigned flags = 0;
  for (const auto* node : r.nodes) {
    if (const auto* p = cursiv::ast_cast<a::DescribeCommand>(node)) {
      description |= p->object == "#42" && p->description == "9 lives! {a}; b=c?";
      clearing |= p->object == "me" && p->description.empty();
    }
    if (const auto* p = cursiv::ast_cast<a::LockCommand>(node)) lock |= p->key == "(me|#7)&!#8";
    if (const auto* p = cursiv::ast_cast<a::SetCommand>(node)) flags += p->negate && (p->flagName == "dark" || p->flagName == "d");
    if (const auto* p = cursiv::ast_cast<a::ForceCommand>(node))
      if (const auto* action = cursiv::ast_cast<a::SayAction>(p->command))
        force |= action->message == "Hi, #42! x=y;" && action->parent == p;
    if (const auto* p = cursiv::ast_cast<a::LinkCommand>(node))
      if (const auto* target = std::get_if<std::int64_t>(&p->target)) reference |= *target == 123;
    if (const auto* p = cursiv::ast_cast<a::OpenCommand>(node))
      open |= p->directions.size() == 2 && p->directions[0] == "n" && p->directions[1] == "north" && p->destination == "home" && !p->number;
    if (const auto* p = cursiv::ast_cast<a::PcreateCommand>(node)) player |= p->name == "Alice" && p->password == "Case!42";
    if (const auto* p = cursiv::ast_cast<a::StatsCommand>(node)) stats |= p->player.empty();
    if (const auto* p = cursiv::ast_cast<a::LookAction>(node)) look |= p->object.empty();
    if (const auto* p = cursiv::ast_cast<a::KillAction>(node)) kill |= p->player == "Bob" && !p->hasCost;
    if (const auto* p = cursiv::ast_cast<a::PageAction>(node)) page |= p->player == "Bob" && p->message.empty();
    if (const auto* p = cursiv::ast_cast<a::NewPasswordCommand>(node)) password |= p->password.empty();
  }
  check(description && clearing && lock && flags == 2 && force && reference && open && player && stats && look && kill && page && password,
        "native values, normalized text, variants and forced-action ownership retained");
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
    check(owned && seen.size() == r.nodes.size() && r.cst().text() == source, "complete AST ownership and unchanged source CST");
  }
  const auto renamed = language.language.parser->parse("@name me=Bob secret\n@open w;west=#44\n@toad Bob=Little frog\n");
  bool rhs = false, number = false, toad = false;
  for (const auto* n : renamed.nodes) {
    if (const auto* p = cursiv::ast_cast<a::NameCommand>(n)) rhs |= p->name == "Bob secret" && p->password.empty();
    if (const auto* p = cursiv::ast_cast<a::OpenCommand>(n)) number |= p->number == 44;
    if (const auto* p = cursiv::ast_cast<a::ToadCommand>(n)) toad |= p->name == "Little frog";
  }
  check(renamed.accepted && !renamed.cst().recovered() && renamed.diagnostics.empty() && rhs && number && toad,
        "runtime-dependent rename RHS, numeric exit destination and toad name preserved");
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{"@set me=3\n", "tinymud.flag"}, Invalid{"@set me=not_a_flag\n", "tinymud.flag"},
      Invalid{"@pcreate Alice\n", "tinymud.password"}, Invalid{"@pcreate Alice=two words\n", "tinymud.password"},
      Invalid{"@pcreate Two Words=secret\n", "tinymud.player-name"}}) {
    const auto invalid = cursiv::test::validationHelper(language.language, c.source);
    bool code = false;
    for (const auto& d : invalid.diagnostics) code |= d.code == c.code;
    check(invalid.document->parseResult.accepted && !invalid.document->parseResult.cst().recovered() && code, c.code);
  }
  for (const auto bad : {"@desc#42=x\n", "look#42\n", "@link n=#9223372036854775808\n", "say incomplete"}) {
    const auto result = language.language.parser->parse(bad);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(), bad);
  }
  return ok;
}
} // namespace tinymud_native_followup
