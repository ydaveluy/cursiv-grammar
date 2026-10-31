#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "squirrel/core/CoreServices.hpp"

namespace squirrel_native_cases {
namespace ast = squirrel::ast;

template<class Services, class Check>
bool inspect(const Services& services, const char* id, const std::string& source, Check check) {
  const auto parsed = services.parser->parse(source);
  const auto* root = parsed.template value_as<ast::Chunk>();
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !root || parsed.cst().text() != source) {
    std::fprintf(stderr,"squirrel: %s: parse failed: %s\n", id, source.c_str());
    return false;
  }
  bool owned = !root->parent;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second || node.cstNodeId >= parsed.cst().size()) { owned=false; return; }
    const auto& span=parsed.cst()[node.cstNodeId];
    owned &= span.begin<=span.end && span.end<=source.size();
    services.astReflection->visitMembers(node,[&](std::string_view,std::uint32_t,cursiv::AstNode& child){
      owned &= child.parent==&node && parsed.cst().contains(node.cstNodeId,child.cstNodeId);
      self(self,child);
    });
  };
  walk(walk,*root);
  if (owned && seen.size()==parsed.nodes.size() && check(*root)) return true;
  std::fprintf(stderr,"squirrel: %s: AST/parent regression: %s\n",id,source.c_str());
  return false;
}
inline const ast::Expr* rhs(const ast::Chunk& root) {
  const auto* local=root.body.size()==1 ? cursiv::ast_cast<ast::LocalVarDecl>(root.body[0]) : nullptr;
  return local && local->vars.size()==1 ? local->vars[0]->init : nullptr;
}
inline bool run() {
  const auto language=cursiv::create_language<squirrel::SquirrelCoreServices,
      squirrel::SquirrelSharedCoreServices,squirrel::install_core_module>();
  const auto& services=language.language;
  struct Case {const char* id; const char* source;};
  const Case positives[]={
    {"CR-expression-space","local x=1\r+2;"},
    {"CR-index-space","local x=a\r[0];"},
    {"CR-comment-continues","//comment\rhidden=1;"},
    {"CR-return-value","function f(){return\r42;}"},
    {"this","local x=this;"},
    {"base","local x=base;"},
    {"source-line","local x=__LINE__;"},
    {"source-file","local x=__FILE__;"},
    {"rawcall","local x=rawcall(f,this,3);"},
    {"rawcall-initializers","local x=rawcall(f,this,3){slot=1,[\"key\"]=2};"},
    {"call-initializers","local x=f(){slot=1,[1,2]=3,};"},
    {"initializer-constructor-name","local x=f(){constructor=1};"},
    {"call-optional-comma","local x=f(a b, c);"},
    {"call-comma-newline","local x=f(a\n,b);"},
    {"call-newline","local x=f\n(1);"},
    {"member-newline","local x=this\n.\nfield;"},
    {"table-separators","local t={a=1 b=2, c=3,};"},
    {"table-constructor","local t={constructor(){return this;}};"},
    {"quoted-constructor-key","local t={\"constructor\":1};"},
    {"class-simple","class C extends Base {x=1;y=2}"},
    {"class-computed-target","class slots[\"C\"] extends flag ? A : B {}"},
    {"class-root-target","class ::ns.C extends (chooseBase()) {}"},
    {"class-call-target-member","class getRoot().C extends bases[i] {}"},
    {"class-expression","local C=class extends bases[i]{value=1};"},
    {"class-base-runtime-type","class C extends 1 {}"},
    {"class-members-separators","class C{a=1 b=2; function f(){return this;}}"},
    {"closed-class-separator","class C{} local x=1;"},
    {"closed-block-separator","{} local x=1;"},
    {"enum-newlines-optional-comma","enum\nE\n{\nA\nB\n=\n2,\n} local x=1;"},
    {"constructor-name-contexts","local constructor=1; function f(constructor){return constructor;}"},
    {"constructor-property","local x=o.constructor;"},
    {"enum-constructor-name","enum constructor{constructor,value}"},
    {"default-parameter-newlines","function f(\na\n=\n1\n,\nb\n=\n2\n){return a+b;}"},
    {"return-then-array","function f(){return\n[1];}"},
    {"anonymous-class-for-expression","for(class{};;);"},
    {"parenthesized-class-expression","(class{});"},
    {"parenthesized-function-expression","(function(){});"},
  };
  for(const auto& c:positives)
    if(!inspect(services,c.id,c.source,[](const ast::Chunk&){return true;}))return false;
  const Case negatives[]={
    {"CR-not-statement-end","local a=1\rlocal b=2;"},
    {"formfeed-not-whitespace","local\fx=1;"},
    {"call-trailing-comma","local x=f(1,);"},
    {"call-trailing-comma-newline","local x=f(1,\n);"},
    {"rawcall-trailing-comma","local x=rawcall(f,this,);"},
    {"table-leading-comma","local t={,a=1};"},
    {"table-repeated-comma","local t={a=1,,};"},
    {"unquoted-constructor-field","local t={constructor=1};"},
    {"class-leading-semicolon","class C{;a=1}"},
    {"class-repeated-semicolon","class C{a=1;;}"},
    {"enum-leading-comma","enum E{,A}"},
    {"enum-repeated-comma","enum E{A,,B}"},
    {"initializer-leading-comma","local x=f(){,a=1};"},
    {"initializer-repeated-comma","local x=f(){a=1,,};"},
    {"initializer-json-key","local x=f(){\"x\":1};"},
    {"class-json-key","class C{\"x\":1}"},
    {"direct-call-extends-body","class C extends f() {}"},
    {"bare-anonymous-function","function(){}"},
    {"bare-anonymous-class","class{}"},
    {"newline-index","local x=a\n[0];"},
    {"computed-slot-needs-comma","local t={a=1\n[0]=2};"},
  };
  const auto reject=[&](const char* id,const std::string& source){
    const auto r=services.parser->parse(source);
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty())return true;
    std::fprintf(stderr,"squirrel: %s: native-invalid syntax accepted: %s\n",id,source.c_str());return false;
  };
  for(const auto& c:negatives)if(!reject(c.id,c.source))return false;
  const char* reserved[]={
    "while",
    "do",
    "if",
    "else",
    "break",
    "continue",
    "return",
    "null",
    "function",
    "local",
    "for",
    "foreach",
    "in",
    "typeof",
    "base",
    "delete",
    "try",
    "catch",
    "throw",
    "clone",
    "yield",
    "resume",
    "switch",
    "case",
    "default",
    "this",
    "class",
    "extends",
    "instanceof",
    "true",
    "false",
    "static",
    "enum",
    "const",
    "__LINE__",
    "__FILE__",
    "rawcall",
  };
  for(const char* word:reserved) {
    if(!reject("reserved local",std::string("local ")+word+"=1;"))return false;
    if(!reject("reserved parameter",std::string("function f(")+word+"){}"))return false;
    if(!reject("reserved property",std::string("local x=o.")+word+";"))return false;
  }
  const char* names[]={
    "constructor",
    "constructorValue",
    "this_value",
    "baseClass",
    "while_value",
    "rawcaller",
    "__LINE__x",
    "__FILE__name",
    "This",
    "Base",
  };
  for(const char* name:names)
    if(!inspect(services,"identifier boundary",std::string("local ")+name+"=1;",
        [&](const ast::Chunk& root){
          const auto* decl=root.body.size()==1?cursiv::ast_cast<ast::LocalVarDecl>(root.body[0]):nullptr;
          return decl&&decl->vars.size()==1&&decl->vars[0]->name==name;
        }))return false;
  if(!inspect(services,"native atom kinds","local x=this;",[](const ast::Chunk& root){return cursiv::ast_cast<ast::ThisExpr>(rhs(root));}))return false;
  if(!inspect(services,"base atom","local x=base;",[](const ast::Chunk& root){return cursiv::ast_cast<ast::BaseExpr>(rhs(root));}))return false;
  for(const char* name:{"__LINE__","__FILE__"})
    if(!inspect(services,"source constant",std::string("local x=")+name+";",[&](const ast::Chunk& root){
          const auto* c=cursiv::ast_cast<ast::SourceConstant>(rhs(root));return c&&c->name==name;
        }))return false;
  if(!inspect(services,"rawcall AST","local x=rawcall(f,this,3){slot=1,[\"key\"]=2};",[](const ast::Chunk& root){
        const auto* call=cursiv::ast_cast<ast::RawCallExpr>(rhs(root));
        const auto* callee=call&&call->args.size()==3?cursiv::ast_cast<ast::Ident>(call->args[0]):nullptr;
        if(!callee||callee->name!="f"||!cursiv::ast_cast<ast::ThisExpr>(call->args[1])||call->initializers.size()!=2)return false;
        const auto& key=call->initializers[1]->key;
        const auto* expr=std::get_if<ast::Expr*>(&key);
        const auto* text=expr?cursiv::ast_cast<ast::StringLit>(*expr):nullptr;
        return text&&text->value=="key";
      }))return false;
  if(!inspect(services,"call args and computed initializer","local x=f(a b,c){[1,2]=3};",[](const ast::Chunk& root){
        const auto* call=cursiv::ast_cast<ast::CallExpr>(rhs(root));
        if(!call||call->args.size()!=3||call->initializers.size()!=1)return false;
        const auto* expr=std::get_if<ast::Expr*>(&call->initializers[0]->key);
        const auto* comma=expr?cursiv::ast_cast<ast::BinaryExpr>(*expr):nullptr;
        return comma&&comma->op==",";
      }))return false;
  if(!inspect(services,"table constructor","local t={constructor(){return this;}};",[](const ast::Chunk& root){
        const auto* table=cursiv::ast_cast<ast::TableExpr>(rhs(root));
        return table&&table->items.size()==1&&std::holds_alternative<ast::ConstructorDecl*>(table->items[0]);
      }))return false;
  if(!inspect(services,"class computed target and conditional base","class slots[\"C\"] extends flag?A:B{}",[](const ast::Chunk& root){
        const auto* decl=root.body.size()==1?cursiv::ast_cast<ast::ClassDecl>(root.body[0]):nullptr;
        const auto* target=decl?cursiv::ast_cast<ast::IndexExpr>(decl->target):nullptr;
        const auto* key=target?cursiv::ast_cast<ast::StringLit>(target->index):nullptr;
        return key&&key->value=="C"&&cursiv::ast_cast<ast::CondExpr>(decl->base);
      }))return false;
  if(!inspect(services,"root class target and grouped base","class ::ns.C extends (chooseBase()){}",[](const ast::Chunk& root){
        const auto* decl=root.body.size()==1?cursiv::ast_cast<ast::ClassDecl>(root.body[0]):nullptr;
        const auto* member=decl?cursiv::ast_cast<ast::MemberExpr>(decl->target):nullptr;
        const auto* name=member?cursiv::ast_cast<ast::Ident>(member->object):nullptr;
        const auto* base=decl?cursiv::ast_cast<ast::ParenExpr>(decl->base):nullptr;
        return name&&name->rootScoped&&name->name=="ns"&&member->field=="C"&&base&&cursiv::ast_cast<ast::CallExpr>(base->inner);
      }))return false;
  if(!inspect(services,"anonymous class base expression","local C=class extends bases[i]{};",[](const ast::Chunk& root){
        const auto* c=cursiv::ast_cast<ast::ClassExpr>(rhs(root));return c&&cursiv::ast_cast<ast::IndexExpr>(c->base);
      }))return false;
  if(!inspect(services,"closed class separator","class C{} local x=1;",[](const ast::Chunk& root){
        return root.body.size()==2&&cursiv::ast_cast<ast::ClassDecl>(root.body[0])&&cursiv::ast_cast<ast::LocalVarDecl>(root.body[1]);
      }))return false;
  if(!inspect(services,"enum newline and optional comma","enum\nE\n{\nA\nB\n=\n2,\n} local x=1;",[](const ast::Chunk& root){
        const auto* e=root.body.size()==2?cursiv::ast_cast<ast::EnumDecl>(root.body[0]):nullptr;
        return e&&e->items.size()==2&&e->items[0]->name=="A"&&!e->items[0]->value&&e->items[1]->name=="B"&&cursiv::ast_cast<ast::IntLit>(e->items[1]->value);
      }))return false;
  if(!inspect(services,"CR keeps line comment open","//comment\rhidden=1;",[](const ast::Chunk& root){return root.body.empty();}))return false;
  return true;
}
} // namespace squirrel_native_cases
