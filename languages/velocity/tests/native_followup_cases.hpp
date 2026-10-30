#pragma once
#include <cstdio>
#include <exception>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "velocity/core/ast.hpp"

namespace velocity_followup {
template <class Language> bool run(const Language& language) {
  struct Case { const char* name; std::string_view source; bool accepted; };
  const Case cases[] = {
      {"empty", R"VTL()VTL", true},
      {"punctuation", R"VTL(hi () [] . > <)VTL", true},
      {"lone-dollar", R"VTL($)VTL", true},
      {"lone-hash", R"VTL(#)VTL", true},
      {"lone-slash", R"VTL(\)VTL", true},
      {"underscore", R"VTL($_name)VTL", true},
      {"hyphen", R"VTL($foo-bar)VTL", true},
      {"formal-expr", R"VTL(#set($x = ${y}))VTL", true},
      {"fallback", R"VTL(${foo|"bar"})VTL", true},
      {"quiet-fallback", R"VTL($!{foo|"bar"})VTL", true},
      {"bool", R"VTL(#set($x=true))VTL", true},
      {"null", R"VTL(#set($x=null))VTL", false},
      {"id-expr", R"VTL(#set($x=foo))VTL", false},
      {"formal-quiet", R"VTL($!{foo.bar})VTL", true},
      {"simple-refs", R"VTL(Hello $name ${customer.address} $map[1] $f(1,2))VTL", true},
      {"comment", R"VTL(a## hi
b#* multi *#c)VTL", true},
      {"raw-block", R"VTL(#[[$x #end]]#)VTL", true},
      {"escape", R"VTL(\$x \#if(true))VTL", true},
      {"if", R"VTL(#if(true)a#elseif(false)b#else c#end)VTL", true},
      {"foreach", R"VTL(#foreach($x in [1..3])$x#end)VTL", true},
      {"set-number", R"VTL(#set($n=1+2*3))VTL", true},
      {"number-forms", R"VTL(#set($a=.5)#set($b=1.)#set($c=1e2))VTL", true},
      {"double-string", R"VTL(#set($s="a""b"))VTL", true},
      {"single-string", R"VTL(#set($s='a''b'))VTL", true},
      {"backslash-string", R"VTL(#set($s="a\"b"))VTL", false},
      {"string-newline", R"VTL(#set($s="a
b"))VTL", true},
      {"map", R"VTL(#set($m={"a":1,"b":2}))VTL", true},
      {"range", R"VTL(#set($r=[1..3]))VTL", true},
      {"list", R"VTL(#set($r=[1,2,3]))VTL", true},
      {"paren-number", R"VTL(#set($x=(1+2)))VTL", true},
      {"bare-range", R"VTL(#set($x=1..3))VTL", false},
      {"bad-set", R"VTL(#set($x=))VTL", false},
      {"bad-if", R"VTL(#if()x#end)VTL", false},
      {"bad-end", R"VTL(#end)VTL", false},
      {"macro", R"VTL(#macro(hello $name)Hi $name#end #hello("you"))VTL", true},
      {"macro-comma", R"VTL(#macro(hello $a $b)$a$b#end #hello(1,2))VTL", true},
      {"block-macro", R"VTL(#@hello(1)body#end)VTL", true},
      {"define", R"VTL(#define($block)x#end)VTL", true},
      {"stop", R"VTL(#stop)VTL", true},
      {"stop-args", R"VTL(#stop("done"))VTL", true},
      {"break", R"VTL(#break)VTL", true},
      {"break-args", R"VTL(#break($foreach))VTL", true},
      {"eval", R"VTL(#evaluate("1+2"))VTL", true},
      {"include", R"VTL(#include("a","b"))VTL", true},
      {"parse", R"VTL(#parse("a"))VTL", true},
      {"dollar-bang", R"VTL($!)VTL", true},
      {"dollar-empty", R"VTL(${})VTL", true},
      {"dollar-bang-empty", R"VTL($!{})VTL", true},
      {"dollar-digit", R"VTL($1)VTL", true},
      {"dollar-adjacent", R"VTL($?#?)VTL", true},
      {"hash-unclosed", R"VTL(#if)VTL", false},
      {"hash-newline", R"VTL(#
)VTL", true},
      {"plain-closers", R"VTL() ])VTL", true},
      {"stop-empty", R"VTL(#stop())VTL", true},
      {"stop-two", R"VTL(#stop(1,2))VTL", false},
      {"break-two", R"VTL(#break(1,2))VTL", false},
      {"range-negative", R"VTL(#set($r=[-1..3]))VTL", true},
      {"range-expr", R"VTL(#set($r=[1+2..3]))VTL", false},
      {"range-reference", R"VTL(#set($r=[$lo..$hi]))VTL", true},
      {"range-float", R"VTL(#set($r=[1.0..3]))VTL", false},
      {"fallback-id", R"VTL(${x|foo})VTL", false},
      {"fallback-expression", R"VTL(${x|1+2})VTL", true},
      {"formal-default-expression", R"VTL(#set($x=${y|2}))VTL", true},
      {"bare-call", R"VTL($f(1,2))VTL", true},
      {"method-call", R"VTL($obj.f(1,2))VTL", true},
      {"method-repeat-call", R"VTL($obj.f()(1))VTL", true},
      {"set-invalid-target", R"VTL(#set(1=2))VTL", false},
      {"macro-default", R"VTL(#macro(m $x=1)$x#end)VTL", true},
      {"comma-call", R"VTL(#m(1,2))VTL", true},
      {"comma-leading", R"VTL(#m(,1))VTL", true},
      {"comma-trailing", R"VTL(#m(1,))VTL", false},
  };
  bool ok = true;
  for (const auto& test : cases) {
    try {
      const auto result = language.language.parser->parse(test.source);
      const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
      if (clean != test.accepted) {
        std::fprintf(stderr, "velocity native: %s expected=%d accepted=%d recovered=%d diagnostics=%zu\n", test.name, test.accepted, result.accepted, result.cst().recovered(), result.diagnostics.size());
        ok = false;
      }
    } catch (const std::exception& error) {
      std::fprintf(stderr, "velocity native: %s threw %s\n", test.name, error.what());
      ok = false;
    }
  }
  const std::string_view source = R"VTL(#set($x=${y|2}) #if(true)yes#end #stop("done") #break($foreach) #macro(m $a=1)$a#end #set($r=[-1..3]))VTL";
  const auto result = language.language.parser->parse(source);
  namespace ast = velocity::ast;
  bool formal = false, macro = false, stop = false, breaker = false, boolean = false, range = false;
  for (const auto* node : result.nodes) {
    if (const auto* d = cursiv::ast_cast<ast::DefaultExpr>(node)) {
      const auto* number = cursiv::ast_cast<ast::IntLit>(d->fallback);
      const auto* f = cursiv::ast_cast<ast::FormalRef>(d->reference);
      const auto* v = cursiv::ast_cast<ast::VariableRef>(d->reference);
      formal |= f && f->name == "y" && number && number->value == 2;
      macro |= v && v->name == "a" && number && number->value == 1;
    }
    if (const auto* d = cursiv::ast_cast<ast::StopDirective>(node)) {
      const auto* text = cursiv::ast_cast<ast::StringLit>(d->argument);
      stop |= text && text->value == "done";
    }
    if (const auto* d = cursiv::ast_cast<ast::BreakDirective>(node)) {
      const auto* v = cursiv::ast_cast<ast::VariableRef>(d->argument);
      breaker |= v && v->name == "foreach";
    }
    if (const auto* b = cursiv::ast_cast<ast::BoolLit>(node)) boolean |= b->value;
    if (const auto* b = cursiv::ast_cast<ast::BinaryExpr>(node); b && b->op == ast::Operator::range) {
      const auto* u = cursiv::ast_cast<ast::UnaryExpr>(b->left);
      const auto* lo = u ? cursiv::ast_cast<ast::IntLit>(u->operand) : nullptr;
      const auto* hi = cursiv::ast_cast<ast::IntLit>(b->right);
      range |= u && u->op == ast::Operator::negate && lo && lo->value == 1 && hi && hi->value == 3;
    }
  }
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  if (result.value) walk(walk, *result.value);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !formal || !macro || !stop || !breaker || !boolean || !range || !owned || seen.size() != result.nodes.size()) {
    std::fprintf(stderr, "velocity native: values/ownership formal=%d macro=%d stop=%d break=%d bool=%d range=%d owned=%d\n", formal, macro, stop, breaker, boolean, range, owned);
    ok = false;
  }
  {
    const auto quotes = language.language.parser->parse(R"VTL(#set($a='$x') #set($b="$x"))VTL");
    bool single = false, doubleQuote = false;
    for (const auto* node : quotes.nodes)
      if (const auto* text = cursiv::ast_cast<ast::StringLit>(node); text && text->value == "$x") {
        single |= !text->doubleQuoted;
        doubleQuote |= text->doubleQuoted;
      }
    if (!quotes.accepted || quotes.cst().recovered() || !quotes.diagnostics.empty() || !single || !doubleQuote) {
      std::fprintf(stderr, "velocity native: quote distinction lost from AST\n");
      ok = false;
    }
  }
  for (const bool method : {false, true}) {
    const auto call = language.language.parser->parse(method ? "$obj.f(1,2)" : "$f(1,2)");
    unsigned calls = 0;
    bool shape = !method;
    for (const auto* node : call.nodes)
      if (const auto* c = cursiv::ast_cast<ast::CallAccess>(node)) {
        ++calls;
        const auto* property = cursiv::ast_cast<ast::PropertyAccess>(c->object);
        shape |= property && property->name == "f" && c->args.size() == 2;
      }
    if (!call.accepted || call.cst().recovered() || !call.diagnostics.empty() || calls != unsigned(method) || !shape) {
      std::fprintf(stderr, "velocity native: bare reference vs method call AST method=%d calls=%u shape=%d accepted=%d recovered=%d diagnostics=%zu\n", method, calls, shape, call.accepted, call.cst().recovered(), call.diagnostics.size());
      for (const auto* node : call.nodes) {
        if (const auto* v = cursiv::ast_cast<ast::VariableRef>(node)) std::fprintf(stderr, "variable=%.*s\n", int(v->name.size()), v->name.data());
        if (const auto* v = cursiv::ast_cast<ast::PropertyAccess>(node)) std::fprintf(stderr, "property=%.*s\n", int(v->name.size()), v->name.data());
      }
      ok = false;
    }
  }
  return ok;
}
}  // namespace velocity_followup
