#pragma once

#include <cstdio>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "less/core/CoreServices.hpp"
#include "less/core/ast.hpp"

namespace less_detached_arguments {
namespace a = less::ast;

inline const a::Expr *atom(const a::CommandStatement *command) {
  if (!command || command->terms.size() != 1 || !command->terms[0] ||
      command->terms[0]->op || !command->terms[0]->operand) return nullptr;
  const auto *mul = command->terms[0]->operand;
  if (mul->terms.size() != 1 || !mul->terms[0] || mul->terms[0]->op ||
      mul->terms[0]->items.size() != 1) return nullptr;
  return mul->terms[0]->items[0];
}

inline bool run(const less::LessCoreServices &language) {
  bool good = true;
  const auto check = [&](bool value, const char *message, const char *source) {
    if (!value) {
      good = false;
      std::fprintf(stderr, "less detached argument: %s [%s]\n", message, source);
    }
  };
  struct Case { const char *name, *source; unsigned detached, named; };
  const Case cases[] = {
      {"basic", ".m(@r){@r();}a{.m({color:red;});}", 1, 0},
      {"two-comma", ".m(@a,@b){@a();@b();}a{.m({x:1;},{y:2;});}", 2, 0},
      {"block-then-value", ".m(@r,@n){@r();n:@n;}a{.m({x:1;},2);}", 1, 0},
      {"value-then-block", ".m(@n,@r){@r();n:@n;}a{.m(2,{x:1;});}", 1, 0},
      {"semicolon", ".m(@r;@n){@r();n:@n;}a{.m({x:1;};2);}", 1, 0},
      {"comma-list-semicolon", ".m(@vals;@r){list:@vals;@r();}a{.m(1,2;{x:1;});}", 1, 0},
      {"named-comma", ".m(@r;@n:0){@r();n:@n;}a{.m(@n:2,@r:{x:1;});}", 1, 2},
      {"named-semicolon", ".m(@r;@n:0){@r();n:@n;}a{.m(@r:{x:1;};@n:2);}", 1, 2},
      {"named-list", ".m(@vals;@r){list:@vals;@r();}a{.m(@vals:1,2;@r:{x:1;});}", 1, 2},
      {"hash-path", "a{#ns>.m(@r:{x:1;});}", 1, 1},
      {"class-path", "a{.ns .m(@r:{x:1;});}", 1, 1},
      {"default-comma", ".m(@r:{x:1;},@n:2){@r();n:@n;}a{.m();}", 1, 0},
      {"default-semicolon", ".m(@r:{x:1;};@n:2){@r();n:@n;}a{.m();}", 1, 0},
      {"function", "a{x:foo({color:red;});}", 1, 0},
      {"function-semicolon", "a{x:foo(1,2;{color:red;});}", 1, 0},
      {"empty", ".m(@r){@r();}a{.m({});}", 1, 0},
      {"nested", ".m(@r){@r();}a{.m({b{color:red;}@v:1;x:@v;});}", 1, 0},
      {"comment", ".m(@r){@r();}a{.m(/*arg*/{x:1;});}", 1, 0},
      {"existing-variable", "@dr:{x:1;};a{@dr();}", 0, 0},
      {"default-list", ".m(@r:{x:1;},2;@n:0){}a{.m();}", 1, 0},
  };
  for (const auto &test : cases) {
    const auto result = language.parser->parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && result.value;
    check(clean, "clean parse required", test.source);
    if (!clean)
      for (const auto &d : result.diagnostics)
        std::fprintf(stderr, "kind=%u element=%u bytes=%u..%u\n",
                     static_cast<unsigned>(d.kind), static_cast<unsigned>(d.element),
                     static_cast<unsigned>(d.begin), static_cast<unsigned>(d.end));
    check(result.cst().text() == test.source, "CST source retained", test.source);
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) { check(false, "duplicate AST edge", test.source); return; }
      check(node.cstNodeId < result.cst().size(), "node CST span", test.source);
      language.astReflection->visitMembers(node, [&](auto, auto, auto &child) {
        check(child.parent == &node, "exact field parent", test.source);
        check(node.cstNodeId < result.cst().size() && child.cstNodeId < result.cst().size() &&
                  result.cst().contains(node.cstNodeId, child.cstNodeId), "child CST containment", test.source);
        self(self, child);
      });
    };
    if (result.value) {
      check(result.value->parent == nullptr, "root parent", test.source);
      walk(walk, *result.value);
    }
    check(seen.size() == result.nodes.size(), "all allocations reachable once", test.source);
    unsigned detached = 0, named = 0;
    const a::DetachedRuleset *block = nullptr;
    const a::MixinRef *call = nullptr;
    const a::FuncCall *function = nullptr;
    const a::MixinDef *definition = nullptr;
    for (const auto *node : result.nodes) {
      if (const auto *value = cursiv::ast_cast<a::DetachedRuleset>(node)) {
        ++detached; block = value;
        check(value->body && value->body->parent == value,
              "detached block capture and parent", test.source);
      }
      if (const auto *value = cursiv::ast_cast<a::NamedArgument>(node)) {
        ++named;
        check(value->name && value->value && !value->value->items.empty() &&
                  value->name->parent == value && value->value->parent == value,
              "named argument name/value fields", test.source);
      }
      if (const auto *value = cursiv::ast_cast<a::MixinRef>(node)) call = value;
      if (const auto *value = cursiv::ast_cast<a::FuncCall>(node)) function = value;
      if (const auto *value = cursiv::ast_cast<a::MixinDef>(node)) definition = value;
    }
    check(detached == test.detached && named == test.named,
          "exact detached/named argument node counts", test.source);
    const std::string_view name{test.name};
    if (name == "basic") {
      const auto *property = block && block->body && block->body->items.size() == 1
          ? std::get_if<a::Property *>(&block->body->items[0]) : nullptr;
      const auto *red = property && *property && (*property)->value &&
              (*property)->value->items.size() == 1
          ? cursiv::ast_cast<a::IdentValue>(atom((*property)->value->items[0])) : nullptr;
      check(call && call->args.size() == 1 && call->argumentGroups.empty() &&
                atom(call->args[0]) == block && red && red->name &&
                red->name->parts.size() == 1 &&
                std::holds_alternative<std::string_view>(red->name->parts[0]) &&
                std::get<std::string_view>(red->name->parts[0]) == "red",
            "literal argument and red body value", test.source);
    }
    if (name == "semicolon" || name == "comma-list-semicolon" || name == "named-semicolon" || name == "named-list") {
      check(call && call->args.empty() && call->argumentGroups.size() == 2,
            "semicolon keeps argument groups", test.source);
      if (call && call->argumentGroups.size() == 2 && call->argumentGroups[0] &&
          call->argumentGroups[1] && name == "comma-list-semicolon")
        check(call->argumentGroups[0]->items.size() == 2 &&
                  call->argumentGroups[1]->items.size() == 1 &&
                  atom(call->argumentGroups[1]->items[0]) == block,
              "comma list and detached argument remain separate semicolon groups", test.source);
      if (call && call->argumentGroups.size() == 2 && call->argumentGroups[0] &&
          call->argumentGroups[1] && name == "named-list") {
        const auto *values = call->argumentGroups[0]->items.size() == 1
            ? cursiv::ast_cast<a::NamedArgument>(atom(call->argumentGroups[0]->items[0])) : nullptr;
        const auto *rules = call->argumentGroups[1]->items.size() == 1
            ? cursiv::ast_cast<a::NamedArgument>(atom(call->argumentGroups[1]->items[0])) : nullptr;
        check(values && values->name && values->value && values->name->name == "vals" && values->value->items.size() == 2 &&
                  rules && rules->name && rules->value && rules->name->name == "r" && rules->value->items.size() == 1 &&
                  atom(rules->value->items[0]) == block,
              "named semicolon values preserve comma list and block", test.source);
      }
    }
    if (name == "named-comma") {
      const auto *first = call && call->args.size() == 2
          ? cursiv::ast_cast<a::NamedArgument>(atom(call->args[0])) : nullptr;
      const auto *second = call && call->args.size() == 2
          ? cursiv::ast_cast<a::NamedArgument>(atom(call->args[1])) : nullptr;
      check(call && call->argumentGroups.empty() && first && first->name && first->name->name == "n" &&
                second && second->name && second->value && second->name->name == "r" && second->value->items.size() == 1 &&
                atom(second->value->items[0]) == block,
            "comma named arguments preserve source order and block value", test.source);
    }
    if (name.starts_with("default")) {
      const auto *first = definition && !definition->params.empty()
          ? std::get_if<a::VariableDecl *>(&definition->params[0]) : nullptr;
      check(first && *first && (*first)->value &&
                (*first)->value->items.size() == (name == "default-list" ? 2u : 1u) &&
                atom((*first)->value->items[0]) == block,
            "named parameter default reuses existing ValueList", test.source);
    }
    if (name == "function")
      check(function && function->args.size() == 1 && function->argumentGroups.empty() &&
                atom(function->args[0]) == block,
            "function argument carrier", test.source);
    if (name == "function-semicolon")
      check(function && function->args.empty() && function->argumentGroups.size() == 2 &&
                function->argumentGroups[0] && function->argumentGroups[1] &&
                function->argumentGroups[0]->items.size() == 2 &&
                function->argumentGroups[1]->items.size() == 1 &&
                atom(function->argumentGroups[1]->items[0]) == block,
            "function semicolon comma grouping", test.source);
    if (name == "empty") check(block && block->body && block->body->items.empty(), "empty block preserved", test.source);
    if (name == "nested") check(block && block->body && block->body->items.size() == 3, "nested Block source order", test.source);
  }
  // Less4.6.3 consumes and discards the expression after the block in these
  // two forms. This port's documented profile requires an explicit separator.
  for (const auto *source : {"a{.m({x:1;} 2);}", "a{.m({x:1;} +2);}"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "explicit-separator profile rejects native source-discard tolerance", source);
  }
  for (const auto *source : {"a{x:{color:red;};}", "a{.m(@r:);}",
                             "a{.m(1,@x:2;3);}", "a{.m(@x:1,@y:2;3);}",
                             "a{x:foo(@r:{x:1;});}", "a{.m:foo(@r:{x:1;});}"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "native-invalid argument/general-expression block rejected", source);
  }
  std::printf("less detached arguments: %zu forms, ownership and argument grouping checked\n", std::size(cases));
  return good;
}
} // namespace less_detached_arguments
