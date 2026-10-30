#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "javascript/core/ast.hpp"

namespace javascript_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = javascript::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "javascript native: %s\n", message); ok = false; }
  };
  auto clean = [&](const auto& result, const char* message) {
    const bool value = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
    check(value, message);
    if (!value) for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "javascript kind=%u element=%u bytes=%u..%u\n", unsigned(d.kind), unsigned(d.element), d.begin, d.end);
    return value;
  };
  constexpr std::string_view source = R"js(const \u{10400}=1; const a\u0301=2; object.\u0069f;
class C { #x; static {} async
f(o) { return #x in o + 1; } }
let x=1; x-->0; const n=08; const m=09.5;
)js";
  const auto result = language.language.parser->parse(source);
  const auto* root = result.template value_as<a::Program>();
  clean(result, "native identifier, class, private-brand and numeric forms parse clean");
  bool astral = false, combining = false, property = false, staticBlock = false;
  bool asyncField = false, method = false, brand = false, postfix = false, leadingZero = false;
  unsigned members = 0;
  for (const auto* node : result.nodes) {
    if (const auto* p = cursiv::ast_cast<a::Ident>(node)) {
      astral |= p->name == "𐐀";
      combining |= p->name == "á";
      property |= p->name == "if" && cursiv::ast_cast<a::MemberExpr>(p->parent);
    }
    if (const auto* p = cursiv::ast_cast<a::ClassMember>(node)) {
      ++members;
      staticBlock |= p->staticBlock && p->isStatic;
      const auto* key = cursiv::ast_cast<a::Ident>(p->key);
      asyncField |= key && key->name == "async" && !p->isAsync && !p->isMethod;
      method |= key && key->name == "f" && p->isMethod && !p->isAsync;
    }
    if (const auto* p = cursiv::ast_cast<a::BinaryExpr>(node)) {
      const auto* left = cursiv::ast_cast<a::Ident>(p->left);
      const auto* right = cursiv::ast_cast<a::BinaryExpr>(p->right);
      brand |= p->op == "in" && left && left->name == "#x" && right && right->op == "+";
      const auto* update = cursiv::ast_cast<a::PostfixUpdateExpr>(p->left);
      postfix |= p->op == ">" && update && update->op == "--";
    }
    if (const auto* p = cursiv::ast_cast<a::NumberLit>(node)) leadingZero |= p->text == "09.5";
  }
  check(astral && combining && property && staticBlock && asyncField && method && brand && postfix && leadingZero && members == 4,
        "decoded names, class flags and operator ownership preserved");
  if (root && !result.cst().recovered()) {
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
    walk(walk, *root);
    check(owned && seen.size() == result.nodes.size(), "complete AST ownership");
  }
  const std::string physical = "const t=`a\r\nb`;";
  const auto templateResult = language.language.parser->parse(physical);
  bool normalized = false;
  for (const auto* node : templateResult.nodes)
    if (const auto* p = cursiv::ast_cast<a::TemplateLit>(node))
      normalized |= p->quasis.size() == 1 && p->quasis[0] == "a\nb";
  check(clean(templateResult, "CRLF template parses") && normalized, "raw template value normalizes physical CRLF");
  for (const auto source : {"export default function() {}", "export default async function() {}", "export default class {}"}) {
    const auto r = language.language.parser->parse(source);
    const auto* p = r.template value_as<a::Program>();
    const auto* e = p && p->body.size() == 1 ? cursiv::ast_cast<a::ExportStmt>(p->body[0]) : nullptr;
    bool anonymous = false;
    if (e && e->declaration) {
      if (const auto* f = cursiv::ast_cast<a::FunctionDecl>(e->declaration)) anonymous = f->name.empty();
      if (const auto* c = cursiv::ast_cast<a::ClassDecl>(e->declaration)) anonymous = c->name.empty();
    }
    check(clean(r, source) && e && e->isDefault && anonymous && !e->defaultExpr, "anonymous default declaration retained as a declaration");
  }
  const auto asyncResult = language.language.parser->parse("async\nfunction f() {}");
  const auto* asyncProgram = asyncResult.template value_as<a::Program>();
  const auto* f = asyncProgram && asyncProgram->body.size() == 2 ? cursiv::ast_cast<a::FunctionDecl>(asyncProgram->body[1]) : nullptr;
  check(clean(asyncResult, "async newline function parses") && f && !f->isAsync, "async newline is a separate statement");
  for (const auto source : {"async/*\n*/function f(){}", "async\u2028function f(){}"}) {
    const auto r = language.language.parser->parse(source);
    const auto* p = r.template value_as<a::Program>();
    const auto* function = p && p->body.size() == 2 ? cursiv::ast_cast<a::FunctionDecl>(p->body[1]) : nullptr;
    check(clean(r, source) && function && !function->isAsync, "comment or Unicode line terminator separates async/function");
  }
  const auto restricted = language.language.parser->parse("async/**/function f(){}");
  const auto* restrictedProgram = restricted.template value_as<a::Program>();
  const auto* restrictedFunction = restrictedProgram && restrictedProgram->body.size() == 1 ? cursiv::ast_cast<a::FunctionDecl>(restrictedProgram->body[0]) : nullptr;
  check(clean(restricted, "async comment function parses") && restrictedFunction && restrictedFunction->isAsync,
        "a comment without line terminators keeps async prefix");
  const auto arrowResult = language.language.parser->parse("const fn=async\nx=>x;");
  const auto* arrowProgram = arrowResult.template value_as<a::Program>();
  bool arrow = false;
  for (const auto* n : arrowResult.nodes) if (const auto* p = cursiv::ast_cast<a::ArrowFunctionExpr>(n)) arrow |= !p->isAsync;
  check(clean(arrowResult, "async newline arrow parses") && arrowProgram && arrowProgram->body.size() == 2 && arrow,
        "async initializer and following arrow remain separate");
  for (const auto source : {"#!/usr/bin/env node\nconst x=1;", "--> initial\nconst x=1;", "const x=1;\n --> line\nconst y=2;",
                            "class C { a\nb }", "import * as local from 'm';", "tag`\\8`;", "class C { static constructor() {} static get constructor() {} }"}) {
    const auto v = cursiv::test::validationHelper(language.language, source);
    check(clean(v.document->parseResult, source) && v.diagnostics.empty(), source);
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& c : {
      Invalid{R"js(var \u0069f=1;)js", "javascript.reserved-name"},
      Invalid{"'use strict'; var yield=1;", "javascript.reserved-name"},
      Invalid{R"js(const s="\xGG";)js", "javascript.escape"},
      Invalid{R"js(const s="\u{110000}";)js", "javascript.escape"},
      Invalid{R"js('use strict'; const s="\1";)js", "javascript.strict-escape"},
      Invalid{"`\\8`;", "javascript.template-escape"},
      Invalid{"/a/gg;", "javascript.regex-flags"},
      Invalid{"/a/uv;", "javascript.regex-flags"},
      Invalid{"function f(...x=[]) {}", "javascript.rest-parameter"},
      Invalid{"({get x(a){}});", "javascript.accessor-parameters"},
      Invalid{"({set x(){}});", "javascript.accessor-parameters"},
      Invalid{"function f(a=1){'use strict';}", "javascript.strict-parameters"},
      Invalid{"'use strict'; with(x) {}", "javascript.strict-with"},
      Invalid{"new.target;", "javascript.new-target"},
      Invalid{"class C { async constructor() {} }", "javascript.constructor"},
      Invalid{"class C { constructor; }", "javascript.constructor"},
      Invalid{"class C { static prototype() {} }", "javascript.static-prototype"},
      Invalid{"import {'x'} from 'm';", "javascript.import-binding"}}) {
    const auto v = cursiv::test::validationHelper(language.language, c.source);
    bool code = false;
    for (const auto& d : v.diagnostics) code |= d.code == c.code;
    check(clean(v.document->parseResult, c.source) && code, c.code);
  }
  for (const auto bad : {"class C { a b }", "function f(...x,y) {}", "function f(...x,) {}", "(...x,)=>x;",
                         "\n#!x\nlet a=1;", "\xEF\xBB\xBF#!x\nlet a=1;", "<![CDATA[x]]>",
                         R"js(var \u0030a=1;)js", R"js(var \u0020=1;)js", R"js(var \u{110000}=1;)js",
                         "function f(){new.foo;}", "import.foo;", R"js(function f(){new.\u0074arget;})js", R"js(import.\u006deta;)js",
                         "const r=/a\u2028b/;", "const r=/[a\u2029]/;"}) {
    const auto r = language.language.parser->parse(bad);
    check(!r.accepted || r.cst().recovered() || !r.diagnostics.empty(), bad);
  }
  return ok;
}
} // namespace javascript_native_followup
