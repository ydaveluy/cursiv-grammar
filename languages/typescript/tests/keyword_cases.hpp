#pragma once
#include "yield_cases.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace typescript_keyword_cases {
namespace a=typescript::ast;
inline bool run(const typescript::TypeScriptCoreServices& services){
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message){
    if(!value){std::fprintf(stderr,"typescript keyword %s: %s\n",name,message);ok=false;}
  };
  struct Case{const char* name;const char* source;bool nativeAccepted;const char* raw;};
  const Case cases[]={
    {"binding-if-fixed","const \\u0069f=1;",false,"\\u0069f"},
    {"binding-if-braced","const \\u{69}f=1;",false,"\\u{69}f"},
    {"binding-class-partial","const cl\\u0061ss=1;",false,"cl\\u0061ss"},
    {"case-sensitive-control","const \\u0049f=1;",true,"\\u0049f"},
    {"keyword-prefix-control","const \\u0069fx=1;",true,"\\u0069fx"},
    {"expression-if","const x=\\u0069f;",false,"\\u0069f"},
    {"parameter-if","function f(\\u0069f){}",false,"\\u0069f"},
    {"function-name-if","function \\u0069f(){}",false,"\\u0069f"},
    {"class-name-if","class \\u0069f{}",false,"\\u0069f"},
    {"typealias-name-if","type \\u0069f=number;",false,"\\u0069f"},
    {"interface-name-if","interface \\u0069f{}",false,"\\u0069f"},
    {"enum-name-if","enum \\u0069f{}",false,"\\u0069f"},
    {"typeparameter-if","function f<\\u0069f>(){}",false,"\\u0069f"},
    {"indexparameter-if","type T={[\\u0069f:string]:number};",false,"\\u0069f"},
    {"mappedparameter-if","type T={[\\u0069f in K]:number};",false,"\\u0069f"},
    {"label-if","\\u0069f: ;",false,"\\u0069f"},
    {"import-local-if","import {x as \\u0069f} from \"m\";",false,"\\u0069f"},
    {"import-default-if","import \\u0069f from \"m\";",false,"\\u0069f"},
    {"import-property-if","import {\\u0069f as x} from \"m\";",true,"\\u0069f"},
    {"export-property-if","export {x as \\u0069f};",true,"\\u0069f"},
    {"member-property-if","const x=o.\\u0069f;",true,"\\u0069f"},
    {"object-property-if","const x={\\u0069f:1};",true,"\\u0069f"},
    {"class-property-if","class C{\\u0069f=1;}",true,"\\u0069f"},
    {"type-property-if","type T={\\u0069f:number};",true,"\\u0069f"},
    {"enum-property-if","enum E{\\u0069f=1}",true,"\\u0069f"},
    {"computed-property-if","const x=o[\\u0069f];",false,"\\u0069f"},
    {"object-shorthand-if","const x={\\u0069f};",false,"\\u0069f"},
    {"binding-property-if","const {\\u0069f:x}=o;",true,"\\u0069f"},
    {"ordinary-a","const \\u0061=1;",true,"\\u0061"},
    {"contextual-async","const \\u0061sync=1;",true,"\\u0061sync"},
    {"future-reserved-implements","const \\u0069mplements=1;",true,"\\u0069mplements"},
    {"contextual-typeparameter","function f<\\u0074ype>(){}",true,"\\u0074ype"},
    {"type-reference-if","type T=\\u0069f;",true,"\\u0069f"},
    {"namespace-name-if","namespace \\u0069f{}",false,"\\u0069f"},
    {"namespace-export-if","export as namespace \\u0069f;",false,"\\u0069f"},
    {"private-property-if","class C{#\\u0069f=1;}",true,"#\\u0069f"},
    {"this-parameter","function f(this:C){}",true,"this"},
    {"escaped-this-parameter","function f(\\u0074his:C){}",true,"\\u0074his"},
    {"nested-namespace-property","namespace A.\\u0069f{}",true,"A.\\u0069f"},
  };
  for(const auto& test:cases){
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty()&&parsed.value;
    const auto validation=cursiv::test::validationHelper(services,std::string(test.source));
    std::size_t reserved=0;
    for(const auto& diagnostic:validation.diagnostics){
      reserved+=diagnostic.code&&*diagnostic.code==std::string_view("typescript.reserved-identifier");
      check(!diagnostic.message.starts_with("An error occurred during validation:"),test.name,"validation is total");
    }
    // A local typed check deliberately reports these native parser errors
    // after AST construction. Syntax rejections remain valid native negatives.
    check((clean&&reserved==0)==test.nativeAccepted,test.name,"native effective acceptance (parser versus local-validation stage qualified)");
    if(clean)check(reserved==(test.nativeAccepted?0u:1u),test.name,"one reserved-name diagnostic, none on native property/contextual/type-name controls");
    if(!clean){
      if(test.nativeAccepted)for(const auto& q:parsed.diagnostics)
        std::fprintf(stderr,"kind%u element%u bytes%u..%u\n",unsigned(q.kind),q.element,q.begin,q.end);
      continue;
    }
    check(parsed.cst().text()==test.source,test.name,"whole source unchanged");
    bool rawSeen=false;
    const auto field=[&](std::string_view value){rawSeen|=value==test.raw;};
    for(const auto* node:parsed.nodes){
      if(const auto* n=cursiv::ast_cast<a::Ident>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::FunctionDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::FunctionExpr>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::ClassDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::ClassExpr>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::InterfaceDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::TypeAliasDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::EnumDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::TypeParam>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::MappedType>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::ClassMember>(node))field(n->indexName);
      else if(const auto* n=cursiv::ast_cast<a::TypeMember>(node))field(n->indexName);
      else if(const auto* n=cursiv::ast_cast<a::NamespaceDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::NamespaceExportDecl>(node))field(n->name);
      else if(const auto* n=cursiv::ast_cast<a::ImportSpecifier>(node)){field(n->local);field(n->imported);}
      else if(const auto* n=cursiv::ast_cast<a::ExportSpecifier>(node)){field(n->local);field(n->exported);}
      else if(const auto* n=cursiv::ast_cast<a::LabeledStmt>(node))field(n->label);
      else if(const auto* n=cursiv::ast_cast<a::TypeRef>(node))field(n->name);
    }
    check(rawSeen,test.name,"typed AST preserves the exact escaped source spelling rather than replacing it with decoded text");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while(!work.empty()){
      const auto* node=work.back();work.pop_back();
      if(!seen.insert(node).second){check(false,test.name,"unique typed ownership");continue;}
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child){
        check(child.parent==node&&node->cstNodeId<parsed.cst().size()&&child.cstNodeId<parsed.cst().size()&&parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"every parent and CST containment");work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size()&&!parsed.value->parent,test.name,"all allocations reachable once");
  }
  std::printf("typescript keyword: %zu native/typed-validation controls\n",std::size(cases));
  return ok;
}
} // namespace typescript_keyword_cases
