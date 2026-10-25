#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "swift/core/CoreServices.hpp"
#include "swift/core/ast.hpp"
namespace native_regressions {
template<class T> const T* as(const cursiv::AstNode* n) { return cursiv::ast_cast<T>(n); }
inline bool run(const swift::SwiftCoreServices& services) {
 namespace ast = swift::ast;
 const auto& parser=*services.parser;
 struct Case { const char* name; const char* text; };
 const Case cases[] = {
  {"array-three-strings","let x = [\"a\", \"b\", \"c\"]\n"},
  {"array-inner-outer-spaces","let x = [ \" a \" , \"  b  \" , \" c \" ]\n"},
  {"tuple-binding","let (x, y): (Int, String) = (1, \"a\")\n"},
  {"hashbang","#!/usr/bin/swift\nlet value = 1\n"},
  {"bom-hashbang","﻿#!/usr/bin/swift\nlet value = 1\n"},
  {"protocol-effects","public protocol P { init() async throws(MyError)\nfunc f() async\nvar value: Int { get async throws } }\n"},
  {"bom","﻿let value = 1\n"},
  {"string-comment-text","let s = \"/* literal */ // literal\"\n"},
  {"basics","let x = 1 + 2 * 3\nlet y: Int? = nil\n"},
  {"qualified-type","var value: Module.Outer<Int>.Inner<String>\n"},
  {"qualified-expression","let x = Module.Value.member\n"},
  {"metatype","let t: Module.Value.Type = Module.Value.self\n"},
  {"async-function","func f() async throws -> Int { return try await g() }\n"},
  {"async-type","typealias F = (Int) async throws -> String\n"},
  {"async-closure","let f = { (x: Int) async throws -> Int in return try await g(x) }\n"},
  {"actor","public actor Counter { var value = 0\nnonisolated func ping() async {} }\n"},
  {"any-type","func f(_ x: any P & Q) -> any R {}\n"},
  {"some-type","func f() -> some P & Q {}\n"},
  {"shorthand-if","func f(_ x: Int?) { if let x { print(x) } }\n"},
  {"shorthand-guard","func f(_ x: Int?) { guard let x else { return }; print(x) }\n"},
  {"unavailable","if #unavailable(macOS 13.0) { print(1) }\n"},
  {"availability-patch","if #available(macOS 13.1.2, *) { print(1) }\n"},
  {"for-await","func f() async throws { for try await value in values { print(value) } }\n"},
  {"typed-throws","func f() throws(MyError) -> Int { throw error }\n"},
  {"typed-throws-type","typealias F = () throws(MyError) -> Int\n"},
  {"accessor-effects","struct S { var x: Int { get async throws { return try await g() } } }\n"},
  {"prefix-chain","let x = !!value\nlet y = -~value\n"},
  {"inout-prefix","f(&value)\n"},
  {"escaped-name","let `class` = 1\nprint(`class`)\n"},
  {"unicode-name","let café = 42\nlet 日本語 = café\n"},
  {"negative-literals","let a = -42\nlet b = -1.25\n"},
  {"raw-interpolation","let x = #\"a\\#(value)b\"#\n"},
  {"multiple-interpolation","let x = \"a\\(x, radix: 16)b\"\n"},
  {"if-expression","let x = if flag { 1 } else { 2 }\n"},
  {"switch-expression","let x = switch value { case 1: 10\ndefault: 20 }\n"},
  {"pack","func f<each T>(_ x: repeat each T) {}\n"},
  {"ownership","func f(_ x: borrowing Value, _ y: consuming Value) { let a = copy x\nlet b = consume y }\n"},
  {"raw-regex","let regex = #/[a-z]+/#\n"},
  {"bare-regex","let regex = /[a-z]+/\n"},
  {"protocol-init","protocol P { init()\nfunc f()\nvar value: Int { get } }\n"},
  {"function-attrs","@MainActor public func f(_ x: @Sendable () async -> Void) async {}\n"},
  {"parameter-type-attrs","func f(_ x: @escaping @Sendable () -> Void) {}\n"},
  {"generic-trailing-comma","func f<T,>(_ x: T,) {}\n"},
  {"keypath","let x = \\Module.Person.name\n"},
  {"dollar-in-name","let foo$bar = 1\nprint(foo$bar)\n"},
  {"escaped-punctuation","let `a-b` = 1\nprint(`a-b`)\n"},
  {"escaped-spaces","let `a b` = 1\nprint(`a b`)\n"},
  {"string-spaces","let s = \"  \\(value)   \"\n"},
  {"string-multispace-continuation","let s = \"\"\"\na\\   \nb\n\"\"\"\n"},
  {"self-postfix","self.foo(x: 1)\nsuper.value[index: 2]\nself.init()\n"},
  {"implicit-postfix","let x = .foo.bar(1)\n"},
  {"division","let x = a / b / c\n"},
  {"contextual-functions","copy(x)\nconsume(y)\nactor()\n"},
 };
 for(const auto& c:cases) {
  const std::string text=c.text;
  const auto result=parser.parse(text);
  std::unordered_set<const cursiv::AstNode*> seen;
  bool parents=result.value!=nullptr;
  const auto walk=[&](auto&& self,const cursiv::AstNode& n)->void {
   if(!seen.insert(&n).second){parents=false;return;}
   services.astReflection->visitMembers(n,[&](std::string_view,std::uint32_t,cursiv::AstNode& child){
    parents &= child.parent==&n; self(self,child);
   });
  };
  if(result.value)walk(walk,*result.value);
  parents &= seen.size()==result.nodes.size();
  const auto validation=cursiv::test::validationHelper(services,text);
  if(!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !parents || !validation.diagnostics.empty()) {
   std::fprintf(stderr,"swift: native case failed: %s\n",c.name);
   for(const auto& d:validation.diagnostics) std::fprintf(stderr,"%s\n",d.message.c_str());
   return false;
  }
 }
 bool ok=true;
 const auto check=[&](bool value,const char* message){if(!value){std::fprintf(stderr,"swift: native AST: %s\n",message);ok=false;}};
 const auto variable=[](const cursiv::ParseResult& r,std::size_t i=0){
  const auto* f=as<ast::File>(r.value);
  return f && i<f->statements.size()?as<ast::VariableDecl>(f->statements[i]):nullptr;
 };
 const auto initializer=[&](const cursiv::ParseResult& r,std::size_t i=0)->const ast::Expr* {
  const auto* v=variable(r,i);return v && v->bindings.size()==1?v->bindings[0]->initializer:nullptr;
 };
 {
  const auto r=parser.parse("var value: Module.Outer<Int>.Inner<String>\n");
  const auto* v=variable(r);const auto* p=v?as<ast::IdentifierPattern>(v->bindings[0]->pattern):nullptr;
  const auto* inner=p?as<ast::NamedType>(p->type):nullptr;
  const auto* outer=inner?inner->qualifier:nullptr;const auto* module=outer?outer->qualifier:nullptr;
  check(inner && inner->name=="Inner" && inner->genericArgs.size()==1 && outer && outer->name=="Outer" && outer->genericArgs.size()==1 && module && module->name=="Module" && !module->qualifier && inner->parent==p && outer->parent==inner && module->parent==outer,"qualified types retain each name, arguments and owner");
 }
 {
  const auto r=parser.parse("let x: Module.Value.Type = Module.Value.self\n");
  const auto* v=variable(r);const auto* p=v?as<ast::IdentifierPattern>(v->bindings[0]->pattern):nullptr;
  const auto* type=p?as<ast::MetatypeType>(p->type):nullptr;
  const auto* value=type?as<ast::NamedType>(type->base):nullptr;
  const auto* expr=as<ast::SelfPostfixExpr>(initializer(r));
  check(value && value->name=="Value" && value->qualifier && value->qualifier->name=="Module" && expr && as<ast::MemberExpr>(expr->base),"metatype and dot-self have their own wrappers");
 }
 {
  const auto r=parser.parse("func f() async throws(MyError) -> Int { return try await g() }\n");
  const auto* f=as<ast::File>(r.value);const auto* fn=f?as<ast::FunctionDecl>(f->statements[0]):nullptr;
  const auto* thrown=fn?as<ast::NamedType>(fn->thrownType):nullptr;
  const auto* ret=fn && fn->body.size()==1?as<ast::ReturnStmt>(fn->body[0]):nullptr;
  const auto* attempt=ret?as<ast::TryExpr>(ret->value):nullptr;const auto* wait=attempt?as<ast::AwaitExpr>(attempt->operand):nullptr;
  check(fn && fn->asyncKind=="async" && fn->throwsKind=="throws" && thrown && thrown->name=="MyError" && attempt && attempt->kind=="try" && wait && as<ast::CallExpr>(wait->operand),"typed effects and nested try/await preserve their operands");
 }
 {
  const auto r=parser.parse("let x = a + try? await f()\n");const auto* chain=as<ast::BinaryChainExpr>(initializer(r));
  const auto* op=chain && chain->items.size()==1?as<ast::BinaryOp>(chain->items[0]):nullptr;
  const auto* attempt=op?as<ast::TryExpr>(op->operand):nullptr;
  check(op && op->op=="+" && attempt && attempt->kind=="try?" && as<ast::AwaitExpr>(attempt->operand),"sequence operands use the same effect expressions as the first operand");
 }
 {
  const auto r=parser.parse("let s = \"  \\(value, radix: 16)   \"\n");
  const auto* s=as<ast::StringLit>(initializer(r));
  check(s && s->parts.size()==3 && s->parts[0]->text=="  " && s->parts[1]->interpolation && s->parts[1]->arguments.size()==2 && s->parts[1]->arguments[1]->label=="radix" && as<ast::IntLit>(s->parts[1]->arguments[1]->value) && s->parts[2]->text=="   ","string whitespace and interpolation argument labels are retained");
 }
 {
  const auto r=parser.parse("let `a b` = .foo.bar(1)\n");const auto* v=variable(r);const auto* p=v?as<ast::IdentifierPattern>(v->bindings[0]->pattern):nullptr;
  const auto* call=as<ast::CallExpr>(initializer(r));const auto* member=call?as<ast::MemberExpr>(call->callee):nullptr;const auto* base=member?as<ast::ImplicitMemberExpr>(member->base):nullptr;
  check(p && p->name=="a b" && member && member->name=="bar" && base && base->name=="foo","raw identifier values and implicit member postfix chains");
 }
 {
  const auto r=parser.parse("let x = if flag { 1 } else { 2 }\n");const auto* expr=as<ast::ControlExpr>(initializer(r));const auto* branch=expr?as<ast::IfStmt>(expr->statement):nullptr;
  check(branch && branch->conditions.size()==1 && branch->thenBranch && branch->elseBranch && branch->parent==expr,"an if expression owns its statement and branch bodies");
 }
 for(const char* text:{"let x = \"\\q\"\n","let x = \"\\u{D800}\"\n","let x = - -value\n","struct S { var x: Int { set async {} } }\n","if let (x,y) {}\n"}) {
  const auto r=parser.parse(text);const auto validation=cursiv::test::validationHelper(services,text);
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && !validation.diagnostics.empty(),"invalid source reaches the runtime validator");
 }
 std::printf("swift: %zu native syntax cases, complete AST ownership and runtime checks\n",std::size(cases));
 return ok;
}
}
