#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "pascal/core/CoreServices.hpp"
#include "pascal/core/ast.hpp"
namespace native_regressions {
template <class T>
const T* as(const cursiv::AstNode* n) {
  return cursiv::ast_cast<T>(n);
}
inline bool run(const pascal::PascalCoreServices& services) {
  namespace ast = pascal::ast;
  const auto& parser = *services.parser;
  struct Case {
    const char* name;
    const char* text;
  };
  const Case cases[] = {
      {"underscore", R"PASCAL(program demo;
var _x:integer;
begin
_x:=42; writeln(_x);
end.
)PASCAL"},
      {"escaped-keyword", R"PASCAL(program demo;
var &begin:integer;
begin
&begin:=2; writeln(&begin);
end.
)PASCAL"},
      {"radix", R"PASCAL(program demo;

begin
writeln($FF,' ',%101,' ',&17);
end.
)PASCAL"},
      {"max-uint", R"PASCAL(program demo;

begin
writeln(18446744073709551615);
end.
)PASCAL"},
      {"real-spelling", R"PASCAL(program demo;

begin
writeln(1.25E+02:0:2);
end.
)PASCAL"},
      {"charcodes", R"PASCAL(program demo;

begin
writeln('A'#13#10#$42#&103#%1000100'Z');
end.
)PASCAL"},
      {"charcode-unicode", R"PASCAL(program demo;

begin
writeln(ord(#$1F600[1]));
end.
)PASCAL"},
      {"quote-escape", R"PASCAL(program demo;

begin
writeln('don''t');
end.
)PASCAL"},
      {"line-comment", R"PASCAL(program demo;

begin
// comment
writeln(42);
end.
)PASCAL"},
      {"nested-comment", R"PASCAL({$mode objfpc}
program demo;

begin
(* outer (* nested *) end *) writeln(42);
end.
)PASCAL"},
      {"constant-expression", R"PASCAL(program demo;
const x=1+2*3;
begin
writeln(x);
end.
)PASCAL"},
      {"empty-call", R"PASCAL(program demo;
function f():integer; begin f:=42; end;
begin
writeln(f());
end.
)PASCAL"},
      {"qualified-call", R"PASCAL(program demo;

begin
System.Writeln(42);
end.
)PASCAL"},
      {"parenthesized-postfix", R"PASCAL(program demo;
var a:array[1..2] of integer;
begin
writeln((a)[1]);
end.
)PASCAL"},
      {"address-field", R"PASCAL(program demo;
type rec=record a:integer; end; var r:rec; p:pointer;
begin
p:=@r.a; writeln(p<>nil);
end.
)PASCAL"},
      {"shadow-chr", R"PASCAL(program demo;
function chr(x:integer):integer; begin chr:=x+1; end;
begin
writeln(chr(1));
end.
)PASCAL"},
      {"shadow-true", R"PASCAL(program demo;
var true:integer;
begin
true:=2; writeln(true);
end.
)PASCAL"},
      {"recursive-prefix", R"PASCAL(program demo;

begin
writeln(- -1,' ',not not true);
end.
)PASCAL"},
      {"bit-precedence", R"PASCAL(program demo;

begin
writeln(1 shl 2 + 1 xor 3 and 1);
end.
)PASCAL"},
      {"constant-range", R"PASCAL(program demo;
const n=1+2; type arr=array[1..n] of integer; var a:arr;
begin
writeln(sizeof(a));
end.
)PASCAL"},
      {"empty-record", R"PASCAL(program demo;
type rec=record end; var r:rec;
begin

end.
)PASCAL"},
      {"record-trailing", R"PASCAL(program demo;
type rec=record a:integer; end; var r:rec;
begin
r.a:=42;
end.
)PASCAL"},
      {"variant-empty", R"PASCAL(program demo;
type rec=record case integer of 0:(); 1:(a:integer;); end;
begin

end.
)PASCAL"},
      {"case-trailing", R"PASCAL(program demo;

begin
case 1 of 1:writeln(1); end;
end.
)PASCAL"},
      {"case-range", R"PASCAL(program demo;

begin
case 2 of 1..3:writeln(2); else writeln(0); end;
end.
)PASCAL"},
      {"case-else-no-semicolon", R"PASCAL(program demo;

begin
case 1 of 1:writeln(1) else writeln(2) end;
end.
)PASCAL"},
      {"open-array", R"PASCAL({$mode objfpc}
program demo;
procedure p(const a:array of integer); begin writeln(length(a)); end;
begin
p([1,2,3]);
end.
)PASCAL"},
      {"dynamic-array", R"PASCAL({$mode objfpc}
program demo;
var a:array of integer;
begin
setlength(a,2);
end.
)PASCAL"},
      {"param-kinds", R"PASCAL({$mode objfpc}
program demo;
var a,b:integer; procedure p(const x:integer; out y:integer); begin y:=x; end;
begin
p(a,b);
end.
)PASCAL"},
      {"untyped-param", R"PASCAL(program demo;
procedure p(var x); begin end;
begin

end.
)PASCAL"},
      {"forward", R"PASCAL(program demo;
procedure p; forward; procedure p; begin end;
begin
p;
end.
)PASCAL"},
      {"procedure-type", R"PASCAL(program demo;
type callback=procedure; var p:callback; procedure f; begin end;
begin
p:=@f; p();
end.
)PASCAL"},
      {"unit-empty", R"PASCAL(unit testunit; interface implementation end.
)PASCAL"},
      {"unit-functions",
       R"PASCAL(unit testunit; interface function f():integer; implementation function f():integer; begin f:=42; end; end.
)PASCAL"},
      {"unit-init-final",
       R"PASCAL(unit testunit; interface implementation initialization writeln(1); finalization writeln(2); end.
)PASCAL"},
      {"unit-old-init",
       R"PASCAL(unit testunit; interface implementation begin writeln(1); end.
)PASCAL"},
      {"implicit-program", R"PASCAL(begin writeln(42); end.
)PASCAL"},
      {"for-in", R"PASCAL({$mode objfpc}
program demo;
var i:integer;
begin
for i in [1,2,3] do writeln(i);
end.
)PASCAL"},
      {"break-continue", R"PASCAL({$mode objfpc}
program demo;

begin
while true do begin break; continue; end;
end.
)PASCAL"},
      {"proc-parameter", R"PASCAL({$mode iso}
program demo; procedure p(procedure f); begin f; end; procedure q; begin end; begin p(q); end.
)PASCAL"},
      {"typed-constant",
       R"PASCAL(program demo; const x:integer=42; begin writeln(x); end.
)PASCAL"},
      {"wide-decimal",
       R"PASCAL(program p; begin writeln(18446744073709551616); end.
)PASCAL"},
  };
  for (const auto& c : cases) {
    const std::string text = c.text;
    const auto r = parser.parse(text);
    bool parents = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& n) -> void {
      if (!seen.insert(&n).second) {
        parents = false;
        return;
      }
      services.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &n;
            self(self, child);
          });
    };
    if (r.value) walk(walk, *r.value);
    parents &= seen.size() == r.nodes.size();
    const auto validation = cursiv::test::validationHelper(services, text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !parents || !validation.diagnostics.empty()) {
      std::fprintf(stderr, "pascal: native case failed: %s\n", c.name);
      for (const auto& d : validation.diagnostics)
        std::fprintf(stderr, "%s\n", d.message.c_str());
      return false;
    }
  }
  bool ok = true;
  const auto check = [&](bool yes, const char* what) {
    if (!yes) {
      std::fprintf(stderr, "pascal: AST: %s\n", what);
      ok = false;
    }
  };
  const auto block = [](const cursiv::ParseResult& r) {
    const auto* p = as<ast::Program>(r.value);
    return p ? as<ast::CompoundStmt>(p->block->body) : nullptr;
  };
  const auto argument = [&](const cursiv::ParseResult& r,
                            std::size_t i = 0) -> const ast::Expr* {
    const auto* b = block(r);
    const auto* s = b && !b->statements.empty()
                        ? as<ast::ExprStmt>(b->statements[0])
                        : nullptr;
    const auto* c = s ? as<ast::CallExpr>(s->value) : nullptr;
    return c && i < c->args.size() ? c->args[i]->value : nullptr;
  };
  for (const auto* invalid : {"program if; begin end.",
                              "program p; var BeGiN:integer; begin end.",
                              "program p; const while=1; begin end."}) {
    const auto result = parser.parse(invalid);
    check(!result.accepted || result.cst().recovered() ||
              !result.diagnostics.empty(), "reserved words cannot be ordinary names");
  }
  {
    const auto result = parser.parse(
        "program p; var ifx,&if:integer; begin ifx:=1; &if:=ifx; end.");
    const auto* program = as<ast::Program>(result.value);
    const auto* decl = program && !program->block->decls.empty()
                           ? as<ast::VarDecl>(program->block->decls[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && decl && decl->names.size() == 2 &&
              decl->names[0] == "ifx" && decl->names[1] == "if",
          "word boundaries and escaped reserved names retain values");
  }
  {
    const auto result = parser.parse(
        "program p; begin writeln(+2 ** 3 ** 2,-2 ** 2); end.");
    const auto* power = as<ast::BinaryExpr>(argument(result));
    const auto* first = power ? as<ast::BinaryExpr>(power->left) : nullptr;
    const auto* positive = first ? as<ast::UnaryExpr>(first->left) : nullptr;
    const auto* negative = as<ast::UnaryExpr>(argument(result, 1));
    const auto* negativePower = negative ? as<ast::BinaryExpr>(negative->operand) : nullptr;
    check(result.accepted && !result.cst().recovered() && power &&
              power->op == "**" && first && first->op == "**" &&
              positive && positive->op == "+" && positive->parent == first &&
              negative && negative->op == "-" && negativePower &&
              negativePower->op == "**" && negativePower->parent == negative,
          "FPC power associates left, unary plus is a factor and minus wraps power");
  }
  {
    const std::string text =
        "program p; begin writeln($FF,%101,&17,18446744073709551615,1.25E+02); "
        "end.";
    const auto r = parser.parse(text);
    const char* integers[] = {"$FF", "%101", "&17", "18446744073709551615"};
    for (std::size_t i = 0; i < 4; ++i) {
      const auto* n = as<ast::NumberLit>(argument(r, i));
      check(n && n->spelling == integers[i],
            "integer spelling and full unsigned range");
    }
    const auto* real = as<ast::NumberLit>(argument(r, 4));
    check(real && real->spelling == "1.25E+02",
          "real spelling retains exponent and precision");
  }
  {
    const std::string text = "program p; begin writeln('A'#13#10#$42'Z'); end.";
    const auto r = parser.parse(text);
    const auto* s = as<ast::StringLit>(argument(r));
    check(s && s->spelling == "'A'#13#10#$42'Z'",
          "adjacent quoted segments and character codes are lossless");
  }
  {
    const std::string text = "program p; const n=1+2*3; begin end.";
    const auto r = parser.parse(text);
    const auto* p = as<ast::Program>(r.value);
    const auto* c = p ? as<ast::ConstDecl>(p->block->decls[0]) : nullptr;
    const auto* add = c ? as<ast::BinaryExpr>(c->value) : nullptr;
    const auto* mul = add ? as<ast::BinaryExpr>(add->right) : nullptr;
    check(c && c->name == "n" && add && add->op == "+" && mul &&
              mul->op == "*" && mul->parent == add,
          "constant-expression precedence and parents");
  }
  {
    const std::string text = "program p; begin System.Writeln(); end.";
    const auto r = parser.parse(text);
    const auto* b = block(r);
    const auto* s = b ? as<ast::ExprStmt>(b->statements[0]) : nullptr;
    const auto* c = s ? as<ast::CallExpr>(s->value) : nullptr;
    const auto* f = c ? as<ast::FieldExpr>(c->callee) : nullptr;
    const auto* n = f ? as<ast::NameExpr>(f->object) : nullptr;
    check(c && c->args.empty() && f && f->field == "Writeln" && n &&
              n->name == "System" && f->parent == c && n->parent == f,
          "qualified empty calls own the callee expression");
  }
  {
    const std::string text = "program p; begin p:=@r.a; end.";
    const auto r = parser.parse(text);
    const auto* b = block(r);
    const auto* a = b ? as<ast::AssignStmt>(b->statements[0]) : nullptr;
    const auto* u = a ? as<ast::UnaryExpr>(a->value) : nullptr;
    check(u && u->op == "@" && as<ast::FieldExpr>(u->operand),
          "address-of wraps the full variable designator");
  }
  {
    const std::string text =
        "unit u; interface function f():integer; implementation function "
        "f():integer; begin f:=42; end; initialization writeln(1); "
        "finalization writeln(2); end.";
    const auto r = parser.parse(text);
    const auto* u = as<ast::Unit>(r.value);
    const auto* signature = u && u->interfaceDecls.size() == 1
                                ? as<ast::FuncDecl>(u->interfaceDecls[0])
                                : nullptr;
    const auto* definition = u && u->implementationDecls.size() == 1
                                 ? as<ast::FuncDecl>(u->implementationDecls[0])
                                 : nullptr;
    check(u && u->name.size() == 1 && u->name[0] == "u" && signature &&
              !signature->body && definition && definition->body &&
              !u->initialization.empty() && !u->finalization.empty() &&
              signature->parent == u && definition->parent == u,
          "unit sections and routine signatures retain distinct ownership");
  }
  {
    const std::string text =
        "program p; begin writeln(1 shl 2 + 1 xor 3 and 1); end.";
    const auto r = parser.parse(text);
    const auto* x = as<ast::BinaryExpr>(argument(r));
    const auto* a = x ? as<ast::BinaryExpr>(x->left) : nullptr;
    const auto* s = a ? as<ast::BinaryExpr>(a->left) : nullptr;
    const auto* n = x ? as<ast::BinaryExpr>(x->right) : nullptr;
    check(x && x->op == "xor" && a && a->op == "+" && s && s->op == "shl" &&
              n && n->op == "and",
          "native bit-operator precedence (Free Pascal prints 4)");
  }
  {
    const std::string text =
        "program p; var &begin:integer; begin &begin:=2; "
        "writeln(true,chr(x+1)); end.";
    const auto r = parser.parse(text);
    const auto* p = as<ast::Program>(r.value);
    const auto* v = p ? as<ast::VarDecl>(p->block->decls[0]) : nullptr;
    const auto* b = block(r);
    const auto* s = b && b->statements.size() > 1
                        ? as<ast::ExprStmt>(b->statements[1])
                        : nullptr;
    const auto* c = s ? as<ast::CallExpr>(s->value) : nullptr;
    const auto* t = c && c->args.size() == 2
                        ? as<ast::NameExpr>(c->args[0]->value)
                        : nullptr;
    const auto* chr = c && c->args.size() == 2
                          ? as<ast::CallExpr>(c->args[1]->value)
                          : nullptr;
    const auto* name = chr ? as<ast::NameExpr>(chr->callee) : nullptr;
    check(v && v->names.size() == 1 && v->names[0] == "begin" && t &&
              t->name == "true" && name && name->name == "chr",
          "escaped names and predefined identifiers stay ordinary names");
  }
  for (const char* text :
       {"program p; begin writeln('a\nb'); end.",
        "program p; begin writeln(#1114112); end.",
        "program p; begin 1:=2; end.", "program p; begin 1+2; end."}) {
    const auto r = parser.parse(text);
    const auto validation = cursiv::test::validationHelper(services, text);
    check(r.accepted && !r.cst().recovered() && !validation.diagnostics.empty(),
          "invalid source reaches runtime validation");
  }
  std::printf(
      "pascal: %zu native syntax cases, AST values and complete ownership\n",
      std::size(cases));
  return ok;
}
}  // namespace native_regressions
