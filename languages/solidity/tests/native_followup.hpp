#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "solidity/core/CoreServices.hpp"
#include "solidity/core/ast.hpp"
namespace native_followup {
inline bool run(){
  const auto language=cursiv::create_language<solidity::SolidityCoreServices,
      solidity::SoliditySharedCoreServices,solidity::install_core_module>();
  const auto& parser=*language.language.parser;
  for(const auto source:{"contract contract {}","contract switch {}","contract int256 {}",
      "contract C { uint return; }",
      "contract C { function f() public { assembly { let x := a . slot } } }","contract C { function f() public { assembly { let if := 1 } } }"}){
    const auto r=parser.parse(source);
    if(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty()){
      std::fprintf(stderr,"solidity native: reserved identifier accepted: %s\n",source);return false;
    }
  }
  const auto r=parser.parse("contract C { function f() public { assembly { "
      "let contract := 1 function public(x) -> y { y := x } "
      "let a := true let b := false let c := 0xFF let d := hex\"abcd\" "
      "let z := public(contract) let dotted.name := a.if } throw; } } contract layout {} contract class {}");
  if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()){
    std::fprintf(stderr,"solidity native: Yul/Solidity keyword modes\n");
    for(const auto& d:r.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
    return false;
  }
  bool trueValue=false,falseValue=false,soft=false,path=false,throwStmt=false;
  for(const auto* node:r.nodes){
    throwStmt|=cursiv::ast_cast<solidity::ast::ThrowStmt>(node)!=nullptr;
    if(const auto* literal=cursiv::ast_cast<solidity::ast::YulLiteral>(node)){
      trueValue|=literal->text=="true";falseValue|=literal->text=="false";
    }
    if(const auto* expr=cursiv::ast_cast<solidity::ast::YulPathExpr>(node))path|=expr->path=="a.if";
    if(const auto* decl=cursiv::ast_cast<solidity::ast::YulVariableDeclaration>(node))
      soft|=decl->variables.size()==1&&decl->variables[0]=="contract";
  }
  if(!trueValue||!falseValue||!soft||!path||!throwStmt){std::fprintf(stderr,"solidity native: booleans must be YulLiteral and soft Yul names retained\n");return false;}
  return true;
}
}
