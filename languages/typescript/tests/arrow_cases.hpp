#pragma once

#include "yield_cases.hpp"

namespace typescript_arrow_cases {
namespace a = typescript::ast;
inline std::string body(const a::Expr* node) {
  if (const auto* call=cursiv::ast_cast<a::CallExpr>(node)) {
    std::string result="(call " + typescript_yield_cases::expression(call->callee);
    for (const auto* arg:call->args) result += " " + typescript_yield_cases::expression(arg);
    return result+")";
  }
  if (const auto* function=cursiv::ast_cast<a::FunctionExpr>(node))
    return function->isGenerator ? "generator" : "function";
  return typescript_yield_cases::expression(node);
}
inline bool run(const typescript::TypeScriptCoreServices& services) {
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message) {
    if(!value){std::fprintf(stderr,"typescript arrow %s: %s\n",name,message);ok=false;}
  };
  struct Case {const char* name;const char* source;bool parses;const char* params;const char* body;std::size_t yields,calls;bool generic;};
  // Official5.9.3 parser/AST05. LF-before-arrow is rejected here by the
  // grammar condition; native parser accepts then checkGrammarArrowFunction
  // reports1200. It is not compared as native parseFalse or runtime validity.
  const Case cases[]={
    {"arrow-assignment-rhs","function* g(){x=()=>yield(1);}",true,"","(call yield 1)",0,1,false},
    {"noin-arrow-consequent","for(using x=a?()=>b in c:d;;){}",true,"","(in b c)",0,0,false},
    {"noin-arrow-alternate","for(using x=a?b:()=>c in d;;){}",false,"","",0,0,false},
    {"noin-parenthesized-arrow","for(using x=(()=>a in b);;){}",true,"","(in a b)",0,0,false},
    {"noin-call-arrow","for(using x=f(()=>a in b);;){}",true,"","(in a b)",0,0,false},
    {"noin-new-arrow","for(using x=new F(()=>a in b);;){}",true,"","(in a b)",0,0,false},
    {"unary-arrow-invalid","const x=+()=>1;",false,"","",0,0,false},
    {"binary-arrow-invalid","const x=1+()=>2;",false,"","",0,0,false},
    {"parenthesized-binary-arrow","const x=1+(()=>2);",true,"","2",0,0,false},
    {"generic-arrow","const f=<T>(x:T)=>x;",true,"x","x",0,0,true},
    {"noin-generic-arrow","for(using f=<T>(x:T)=>x in y;;){}",false,"","",0,0,false},
    {"noin-generic-parenthesized-body","for(using f=<T>(x:T)=>(x in y);;){}",true,"x","(in x y)",0,0,true},
    {"generator-yield-single-param","function* g(){const f=yield=>1;}",false,"","",0,0,false},
    {"generator-yield-parenthesized-param","function* g(){const f=(yield)=>1;}",false,"","",0,0,false},
    {"arrow-linebreak-invalid","const f=(x)\n=>x;",false,"","",0,0,false},
    {"async-arrow-linebreak-invalid","function* g(){const f=async\n(x)=>yield(1);}",false,"","",0,0,false},
    {"ordinary-yield-arrow-param","const f=yield=>yield(1);",true,"yield","(call yield 1)",0,1,false},
    {"yield-arrow-operand","function* g(){const x=yield ()=>1;}",true,"","1",1,0,false},
    {"arrow-unambiguous-yield-body","const f=()=>yield 1;",true,"","(yield 1)",1,0,false},
    {"arrow-generator-body","const f=()=>function*(){yield 1;};",true,"","generator",1,0,false},
  };
  for(const auto& test:cases){
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty()&&parsed.value;
    check(clean==test.parses,test.name,"native syntax or explicit later grammar restriction");
    if(clean!=test.parses)for(const auto& d:parsed.diagnostics)
      std::fprintf(stderr,"  kind=%u element=%u bytes=%u..%u\n",unsigned(d.kind),d.element,d.begin,d.end);
    if(!clean)continue;
    check(parsed.cst().text()==test.source,test.name,"complete source retained");
    std::vector<const a::ArrowFunctionExpr*> arrows;
    std::size_t yields=0,calls=0;
    for(const auto* node:parsed.nodes){
      if(const auto* arrow=cursiv::ast_cast<a::ArrowFunctionExpr>(node))arrows.push_back(arrow);
      yields+=cursiv::ast_cast<a::YieldExpr>(node)!=nullptr;
      if(const auto* call=cursiv::ast_cast<a::CallExpr>(node))calls+=typescript_yield_cases::namedYield(call->callee);
    }
    check(arrows.size()==1&&yields==test.yields&&calls==test.calls,test.name,"native Arrow/Yield/call classification and context reset");
    if(arrows.size()!=1)continue;
    const auto* arrow=arrows[0];
    std::string parameters;
    for(const auto* param:arrow->params){
      if(!parameters.empty())parameters+=",";
      const auto* id=param?cursiv::ast_cast<a::Ident>(param->pattern):nullptr;
      parameters+=id?std::string(id->name):"?";
      check(param&&param->parent==arrow&&param->pattern&&param->pattern->parent==param,test.name,"parameter pattern has its direct typed owner");
    }
    check(parameters==test.params&&arrow->typeParams.size()==(test.generic?1u:0u)&&!arrow->isAsync,test.name,"parameter spelling/generic shape/async flag retained");
    if(test.generic&&arrow->typeParams.size()==1)check(arrow->typeParams[0]->name=="T",test.name,"generic type parameter spelling");
    check(!arrow->block&&arrow->exprBody&&arrow->exprBody->parent==arrow&&body(arrow->exprBody)==test.body,test.name,"native concise-body tree and ownership");
    const std::string_view name(test.name);
    bool placement=true;
    if(name=="arrow-assignment-rhs"){
      const auto* parent=cursiv::ast_cast<a::AssignExpr>(arrow->parent);
      placement=parent&&parent->value==arrow&&parent->op=="=";
    }else if(name=="noin-arrow-consequent"){
      const auto* parent=cursiv::ast_cast<a::ConditionalExpr>(arrow->parent);
      placement=parent&&parent->consequent==arrow;
    }else if(name=="parenthesized-binary-arrow"){
      const auto* parent=cursiv::ast_cast<a::BinaryExpr>(arrow->parent);
      placement=parent&&parent->right==arrow&&parent->op=="+";
    }else if(name=="noin-call-arrow"){
      const auto* parent=cursiv::ast_cast<a::CallExpr>(arrow->parent);
      placement=parent&&parent->args.size()==1&&parent->args[0]==arrow;
    }else if(name=="noin-new-arrow"){
      const auto* parent=cursiv::ast_cast<a::NewExpr>(arrow->parent);
      placement=parent&&parent->args.size()==1&&parent->args[0]==arrow;
    }else if(name=="yield-arrow-operand"){
      const auto* parent=cursiv::ast_cast<a::YieldExpr>(arrow->parent);
      placement=parent&&parent->argument==arrow;
    }
    check(placement,test.name,"Arrow stays at the native assignment/conditional/call/Yield operand level");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while(!work.empty()){
      const auto* node=work.back();work.pop_back();
      if(!seen.insert(node).second){check(false,test.name,"unique typed owner");continue;}
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child){
        check(child.parent==node&&node->cstNodeId<parsed.cst().size()&&child.cstNodeId<parsed.cst().size()&&parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"typed parents/CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size()&&!parsed.value->parent,test.name,"all allocated nodes reachable exactly once");
  }
  std::printf("typescript arrow: %zu native/context controls\n",std::size(cases));
  return ok;
}
} // namespace typescript_arrow_cases
