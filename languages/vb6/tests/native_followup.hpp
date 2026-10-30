#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "vb6/core/ast.hpp"
namespace vb6_native_followup {
namespace ast=vb6::ast;
inline bool run(const cursiv::Parser& parser){
 const char* source="Sub Test()\nOn Error GoTo Handler\n10 x = 12%\ny = 1!\nz = 1D-3\n"
   "ReDim a(1 To 4) As Long\nOpen \"sample\" As #fileHandle\nClose #fileHandle\n"
   "Debug.Print 1;2,3\nCall ReadExternal(obj.[Some Ident])\nResume 10\nHandler:\nResume 0\nEnd Sub\n";
 const auto r=parser.parse(source);
 if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()){
  std::fprintf(stderr,"vb6 native documentation cases failed: %s\n",source);
  for(const auto& d:r.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));return false;
 }
 bool numericLabel=false,resumeZero=false,resumeLine=false,foreign=false,redim=false,integer=false,single=false,exponent=false,handle=false,semicolon=false,comma=false,defaultMode=false;
 for(const auto* node:r.nodes){
  if(const auto* n=cursiv::ast_cast<ast::LabelStmt>(node))numericLabel|=n->name=="10";
  if(const auto* n=cursiv::ast_cast<ast::ResumeStmt>(node)){resumeZero|=n->label=="0";resumeLine|=n->label=="10";}
  if(const auto* n=cursiv::ast_cast<ast::MemberAccessExpr>(node))foreign|=n->name=="[Some Ident]";
  if(const auto* n=cursiv::ast_cast<ast::ReDimItem>(node))redim|=n->asType&&n->asType->name=="Long"&&n->bounds.size()==1;
  if(const auto* n=cursiv::ast_cast<ast::IntLit>(node))integer|=n->text=="12%";
  if(const auto* n=cursiv::ast_cast<ast::FloatLit>(node)){single|=n->text=="1!";exponent|=n->text=="1D-3";}
  if(const auto* n=cursiv::ast_cast<ast::FileNumberLit>(node))handle|=n->text=="#fileHandle";
  if(const auto* n=cursiv::ast_cast<ast::OpenStmt>(node))defaultMode|=!n->mode;
  if(const auto* n=cursiv::ast_cast<ast::Arg>(node)){semicolon|=n->separatorBefore==";";comma|=n->separatorBefore==",";}
 }
 if(!numericLabel||!resumeZero||!resumeLine||!foreign||!redim||!integer||!single||!exponent||!handle||!semicolon||!comma||!defaultMode){
  std::fprintf(stderr,"vb6 documentation AST: label%d resumeZero%d resumeLine%d foreign%d redim%d int%d single%d exponent%d handle%d semi%d comma%d mode%d\n",numericLabel,resumeZero,resumeLine,foreign,redim,integer,single,exponent,handle,semicolon,comma,defaultMode);return false;
 }
 const auto conditional=parser.parse("Sub Test()\n#If DEBUG Then\nx=1\n#Else\nx=2\n#End If\nEnd Sub\n");
 if(!conditional.accepted||conditional.cst().recovered()||!conditional.diagnostics.empty()){
  std::fprintf(stderr,"vb6: named file-handle terminal stole conditional compilation head\n");return false;
 }
 return true;
}
}
