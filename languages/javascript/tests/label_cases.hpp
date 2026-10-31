#pragma once

#include <cstdio>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "javascript/core/CoreServices.hpp"
#include "javascript/core/TemplateValue.hpp"
#include "javascript/core/ast.hpp"

namespace javascript_label_cases {
namespace a=javascript::ast;
inline bool run(const javascript::JavaScriptCoreServices& services) {
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message) {
    if (!value) { std::fprintf(stderr,"javascript labels %s: %s\n",name,message); ok=false; }
  };
  struct Case { const char* name; const char* source; bool effective; const char* label; };
  // Node/V8 compiles, never executes these loops. Reserved-name early errors
  // are local validation in this port. Explicit module markers are separate
  // witnesses, not proof of an external Script/Module goal API.
  const Case cases[] = {
    {"label-async", "async: for(;;){break async;}", true, "async"},
    {"label-as", "as: for(;;){break as;}", true, "as"},
    {"label-from", "from: for(;;){break from;}", true, "from"},
    {"label-of", "of: for(;;){break of;}", true, "of"},
    {"label-target", "target: for(;;){break target;}", true, "target"},
    {"label-meta", "meta: for(;;){break meta;}", true, "meta"},
    {"label-get", "get: for(;;){break get;}", true, "get"},
    {"label-set", "set: for(;;){break set;}", true, "set"},
    {"label-yield", "yield: for(;;){break yield;}", true, "yield"},
    {"label-await", "await: for(;;){break await;}", true, "await"},
    {"label-static", "static: for(;;){break static;}", true, "static"},
    {"label-let", "let: for(;;){break let;}", true, "let"},
    {"label-implements", "implements: for(;;){break implements;}", true, "implements"},
    {"label-interface", "interface: for(;;){break interface;}", true, "interface"},
    {"label-package", "package: for(;;){break package;}", true, "package"},
    {"label-private", "private: for(;;){break private;}", true, "private"},
    {"label-protected", "protected: for(;;){break protected;}", true, "protected"},
    {"label-public", "public: for(;;){break public;}", true, "public"},
    {"continue-async", "async:while(true){continue async;}", true, "async"},
    {"unicode-label", "étiquette:while(true){break étiquette;}", true, "étiquette"},
    {"escaped-label", "\\u0061sync:while(true){break \\u0061sync;}", true, "async"},
    {"break-comment-line", "loop:while(true){break/*\n*/loop;}", true, "loop"},
    {"continue-comment-line", "loop:while(true){continue/*\n*/loop;}", true, "loop"},
    {"await-nested-reset", "async function f(){function g(){await:while(true){break await;}}}", true, "await"},
    {"hard-reserved-label", "class:while(true){}", false, ""},
    {"escaped-hard-reserved-label", "\\u0069f:while(true){}", false, ""},
    {"strict-yield-label", "\"use strict\";yield:while(true){break yield;}", false, ""},
    {"async-await-label", "async function f(){await:while(true){break await;}}", false, ""},
    {"generator-yield-label", "function* f(){yield:while(true){break yield;}}", false, ""},
    {"module-await-label-explicit-marker", "export {}; await:while(true){break await;}", false, ""},
    {"module-static-label-explicit-marker", "export {}; static:while(true){break static;}", false, ""},
  };
  for (const auto& test:cases) {
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    const auto validated=cursiv::test::validationHelper(services,std::string(test.source));
    check((clean && validated.diagnostics.empty())==test.effective,test.name,"native compiled acceptance versus parse+local validation");
    if (!test.effective) {
      if (clean) {
        bool reserved=false;
        for (const auto& diagnostic:validated.diagnostics)
          reserved |= diagnostic.code && *diagnostic.code==std::string_view("javascript.reserved-name");
        check(reserved,test.name,"reserved-name rejection is a targeted local diagnostic");
      }
      continue;
    }
    check(clean && parsed.cst().text()==test.source,test.name,"complete clean source");
    if (!clean) continue;
    std::size_t labels=0,jumps=0;
    const std::string_view name(test.name);
    const bool newline=name=="break-comment-line" || name=="continue-comment-line";
    bool followingName=false;
    for (const auto* node:parsed.nodes) {
      if (const auto* label=cursiv::ast_cast<a::LabeledStmt>(node)) {
        ++labels; check(label->label==test.label && label->body && label->body->parent==label,
                        test.name,"decoded label spelling and immediate body owner");
      }
      if (const auto* jump=cursiv::ast_cast<a::BreakStmt>(node)) {
        ++jumps; check(jump->label==(newline?"":test.label),test.name,"break target respects decoded spelling and no-LineTerminator restriction");
      }
      if (const auto* jump=cursiv::ast_cast<a::ContinueStmt>(node)) {
        ++jumps; check(jump->label==(newline?"":test.label),test.name,"continue target respects decoded spelling and no-LineTerminator restriction");
      }
      if (const auto* statement=cursiv::ast_cast<a::ExprStmt>(node))
        if (const auto* identifier=cursiv::ast_cast<a::Ident>(statement->expression))
          followingName |= identifier->name=="loop";
    }
    check(labels==1 && jumps==1,test.name,"one label and one jump retained");
    if (newline) check(followingName,test.name,"name following newline comment remains its own statement");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while (!work.empty()) {
      const auto* node=work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false,test.name,"unique AST owner"); continue; }
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child) {
        check(child.parent==node && node->cstNodeId<parsed.cst().size() && child.cstNodeId<parsed.cst().size() &&
              parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"every child parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size() && !parsed.value->parent,test.name,"all allocated nodes reachable once");
  }
  std::pmr::monotonic_buffer_resource memory;
  for (std::string_view text:{"","`","}","`a","x`","`a${"}) {
    const auto value=javascript::drop_ends<1>(text,&memory);
    check(!value && !value.error.message.empty(),"template-converter","malformed tail returns an error");
  }
  for (std::string_view text:{"","`","}","`${x","}a`","x${"}) {
    const auto value=javascript::drop_ends<2>(text,&memory);
    check(!value && !value.error.message.empty(),"template-converter","malformed head/middle returns an error");
  }
  {
    const auto empty=javascript::drop_ends<1>("``",&memory);
    const auto head=javascript::drop_ends<2>("`a${",&memory);
    const auto middle=javascript::drop_ends<2>("}b${",&memory);
    const auto tail=javascript::drop_ends<1>("}c`",&memory);
    const auto newline=javascript::drop_ends<1>("`a\r\nb\rc`",&memory);
    check(empty && empty.value->empty() && head && *head.value=="a" && middle && *middle.value=="b" &&
          tail && *tail.value=="c" && newline && *newline.value=="a\nb\nc",
          "template-converter","valid empty/head/middle/tail and CRLF normalization preserved");
  }
  for (const auto* source:{"const x=`a", "const x=`${1", "tag`a${`b"}) {
    const auto parsed=services.parser->parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),"template-recovery","malformed templates return diagnostics without a converter exception");
  }
  return ok;
}
}  // namespace javascript_label_cases
