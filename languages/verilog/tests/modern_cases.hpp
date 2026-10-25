#pragma once
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "verilog/core/ast.hpp"
namespace verilog_audit {
template<class Language> bool modern_cases(const Language& language) {
  namespace ast=verilog::ast;
  const auto& parser=*language.language.parser;
  bool ok=true;unsigned checks=0;
  auto check=[&](bool condition,const char*message){++checks;if(!condition){std::fprintf(stderr,"verilog audit: %s\n",message);ok=false;}};
  struct Case {std::string_view name,source;};
  constexpr Case valid[]={
    {"ansi-param-inherited", R"VERILOG(module m #(parameter integer A=1,B=2,parameter C=3)(output [A:0] y); assign y=B; endmodule
)VERILOG"},
    {"ansi-port-inherited", R"VERILOG(module m(input signed [3:0] a,b,output [3:0] y); assign y=a+b; endmodule
)VERILOG"},
    {"attr-after-operator", R"VERILOG(module m; wire x=1 + (* source="*)" *) 2; endmodule
)VERILOG"},
    {"attr-unary", R"VERILOG(module m; wire x=~ (* source="*)" *) 1; endmodule
)VERILOG"},
    {"based-spaces", R"VERILOG(module m; wire [7:0] x=8 'h FF; endmodule
)VERILOG"},
    {"concat", R"VERILOG(module m; wire [7:0] x={2{2'b10,2'b01}}; endmodule
)VERILOG"},
    {"conditional-attributes", R"VERILOG(module m; wire x=1 ? (* n=1 *) 2:3; endmodule
)VERILOG"},
    {"conditional-params", R"VERILOG(module m #(parameter A=1,
`ifdef UNDEFINED
 parameter B=2,
`else
 parameter B=3,
`endif
 parameter C=4)(); endmodule
)VERILOG"},
    {"conditional-ports", R"VERILOG(module m(input a,
`ifdef UNDEFINED
 output b,
`else
 output c,
`endif
 output d); endmodule
)VERILOG"},
    {"empty-connections", R"VERILOG(module sub; endmodule module m; sub u(); endmodule
)VERILOG"},
    {"escaped-id", R"VERILOG(module \m.name (input \a.b , output \y.z ); assign \y.z = \a.b ; endmodule
)VERILOG"},
    {"escaped-keyword", R"VERILOG(module \module ; wire \wire ; endmodule
)VERILOG"},
    {"fork-null", R"VERILOG(module m; initial fork ; ; join endmodule
)VERILOG"},
    {"function-inherited-port", R"VERILOG(module m; function [7:0] f(input [7:0] a,b); f=a+b; endfunction wire [7:0] x=f(1,2); endmodule
)VERILOG"},
    {"function-null", R"VERILOG(module m; function f; input x; ; endfunction endmodule
)VERILOG"},
    {"function-sign", R"VERILOG(module m; function signed [7:0] f(input [7:0] a); f=a; endfunction wire [7:0] x=f(1); endmodule
)VERILOG"},
    {"generate-block", R"VERILOG(module m; genvar i; generate for(i=0;i<2;i=i+1) begin:g wire [i:0] x; end endgenerate endmodule
)VERILOG"},
    {"generate-empty", R"VERILOG(module m; generate if(1); else ; case(1) 1:; default:; endcase endgenerate endmodule
)VERILOG"},
    {"macro-module-name", R"VERILOG(`define MOD m
module `MOD; endmodule
)VERILOG"},
    {"module", R"VERILOG(module m; endmodule
)VERILOG"},
    {"null-always", R"VERILOG(module m; always #1 ; endmodule
)VERILOG"},
    {"null-block", R"VERILOG(module m; initial begin ; ; end endmodule
)VERILOG"},
    {"null-case", R"VERILOG(module m; initial case(1) 1:; default:; endcase endmodule
)VERILOG"},
    {"null-if", R"VERILOG(module m; initial if(1); else ; endmodule
)VERILOG"},
    {"null-loops", R"VERILOG(module m; integer i; initial begin for(i=0;i<2;i=i+1); while(i); repeat(2); forever #1 ; end endmodule
)VERILOG"},
    {"open-connections", R"VERILOG(module sub(input a,input b,output c); assign c=a; endmodule module m; sub u(,1,); endmodule
)VERILOG"},
    {"operator-attributes", R"VERILOG(module m; wire x=1 * (* n=1 *) 2 + (* n=2 *) 3; endmodule
)VERILOG"},
    {"port-holes", R"VERILOG(module m(a,,); input a; endmodule
)VERILOG"},
    {"power-assoc", R"VERILOG(module m; localparam N=2**3**2; endmodule
)VERILOG"},
    {"preprocessor-defined", R"VERILOG(`define YES
module m;
`ifdef YES
 wire active;
`else
 invalid tokens;
`endif
endmodule
)VERILOG"},
    {"preprocessor-else", R"VERILOG(module m;
`ifdef NOT_DEFINED
 wire hidden;
`else
 wire visible;
`endif
endmodule
)VERILOG"},
    {"preprocessor-multiline", R"VERILOG(`define ADD(a,b) ((a)+ \
(b))
module m; wire [3:0] x=`ADD(1,2); endmodule
)VERILOG"},
    {"preprocessor-number", R"VERILOG(`define N 3
module m; wire [3:0] x=`N; endmodule
)VERILOG"},
    {"real-separators-exp", R"VERILOG(module m; real x; initial x=1_00e+0_1; endmodule
)VERILOG"},
    {"real-separators-fixed", R"VERILOG(module m; real x; initial x=1_00.0_1; endmodule
)VERILOG"},
    {"real-separators", R"VERILOG(module m; real x; initial x=1_000.25_0e+1_0; endmodule
)VERILOG"},
    {"selections", R"VERILOG(module m(input [7:0] a,output [1:0] b); assign b=a[3+:2]; endmodule
)VERILOG"},
    {"specify-condition", R"VERILOG(module m(input a,b,output y); assign y=a; specify if(b) (a=>y)=1; ifnone(a=>y)=2; endspecify endmodule
)VERILOG"},
    {"specify-min-typ-max", R"VERILOG(module m(input a,output y); buf(y,a); specify (a=>y)=(1:2:3,2:3:4); endspecify endmodule
)VERILOG"},
    {"specify-path-edge", R"VERILOG(module m(input c,d,output q); assign q=d; specify (posedge c => (q +: d)) = (1,2); endspecify endmodule
)VERILOG"},
    {"specparam-pulse", R"VERILOG(module m(input a,output y); assign y=a; specify specparam PATHPULSE$= (1,2); endspecify endmodule
)VERILOG"},
    {"string-escapes", R"VERILOG(module m; initial $display("a\"b\\c\n"); endmodule
)VERILOG"},
    {"string-newline", R"VERILOG(module m; initial $display("a\
b"); endmodule
)VERILOG"},
    {"system-empty-all", R"VERILOG(module m; initial $display(,); endmodule
)VERILOG"},
    {"system-empty-first", R"VERILOG(module m; initial $display(,1); endmodule
)VERILOG"},
    {"system-empty-last", R"VERILOG(module m; initial $display("x",); endmodule
)VERILOG"},
    {"system-empty-middle", R"VERILOG(module m; initial $display("x",,1); endmodule
)VERILOG"},
    {"task-inherited-port", R"VERILOG(module m; task t(input a,b,output c); c=a+b; endtask endmodule
)VERILOG"},
    {"timing-condition", R"VERILOG(module m(input c,d); specify $setup(d &&& c,posedge c,1); endspecify endmodule
)VERILOG"},
    {"udp-initial", R"VERILOG(primitive p(q,c,d); output q; reg q; input c,d; initial q=1'b0; table (01)0:?:0; (01)1:?:1; ??:?:-; endtable endprimitive module m; wire q; p u(q,1'b0,1'b0); endmodule
)VERILOG"},
  };
  for(const auto& entry:valid){
    auto r=parser.parse(entry.source);
    const bool clean=r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if(!clean)std::fprintf(stderr,"verilog native form: %.*s\n",int(entry.name.size()),entry.name.data());
    check(clean,"native-positive form parses without repair");
  }
  auto root = [&](const cursiv::ParseResult& r) { return cursiv::ast_cast<ast::SourceText>(r.value); };
  auto module = [&](const cursiv::ParseResult& r, std::size_t i=0) {
    const auto* source=root(r);
    return source && source->descriptions.size()>i ? cursiv::ast_cast<ast::ModuleDecl>(source->descriptions[i]) : nullptr;
  };
  auto integer = [](const ast::Expr* e,std::string_view spelling) {
    auto x=cursiv::ast_cast<ast::IntLit>(e);return x && x->text==spelling;
  };
  {
    auto r=parser.parse("module m; real x; initial x=1_000.25_0e+1_0; endmodule");
    auto m=module(r);auto i=m && m->items.size()==2 ? cursiv::ast_cast<ast::InitialConstruct>(m->items[1]):nullptr;
    auto a=i?cursiv::ast_cast<ast::AssignStmt>(i->body):nullptr;
    auto v=a?cursiv::ast_cast<ast::RealLit>(a->value):nullptr;
    check(v && v->text=="1_000.25_0e+1_0", "whole real spelling with separators");
    check(v && v->parent==a && a->parent==i && i->parent==m, "real assignment parents");
  }
  {
    auto r=parser.parse("module \\m.name (input \\a.b ,output \\y.z ); assign \\y.z = \\a.b ; endmodule");
    auto m=module(r);auto p=m && m->ports.size()==2 ? cursiv::ast_cast<ast::PortDecl>(m->ports[0]):nullptr;
    check(m && m->name=="m.name" && p && p->items.size()==1 && p->items[0]->name=="a.b", "escaped names omit lexical slash and terminator");
    check(p && p->parent==m && p->items[0]->parent==p, "escaped port parents");
  }
  {
    auto r=parser.parse("module m; wire x=1 + (* source=\"*)\" *) 2 * (* a=3 *) 3; endmodule");
    auto m=module(r);auto n=m && m->items.size()==1 ? cursiv::ast_cast<ast::NetDecl>(m->items[0]):nullptr;
    auto a=n && n->items.size()==1 ? cursiv::ast_cast<ast::BinaryExpr>(n->items[0]->init):nullptr;
    auto b=a?cursiv::ast_cast<ast::BinaryExpr>(a->right):nullptr;
    check(a && a->op=="+" && integer(a->left,"1") && b && b->op=="*" && integer(b->left,"2") && integer(b->right,"3"), "operator attributes do not enter operator value");
    check(a && a->parent==n->items[0] && b && b->parent==a && b->left->parent==b, "attributed expression parents");
  }
  {
    auto r=parser.parse("module m; initial begin ; ; end endmodule");
    auto m=module(r);auto i=m && m->items.size()==1?cursiv::ast_cast<ast::InitialConstruct>(m->items[0]):nullptr;
    auto b=i?cursiv::ast_cast<ast::BlockStmt>(i->body):nullptr;
    check(b && b->stmts.size()==2 && cursiv::ast_cast<ast::NullStmt>(b->stmts[0]) && cursiv::ast_cast<ast::NullStmt>(b->stmts[1]), "both empty statements retained");
    check(b && b->parent==i && b->stmts[0]->parent==b && b->stmts[1]->parent==b, "empty statement parents");
  }
  {
    auto r=parser.parse("primitive p(q,c,d); output q; reg q; input c,d; initial q=1'b0; table (01)0:?:0; (01)1:?:1; ??:?:-; endtable endprimitive");
    auto d=root(r);auto u=d && d->descriptions.size()==1?cursiv::ast_cast<ast::UdpDecl>(d->descriptions[0]):nullptr;
    auto n=u?cursiv::ast_cast<ast::NumberLit>(u->initVal):nullptr;
    check(u && u->portNames.size()==3 && u->portNames[0]=="q" && u->portNames[1]=="c" && u->portNames[2]=="d" && u->initName=="q", "UDP header order and initial target retained");
    check(n && n->size=="1" && n->base=="'b" && n->value=="0" && n->parent==u, "UDP initial value and parent");
    check(u && u->rows.size()==3 && u->rows[0]->inputs.size()==2 && u->rows[0]->inputs[0]=="(01)" && u->rows[0]->current=="?" && u->rows[0]->next=="0" && u->rows[0]->parent==u, "UDP table columns and parents");
  }
  {
    auto r=parser.parse("module m(input c,d); specify $setup(d &&& c,posedge c,1); endspecify endmodule");
    auto m=module(r);auto s=m && m->items.size()==1?cursiv::ast_cast<ast::SpecifyBlock>(m->items[0]):nullptr;
    auto t=s && s->items.size()==1?cursiv::ast_cast<ast::SystemTimingCheck>(s->items[0]):nullptr;
    auto d=t && t->args.size()==3?cursiv::ast_cast<ast::HierIdent>(t->args[0]->value):nullptr;
    auto c=t && t->args.size()==3?cursiv::ast_cast<ast::HierIdent>(t->args[0]->condition):nullptr;
    check(t && t->kind=="$setup" && d && d->name=="d" && c && c->name=="c" && t->args[1]->control=="posedge" && integer(t->args[2]->value,"1"), "timing condition is distinct from logical-and and reduction");
    check(t && t->parent==s && d && d->parent==t->args[0] && c && c->parent==t->args[0] && t->args[0]->parent==t, "timing argument parents");
  }
  {
    auto r=parser.parse("module m(input c,d); specify $setuphold(c,d,1,2,,,c); endspecify endmodule");
    auto m=module(r);auto s=m && m->items.size()==1?cursiv::ast_cast<ast::SpecifyBlock>(m->items[0]):nullptr;
    auto t=s && s->items.size()==1?cursiv::ast_cast<ast::SystemTimingCheck>(s->items[0]):nullptr;
    check(t && t->args.size()==7 && !t->args[4]->value && !t->args[5]->value && t->args[6]->value, "timing optional argument positions retained");
  }
  {
    auto r=parser.parse("module m(input a,output y); specify (a=>y)=(1:2:3,2:3:4); endspecify endmodule");
    auto m=module(r);auto s=m && m->items.size()==1?cursiv::ast_cast<ast::SpecifyBlock>(m->items[0]):nullptr;
    auto path=s && s->items.size()==1?cursiv::ast_cast<ast::PathDecl>(s->items[0]):nullptr;
    auto d=path && path->delays.size()==2?cursiv::ast_cast<ast::MinTypMaxExpr>(path->delays[0]):nullptr;
    check(d && integer(d->min,"1") && integer(d->typ,"2") && integer(d->max,"3"), "path delay minimum typical maximum retained");
    check(d && d->parent==path && d->typ->parent==d, "path delay parents");
  }
  {
    auto r=parser.parse("module m; specify specparam PATHPULSE$a$y=(1,2); endspecify endmodule");
    auto m=module(r);auto s=m && m->items.size()==1?cursiv::ast_cast<ast::SpecifyBlock>(m->items[0]):nullptr;
    auto p=s && s->items.size()==1?cursiv::ast_cast<ast::ParamDecl>(s->items[0]):nullptr;
    auto v=p && p->items.size()==1?cursiv::ast_cast<ast::TimingLimits>(p->items[0]->init):nullptr;
    check(p && p->kind=="specparam" && p->items[0]->name=="PATHPULSE$a$y" && v && v->values.size()==2 && integer(v->values[0],"1") && integer(v->values[1],"2"), "pulse limit spelling and both values retained");
    check(v && v->parent==p->items[0] && v->values[0]->parent==v, "pulse limit parents");
  }
  {
    constexpr std::string_view source="`define YES\nmodule m;\n`ifdef YES\n wire active;\n`else\n invalid tokens;\n`endif\nendmodule\n";
    auto r=parser.parse(source);auto d=root(r);auto directive=d && d->descriptions.size()==2?cursiv::ast_cast<ast::PreprocessorItem>(d->descriptions[0]):nullptr;
    auto m=module(r,1);auto conditional=m && m->items.size()==1?cursiv::ast_cast<ast::PreprocessorItem>(m->items[0]):nullptr;
    check(directive && directive->text=="`define YES" && conditional && conditional->text=="`ifdef YES\n wire active;\n`else\n invalid tokens;\n`endif", "conditional source retains active and inactive branches verbatim");
    check(directive && directive->parent==d && conditional && conditional->parent==m, "source directive parents");
  }
  {
    auto r=parser.parse("`define ADD(a,b) ((a)+ \\\n(b))\nmodule m; wire [3:0] x=`ADD(1,2); endmodule");
    auto d=root(r);auto directive=d && d->descriptions.size()==2?cursiv::ast_cast<ast::PreprocessorItem>(d->descriptions[0]):nullptr;
    auto m=module(r,1);auto n=m && m->items.size()==1?cursiv::ast_cast<ast::NetDecl>(m->items[0]):nullptr;
    auto macro=n && n->items.size()==1?cursiv::ast_cast<ast::PreprocessorExpr>(n->items[0]->init):nullptr;
    check(directive && directive->text=="`define ADD(a,b) ((a)+ \\\n(b))" && macro && macro->text=="`ADD(1,2)", "multiline definition and expression invocation retained");
    check(macro && macro->parent==n->items[0], "macro expression parent");
  }
  {
    auto r=parser.parse("config cfg; design work.top; instance top.u use work.cell1:config; endconfig");
    auto d=root(r);auto c=d && d->descriptions.size()==1?cursiv::ast_cast<ast::ConfigDecl>(d->descriptions[0]):nullptr;
    check(c && c->designCells.size()==1 && c->designCells[0]=="work.top" && c->rules.size()==1 && c->rules[0]->target=="top.u" && c->rules[0]->useCell=="work.cell1", "config library and full hierarchy retained");
  }
  {
    auto r=parser.parse("module m; function signed [7:0] f(input [7:0] a); f=a; endfunction endmodule");
    auto m=module(r);auto f=m && m->items.size()==1?cursiv::ast_cast<ast::FunctionDecl>(m->items[0]):nullptr;
    check(f && f->isSigned && f->range && f->ansiPorts.size()==1, "function signed return retained");
  }
  {
    auto r=parser.parse("module m; initial $display(,1,,); endmodule");
    auto m=module(r);auto i=m && m->items.size()==1?cursiv::ast_cast<ast::InitialConstruct>(m->items[0]):nullptr;
    auto t=i?cursiv::ast_cast<ast::TaskEnableStmt>(i->body):nullptr;
    auto c=t?cursiv::ast_cast<ast::CallExpr>(t->call):nullptr;
    check(c && c->args.size()==4 && cursiv::ast_cast<ast::OmittedArg>(c->args[0]) && integer(c->args[1],"1") && cursiv::ast_cast<ast::OmittedArg>(c->args[2]) && cursiv::ast_cast<ast::OmittedArg>(c->args[3]), "system call omitted argument positions retained");
    check(c && c->parent==t && c->args[0]->parent==c && c->args[1]->parent==c, "omitted argument parents");
  }
  {
    auto r=parser.parse("module m; initial $display(); endmodule");
    auto m=module(r);auto i=m && m->items.size()==1?cursiv::ast_cast<ast::InitialConstruct>(m->items[0]):nullptr;
    auto t=i?cursiv::ast_cast<ast::TaskEnableStmt>(i->body):nullptr;
    auto c=t?cursiv::ast_cast<ast::CallExpr>(t->call):nullptr;
    check(c && c->args.empty(), "empty call has zero arguments");
  }
  {
    auto r=parser.parse("module m #(parameter A=1,\n`ifdef X\n parameter B=2,\n`else\n parameter B=3,\n`endif\n parameter C=4)(); endmodule");
    auto m=module(r);auto d=m && m->params.size()==3?cursiv::ast_cast<ast::PreprocessorItem>(m->params[1]):nullptr;
    auto last=m && m->params.size()==3?cursiv::ast_cast<ast::ParamDecl>(m->params[2]):nullptr;
    check(d && d->text.find("parameter B=2,")!=std::string_view::npos && d->text.find("parameter B=3,")!=std::string_view::npos && last && last->items.size()==1 && last->items[0]->name=="C" && integer(last->items[0]->init,"4"), "conditional parameter island owns internal commas");
    check(d && d->parent==m && last && last->parent==m, "conditional parameter parents");
  }
  {
    auto r=parser.parse("module m(input a,\n`ifdef X\n output b,\n`else\n output c,\n`endif\n output d); endmodule");
    auto m=module(r);auto d=m && m->ports.size()==3?cursiv::ast_cast<ast::PreprocessorItem>(m->ports[1]):nullptr;
    auto last=m && m->ports.size()==3?cursiv::ast_cast<ast::PortDecl>(m->ports[2]):nullptr;
    check(d && last && last->items.size()==1 && last->items[0]->name=="d", "conditional port island owns internal commas");
    check(d && d->parent==m && last && last->parent==m, "conditional port parents");
  }
  {
    auto r=parser.parse("`define MOD m\nmodule `MOD; endmodule");auto m=module(r,1);
    check(m && m->name=="`MOD", "unexpanded macro in identifier retains source spelling");
  }
  {
    auto r=parser.parse("module m(a,,); input a; endmodule");auto m=module(r);
    auto first=m && m->ports.size()==3?cursiv::ast_cast<ast::PortItem>(m->ports[0]):nullptr;
    auto second=m && m->ports.size()==3?cursiv::ast_cast<ast::PortItem>(m->ports[1]):nullptr;
    auto last=m && m->ports.size()==3?cursiv::ast_cast<ast::PortItem>(m->ports[2]):nullptr;
    check(first && first->refs.size()==1 && second && second->refs.empty() && last && last->refs.empty(), "module header keeps middle and trailing empty ports");
    check(first && first->parent==m && second && second->parent==m && last && last->parent==m, "empty module port parents");
  }
  auto validate = [&](std::string source,std::string_view code,unsigned count) {
    cursiv::Document d(cursiv::TextDocument::create("file:///audit.v","verilog",0,std::move(source)));
    d.language=&language.language;d.parseResult=parser.parse(d.textDocument().getText());
    if(d.parseResult.value)d.parseResult.value->parent=&d;
    check(d.parseResult.accepted && !d.parseResult.cst().recovered(),"invalid semantic fixture parses without repair");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(d,diagnostics);
    unsigned found=0;
    for(const auto& diagnostic:diagnostics)
      if(diagnostic.code && diagnostic.code->isString() && diagnostic.code->string()==code)++found;
    check(found==count,"runtime validator code and count");
  };
  validate("module m; initial f()=1; endmodule","verilog.assignment-target",1);
  validate("module m; reg x; initial x=1; endmodule","verilog.assignment-target",0);
  validate("module m; function f; f=1; endfunction endmodule","verilog.function-input",1);
  validate("module m; function f(input x); f=x; endfunction endmodule","verilog.function-input",0);
  validate("module m; specify $setup(1,,3); endspecify endmodule","verilog.timing-arguments",1);
  validate("module m; specify $setup(1,2,3); endspecify endmodule","verilog.timing-arguments",0);
  validate("module m; initial ; endmodule","verilog.initial-statement",1);
  validate("module m; reg x; initial force x; endmodule","verilog.procedural-value",1);
  validate("module m; reg x; initial release x=1; endmodule","verilog.procedural-value",1);
  validate("module m; reg x; initial force x=1; endmodule","verilog.procedural-value",0);
  validate("module m; wire x=f(1,,3); endmodule","verilog.call-argument",1);
  validate("module m; initial $display(,1); endmodule","verilog.call-argument",0);
  std::printf("verilog audit: %zu native forms, %u checks\n",std::size(valid),checks);
  return ok;
}
}  // namespace verilog_audit
