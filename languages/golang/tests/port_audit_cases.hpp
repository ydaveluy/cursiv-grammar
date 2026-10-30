#pragma once

#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "golang/core/ast.hpp"

namespace port_audit_cases {

inline bool run(const cursiv::Parser& parser) {
  using namespace golang;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "golang port audit: %s\n", message);
      ok = false;
    }
  };
  const auto file = [&](const cursiv::ParseResult& r) {
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && r.value,
          "expected a clean parse");
    return cursiv::ast_cast<ast::SourceFile>(r.value);
  };
  for (const auto source : {
           "package for\n", "package p\nvar if = 1\n",
           "package p\ntype struct int\n", "package p\nvar _ = '\\'\n",
           "package p\nvar _ = '\\\"'\n", "package p\nvar _ = \"\\'\"\n",
           "package p\nvar _ = '\\400'\n", "package p\nvar _ = \"\\777\"\n",
           "package p\nvar _ = '\\uD800'\n", "package p\nvar _ = \"\\U00110000\"\n"}) {
    const auto r = parser.parse(source);
    check(!r.accepted || r.cst().recovered() || !r.diagnostics.empty(),
          "reserved keywords and native-invalid rune/string escapes are refused");
  }
  {
    const auto r = parser.parse(R"GO(package p
var runeValue = '\U0010FFFF'
var quote = '\''
var text = "\"\377\u00FF"
var returnValue = 1
)GO");
    const auto* f = file(r);
    const auto* d = f && f->decls.size() == 4 ?
        cursiv::ast_cast<ast::VarDecl>(f->decls[2]) : nullptr;
    const auto* text = d && d->specs.size() == 1 && d->specs[0]->values.size() == 1 ?
        cursiv::ast_cast<ast::StringLit>(d->specs[0]->values[0]) : nullptr;
    check(text && text->text == R"GO("\"\377\u00FF")GO",
          "valid escapes retain raw Go AST spelling; keyword prefixes remain names");
  }
  {
    const auto r = parser.parse("\xEF\xBB\xBFpackage p\nvar value=1\n");
    const auto* f = file(r);
    const auto* d = f && f->decls.size() == 1
        ? cursiv::ast_cast<ast::VarDecl>(f->decls[0]) : nullptr;
    check(f && f->packageName == "p" && d && d->specs.size() == 1 &&
              d->specs[0]->names.size() == 1 && d->specs[0]->names[0] == "value",
          "initial BOM preserves package and declarations");
  }
  {
    const auto r = parser.parse(
        "package p\ntype S struct { T; *pkg.T; G[int]; X,Y int }\n");
    const auto* f = file(r);
    const auto* d = f && f->decls.size() == 1
        ? cursiv::ast_cast<ast::TypeDecl>(f->decls[0]) : nullptr;
    const auto* s = d && d->specs.size() == 1
        ? cursiv::ast_cast<ast::StructType>(d->specs[0]->type) : nullptr;
    check(s && s->fields.size() == 4, "struct retains every field");
    if (s && s->fields.size() == 4) {
      for (unsigned i = 0; i < 3; ++i)
        check(s->fields[i]->embedded && s->fields[i]->names.empty() &&
                  s->fields[i]->parent == s,
              "embedded fields retain their status and ownership");
      const auto* t = cursiv::ast_cast<ast::NamedType>(s->fields[1]->type);
      const auto* generic = cursiv::ast_cast<ast::NamedType>(s->fields[2]->type);
      check(s->fields[1]->embeddedPointer && t && t->pkg == "pkg" &&
                t->name == "T" && generic && generic->name == "G" &&
                generic->typeArgs.size() == 1 && !s->fields[3]->embedded &&
                s->fields[3]->names.size() == 2,
            "pointer, generic and named fields remain distinct");
    }
  }
  {
    const auto r = parser.parse(
        "package p\nfunc f(){make:=g;make(1);new:=g;new(2);nil:=g;nil();_ = nil}\n");
    const auto* f = file(r);
    const auto* fn = f && f->decls.size() == 1
        ? cursiv::ast_cast<ast::FuncDecl>(f->decls[0]) : nullptr;
    const auto* b = fn ? fn->body : nullptr;
    check(b && b->stmts.size() == 7, "predeclared names can be shadowed");
    if (b && b->stmts.size() == 7) {
      const char* names[] = {"make", "new", "nil"};
      for (unsigned i = 0; i < 3; ++i) {
        const auto* d = cursiv::ast_cast<ast::ShortVarDecl>(b->stmts[2*i]);
        const auto* e = cursiv::ast_cast<ast::ExprStmt>(b->stmts[2*i+1]);
        const auto* call = e ? cursiv::ast_cast<ast::CallExpr>(e->value) : nullptr;
        const auto* name = call ? cursiv::ast_cast<ast::Ident>(call->callee) : nullptr;
        check(d && d->names.size() == 1 && d->names[0] == names[i] && name &&
                  name->name == names[i] && name->parent == call,
              "shadowed builtins retain ordinary declarations and calls");
      }
      const auto* assign = cursiv::ast_cast<ast::AssignStmt>(b->stmts[6]);
      const auto* name = assign && assign->rhs.size() == 1
          ? cursiv::ast_cast<ast::Ident>(assign->rhs[0]) : nullptr;
      check(name && name->name == "nil", "nil remains an identifier in expressions");
    }
  }
  {
    const auto r = parser.parse("package p\nvar _ = interface{M()}.M\n");
    const auto* f = file(r);
    const auto* d = f && f->decls.size() == 1
        ? cursiv::ast_cast<ast::VarDecl>(f->decls[0]) : nullptr;
    const auto* value = d && d->specs.size() == 1 && d->specs[0]->values.size() == 1
        ? cursiv::ast_cast<ast::MethodExpr>(d->specs[0]->values[0]) : nullptr;
    const auto* type = value ? cursiv::ast_cast<ast::InterfaceType>(value->type) : nullptr;
    check(value && value->method == "M" && type && type->methods.size() == 1 &&
              type->methods[0]->name == "M" && type->parent == value,
          "literal interface method expressions retain their type and method");
  }
  {
    const auto r = parser.parse("package p\nfunc f(x any){switch x.(type){case nil,int,nil:}}\n");
    const auto* f = file(r);
    const auto* fn = f && f->decls.size() == 1
        ? cursiv::ast_cast<ast::FuncDecl>(f->decls[0]) : nullptr;
    const auto* b = fn ? fn->body : nullptr;
    const auto* sw = b && b->stmts.size() == 1
        ? cursiv::ast_cast<ast::TypeSwitchStmt>(b->stmts[0]) : nullptr;
    const auto* c = sw && sw->cases.size() == 1 ? sw->cases[0] : nullptr;
    check(c && c->types.size() == 3, "type cases retain nil positions and duplicates for validation");
    if (c && c->types.size() == 3) {
      const char* names[] = {"nil", "int", "nil"};
      for (unsigned i = 0; i < 3; ++i) {
        const auto* t = cursiv::ast_cast<ast::NamedType>(c->types[i]);
        check(t && t->name == names[i] && t->parent == c,
              "type case spellings remain in source order");
      }
    }
  }
  return ok;
}

}
