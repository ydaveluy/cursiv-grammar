#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "vba/core/ast.hpp"
namespace vba_native_followup {
namespace ast=vba::ast;
inline bool run(const cursiv::Parser& parser){
 const char* source="Rem\nPrivate Declare PtrSafe Sub Raw Lib \"kernel32\" (ByVal buffer As Any)\n"
   "Private Sub Test(ParamArray values() As Variant)\nDim label As String * 12\n"
   "ReDim a(1 To 4) As Long\nLock #1\nLock #1, 2 To 3\nEnd Sub\n";
 const auto r=parser.parse(source);
 if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()){
  std::fprintf(stderr,"vba MS-VBAL cases failed: %s\n",source);
  for(const auto& d:r.diagnostics)
    std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
  return false;
 }
 bool any=false,paramarray=false,fixedString=false,redim=false,wholeFile=false,range=false;
 for(const auto* node:r.nodes){
  if(const auto* n=cursiv::ast_cast<ast::Param>(node)){
   any|=n->asType&&n->asType->name=="Any";
   paramarray|=n->paramArray&&n->isArray&&n->asType&&n->asType->name=="Variant";
  }
  if(const auto* n=cursiv::ast_cast<ast::TypeRef>(node))fixedString|=n->name=="String"&&n->stringLength;
  if(const auto* n=cursiv::ast_cast<ast::ReDimItem>(node))redim|=n->asType&&n->asType->name=="Long"&&n->bounds.size()==1;
  if(const auto* n=cursiv::ast_cast<ast::LockStmt>(node)){wholeFile|=!n->startRecord&&!n->endRecord;range|=n->startRecord&&n->endRecord;}
 }
 if(!any||!paramarray||!fixedString||!redim||!wholeFile||!range){std::fprintf(stderr,"vba MS-VBAL AST: Any%d ParamArray%d String%d ReDim%d Lock%d range%d\n",any,paramarray,fixedString,redim,wholeFile,range);return false;}
 for (const char* invalidSource : {
     "Sub Test()\nx=[]\nEnd Sub\n",
     "Sub Test()\nSelect Case x\nCase Is New Thing\nEnd Select\nEnd Sub\n"}) {
  const auto invalid=parser.parse(invalidSource);
  if(invalid.accepted&&!invalid.cst().recovered()&&invalid.diagnostics.empty()){
   std::fprintf(stderr,"vba: invalid FOREIGN-NAME/comparison accepted: %s\n",invalidSource);return false;
  }
 }
 return true;
}
}
