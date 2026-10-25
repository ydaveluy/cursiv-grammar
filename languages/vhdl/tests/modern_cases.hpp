#pragma once
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "vhdl/core/ast.hpp"
namespace vhdl_audit {
template<class Language> bool modern_cases(const Language& language) {
  namespace ast=vhdl::ast;
  const auto& parser=*language.language.parser;
  bool ok=true;unsigned checks=0;
  auto check=[&](bool condition,const char* message){++checks;if(!condition){std::fprintf(stderr,"vhdl audit: %s\n",message);ok=false;}};
  struct Case {std::string_view name,source;};
  constexpr Case valid[]={
    {"allocator",R"VHDL(package p is constant x : integer := new integer'(1); end package p;
)VHDL"},
    {"array-unbounded",R"VHDL(package p is type arr is array(integer range <>,natural range <>) of bit; end package;
)VHDL"},
    {"attribute-range",R"VHDL(package p is constant x : integer := x'range; end package p;
)VHDL"},
    {"attribute-signature",R"VHDL(package p is attribute mine of f[integer, integer, integer return integer] : function is 1; end package;
)VHDL"},
    {"based-real",R"VHDL(package p is constant x : integer := 16#A.F#e+2; end package p;
)VHDL"},
    {"bit-decimal-length",R"VHDL(package p is constant x : integer := 16D"255"; end package p;
)VHDL"},
    {"bit-decimal",R"VHDL(package p is constant x : integer := D"255"; end package p;
)VHDL"},
    {"bit-empty",R"VHDL(package p is constant x : integer := B""; end package p;
)VHDL"},
    {"bit-hex-logic",R"VHDL(package p is constant x : integer := X"0Z-"; end package p;
)VHDL"},
    {"bit-length",R"VHDL(package p is constant x : integer := 8X"0F"; end package p;
)VHDL"},
    {"bit-logic",R"VHDL(package p is constant x : integer := B"10XZ-"; end package p;
)VHDL"},
    {"bit-signed",R"VHDL(package p is constant x : integer := 8SX"F"; end package p;
)VHDL"},
    {"bit-unsigned",R"VHDL(package p is constant x : integer := 16UX"0F"; end package p;
)VHDL"},
    {"character",R"VHDL(package p is constant x : integer := 'a'; end package p;
)VHDL"},
    {"configuration-binding",R"VHDL(configuration cfg of e is for rtl for all:c use entity work.foo(rtl); end for; end for; end cfg;
)VHDL"},
    {"entity-instantiation",R"VHDL(entity e is end entity; architecture rtl of e is  begin u: entity work.foo(rtl) port map(a=>s); end architecture;
)VHDL"},
    {"entity",R"VHDL(entity e is end entity e;
)VHDL"},
    {"extended-identifier",R"VHDL(entity \some name\ is end entity \some name\;
)VHDL"},
    {"extended-slash",R"VHDL(entity \a\\b\ is end entity \a\\b\;
)VHDL"},
    {"external-absolute",R"VHDL(package p is constant x : integer := <<signal .tb.dut.s : bit>>; end package p;
)VHDL"},
    {"external-indexed",R"VHDL(package p is constant x : integer := <<signal .tb.g(2).s : bit>>; end package p;
)VHDL"},
    {"external-package",R"VHDL(package p is constant x : integer := <<constant @work.pkg.n : integer>>; end package p;
)VHDL"},
    {"external-relative",R"VHDL(package p is constant x : integer := <<variable ^.^.tb.v : integer>>; end package p;
)VHDL"},
    {"generate-alternative-labels",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: if first:a generate s<=1; elsif second:b generate s<=2; else third: generate s<=3; end generate g; end architecture;
)VHDL"},
    {"generate-body-end",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: if a generate begin s<=1; end; else generate begin s<=2; end; end generate g; end architecture;
)VHDL"},
    {"generate-case-labels",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: case k generate when first:0=>s<=1; when rest:others=>s<=2; end generate g; end architecture;
)VHDL"},
    {"generate-case",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: case k generate when 0=>s<=1; when others=>s<=2; end generate g; end architecture;
)VHDL"},
    {"generate-for",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: for i in 0 to 3 generate s<=i; end generate g; end architecture;
)VHDL"},
    {"generate-if",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: if a generate s<=1; elsif b generate s<=2; else generate s<=3; end generate g; end architecture;
)VHDL"},
    {"generate-labeled-body-end",R"VHDL(entity e is end entity; architecture rtl of e is  begin g: if first:a generate begin s<=1; end first; else second: generate begin s<=2; end second; end generate g; end architecture;
)VHDL"},
    {"generic-package",R"VHDL(package p is generic(type t); end package; package q is new work.p generic map(t=>integer);
)VHDL"},
    {"generic-subprogram",R"VHDL(package p is function f generic(type t) parameter(x:t) return t; end package;
)VHDL"},
    {"interface-file",R"VHDL(package p is procedure f(file x:text); end package;
)VHDL"},
    {"interface-generic-type",R"VHDL(package p is generic(type t); end package;
)VHDL"},
    {"interface-subprogram-default",R"VHDL(package p is generic(type t; function f(x:t) return t is <>); end package;
)VHDL"},
    {"logical-parenthesized",R"VHDL(package p is constant x : integer := (a and b) or c; end package p;
)VHDL"},
    {"logical",R"VHDL(package p is constant x : integer := a and b and c; end package p;
)VHDL"},
    {"mixed-case",R"VHDL(EnTiTy Demo Is PoRt(A: In BiT; Q: OuT BiT); EnD EnTiTy dEMo;
)VHDL"},
    {"number-separators",R"VHDL(package p is constant x : integer := 1_000 + 2_0; end package p;
)VHDL"},
    {"operator-call",R"VHDL(package p is constant x : integer := "+"(1,2); end package p;
)VHDL"},
    {"physical-selected",R"VHDL(package p is constant x : integer := 5 work.pkg.ns; end package p;
)VHDL"},
    {"physical",R"VHDL(package p is constant x : integer := 5 ns; end package p;
)VHDL"},
    {"power-chain",R"VHDL(package p is constant x : integer := 2**3**2; end package p;
)VHDL"},
    {"precedence",R"VHDL(package p is constant x : integer := 1+2*3**2; end package p;
)VHDL"},
    {"process-all",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process(all) begin null; end process p; end architecture;
)VHDL"},
    {"qualified",R"VHDL(package p is constant x : integer := integer'(1+2); end package p;
)VHDL"},
    {"real-exponent",R"VHDL(package p is constant x : integer := 1.0e+2; end package p;
)VHDL"},
    {"record-constraint",R"VHDL(package p is subtype r is rec_t(a(0 to 3),b(1 to 4)); end package;
)VHDL"},
    {"reduce",R"VHDL(package p is constant x : integer := and X"F"; end package p;
)VHDL"},
    {"seq-conditional-var",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process variable x:integer; begin x:=1 when a else 2; end process p; end architecture;
)VHDL"},
    {"seq-force-plain",R"VHDL(entity e is end; architecture a of e is begin p:process begin s<=force 1; s<=release; end process; end;
)VHDL"},
    {"seq-if",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process  begin a: if x then null; elsif y then null; else null; end if a; end process p; end architecture;
)VHDL"},
    {"seq-loop",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process  begin a: for i in 0 to 3 loop next a when false; exit a when true; end loop a; end process p; end architecture;
)VHDL"},
    {"seq-null",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process  begin null; end process p; end architecture;
)VHDL"},
    {"seq-selected-var",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process variable x:integer; begin with a select x:=1 when 0, 2 when others; end process p; end architecture;
)VHDL"},
    {"sign-power",R"VHDL(package p is constant x : integer := -2**2; end package p;
)VHDL"},
    {"sign-product",R"VHDL(package p is constant x : integer := -2*3+4; end package p;
)VHDL"},
    {"signature-attribute",R"VHDL(package p is constant x : integer := f[integer,integer,integer return integer]'delayed(1 ns); end package p;
)VHDL"},
    {"signature-three",R"VHDL(package p is alias f is foo[integer, integer, integer return integer]; end package;
)VHDL"},
    {"string-doubled",R"VHDL(package p is constant x : integer := "one ""two"""; end package p;
)VHDL"},
    {"wait-clauses",R"VHDL(entity e is end entity; architecture rtl of e is  begin p: process  begin wait on a,b until x for 1 ns; end process p; end architecture;
)VHDL"},
    {"waveform-conditional",R"VHDL(entity e is end entity; architecture rtl of e is  begin s <= 1 when a else 2 when b else 3; end architecture;
)VHDL"},
    {"waveform-selected",R"VHDL(entity e is end entity; architecture rtl of e is  begin with a select s <= 1 when 0|1, 2 when others; end architecture;
)VHDL"},
    {"waveform",R"VHDL(entity e is end entity; architecture rtl of e is  begin s <= transport 1 after 2 ns, 0 after 3 ns; end architecture;
)VHDL"},
  };
  for(const auto& entry:valid){
    cursiv::Document d(cursiv::TextDocument::create("file:///native.vhd","vhdl",0,std::string(entry.source)));
    d.language=&language.language;d.parseResult=parser.parse(d.textDocument().getText());
    const bool clean=d.parseSucceeded() && !d.parseRecovered() && d.parseResult.diagnostics.empty();
    if(!clean)std::fprintf(stderr,"vhdl native form: %.*s\n",int(entry.name.size()),entry.name.data());
    check(clean,"native-positive form parses without repair");
    if(d.parseResult.value)d.parseResult.value->parent=&d;
    std::vector<cursiv::Diagnostic> diagnostics;language.language.documentValidator->validateDocument(d,diagnostics);
    unsigned count=0;for(const auto& x:diagnostics)if(x.code && x.code->isString() && x.code->string().starts_with("vhdl."))++count;
    if(count)std::fprintf(stderr,"vhdl native validation: %.*s\n",int(entry.name.size()),entry.name.data());
    check(count==0,"native syntax has no added validator errors");
  }
  auto unit=[&](const cursiv::ParseResult& r,std::size_t index=0) -> const ast::Item* {
    const auto* file=cursiv::ast_cast<ast::DesignFile>(r.value);
    const auto* item=file && file->units.size()>index?cursiv::ast_cast<ast::DesignUnit>(file->units[index]):nullptr;
    return item?item->unit:nullptr;
  };
  auto value=[&](const cursiv::ParseResult& r) -> const ast::Expr* {
    auto p=cursiv::ast_cast<ast::PackageDecl>(unit(r));
    auto d=p && p->decls.size()==1?cursiv::ast_cast<ast::ObjectDecl>(p->decls[0]):nullptr;
    return d?d->defaultValue:nullptr;
  };
  auto literal=[](const ast::Expr* node,std::string_view text) {
    const auto* x=cursiv::ast_cast<ast::Literal>(node);return x && x->text==text;
  };
  auto named=[](const ast::Expr* node,std::string_view text) {
    const auto* x=cursiv::ast_cast<ast::Ident>(node);return x && x->name==text;
  };
  {
    auto r=parser.parse("package p is constant x:integer:=2**3**2; end package p;");
    auto b=cursiv::ast_cast<ast::BinaryExpr>(value(r));auto left=b?cursiv::ast_cast<ast::BinaryExpr>(b->left):nullptr;
    check(b && b->op=="**" && literal(b->right,"2") && left && literal(left->left,"2") && literal(left->right,"3"),"exponentiation matches native left grouping and value64");
    check(left && left->parent==b && left->left->parent==left && b->right->parent==b,"power operand parents");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=-2*3+4; end package p;");
    auto add=cursiv::ast_cast<ast::BinaryExpr>(value(r));auto neg=add?cursiv::ast_cast<ast::UnaryExpr>(add->left):nullptr;
    auto mul=neg?cursiv::ast_cast<ast::BinaryExpr>(neg->operand):nullptr;
    check(add && add->op=="+" && neg && neg->op=="-" && mul && mul->op=="*" && literal(mul->left,"2") && literal(mul->right,"3"),"sign wraps a complete term before addition");
    check(neg && neg->parent==add && mul && mul->parent==neg,"signed term parents");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=5 work.pkg.ns; end package p;");
    auto l=cursiv::ast_cast<ast::Literal>(value(r));auto u=l?cursiv::ast_cast<ast::SelectedName>(l->unit):nullptr;
    auto pfx=u?cursiv::ast_cast<ast::SelectedName>(u->prefix):nullptr;
    check(l && l->text=="5" && u && u->suffix=="ns" && pfx && pfx->suffix=="pkg" && named(pfx->prefix,"work"),"physical literal keeps complete selected unit");
    check(u && u->parent==l && pfx && pfx->parent==u && pfx->prefix->parent==pfx,"physical unit parents");
  }
  for(std::string_view text:{"B\"\"","8SX\"F\"","16D\"255\"","B\"10XZ-\""}) {
    const std::string source="package p is constant x:integer:="+std::string(text)+"; end package p;";
    auto r=parser.parse(source);auto l=cursiv::ast_cast<ast::Literal>(value(r));
    check(l && l->text==text && !l->unit,"bit string retains length base sign and graphic spelling");
  }
  {
    auto r=parser.parse("package p is constant x:integer:='a'; end package p;");
    auto l=cursiv::ast_cast<ast::Literal>(value(r));check(l && l->text=="'a'" && !l->unit,"character is a literal rather than an identifier");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=\"one \"\"two\"\"\"; end package p;");
    check(literal(value(r),"\"one \"\"two\"\"\""),"string retains doubled quote spelling");
  }
  {
    auto r=parser.parse("package p is alias f is foo[integer,integer,integer return integer]; end package p;");
    auto p=cursiv::ast_cast<ast::PackageDecl>(unit(r));auto a=p && p->decls.size()==1?cursiv::ast_cast<ast::AliasDecl>(p->decls[0]):nullptr;
    auto s=a?a->signature:nullptr;
    check(a && named(a->target,"foo") && s && s->params.size()==3 && named(s->params[2],"integer") && named(s->returnType,"integer"),"alias signature retains every parameter");
    check(a && a->parent==p && s && s->parent==a && s->params[0]->parent==s && s->returnType->parent==s,"signature parents");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=f[integer,integer,integer return integer]'delayed(1 ns); end package p;");
    auto a=cursiv::ast_cast<ast::AttributeName>(value(r));auto s=a?a->signature:nullptr;
    auto arg=a?cursiv::ast_cast<ast::Literal>(a->arg):nullptr;
    check(a && a->designator=="delayed" && named(a->prefix,"f") && s && s->params.size()==3 && arg && arg->text=="1" && named(arg->unit,"ns"),"attribute signature and argument retained");
    check(a && a->prefix->parent==a && s && s->parent==a && arg && arg->parent==a && arg->unit->parent==arg,"attribute signature parents");
  }
  {
    auto r=parser.parse("package p is attribute mine of f[integer,integer,integer return integer]:function is 1; end package p;");
    auto p=cursiv::ast_cast<ast::PackageDecl>(unit(r));auto a=p && p->decls.size()==1?cursiv::ast_cast<ast::AttributeSpec>(p->decls[0]):nullptr;
    auto e=a && a->entities.size()==1?a->entities[0]:nullptr;
    check(a && a->designator=="mine" && e && e->name=="f" && e->signature && e->signature->params.size()==3,"attribute entity designator retains its signature");
    check(e && e->parent==a && e->signature && e->signature->parent==e,"entity designator parents");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=<<signal .tb.g(2).s:bit>>; end package p;");
    auto x=cursiv::ast_cast<ast::ExternalName>(value(r));auto s=x?cursiv::ast_cast<ast::SelectedName>(x->path):nullptr;
    auto c=s?cursiv::ast_cast<ast::CallName>(s->prefix):nullptr;
    check(x && x->kind=="signal" && x->root=="." && s && s->suffix=="s" && c && c->args.size()==1 && literal(c->args[0]->actual,"2") && x->subtype && named(x->subtype->typeMark,"bit"),"external name retains root indexed path and subtype");
    check(x && s && s->parent==x && c && c->parent==s && c->args[0]->parent==c && x->subtype && x->subtype->parent==x,"external name parents");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=<<variable ^.^.tb.v:integer>>; end package p;");
    auto x=cursiv::ast_cast<ast::ExternalName>(value(r));check(x && x->kind=="variable" && x->root=="^.^.","external relative root retained");
  }
  {
    auto r=parser.parse("package p is constant x:integer:=<<constant @work.pkg.n:integer>>; end package p;");
    auto x=cursiv::ast_cast<ast::ExternalName>(value(r));check(x && x->kind=="constant" && x->root=="@work.","external package root retained");
  }
  {
    auto r=parser.parse("entity e is end; architecture a of e is begin g: if first:x generate begin s<=1; end first; elsif second:y generate s<=2; else third: generate begin s<=3; end third; end generate g; end architecture a;");
    auto a=cursiv::ast_cast<ast::ArchitectureBody>(unit(r,1));auto g=a && a->statements.size()==1?cursiv::ast_cast<ast::GenerateStmt>(a->statements[0]):nullptr;
    check(g && g->label=="g" && g->endLabel=="g" && g->branches.size()==3 && g->branches[0]->label=="first" && named(g->branches[0]->condition,"x") && g->branches[0]->body && g->branches[0]->body->endName=="first" && g->branches[1]->label=="second" && g->branches[2]->label=="third" && !g->branches[2]->condition && g->branches[2]->body->endName=="third","generate alternatives retain labels conditions body endings and else");
    check(g && g->parent==a && g->branches.size()==3 && g->branches[0]->parent==g && g->branches[0]->body->parent==g->branches[0] && g->branches[0]->condition->parent==g->branches[0],"generate branch parents");
  }
  {
    auto r=parser.parse("entity e is end; architecture a of e is begin g: case k generate when first:0=>s<=1; when rest:others=>s<=2; end generate g; end architecture a;");
    auto a=cursiv::ast_cast<ast::ArchitectureBody>(unit(r,1));auto g=a && a->statements.size()==1?cursiv::ast_cast<ast::GenerateStmt>(a->statements[0]):nullptr;
    auto first=g && g->caseAlts.size()==2?cursiv::ast_cast<ast::CaseGenAlt>(g->caseAlts[0]):nullptr;
    auto last=g && g->caseAlts.size()==2?cursiv::ast_cast<ast::CaseGenAlt>(g->caseAlts[1]):nullptr;
    check(first && first->label=="first" && first->choices.size()==1 && literal(first->choices[0],"0") && last && last->label=="rest" && last->choices.size()==1 && named(last->choices[0],"others"),"case generate alternatives retain labels and choices");
    check(first && first->parent==g && first->body && first->body->parent==first,"case generate parents");
  }
  {
    auto r=parser.parse("entity e is end; architecture a of e is begin u:entity work.foo(rtl) port map(a=>s); end architecture a;");
    auto a=cursiv::ast_cast<ast::ArchitectureBody>(unit(r,1));auto c=a && a->statements.size()==1?cursiv::ast_cast<ast::ComponentInst>(a->statements[0]):nullptr;
    auto name=c?cursiv::ast_cast<ast::SelectedName>(c->name):nullptr;
    check(c && c->kind=="entity" && c->archId=="rtl" && name && named(name->prefix,"work") && name->suffix=="foo","architecture selection is distinct from a function call");
    check(c && c->parent==a && name && name->parent==c,"entity instantiation parents");
  }
  auto validate=[&](std::string source,std::string_view code,unsigned expected) {
    cursiv::Document d(cursiv::TextDocument::create("file:///audit.vhd","vhdl",0,std::move(source)));
    d.language=&language.language;d.parseResult=parser.parse(d.textDocument().getText());
    check(d.parseSucceeded() && !d.parseRecovered(),"semantic fixture parses without repair");
    if(d.parseResult.value)d.parseResult.value->parent=&d;
    std::vector<cursiv::Diagnostic> diagnostics;language.language.documentValidator->validateDocument(d,diagnostics);
    unsigned count=0;
    for(const auto& x:diagnostics)if(x.code && x.code->isString() && x.code->string()==code){
      ++count;
      if(code=="vhdl.closing-name")check(x.range.begin==d.textDocument().getText().find("second"),"closing name diagnostic selects inherited field");
    }
    check(count==expected,"runtime diagnostic code and count");
  };
  validate("entity first is end entity second;","vhdl.closing-name",1);
  validate("entity Demo is end entity dEMo;","vhdl.closing-name",0);
  validate("entity e is end; architecture a of e is begin p:process begin if x then null; end if wrong; end process p; end;","vhdl.closing-label",1);
  validate("entity e is end; architecture a of e is begin if x generate s<=1; end generate; end;","vhdl.generate-label",1);
  validate("package p is constant x:integer:=a and b or c; end;","vhdl.logical-operators",1);
  validate("package p is constant x:integer:=(a and b) or c; end;","vhdl.logical-operators",0);
  validate("package p is constant x:integer:=a nand b nand c; end;","vhdl.operator-chain",1);
  validate("package p is constant x:integer:=a sll b sll c; end;","vhdl.operator-chain",1);
  validate("package p is constant x:integer:=a<b<c; end;","vhdl.operator-chain",1);
  validate("package p is constant x:integer:=2#9#; end;","vhdl.literal-base",1);
  validate("package p is constant x:integer:=16#F#; end;","vhdl.literal-base",0);
  validate("package p is constant x:integer:=1e2.3; end;","vhdl.literal-exponent",1);
  validate("entity bad__name is end;","vhdl.identifier",1);
  validate("package p is function f; end;","vhdl.subprogram-return",1);
  validate("","vhdl.design-file",1);
  validate("entity e is end; architecture a of e is begin s<=force 1; end;","vhdl.concurrent-force",1);
  std::printf("vhdl audit: %zu native syntax forms, %u checks\n",std::size(valid),checks);
  return ok;
}
}  // namespace vhdl_audit
