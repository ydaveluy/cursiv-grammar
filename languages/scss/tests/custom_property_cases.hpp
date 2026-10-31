#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "scss/core/CoreServices.hpp"
#include "scss/core/ValueConverters.hpp"
#include "scss/core/ast.hpp"

namespace scss_custom_cases {
namespace a = scss::ast;
// Test-only typed source projection; no CST or source rescanning. Interpolation
// payloads are checked independently through their ordinary expression AST.
inline std::string project(const a::RawCustomValue& value) {
  using Item = std::variant<std::string_view, const cursiv::AstNode*>;
  std::vector<Item> work;
  const auto push = [&](const auto& parts) {
    for (std::size_t i=parts.size(); i>0; --i) work.emplace_back(parts[i-1]);
  };
  push(value.parts);
  std::string result;
  while (!work.empty()) {
    const auto item=work.back(); work.pop_back();
    if (const auto* text=std::get_if<std::string_view>(&item)) { result += *text; continue; }
    const auto* node=std::get<const cursiv::AstNode*>(item);
    if (const auto* text=cursiv::ast_cast<a::RawText>(node)) result += text->text;
    else if (cursiv::ast_cast<a::Interpolation>(node)) result += "<i>";
    else if (const auto* group=cursiv::ast_cast<a::RawCustomGroup>(node)) {
      work.emplace_back(group->close); push(group->parts); work.emplace_back(group->open);
    } else result += "?";
  }
  return result;
}
inline bool run(const scss::ScssCoreServices& services) {
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message) {
    if (!value) { std::fprintf(stderr,"scss raw-custom %s: %s\n",name,message); ok=false; }
  };
  struct Case { const char* name; const char* source; bool parses; unsigned mode; const char* projection; std::size_t interpolations; const char* names="x"; const char* varName="missing"; std::size_t customCount=1; };
  const Case cases[] = {
    {"raw-variable", "a{--x:$missing;}", true, 1, "$missing", 0},
    {"raw-operators", "a{--x:1 + 2 / 3 !default !global !important;}", true, 1, "1 + 2 / 3 !default !global !important", 0},
    {"empty", "a{--x:;}", true, 1, "", 0},
    {"space-only", "a{--x:   ;}", true, 1, "   ", 0},
    {"no-final-semi", "a{--x: $missing }", true, 1, " $missing ", 0},
    {"nested-braces", "a{--x:{color:red; child:{x:y;}};}", false, 0, "", 0},
    {"balanced-all", "a{--x:([a;{b:c}]);}", true, 1, "([a;{b:c}])", 0},
    {"raw-slashes", "a{--x://not-a-comment;}", true, 1, "//not-a-comment", 0},
    {"raw-comment", "a{--x:a/* keep */b;}", true, 1, "a/* keep */b", 0},
    {"raw-unmatched-comment", "a{--x:a/*;", false, 0, "", 0},
    {"interpolation", "$x:red;a{--x:#{$x};}", true, 1, "<i>", 1},
    {"leading-space-interpolation", "a{--x: #{$x};}", true, 1, " <i>", 1},
    {"leading-space-dash-variable", "a{--x: #{$pagination-padding-x};}", true, 1, " <i>", 1},
    {"dash-variable-interpolation", "a{--x:#{$pagination-padding-x};}", true, 1, "<i>", 1},
    {"bootstrap-custom-name-value", "a{--#{$prefix}pagination-padding-x: #{$pagination-padding-x};}", true, 1, " <i>", 1, "#{$prefix}pagination-padding-x"},
    {"interpolation-expression", "a{--x:pre#{1 + 2}post;}", true, 1, "pre<i>post", 1},
    {"interpolation-in-group", "$x:red;a{--x:(a #{$x} [b]);}", true, 1, "(a <i> [b])", 1},
    {"interpolation-in-quote", "$x:red;a{--x:\"a #{$x} b\";}", true, 1, "\"a <i> b\"", 1},
    {"quoted-delimiters", "a{--x:\";})]\";}", true, 1, "\";})]\"", 0},
    {"single-quoted-delimiters", "a{--x:';})]';}", true, 1, "';})]'", 0},
    {"escaped-close", "a{--x:foo\\}bar;}", true, 1, "foo\\}bar", 0},
    {"escaped-semi", "a{--x:foo\\;bar;}", true, 1, "foo\\;bar", 0},
    {"newlines", "a{--x: a\r\n  b\rc\nd;}", true, 1, " a\r\n  b\rc\nd", 0},
    {"interpolation-comments", "a{--x:#{1 /*comment*/ + 2};}", true, 1, "<i>", 1},
    {"unmatched-paren", "a{--x:(a;}", false, 0, "", 0},
    {"mismatched-group", "a{--x:[a);}", false, 0, "", 0},
    {"unclosed-quote", "a{--x:\"a;}", false, 0, "", 0},
    {"bad-interpolation", "a{--x:#{1 + };}", false, 0, "", 0},
    {"ordinary-control", "a{color:$missing;}", true, 0, "", 0},
    {"interpolated-name", "$x:color;a{--#{$x}:$missing;}", true, 1, "$missing", 0},
    {"spaced-braces", "a{--x: {color:red; child:{x:y;}};}", true, 1, " {color:red; child:{x:y;}}", 0},
    {"prefixed-braces", "a{--x:x{color:red;};}", true, 1, "x{color:red;}", 0},
    {"newline-before-braces", "a{--x:\n{color:red;};}", true, 1, "\n{color:red;}", 0},
    {"comment-before-braces", "a{--x:/*c*/{color:red;};}", true, 1, "/*c*/{color:red;}", 0},
    {"leading-close-paren", "a{--x:);}", false, 0, "", 0},
    {"nonempty-close-paren", "a{--x:x);}", false, 0, "", 0},
    {"nested-property-sass-value", "a{font:{--x:$missing;}}", true, 2, "", 0},
    {"nested-property-if-sass-value", "a{font:{@if true{--x:$missing;}}}", true, 2, "", 0},
    {"nested-property-empty-invalid", "a{font:{--x:;}}", false, 0, "", 0},
    {"style-if-raw-value", "a{@if true{--x:$missing;}}", true, 1, "$missing", 0},
    {"nested-for", "a{font:{@for $i from 1 to 2{--x:$i;}}}", true, 2, "", 0, "x", "i", 1},
    {"nested-while", "a{font:{@while false{--x:$missing;}}}", true, 2, "", 0, "x", "missing", 1},
    {"nested-each", "a{font:{@each $i in 1,2{--x:$i;}}}", true, 2, "", 0, "x", "i", 1},
    {"nested-if-else", "a{font:{@if true{--x:$missing;}@else if false{--y:$missing;}@else{--z:$missing;}}}", true, 2, "", 0, "x,y,z", "missing", 3},
    {"double-nested-property", "a{font:{margin:{--x:$missing;}}}", true, 2, "", 0, "x", "missing", 1},
    {"nested-include-content-reset", "a{font:{@include m{--x:$missing;}}}", true, 1, "$missing", 0, "x", "missing", 1},
    {"nested-media-disallowed", "a{font:{@media screen{--x:$missing;}}}", false, 0, "", 0, "x", "missing", 1},
    {"nested-unknown-disallowed", "a{font:{@custom x;}}", false, 0, "", 0, "x", "missing", 1},
  };
  for (const auto& test:cases) {
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean==test.parses,test.name,"native parser-only syntax verdict");
    if (clean!=test.parses) for (const auto& d:parsed.diagnostics)
      std::fprintf(stderr,"  kind=%u element=%u bytes=%u..%u\n",unsigned(d.kind),d.element,d.begin,d.end);
    if (!clean) continue;
    check(parsed.cst().text()==test.source,test.name,"complete source retained");
    std::vector<const a::VariableDeclaration*> customs;
    std::size_t interpolations=0, customCount=0;
    for (const auto* node:parsed.nodes) {
      if (const auto* n=cursiv::ast_cast<a::VariableDeclaration>(node); n && n->isCustomProperty) {
        customs.push_back(n); ++customCount;
      }
      if (const auto* n=cursiv::ast_cast<a::Interpolation>(node)) {
        ++interpolations;
        check(n->inner && n->inner->parent==n,test.name,"typed interpolation owns its ordinary Sass expression");
        const std::string_view name(test.name);
        if (name=="leading-space-interpolation" || name=="leading-space-dash-variable" ||
            name=="dash-variable-interpolation" || name=="bootstrap-custom-name-value") {
          const auto* variable=cursiv::ast_cast<a::VariableRef>(n->inner);
          const auto expected=name=="leading-space-interpolation" ? "x" : "pagination-padding-x";
          check(variable && variable->name==expected && !variable->isCustomProperty,
                test.name,"raw interpolation retains ordinary variable spelling, including dashes");
        }
        if (name=="interpolation-expression" || name=="interpolation-comments") {
          const auto* sum=cursiv::ast_cast<a::BinaryExpr>(n->inner);
          const auto* left=sum?cursiv::ast_cast<a::NumberLit>(sum->left):nullptr;
          const auto* right=sum?cursiv::ast_cast<a::NumberLit>(sum->right):nullptr;
          check(sum && sum->op==a::Op::plus && left && right && left->magnitude==1 && right->magnitude==2,
                test.name,"interpolation restores normal operators, whitespace and comments");
        }
      }
    }
    check(customCount==(test.mode?test.customCount:0u),test.name,"one native custom declaration or ordinary control");
    std::ranges::sort(customs,[&](const auto* left,const auto* right) {
      return parsed.cst()[left->cstNodeId].begin < parsed.cst()[right->cstNodeId].begin;
    });
    std::string names;
    for (const auto* custom:customs) {
      if (!names.empty()) names += ",";
      names += custom->name;
    }
    if (test.mode) check(names==(std::string_view(test.name)=="interpolated-name" ? "#{$x}" : test.names),
                         test.name,"all custom declaration names retain source order and spelling");
    for (const auto* custom:customs) {
      check(custom->value && custom->value->parent==custom,test.name,"one immediate value owner");
      if (test.mode==1) {
        const auto* raw=cursiv::ast_cast<a::RawCustomValue>(custom->value);
        check(raw && project(*raw)==test.projection,test.name,"raw source text, delimiters, whitespace and comments preserved exactly");
        check(interpolations==test.interpolations,test.name,"only actual Sass interpolation is structured");
        check(!custom->isDefault && !custom->isGlobal && custom->prio==a::Prio::none,
              test.name,"raw !default/!global/!important are not Sass variable flags");
        // Literal $missing and arithmetic remain text, not evaluated AST nodes.
        if (std::string_view(test.name)=="raw-variable" || std::string_view(test.name)=="raw-operators")
          for (const auto* node:parsed.nodes)
            check(!cursiv::ast_cast<a::VariableRef>(node) && !cursiv::ast_cast<a::BinaryExpr>(node),
                  test.name,"raw variable/operator text does not become a Sass expression");
      } else {
        const auto* variable=cursiv::ast_cast<a::VariableRef>(custom->value);
        check(variable && variable->name==test.varName && !variable->isCustomProperty,
              test.name,"nested declaration context retains native Sass value under controls");
      }
    }
    if (std::string_view(test.name)=="ordinary-control") {
      bool found=false;
      for (const auto* node:parsed.nodes) if (const auto* property=cursiv::ast_cast<a::PropertyDeclaration>(node)) {
        const auto* variable=property->value.size()==1 ? cursiv::ast_cast<a::VariableRef>(property->value[0]) : nullptr;
        found |= property->property=="color" && variable && variable->name=="missing";
      }
      check(found,test.name,"ordinary declaration keeps its native Sass variable value");
    }
    if (std::string_view(test.name)=="newlines")
      check(std::string_view(test.source).find('\r')!=std::string_view::npos &&
            std::string_view(test.projection).find("\r\n")!=std::string_view::npos,
            test.name,"real CR/CRLF bytes reach parser and raw value");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while (!work.empty()) {
      const auto* node=work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false,test.name,"unique AST ownership"); continue; }
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child) {
        check(child.parent==node && node->cstNodeId<parsed.cst().size() && child.cstNodeId<parsed.cst().size() &&
              parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"every parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size() && !parsed.value->parent,test.name,"all allocated nodes reachable once");
  }
  for (std::string_view text:{"","a","'","'a","a'","'a\""}) {
    const auto r=scss::unquote(text); check(!r && !r.error.message.empty(),"unquote-contract","malformed fragment returns an error");
  }
  for (std::string_view text:{"","/*","/*x*/","'","'a","'a'junk","'a'/*"}) {
    const auto r=scss::quoted_url(text); check(!r && !r.error.message.empty(),"quoted-url-contract","truncated fragment returns an error without indexing outside source");
  }
  for (std::string_view text:{"","&","a"}) {
    const auto r=scss::drop_parent(text); check(!r && !r.error.message.empty(),"parent-suffix-contract","invalid fragment returns an error");
  }
  {
    const auto string=scss::unquote("'a\\n'");
    const auto url=scss::quoted_url(" /* lead */ 'café.svg' /* tail */ ");
    const auto parent=scss::drop_parent("&-x");
    check(string && *string.value=="a\\n" && url && *url.value=="café.svg" && parent && *parent.value=="-x",
          "converter-values","valid existing raw values remain unchanged");
  }
  for (const auto* source:{"a{v:url(/*x*/'a);}","a{v:'a;}","a{&"}) {
    const auto parsed=services.parser->parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),"recovery-totality","malformed neighbors return diagnostics without a thrown converter");
  }
  return ok;
}
}  // namespace scss_custom_cases
