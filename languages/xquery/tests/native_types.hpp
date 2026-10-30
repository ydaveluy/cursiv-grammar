#pragma once
#include <cstdio>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "xquery/core/ast.hpp"

namespace xquery_native_types {
template<class Language> bool run(const Language& language) {
  namespace ast=xquery::ast;
  const auto& parser=*language.language.parser;
  auto checkType=[&](std::string_view source,const char* evidence,auto predicate) {
    const auto result=parser.parse(source);
    const ast::ItemType* type=nullptr;
    for(const auto* node:result.nodes)
      if(const auto* test=cursiv::ast_cast<ast::InstanceOfExpr>(node);test&&test->type)
        type=test->type->itemType;
    if(!result.accepted||result.cst().recovered()||!result.diagnostics.empty()||!type||!predicate(*type)) {
      std::fprintf(stderr,"xquery type regression (%s): %.*s\n",evidence,int(source.size()),source.data());
      for(const auto& d:result.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
      if(type)std::fprintf(stderr,"  type=%.*s names=%zu children=%zu fields=%zu functionParams=%zu wildcard=%d root=%d nil=%d\n",int(type->name.size()),type->name.data(),type->names.size(),type->itemParams.size(),type->fields.size(),type->functionParams.size(),type->wildcard,type->rootSelector,type->nillable);
      return false;
    }
    return true;
  };
  // Fresh BaseX 12.4 parse() positives; no query evaluation or schema analysis.
  if(!checkType("1 instance of document-node(element(a))","BaseX12.4",[](const auto& t){
    return t.name=="document-node"&&t.names.empty()&&t.itemParams.size()==1&&
      t.itemParams[0]->name=="element"&&t.itemParams[0]->names.size()==1&&
      t.itemParams[0]->names[0]=="a"&&t.itemParams[0]->parent==&t;
  })||!checkType("1 instance of document-node(a|b)","BaseX12.4",[](const auto& t){
    return t.name=="document-node"&&t.itemParams.empty()&&t.names.size()==2&&t.names[0]=="a"&&t.names[1]=="b";
  })||!checkType("1 instance of element(a|b)","BaseX12.4",[](const auto& t){
    return t.name=="element"&&t.names.size()==2&&t.names[0]=="a"&&t.names[1]=="b"&&t.typeName.empty()&&!t.nillable;
  })||!checkType("1 instance of attribute(a|b)","BaseX12.4",[](const auto& t){
    return t.name=="attribute"&&t.names.size()==2&&t.typeName.empty();
  })||!checkType("1 instance of record()","BaseX12.4",[](const auto& t){
    return t.name=="record"&&!t.wildcard&&t.fields.empty();
  })||!checkType("1 instance of record(foo as xs:string)","BaseX12.4",[](const auto& t){
    return !t.wildcard&&t.fields.size()==1&&t.fields[0]->name=="foo"&&!t.fields[0]->isStatic&&
      t.fields[0]->type&&t.fields[0]->type->itemType&&t.fields[0]->type->itemType->name=="xs:string";
  })||!checkType("1 instance of record(\"some name\" as xs:string)","BaseX12.4",[](const auto& t){
    return t.fields.size()==1&&t.fields[0]->name=="some name";
  })||!checkType("1 instance of record(function as item())","BaseX12.4",[](const auto& t){
    return t.fields.size()==1&&t.fields[0]->name=="function";
  })||!checkType("1 instance of jnode()","BaseX12.4",[](const auto& t){
    return t.name=="jnode"&&!t.wildcard&&!t.rootSelector&&t.names.empty();
  })||!checkType("1 instance of jnode(*)","BaseX12.4",[](const auto& t){
    return t.name=="jnode"&&t.wildcard&&!t.rootSelector&&t.names.empty();
  })||!checkType("1 instance of jnode(())","BaseX12.4",[](const auto& t){
    return t.name=="jnode"&&!t.wildcard&&t.rootSelector&&t.names.empty();
  })||!checkType("1 instance of function(*)","BaseX12.4",[](const auto& t){
    return t.name=="function"&&t.wildcard&&t.functionParams.empty()&&!t.returnType;
  })||!checkType("1 instance of function(item(),xs:string?) as item()*","BaseX12.4",[](const auto& t){
    return !t.wildcard&&t.params.empty()&&t.functionParams.size()==2&&t.functionParams[0]->name.empty()&&
      t.functionParams[0]->type&&t.functionParams[1]->type&&t.functionParams[1]->type->occurrence=="?"&&
      t.returnType&&t.returnType->occurrence=="*";
  })||!checkType("1 instance of function($arg as item(),xs:string?) as item()*","BaseX12.4",[](const auto& t){
    return t.functionParams.size()==2&&t.functionParams[0]->name=="arg"&&t.functionParams[1]->name.empty()&&
      t.functionParams[0]->parent==&t&&t.functionParams[0]->type&&
      t.functionParams[0]->type->parent==t.functionParams[0]&&t.returnType&&t.returnType->parent==&t;
  }))return false;

  // QT4CG commit 3e9f3bd: record(*) and %static are not implemented by BaseX12.4;
  // typed XML tests require schema support. These are primary-derived AST cases.
  if(!checkType("1 instance of record(*)","QT4CG AnyRecordType",[](const auto& t){
    return t.name=="record"&&t.wildcard&&t.fields.empty();
  })||!checkType("1 instance of element(a,xs:string)","QT4CG ElementNodeType",[](const auto& t){
    return t.name=="element"&&t.names.size()==1&&t.names[0]=="a"&&t.typeName=="xs:string"&&!t.nillable;
  })||!checkType("1 instance of element(a|xs:string)","QT4CG ElementNodeType",[](const auto& t){
    return t.names.size()==2&&t.names[1]=="xs:string"&&t.typeName.empty()&&!t.nillable;
  })||!checkType("1 instance of element(a,xs:string?)","QT4CG ElementNodeType",[](const auto& t){
    return t.names.size()==1&&t.typeName=="xs:string"&&t.nillable;
  })||!checkType("1 instance of attribute(a,xs:string)","QT4CG AttributeNodeType",[](const auto& t){
    return t.names.size()==1&&t.names[0]=="a"&&t.typeName=="xs:string";
  })||!checkType("1 instance of document-node(schema-element(a))","QT4CG DocumentNodeType",[](const auto& t){
    return t.name=="document-node"&&t.itemParams.size()==1&&t.itemParams[0]->name=="schema-element"&&
      t.itemParams[0]->names.size()==1&&t.itemParams[0]->names[0]=="a";
  }))return false;
  const auto fields=parser.parse("declare record local:R(%static foo as xs:string := \"x\", plain as item()); 1");
  bool staticField=false,plainField=false;
  for(const auto* node:fields.nodes)if(const auto* field=cursiv::ast_cast<ast::FieldDecl>(node)){
    staticField|=field->name=="foo"&&field->isStatic&&field->def&&field->type;
    plainField|=field->name=="plain"&&!field->isStatic&&!field->def&&field->type;
  }
  if(!fields.accepted||fields.cst().recovered()||!fields.diagnostics.empty()||!staticField||!plainField){
    std::fprintf(stderr,"xquery primary ExtendedFieldDeclaration: static=%d plain=%d\n",staticField,plainField);return false;
  }
  // NCName is required for record fields, jnode names and PI test names.
  // All four exact sources are freshly rejected by BaseX12.4 parse().
  for(const auto source:{"1 instance of record(p:a as item())","1 instance of record(Q{urn}a as item())",
                         "1 instance of jnode(p:a)","1 instance of processing-instruction(p:a)"}){
    const auto invalid=parser.parse(source);
    if(invalid.accepted&&!invalid.cst().recovered()&&invalid.diagnostics.empty()){
      std::fprintf(stderr,"xquery: QName accepted where NCName is required: %s\n",source);return false;
    }
  }
  return true;
}
}  // namespace xquery_native_types
