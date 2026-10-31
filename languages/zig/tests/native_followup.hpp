#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "zig/core/ast.hpp"
namespace zig_native_followup {
template<class Services> bool run(const Services& services) {
  namespace ast=zig::ast;
  const auto& parser=*services.parser;
  // Exact literals freshly accepted by Zig0.17 fmt and ast-check.
  for(const auto source:{R"Z(const x='😀';)Z",R"Z(const x='\xff';)Z",
                         R"Z(const x='\u{d800}';)Z",R"Z(const x="\x00\u{1f600}";)Z",
                         R"Z(const @"\u{61}"=1;)Z",R"Z(const @"return"=1;)Z",
                         "const returnValue=1;","const x=0x1.2;"}) {
    const auto r=parser.parse(source);
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()){
      std::fprintf(stderr,"zig native literal rejected: %s\n",source);
      for(const auto& d:r.diagnostics)
        std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
      return false;
    }
  }
  // Fmt alone accepts many of these tokens; fresh ast-check rejects their values.
  for(const auto source:{R"Z(const x='ab';)Z",R"Z(const x='';)Z",R"Z(const x='\q';)Z",
                         R"Z(const x='\x1';)Z",R"Z(const x='\u{110000}';)Z",
                         R"Z(const x="\q";)Z",R"Z(const x="\x1";)Z",
                         R"Z(const x="\u{d800}";)Z",R"Z(const x="\u{110000}";)Z",
                         R"Z(const @""=1;)Z",R"Z(const @"\x00"=1;)Z",R"Z(const @"\q"=1;)Z",
                         "const x=\"a\tb\";","const x='\xff';",
                         "extern fn f(noalias ...) void;","extern fn f(comptime ...) void;"}) {
    const auto r=parser.parse(source);
    if(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty()){
      std::fprintf(stderr,"zig invalid native source accepted without diagnostics: %s\n",source);return false;
    }
  }
  const auto variadic=parser.parse("extern fn f(\n /// rest arguments\n ...\n) void;");
  bool documented=false;
  for(const auto* node:variadic.nodes)if(const auto* param=cursiv::ast_cast<ast::ParamDecl>(node))
    documented|=param->variadic&&param->docComment&&param->docComment->find("rest arguments")!=std::string_view::npos;
  if(!variadic.accepted||variadic.cst().recovered()||!variadic.diagnostics.empty()||!documented){
    std::fprintf(stderr,"zig: variadic parameter documentation lost\n");return false;
  }
  const auto r=parser.parse(R"Z(const @"return"="\x00\u{1f600}"; const c='😀';)Z");
  bool name=false,string=false,character=false;
  for(const auto* node:r.nodes){
    if(const auto* n=cursiv::ast_cast<ast::VarDeclProto>(node))name|=n->name==R"Z(@"return")Z";
    if(const auto* n=cursiv::ast_cast<ast::StringLiteral>(node))string|=n->text==R"Z("\x00\u{1f600}")Z";
    if(const auto* n=cursiv::ast_cast<ast::CharLiteral>(node))character|=n->text=="'😀'";
  }
  if(!name||!string||!character){std::fprintf(stderr,"zig literal spelling lost: name%d string%d char%d\n",name,string,character);return false;}
  auto validate=[&](std::string source,std::string_view code,unsigned expected){
    cursiv::Document d(cursiv::TextDocument::create("file:///native-followup.zig","zig",0,std::move(source)));
    d.language=&services;d.parseResult=parser.parse(d.textDocument().getText());
    if(!d.parseSucceeded()||d.parseRecovered()||!d.parseResult.diagnostics.empty()){
      const auto text=d.textDocument().getText();
      std::fprintf(stderr,"zig native validation source does not parse: %.*s\n",int(text.size()),text.data());return false;
    }
    if(d.parseResult.value)d.parseResult.value->parent=&d;
    std::vector<cursiv::Diagnostic> diagnostics;services.documentValidator->validateDocument(d,diagnostics);
    unsigned count=0;for(const auto& d:diagnostics)if(d.code&&d.code->isString()&&d.code->string()==code)++count;
    if(count!=expected){std::fprintf(stderr,"zig native validation %.*s expected%u actual%u\n",int(code.size()),code.data(),expected,count);return false;}
    return true;
  };
  return validate(R"Z(const T=struct { a:u8, @"a":u8, };)Z","zig.duplicate-field",2)&&
         validate(R"Z(const T=struct { a:u8, @"\u{61}":u8, };)Z","zig.duplicate-field",2)&&
         validate("const x=0x1pA;","zig.number",1)&&validate("const x=0x1.2;","zig.number",0)&&
         validate("const T=[]align(4) align(8) u8;","zig.type-modifier",1)&&
         validate("const T=*addrspace(.generic) addrspace(.generic) u8;","zig.type-modifier",1)&&
         validate("const T=*const const u8;","zig.type-modifier",1)&&
         validate("const T=*volatile volatile u8;","zig.type-modifier",1)&&
         validate("const T=[*]allowzero allowzero u8;","zig.type-modifier",1)&&
         validate("const T=*const volatile allowzero u8;","zig.type-modifier",0);
}
}
