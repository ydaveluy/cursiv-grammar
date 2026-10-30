#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "verilog/core/ast.hpp"
namespace verilog_native_followup {
inline bool run(const cursiv::Parser& parser) {
  namespace ast=verilog::ast;
  const char* source=
      "`define defineVALUE\nmodule m; `defineVALUE wire retained; endmodule\n"
      "module paths(input a,b,c,output y,z); specify "
      "(a=>y)=1; (a,b*>y,z)=2; (posedge a=>(y+:c))=3; "
      "(negedge a*>(z-:c))=4; endspecify endmodule\n"
      "config cfg; design m; instance m.x use lib.cell1; "
      "instance m.y use lib.cell2:config; endconfig\n";
  const auto r=parser.parse(source);
  if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()) {
    std::fprintf(stderr,"verilog native prefix/path/config regression failed: %s\n",source);
    for(const auto& d:r.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
    return false;
  }
  bool retained=false,cell=false,config=false,parallel=false,full=false,edgeParallel=false,edgeFull=false;
  for(const auto* node:r.nodes) {
    if(const auto* n=cursiv::ast_cast<ast::VarItem>(node))retained|=n->name=="retained";
    if(const auto* n=cursiv::ast_cast<ast::ConfigRule>(node)){
      cell|=n->useCell=="lib.cell1"&&!n->useConfig;
      config|=n->useCell=="lib.cell2"&&n->useConfig;
    }
    if(const auto* n=cursiv::ast_cast<ast::PathDecl>(node)){
      parallel|=n->pathOp=="=>"&&n->edge.empty()&&n->inputs.size()==1&&n->outputs.size()==1;
      full|=n->pathOp=="*>"&&n->edge.empty()&&n->inputs.size()==2&&n->outputs.size()==2;
      edgeParallel|=n->pathOp=="=>"&&n->edge=="posedge"&&n->polarity=="+"&&n->dataSource;
      edgeFull|=n->pathOp=="*>"&&n->edge=="negedge"&&n->polarity=="-"&&n->dataSource;
    }
  }
  if(!retained||!cell||!config||!parallel||!full||!edgeParallel||!edgeFull){
    std::fprintf(stderr,"verilog native AST: retained%d cell%d config%d parallel%d full%d edgeParallel%d edgeFull%d\n",retained,cell,config,parallel,full,edgeParallel,edgeFull);
    return false;
  }
  return true;
}
}
