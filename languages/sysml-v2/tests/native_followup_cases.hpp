#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "sysmlv2/core/CoreServices.hpp"
#include "sysmlv2/core/ast.hpp"

namespace native_followup {
inline bool run(const sysmlv2::SysmlV2CoreServices& services) {
  namespace ast = sysmlv2::ast;
  bool ok = true;
  const auto check = [&](bool valid, const char* message) {
    if (!valid) { std::fprintf(stderr, "sysmlv2 followup: %s\n", message); ok = false; }
  };
  const auto clean = [](const auto& r) { return r.accepted && !r.cst().recovered() && r.diagnostics.empty() && r.value; };
  const auto owns = [&](const auto& r) {
    bool valid = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      services.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        valid &= child.parent == &node; self(self, child);
      });
    };
    if (r.value) walk(walk, *r.value);
    return valid && seen.size() == r.nodes.size();
  };
  const auto member = [](const auto& r, std::string_view name) -> const ast::Element* {
    const auto* root = cursiv::ast_cast<ast::RootNamespace>(r.value);
    if (root) for (const auto* item : root->members) if (item->name == name) return item;
    return nullptr;
  };
  {
    const std::string text = std::string("//* part deliberatelyIgnored; */\n\f") + R"SYS(
part <'short name'> 'Fuel\tTank';
port p : ~P;
attribute initial := 1;
attribute fallback default = 2;
attribute both default := 3;
attribute fixed = 4;
doc <D> 'Documentation' locale "en" /* text */
language "sysml" /* representation */
connect [1] source ::> a to b;
connect (a, b, c);
state def S parallel {}
action def A { loop {} until done; while ready {} until done; }
private filter true;
doc multilineDoc /*
 * first
 * second
 */
)SYS";
    const auto r = services.parser->parse(text);
    const auto* root = cursiv::ast_cast<ast::RootNamespace>(r.value);
    const auto* named = member(r, "Fuel\tTank");
    const auto* p = member(r, "p");
    const auto* initial = member(r, "initial");
    const auto* fallback = member(r, "fallback");
    const auto* both = member(r, "both");
    const auto* fixed = member(r, "fixed");
    const auto* state = member(r, "S");
    const auto* action = member(r, "A");
    const auto* multi = member(r, "multilineDoc");
    check(clean(r) && owns(r) && root && root->members.size() == 14 && r.cst().text() == text &&
          cursiv::test::validationHelper(services, text).diagnostics.empty(), "native structural forms, notes, form-feed, CST and ownership");
    check(named && named->shortName == "short name" && p && p->specs.size() == 1 && p->specs[0]->kind == ":" &&
          p->specs[0]->isConjugated && p->specs[0]->target == "P", "unrestricted name decoding and conjugated typing");
    check(initial && initial->isInitial && !initial->isDefault && fallback && fallback->isDefault && !fallback->isInitial &&
          both && both->isDefault && both->isInitial && fixed && !fixed->isDefault && !fixed->isInitial,
          "feature default and initial distinctions");
    check(multi && multi->text == "first\nsecond\n", "native multiline comment body processing");
    check(root && root->members.size() == 14 && root->members[6]->kind == "doc" && root->members[6]->shortName == "D" && root->members[6]->lang == "en" &&
          root->members[6]->text == "text " && root->members[7]->text == "representation " &&
          root->members[7]->kind == "rep" && root->members[7]->lang == "sysml" &&
          root->members[8]->connectorEnds.size() == 2 && root->members[8]->connectorEnds[0]->multiplicity == "[1]" &&
          root->members[8]->connectorEnds[0]->declaredName == "source" && root->members[8]->connectorEnds[0]->name == "a" &&
          root->members[9]->connectorEnds.size() == 3, "annotation identification, bare representation and native connector ends");
    check(state && state->isParallel && action && action->members.size() == 2 && action->members[0]->kind == "loop" &&
          !action->members[0]->value && action->members[0]->until && action->members[1]->kind == "while" &&
          action->members[1]->value && action->members[1]->until, "parallel state and loop until fields");
  }
  {
    const std::string text = R"SYS(
attribute named = f(x = 1, y = 2);
attribute constructed = new P(x = 3);
attribute mapped = values->map(x = 4);
attribute selected = values->select Predicate;
attribute collected = values.{ true };
attribute sequence = (1, 2,);
attribute indexed = values#(1,);
attribute bracket = values[1,];
attribute implicit = istype P;
attribute spacedReal = 1 . 25e2;
)SYS";
    const auto r = services.parser->parse(text);
    check(clean(r) && owns(r) && cursiv::test::validationHelper(services, text).diagnostics.empty(),
          "named arguments, collect/function-reference, trailing sequences and implicit self classification");
    const auto* named = member(r, "named");
    const auto* call = named ? cursiv::ast_cast<ast::Postfix>(named->value) : nullptr;
    const auto* x = call && call->args.size() == 2 ? cursiv::ast_cast<ast::NamedArgument>(call->args[0]) : nullptr;
    const auto* y = call && call->args.size() == 2 ? cursiv::ast_cast<ast::NamedArgument>(call->args[1]) : nullptr;
    check(call && call->op == "(" && x && x->name == "x" && x->value && y && y->name == "y" && y->value,
          "argument names and values are owned expression nodes");
    const auto* selected = member(r, "selected");
    const auto* ref = selected ? cursiv::ast_cast<ast::Postfix>(selected->value) : nullptr;
    const auto* collected = member(r, "collected");
    const auto* collect = collected ? cursiv::ast_cast<ast::Postfix>(collected->value) : nullptr;
    const auto* body = collect ? cursiv::ast_cast<ast::Body>(collect->body) : nullptr;
    const auto* seq = member(r, "sequence");
    const auto* items = seq ? cursiv::ast_cast<ast::Seq>(seq->value) : nullptr;
    check(ref && ref->op == "->" && ref->name == "select" && ref->functionReference == "Predicate" &&
          collect && collect->op == "." && body && body->result && items && items->items.size() == 2,
          "function-reference versus collect body and sequence values");
    const auto* real = member(r, "spacedReal");
    const auto* literal = real ? cursiv::ast_cast<ast::Literal>(real->value) : nullptr;
    check(literal && literal->text == "1.25e2", "native RealValue data rule joins significant tokens");
  }
  struct Invalid { const char* text; const char* code; };
  for (const auto& c : {Invalid{"attribute x = f(1, y = 2);", "sysml.arguments"},
                        Invalid{"attribute x = values[];", "sysml.sequence"},
                        Invalid{"part p parallel {};", "sysml.parallel-state"},
                        Invalid{"part p :> ~P;", "sysml.conjugated-typing"},
                        Invalid{"attribute x = value istype 1;", "sysml.type-reference"}}) {
    const auto r = services.parser->parse(c.text);
    const auto v = cursiv::test::validationHelper(services, c.text);
    bool found = false; for (const auto& d : v.diagnostics) found |= d.code == c.code;
    check(clean(r) && found, c.code);
  }
  for (const char* text : {"attribute x = \"bad\\q\";", "part 'bad\\q';", "connect (a);"})
    check(!clean(services.parser->parse(text)), "invalid escape or unary n-ary connector");
  return ok;
}
}  // namespace native_followup
