#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "vhdl/core/ast.hpp"
namespace vhdl_native_followup {
template<class Language> bool run(const Language& language) {
  namespace ast=vhdl::ast;
  const auto& parser=*language.language.parser;
  const char* source=
      "entity e is end entity; architecture rtl of e is begin "
      "with k select ? s<=1 when 0,2 when others; "
      "p:postponed process begin "
      "with k select ? s<=1 when 0,2 when others; "
      "with k select ? v:=1 when 0,2 when others; "
      "case ? k is when others=>null; end case ?; "
      "wait; end postponed process; end architecture; "
      "package body pkg is procedure q is begin null; end procedure; end package body;";
  const auto r=parser.parse(source);
  auto diagnostics=[](const auto& result){
    for(const auto& d:result.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
  };
  if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()) {
    std::fprintf(stderr,"vhdl matching selected assignment failed: %s\n",source);diagnostics(r);return false;
  }
  bool concurrent=false,signal=false,variable=false,matchingCase=false,postponed=false,subprogram=false;
  for(const auto* node:r.nodes){
    if(const auto* n=cursiv::ast_cast<ast::SignalAssign>(node))concurrent|=n->questioned&&n->selector&&n->selWave&&n->selWave->entries.size()==2;
    if(const auto* n=cursiv::ast_cast<ast::SeqSignalAssign>(node))signal|=n->questioned&&n->selector&&n->selWave&&n->selWave->entries.size()==2;
    if(const auto* n=cursiv::ast_cast<ast::SeqVarAssign>(node))variable|=n->questioned&&n->selector&&n->selExpr&&n->selExpr->entries.size()==2;
    if(const auto* n=cursiv::ast_cast<ast::CaseStmt>(node))matchingCase|=n->questioned&&n->endQuestioned&&n->selector;
    if(const auto* n=cursiv::ast_cast<ast::ProcessStmt>(node))postponed|=n->postponed&&n->endPostponed;
    if(const auto* n=cursiv::ast_cast<ast::SubprogramBody>(node))subprogram|=n->endKind=="procedure"&&n->spec&&n->spec->kind=="procedure";
  }
  if(!concurrent||!signal||!variable||!matchingCase||!postponed||!subprogram){
    std::fprintf(stderr,"vhdl native AST: concurrent%d signal%d variable%d case%d postponed%d subprogram%d\n",concurrent,signal,variable,matchingCase,postponed,subprogram);return false;
  }
  for(const char* invalid:{
      "entity e is end; architecture rtl of e is begin with k select s<=?1 when others; end;",
      "entity e is end; architecture rtl of e is begin p:process begin with k select s<=?1 when others; wait; end process; end;",
      "entity e is end; architecture rtl of e is begin p:process begin with k select v:=?1 when others; wait; end process; end;",
      "package p is constant x:integer:=1+-2; end;",
      "package p is constant x:integer:=1-+2; end;",
      "package p is constant x:integer:=2**abs 3; end;",
      "package p is constant x:integer:=2**not 3; end;",
      "package p is constant x:integer:=1ns; end;"}){
    const auto bad=parser.parse(invalid);
    if(bad.accepted&&!bad.cst().recovered()&&bad.diagnostics.empty()){
      std::fprintf(stderr,"vhdl: invalid operator/physical spacing accepted: %s\n",invalid);return false;
    }
  }
  auto validate=[&](std::string_view text,std::string_view code,unsigned expected){
    cursiv::Document d(cursiv::TextDocument::create("file:///native-followup.vhd","vhdl",0,std::string(text)));
    d.language=&language.language;d.parseResult=parser.parse(d.textDocument().getText());
    if(!d.parseSucceeded()||d.parseRecovered()||!d.parseResult.diagnostics.empty()){
      std::fprintf(stderr,"vhdl validation fixture failed to parse: %.*s\n",int(text.size()),text.data());diagnostics(d.parseResult);return false;
    }
    if(d.parseResult.value)d.parseResult.value->parent=&d;
    std::vector<cursiv::Diagnostic> ds;language.language.documentValidator->validateDocument(d,ds);
    unsigned count=0;
    for(const auto& x:ds)if(x.code&&x.code->isString()&&x.code->string()==code)++count;
    if(count!=expected){std::fprintf(stderr,"vhdl validator %.*s expected%u actual%u: %.*s\n",int(code.size()),code.data(),expected,count,int(text.size()),text.data());return false;}
    return true;
  };
  return validate(source,"vhdl.case-matching",0)&&validate(source,"vhdl.process-end",0)&&validate(source,"vhdl.subprogram-end",0)&&
    validate("entity e is end; architecture rtl of e is begin p:process begin case?k is when others=>null; end case; wait; end process; end;","vhdl.case-matching",1)&&
    validate("entity e is end; architecture rtl of e is begin p:process begin case k is when others=>null; end case?; wait; end process; end;","vhdl.case-matching",1)&&
    validate("entity e is end; architecture rtl of e is begin p:process begin wait; end postponed process; end;","vhdl.process-end",1)&&
    validate("package body p is procedure q is begin null; end function; end package body;","vhdl.subprogram-end",1)&&
    validate("package p is constant x:integer:=1e-2; end;","vhdl.literal-exponent",1)&&
    validate("package p is constant x:integer:=16#A#e-2; end;","vhdl.literal-exponent",1)&&
    validate("package p is constant x:real:=1.0e-2; end;","vhdl.literal-exponent",0)&&
    validate("package p is constant x:real:=16#A.F#e-2; end;","vhdl.literal-exponent",0);
}
}
