#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "typescript/core/ast.hpp"
namespace typescript_native_followup {
namespace ast=typescript::ast;
template<class T> const T* as(const cursiv::AstNode* n){return cursiv::ast_cast<T>(n);}
inline bool run(const cursiv::Parser& parser){
  bool ok=true;
  const auto check=[&](bool value,const char* message){if(!value){std::fprintf(stderr,"typescript native: %s\n",message);ok=false;}};
  const auto clean=[&](const auto& r,const char* source){
    const bool valid=r.accepted&&!r.cst().recovered()&&r.diagnostics.empty();
    if(!valid){std::fprintf(stderr,"typescript native: source failed: %s\n",source);for(const auto& d:r.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));}return valid;
  };
  for(const auto source:{"type T=A..B;","type T=typeof A..B;","namespace A..B {}","import * from \"mod\";"}){
    const auto r=parser.parse(source);check(!r.accepted||r.cst().recovered()||!r.diagnostics.empty(),source);
  }
  {
    const char* source="type T=| A extends B ? C : D; type U=& A extends B ? C : D;";
    const auto r=parser.parse(source);check(clean(r,source),"leading conditional type tokens");
    const auto* root=r.value_as<ast::Program>();check(root&&root->body.size()==2,"two conditional aliases");
    if(root&&root->body.size()==2)for(const auto* n:root->body){const auto* a=as<ast::TypeAliasDecl>(n);const auto* t=a?as<ast::ConditionalType>(a->type):nullptr;check(t&&t->checkType&&t->extendsType&&t->trueType&&t->falseType,"all four native conditional operands retained");}
  }
  {
    const char* source="const a=f<T>(); const b=obj.method<U>(); const c=(f)<V>(); const d=f?.<X>(); const e=a?.3:0;";
    const auto r=parser.parse(source);check(clean(r,source),"generic calls on arbitrary/optional callees and decimal conditional");
    const auto* root=r.value_as<ast::Program>();check(root&&root->body.size()==5,"five declarations retained");
    if(root&&root->body.size()==5){for(std::size_t i=0;i<4;++i){const auto* v=as<ast::VarDecl>(root->body[i]);const auto* c=v&&v->declarations.size()==1?as<ast::CallExpr>(v->declarations[0]->init):nullptr;check(c&&c->callee&&c->typeArgs.size()==1&&c->optional==(i==3),"call owns its generic argument and optional flag");}
      const auto* v=as<ast::VarDecl>(root->body[4]);const auto* c=v&&v->declarations.size()==1?as<ast::ConditionalExpr>(v->declarations[0]->init):nullptr;const auto* n=c?as<ast::NumberLit>(c->consequent):nullptr;check(n&&n->text==".3","?. followed by decimal digit is a conditional number");}
  }
  {
    const char* source="export default function(){return 1;} export default class{x=1;}";
    const auto r=parser.parse(source);check(clean(r,source),"anonymous default declarations");
    const auto* root=r.value_as<ast::Program>();const auto* f=root&&root->body.size()==2?as<ast::ExportStmt>(root->body[0]):nullptr;const auto* c=root&&root->body.size()==2?as<ast::ExportStmt>(root->body[1]):nullptr;
    check(f&&f->isDefault&&!f->defaultExpr&&as<ast::FunctionDecl>(f->declaration)&&c&&c->isDefault&&!c->defaultExpr&&as<ast::ClassDecl>(c->declaration),"anonymous exports preserve declaration classes");
  }
  {
    const char* source="import Foo from \"m\"; import Bar,{A as B} from \"n\"; import * as M from \"p\";";
    const auto r=parser.parse(source);check(clean(r,source),"factored import clauses");const auto* root=r.value_as<ast::Program>();
    const auto* a=root&&root->body.size()==3?as<ast::ImportStmt>(root->body[0]):nullptr;const auto* b=root&&root->body.size()==3?as<ast::ImportStmt>(root->body[1]):nullptr;const auto* c=root&&root->body.size()==3?as<ast::ImportStmt>(root->body[2]):nullptr;
    check(a&&a->specifiers.size()==1&&a->specifiers[0]->local=="Foo"&&!a->specifiers[0]->isNamespace&&b&&b->specifiers.size()==2&&b->specifiers[1]->imported=="A"&&b->specifiers[1]->local=="B"&&c&&c->specifiers.size()==1&&c->specifiers[0]->isNamespace&&c->specifiers[0]->local=="M","default/named/namespace import AST roles retained");
  }
  {
    const char* source="const x=a\n!b; const y=async\nz=>z; class C { #foo; has(obj:any){return #foo in obj;} }";
    const auto r=parser.parse(source);check(clean(r,source),"newline boundaries and private identifier in");bool privateId=false,arrow=false,nonnull=false;
    for(const auto* n:r.nodes){if(const auto* id=as<ast::Ident>(n))privateId|=id->name=="#foo";if(const auto* f=as<ast::ArrowFunctionExpr>(n))arrow|=!f->isAsync;nonnull|=as<ast::NonNullExpr>(n)!=nullptr;}
    check(privateId&&arrow&&!nonnull,"private identifier text and separate newline expressions retained");
  }
  {
    const char* source="type T={+ readonly [K in keyof X] - ?: X[K]};";
    const auto r=parser.parse(source);check(clean(r,source),"spaced mapped type signs");
    const auto* root=r.value_as<ast::Program>();const auto* a=root&&root->body.size()==1?as<ast::TypeAliasDecl>(root->body[0]):nullptr;const auto* m=a?as<ast::MappedType>(a->type):nullptr;
    check(m&&m->readonlyMod=="+readonly"&&m->optionalMod=="-?","mapped signs preserve normalized modifier values");
  }
  return ok;
}
}
