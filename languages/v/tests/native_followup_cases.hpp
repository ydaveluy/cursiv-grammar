#pragma once
#include <cstdio>
#include <string_view>
#include <string>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "v/core/ast.hpp"

namespace v_followup {
template<class Language> bool run(const Language& language) {
  struct Case {const char* name; std::string_view source; bool accepted;};
  const Case cases[] = {
      {"empty", R"VCASE(
)VCASE", true},
      {"hello", R"VCASE(fn main() { println('hello') }
)VCASE", true},
      {"public-function", R"VCASE(pub fn add(a int,b int) int {return a+b}
)VCASE", true},
      {"body-required", R"VCASE(fn main()
)VCASE", true},
      {"keyword-variable", R"VCASE(fn main(){ if := 1 }
)VCASE", false},
      {"keyword-prefix", R"VCASE(fn main(){ iff := 1 }
)VCASE", true},
      {"bool-literals", R"VCASE(fn main(){ a:=true
 b:=false }
)VCASE", true},
      {"semicolons", R"VCASE(fn main(){a:=1; b:=2; println(a+b)}
)VCASE", true},
      {"nested-comments", R"VCASE(/* a /* inner */ b */ fn main(){}
)VCASE", true},
      {"single-string", R"VCASE(fn main(){s:='a'}
)VCASE", true},
      {"double-string", R"VCASE(fn main(){s:="a"}
)VCASE", true},
      {"rune", R"VCASE(fn main(){c:=`a`}
)VCASE", true},
      {"rune-many", R"VCASE(fn main(){c:=`ab`}
)VCASE", false},
      {"multiline", R"VCASE(fn main(){s:='a
b'}
)VCASE", true},
      {"raw-string", R"VCASE(fn main(){s:=r'a\b'}
)VCASE", true},
      {"escaped-dollar", R"VCASE(fn main(){s:='a\$x'}
)VCASE", true},
      {"invalid-escape", R"VCASE(fn main(){s:='\q'}
)VCASE", false},
      {"octal-escape", R"VCASE(fn main(){s:='\123'}
)VCASE", true},
      {"binary", R"VCASE(fn main(){n:=0b1010}
)VCASE", true},
      {"octal", R"VCASE(fn main(){n:=0o17}
)VCASE", true},
      {"leading-zero", R"VCASE(fn main(){n:=09}
)VCASE", true},
      {"separator", R"VCASE(fn main(){n:=1_000}
)VCASE", true},
      {"bad-separator", R"VCASE(fn main(){n:=1__0}
)VCASE", false},
      {"hex", R"VCASE(fn main(){n:=0xFF}
)VCASE", true},
      {"hex-float", R"VCASE(fn main(){n:=0x1p2}
)VCASE", false},
      {"float-dot", R"VCASE(fn main(){n:=1.}
)VCASE", true},
      {"float-leading", R"VCASE(fn main(){n:=.5}
)VCASE", true},
      {"range", R"VCASE(fn main(){for x in 1..3 {println(x)}}
)VCASE", true},
      {"array-for", R"VCASE(fn main(){for x in [1,2] {println(x)}}
)VCASE", true},
      {"classic-for", R"VCASE(fn main(){for mut i:=0;i<3;i++ {println(i)}}
)VCASE", false},
      {"go-switch", R"VCASE(fn main(){switch 1 {case 1:println(1)}}
)VCASE", false},
      {"if-init", R"VCASE(fn main(){if a:=1;a>0 {println(a)}}
)VCASE", false},
      {"match", R"VCASE(fn main(){n:=1
 match n {1 {println(1)} else {println(0)}}}
)VCASE", true},
      {"const-single", R"VCASE(const n=1
)VCASE", true},
      {"const-group", R"VCASE(const (n=1
 m=2)
)VCASE", true},
      {"const-public", R"VCASE(pub const n=1
)VCASE", true},
      {"struct-public", R"VCASE(pub struct Point { x int
 y int }
)VCASE", true},
      {"struct-sections", R"VCASE(struct Point {pub mut:
 x int
 y int}
)VCASE", true},
      {"struct-duplicate-mut", R"VCASE(struct Point {pub mut mut:
 x int}
)VCASE", false},
      {"struct-default", R"VCASE(struct Point {x int=1}
)VCASE", true},
      {"import-name", R"VCASE(import math
 fn main(){}
)VCASE", true},
      {"import-alias", R"VCASE(import math as m
 fn main(){}
)VCASE", true},
      {"import-qualified", R"VCASE(import net.http
 fn main(){}
)VCASE", true},
      {"import-go-string", R"VCASE(import 'math'
 fn main(){}
)VCASE", true},
      {"mod-name", R"VCASE(module hello
 fn main(){}
)VCASE", true},
      {"infix-shift", R"VCASE(fn main(){n:=1+2<<3}
)VCASE", true},
      {"infix-bitwise", R"VCASE(fn main(){n:=1|2^3&4}
)VCASE", true},
      {"infix-not", R"VCASE(fn main(){n:=true||false&&true}
)VCASE", true},
      {"interface", R"VCASE(interface Reader {read() int}
)VCASE", true},
      {"enum", R"VCASE(pub enum Color {red
 blue=2}
)VCASE", true},
      {"multiple-return", R"VCASE(fn f()(int,int){return 1,2}
)VCASE", true},
      {"classic-no-mut", R"VCASE(fn main(){for i:=0;i<3;i++ {println(i)}}
)VCASE", true},
      {"unicode-id", R"VCASE(fn main(){α:=1}
)VCASE", false},
      {"unicode-string", R"VCASE(fn main(){s:='α'}
)VCASE", true},
      {"fn-unnamed", R"VCASE(fn f(int){}
)VCASE", false},
      {"fn-names-shared", R"VCASE(fn f(a,b int){}
)VCASE", false},
      {"fn-mut-param", R"VCASE(fn f(mut a []int){}
)VCASE", true},
      {"fn-mut-after", R"VCASE(fn f(a mut int){}
)VCASE", true},
      {"fn-type", R"VCASE(fn main(){a:=fn(x int)int{return x}}
)VCASE", true},
      {"infix-bool-paired", R"VCASE(fn main(){a:=true
 b:=false
 c:=a&&b}
)VCASE", true},
      {"pub-interface", R"VCASE(pub interface Reader {read() int}
)VCASE", true},
      {"typed-map", R"VCASE(fn main(){a:=map[string]int{'a':1}}
)VCASE", false},
      {"empty-array", R"VCASE(fn main(){a:=[]int{}}
)VCASE", true},
      {"literal-field-newlines", R"VCASE(struct Point {x int
 y int}
 fn main(){p:=Point{x:1
 y:2}}
)VCASE", true},
      {"native-name-short", R"VCASE(fn main(){x:=1}
)VCASE", true},
      {"label-break", R"VCASE(fn main(){ outer: for { break outer }}
)VCASE", true},
      {"go-array-conversion", R"VCASE(fn main(){x:=[]byte(1)}
)VCASE", false},
      {"go-type-call-arg", R"VCASE(fn main(){x:=make([]int,1)}
)VCASE", true},
      {"field-tag", R"VCASE(struct Point {x int 'json:x'}
)VCASE", false},
      {"field-embedded", R"VCASE(struct Point {Other}
)VCASE", true},
      {"inline-interface", R"VCASE(struct Point {x interface A {f()}}
)VCASE", false},
      {"unary-plus", R"VCASE(fn main(){x:=+1}
)VCASE", false},
      {"unary-bitnot", R"VCASE(fn main(){x:=^1}
)VCASE", false},
      {"bitclear", R"VCASE(fn main(){x:=1&^2}
)VCASE", false},
      {"bitnot", R"VCASE(fn main(){x:=~1}
)VCASE", true},
      {"unsigned-shift", R"VCASE(fn main(){x:=1>>>2}
)VCASE", true},
      {"logical-and", R"VCASE(fn main(){x:=1&&2}
)VCASE", true},
      {"membership", R"VCASE(fn main(){x:=1 in [1,2]}
)VCASE", true},
      {"map", R"VCASE(fn main(){x:={1:2}}
)VCASE", true},
      {"typed-map-empty", R"VCASE(fn main(){x:=map[string]int{}}
)VCASE", true},
      {"field-shared-names", R"VCASE(struct Point {x,y int}
)VCASE", false},
      {"return-newline", R"VCASE(fn main(){return
x:=1}
)VCASE", false},
      {"method-receiver", R"VCASE(fn (p Point) f(){}
)VCASE", true},
      {"method-multi", R"VCASE(fn (p Point,q Point) f(){}
)VCASE", false},
      {"params-prototype", R"VCASE(fn f(int)
)VCASE", true},
      {"unnamed-multi-result", R"VCASE(fn f()(int,string){return 1,"a"}
)VCASE", true},
      {"stmt-bare-block", R"VCASE(fn main(){{x:=1}}
)VCASE", true},
      {"top-var", R"VCASE(x:=1
)VCASE", false},
      {"array-fixed", R"VCASE(fn main(){x:=[3]int{}}
)VCASE", true},
      {"array-lit-trailing", R"VCASE(fn main(){x:=[1,2,]}
)VCASE", true},
      {"float-dot-id", R"VCASE(fn main(){x:=1.foo}
)VCASE", true},
      {"quote-hex-short", R"VCASE(fn main(){x:='\x1'}
)VCASE", false},
      {"str-backtick-escape", R"VCASE(fn main(){x:='\`'}
)VCASE", true},
      {"match-newline", R"VCASE(fn main(){match x {1 {}
2 {}}}
)VCASE", true},
      {"ast-rich", R"VCASE(import net.http as net
pub const n=0b10
pub struct Point {pub mut:
 x int=2
 y int=3}
pub interface Reader {read() int}
pub fn f(mut p []int) {
 s:=r'a\b'
 c:=`α`
 yes:=true
 no:=false
 empty:=none
 null:=nil
 for i in 1..3 {println(i)}
 outer: for {break outer}
 x:=1+2<<3
}

)VCASE", true},
      {"interface-field-reader", R"VCASE(interface Reader {x int}
)VCASE", true},
      {"interface-embedded-reader", R"VCASE(interface Reader {Other}
)VCASE", true},
      {"nested-struct-point", R"VCASE(fn main(){struct Point{x int}}
)VCASE", true},
      {"nested-const", R"VCASE(fn main(){const xx=1}
)VCASE", false},
      {"float-dot-exponent", R"VCASE(fn main(){x:=1.e2}
)VCASE", true},
      {"iteration-it", R"VCASE(fn main(){x:=it}
)VCASE", true},
      {"binding-it", R"VCASE(fn main(){it:=1}
)VCASE", true},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto result = language.language.parser->parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && result.value;
    if (clean != test.accepted) {
      std::fprintf(stderr, "V native: %s expected=%d accepted=%d recovered=%d diagnostics=%zu\n", test.name,test.accepted,result.accepted,result.cst().recovered(),result.diagnostics.size());
      ok = false;
    }
  }

  namespace ast = v::ast;
  const auto result = language.language.parser->parse(R"RICH(import net.http as net
pub const n=0b10
pub struct Point {pub mut:
 x int=2
 y int=3}
pub interface Reader {read() int}
pub fn f(mut p []int) {
 s:=r'a\b'
 c:=`α`
 yes:=true
 no:=false
 empty:=none
 null:=nil
 for i in 1..3 {println(i)}
 outer: for {break outer}
 x:=1+2<<3
}

)RICH");
  const auto* file = cursiv::ast_cast<ast::SourceFile>(result.value);
  const auto* c = file && file->decls.size() == 4 ? cursiv::ast_cast<ast::ConstDecl>(file->decls[0]) : nullptr;
  const auto* st = file && file->decls.size() == 4 ? cursiv::ast_cast<ast::StructDecl>(file->decls[1]) : nullptr;
  const auto* it = file && file->decls.size() == 4 ? cursiv::ast_cast<ast::InterfaceDecl>(file->decls[2]) : nullptr;
  const auto* fn = file && file->decls.size() == 4 ? cursiv::ast_cast<ast::FunctionDecl>(file->decls[3]) : nullptr;
  bool values = file && file->imports.size() == 1 && file->imports[0]->path == "net.http" &&
      file->imports[0]->alias == "net" && c && c->isPub && c->specs.size() == 1 &&
      st && st->isPub && st->fields.size() == 3 && st->fields[0]->isSection &&
      st->fields[0]->pub && st->fields[0]->mut && st->fields[1]->defaultValue &&
      it && it->isPub && it->type && it->type->name == "Reader" &&
      fn && fn->isPub && fn->signature && fn->signature->params.size() == 1 &&
      fn->signature->params[0]->mut && fn->signature->params[0]->names.size() == 1 &&
      fn->signature->params[0]->names[0] == "p" && fn->body && fn->body->stmts.size() == 9;
  bool rune = false, raw = false, yes = false, no = false, none = false, nil = false, range = false, label = false, precedence = false;
  for (const auto* node : result.nodes) {
    if (const auto* n = cursiv::ast_cast<ast::RuneLit>(node)) rune |= n->text == "`α`";
    if (const auto* n = cursiv::ast_cast<ast::StringLit>(node)) raw |= n->text == R"TXT(r'a\b')TXT";
    if (const auto* n = cursiv::ast_cast<ast::BoolLit>(node)) { yes |= n->value; no |= !n->value; }
    none |= cursiv::ast_cast<ast::NoneLit>(node) != nullptr;
    nil |= cursiv::ast_cast<ast::NilLit>(node) != nullptr;
    if (const auto* n = cursiv::ast_cast<ast::RangeExpr>(node)) {
      const auto* lo = cursiv::ast_cast<ast::IntLit>(n->low);
      const auto* hi = cursiv::ast_cast<ast::IntLit>(n->high);
      range |= lo && hi && lo->text == "1" && hi->text == "3";
    }
    if (const auto* n = cursiv::ast_cast<ast::LabelStmt>(node)) {
      const auto* b = n->body && n->body->body && n->body->body->stmts.size() == 1 ?
          cursiv::ast_cast<ast::BreakStmt>(n->body->body->stmts[0]) : nullptr;
      label |= n->name == "outer" && b && b->label == "outer";
    }
    if (const auto* n = cursiv::ast_cast<ast::BinaryExpr>(node); n && n->op == "+") {
      const auto* shift = cursiv::ast_cast<ast::BinaryExpr>(n->right);
      precedence |= shift && shift->op == "<<";
    }
  }
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  if (result.value) walk(walk, *result.value);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !values || !rune || !raw || !yes || !no || !none || !nil || !range || !label || !precedence ||
      !owned || seen.size() != result.nodes.size()) {
    std::fprintf(stderr, "V native AST values=%d rune=%d raw=%d bool=%d/%d none/nil=%d/%d range=%d label=%d precedence=%d owned=%d nodes=%zu/%zu\n",
        values,rune,raw,yes,no,none,nil,range,label,precedence,owned,seen.size(),result.nodes.size());
    ok = false;
  }

  struct NativeTree {std::string_view source, tree;};
  const NativeTree trees[] = {
    {R"TREE(fn main(){n:=true && false}
)TREE", "(true && false)"},
    {R"TREE(fn main(){n:=true&&false}
)TREE", "(true & (& false))"},
    {R"TREE(fn main(){n:=true&&!false}
)TREE", "(true && (! false))"},
    {R"TREE(fn main(){n:=true || false && true}
)TREE", "((true || false) && true)"},
    {R"TREE(fn main(){n:=a in b == c}
)TREE", "(a in (b == c))"},
  };
  auto render = [&](auto&& self, const ast::Expr* expr) -> std::string {
    if (const auto* n = cursiv::ast_cast<ast::Ident>(expr)) return std::string(n->name);
    if (const auto* n = cursiv::ast_cast<ast::BoolLit>(expr)) return n->value ? "true" : "false";
    if (const auto* n = cursiv::ast_cast<ast::PrefixExpr>(expr)) return "(" + std::string(n->op) + " " + self(self,n->operand) + ")";
    if (const auto* n = cursiv::ast_cast<ast::BinaryExpr>(expr)) return "(" + self(self,n->left) + " " + std::string(n->op) + " " + self(self,n->right) + ")";
    return "?";
  };
  for (const auto& test : trees) {
    const auto parsed = language.language.parser->parse(test.source);
    const auto* f = cursiv::ast_cast<ast::SourceFile>(parsed.value);
    const auto* n = f && f->decls.size() == 1 ? cursiv::ast_cast<ast::FunctionDecl>(f->decls[0]) : nullptr;
    const auto* decl = n && n->body && n->body->stmts.size() == 1 ? cursiv::ast_cast<ast::VarDecl>(n->body->stmts[0]) : nullptr;
    const auto tree = decl && decl->values.size() == 1 ? render(render,decl->values[0]) : "?";
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() || tree != test.tree) {
      std::fprintf(stderr,"V native expression tree expected=%.*s actual=%s\n",int(test.tree.size()),test.tree.data(),tree.c_str());
      ok = false;
    }
  }
  return ok;
}
} // namespace v_followup
