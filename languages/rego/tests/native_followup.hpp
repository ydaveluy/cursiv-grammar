#pragma once
#include <cstdio>
#include <string>
#include "cursiv/test/ValidationHelper.hpp"
inline int rego_native_followup(const rego::RegoCoreServices& services) {
  for(const auto source:{
      "package audit\np := data.default\n", "package audit\ndefault.foo := 1\n",
      "package audit\np if {package.foo}\n", "package audit\nimport data.default as x\n",
      "package audit\np := .5\n", "package audit\np := [,1]\n",
      "package audit\np if {}\n", "package audit\np := [1](2)\n",
      "package audit\ndefault p := [x | x:=1]\n",
      "package a[\"b\"]\nimport data[\"a\"] as b\np:=true\n",
      "package audit\nif := 1\ncontains := 2\nin := 3\nevery := 4\n"}) {
    const auto r=services.parser->parse(source);
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||
       !cursiv::test::validationHelper(services,source).diagnostics.empty()) {
      std::fprintf(stderr,"rego native-valid control failed: %s\n",source);return 1;
    }
  }
  for(const auto source:{
      "package audit\np := package.foo\n", "package audit\np := true.x\n",
      "package audit\nimport data.default\n", "package audit\ndefault f(x,x) := true\n",
      "package audit\np := [,]\n", "package audit\np:=not\n",
      "package audit\nwith:=1\n", "package audit\np:=package\n",
      "package a[x]\n", "package a[1]\n", "package audit\nimport data.a[x]\n",
      "package audit\nimport data.a as b.c\n", "package audit\ndefault p:=f(1)\n",
      "package audit\ndefault p:={\"a\":x}\n", "package audit\np:=1(2)\n",
      "package audit\np:=f()(2)\n", "package audit\np contains x if {x:=1} else:=2\n",
      "package audit\np:=[x | ]\n"})
    if(cursiv::test::validationHelper(services,source).diagnostics.empty()) {
      std::fprintf(stderr,"rego native-invalid form undiagnosed: %s\n",source);return 1;
    }
  const std::string source="package audit\np if {}\n";
  const auto r=services.parser->parse(source);
  const auto* d=r.value_as<rego::ast::Document>();
  const auto* rule=d&&d->statements.size()==2?cursiv::ast_cast<rego::ast::Rule>(d->statements[1]):nullptr;
  const auto* body=rule&&rule->bodies.size()==1?rule->bodies[0]:nullptr;
  const auto* q=body?body->body:nullptr;
  const auto* lit=q&&q->literals.size()==1?cursiv::ast_cast<rego::ast::Literal>(q->literals[0]):nullptr;
  if(!lit||!lit->expr||!cursiv::ast_cast<rego::ast::ObjectLit>(lit->expr->first)||r.cst().text()!=source) {
    std::fprintf(stderr,"rego: if {} lost its native empty-object literal\n");return 1;
  }
  return 0;
}
