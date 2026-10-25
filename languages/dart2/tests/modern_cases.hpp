#pragma once
#include "dart2/core/ast.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace dart2_audit {
namespace ast = dart2::ast;
template <class T, class Result>
std::vector<const T *> nodes(const Result &result) {
  std::vector<const T *> found;
  for (const auto *node : result.nodes)
    if (const auto *value = cursiv::ast_cast<T>(node))
      found.push_back(value);
  return found;
}
template <class Parser> bool modern_cases(const Parser &parser) {
  struct Case {
    std::string_view name;
    std::string_view source;
  };
  static constexpr Case cases[] = {
    {"await-yield-identifiers", R"AUDIT(void main() { var sync = 1; var async = 2; var await = 3; var yield = 4; }
)AUDIT"},
    {"bom", R"AUDIT(﻿void main() {}
)AUDIT"},
    {"class-named-interface", R"AUDIT(class C {}
C? interface;
)AUDIT"},
    {"comments-nested", R"AUDIT(/* first /* nested */ last */ void f() {}
)AUDIT"},
    {"constructor-tearoff", R"AUDIT(class C { C(); }
var maker = C.new;
)AUDIT"},
    {"empty-unit", R"AUDIT(
)AUDIT"},
    {"enhanced-enum", R"AUDIT(enum E { a(1), b.named(2); final int value; const E(this.value); const E.named(this.value); int get twice => value * 2; }
)AUDIT"},
    {"enum-interfaces", R"AUDIT(mixin M {}
abstract class I {}
enum E with M implements I { a; }
)AUDIT"},
    {"enum-member-empty", R"AUDIT(enum E { a; }
)AUDIT"},
    {"external-fields", R"AUDIT(external int top; class C { external int x; external static int y; }
)AUDIT"},
    {"field-function-parameter", R"AUDIT(class C { final void Function() f; C(void this.f()); }
)AUDIT"},
    {"generic-constructor-tearoff", R"AUDIT(class C<T> { C.named(); } var f = C<int>.new; var g = C<int>.named;
)AUDIT"},
    {"generic-enum", R"AUDIT(enum E<T> { a<int>(1); final T value; const E(this.value); }
)AUDIT"},
    {"generic-lambda", R"AUDIT(void f() { var id = <T>(T x) => x; }
)AUDIT"},
    {"generic-metadata", R"AUDIT(class A<T> { const A(); } @A<int>() class C {}
)AUDIT"},
    {"generic-typedef-returnless", R"AUDIT(typedef F<T>(T x);
)AUDIT"},
    {"implicit-mixin", R"AUDIT(mixin M {}
class C with M {}
)AUDIT"},
    {"legacy-typedef", R"AUDIT(typedef int F<T>(T x);
)AUDIT"},
    {"mixin-application", R"AUDIT(mixin M {} class C = Object with M;
)AUDIT"},
    {"nested-generics-and-operators", R"AUDIT(void f() { var x = <List<int>>[]; var a = 1 < 2; var b = 8 >> 1; var c = 8 >>> 1; var d = fun<int>(); }
)AUDIT"},
    {"null-aware-index", R"AUDIT(void f() { var x = values?[0]; }
)AUDIT"},
    {"parameter-forms", R"AUDIT(void f(int x, [int y = 2,]) {}
void g({required int x, String y = "v",}) {}
void h(void callback(int v), int n, p) {}
)AUDIT"},
    {"script-bom", R"AUDIT(﻿#!/usr/bin/env dart
void main() {}
)AUDIT"},
    {"script-tag", R"AUDIT(#!/usr/bin/env dart
void main() {}
)AUDIT"},
    {"string-dollar-boundary", R"AUDIT(var a = "abc$foo$bar";
)AUDIT"},
    {"string-interpolation", R"AUDIT(var s = "hello $name ${f(1)}";
)AUDIT"},
    {"super-parameters", R"AUDIT(class B { B(int x,{required int y}); }
class C extends B { C(super.x,{required super.y}); }
)AUDIT"},
    {"symbol-operator", R"AUDIT(var x = #+;
)AUDIT"},
    {"symbol-spaced", R"AUDIT(var a = # +; var b = #void; var c = #foo.bar; var d = #[]=;
)AUDIT"},
    {"trailing-parameter-comma", R"AUDIT(void f(int x,) {}
typedef F = void Function(int,);
)AUDIT"},
    {"type-arguments-function", R"AUDIT(void f() { var id = function<int>; }
)AUDIT"},
    {"typed-field-parameters", R"AUDIT(class B { B(int x); }
class C extends B { int value; C(int super.x, int this.value); }
)AUDIT"},
    {"unnamed-library", R"AUDIT(@deprecated library; void main() {}
)AUDIT"},
    {"unsigned-shift", R"AUDIT(void f() { var x = value >>> 2; value >>>= 1; }
)AUDIT"},
  };
  bool all = true;
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    using namespace ast;
    const auto* unit = cursiv::ast_cast<CompilationUnit>(result.value);
    bool ok = result.accepted && !result.cst().recovered() && result.diagnostics.empty() && unit;
    if (test.name == "implicit-mixin") {
      const auto values = nodes<ClassDecl>(result);
      ok &= values.size() == 1 && !values[0]->superclass && values[0]->mixins.size() == 1;
    } else if (test.name == "super-parameters" || test.name == "typed-field-parameters") {
      bool super = false, field = false;
      for (const auto* p : nodes<Param>(result)) {
        super |= p->isSuper && p->name == "x" && (test.name != "typed-field-parameters" || p->type);
        field |= p->isThis && p->name == "value" && p->type;
      }
      ok &= super && (test.name != "typed-field-parameters" || field);
    } else if (test.name == "field-function-parameter") {
      const auto values = nodes<Param>(result);
      ok &= values.size() == 1 && values[0]->isThis && values[0]->name == "f" &&
            values[0]->type && values[0]->functionSuffix && values[0]->functionSuffix->params;
    } else if (test.name == "enhanced-enum") {
      const auto values = nodes<EnumDecl>(result);
      ok &= values.size() == 1;
      if (values.size() == 1) {
        const auto* e = values[0];
        ok &= e->entries.size() == 2 && e->members.size() == 4;
        if (e->entries.size() == 2)
          ok &= e->entries[0]->name == "a" && e->entries[0]->args &&
                e->entries[0]->args->items.size() == 1 &&
                e->entries[1]->constructorName == "named" && e->entries[1]->args &&
                e->entries[1]->args->items.size() == 1;
      }
    } else if (test.name == "generic-enum") {
      const auto values = nodes<EnumDecl>(result);
      ok &= values.size() == 1 && values[0]->typeParameters.size() == 1 &&
            values[0]->entries.size() == 1 && values[0]->entries[0]->typeArguments.size() == 1 &&
            values[0]->entries[0]->args && values[0]->members.size() == 2;
    } else if (test.name == "enum-interfaces") {
      const auto values = nodes<EnumDecl>(result);
      ok &= values.size() == 1 && values[0]->mixins.size() == 1 && values[0]->interfaces.size() == 1;
    } else if (test.name == "enum-member-empty") {
      const auto values = nodes<EnumDecl>(result);
      ok &= values.size() == 1 && values[0]->entries.size() == 1 && values[0]->members.empty();
    } else if (test.name == "legacy-typedef" || test.name == "generic-typedef-returnless") {
      const auto values = nodes<TypedefDecl>(result);
      ok &= values.size() == 1 && values[0]->typeParameters.size() == 1 &&
            values[0]->functionParams && values[0]->functionParams->positional.size() == 1 &&
            bool(values[0]->functionReturnType) == (test.name == "legacy-typedef");
    } else if (test.name == "trailing-parameter-comma") {
      const auto functions = nodes<TopLevelFunction>(result);
      const auto types = nodes<ParamTypeList>(result);
      ok &= functions.size() == 1 && functions[0]->params && functions[0]->params->positional.size() == 1 &&
            types.size() == 1 && types[0]->positional.size() == 1;
    } else if (test.name == "script-tag" || test.name == "script-bom") {
      ok &= unit && unit->scriptTag.starts_with("#!") && unit->declarations.size() == 1;
    } else if (test.name == "bom") {
      ok &= unit && unit->declarations.size() == 1;
    } else if (test.name == "generic-lambda") {
      const auto values = nodes<FunctionExpr>(result);
      ok &= values.size() == 1 && values[0]->typeParameters.size() == 1 && values[0]->params &&
            values[0]->params->positional.size() == 1 && values[0]->bodyExpr;
    } else if (test.name == "constructor-tearoff" || test.name == "generic-constructor-tearoff") {
      bool found = false;
      for (const auto* value : nodes<MemberExpr>(result))
        found |= value->property == "new" && value->object &&
                 (test.name != "generic-constructor-tearoff" || cursiv::ast_cast<TypeApplyExpr>(value->object));
      ok &= found;
    } else if (test.name == "type-arguments-function") {
      const auto values = nodes<TypeApplyExpr>(result);
      ok &= values.size() == 1 && values[0]->typeArguments.size() == 1 && values[0]->expression;
    } else if (test.name == "symbol-operator" || test.name == "symbol-spaced") {
      const auto values = nodes<SymbolLit>(result);
      ok &= values.size() == (test.name == "symbol-operator" ? 1u : 4u);
      for (const auto* value : values) ok &= value->text.starts_with("#");
    } else if (test.name == "generic-metadata") {
      bool found = false;
      for (const auto* value : nodes<Metadata>(result))
        found |= value->name == "A" && value->typeArguments.size() == 1 && value->args;
      ok &= found;
    } else if (test.name == "parameter-forms") {
      bool optional = false, named = false, callback = false;
      for (const auto* value : nodes<ParamList>(result)) {
        optional |= value->optionalPositional.size() == 1 && value->optionalPositional[0]->defaultValue;
        named |= value->named.size() == 2 && value->named[0]->isRequired && value->named[1]->defaultValue;
      }
      for (const auto* value : nodes<Param>(result))
        callback |= value->name == "callback" && value->type && value->functionSuffix &&
                    value->functionSuffix->params && value->functionSuffix->params->positional.size() == 1;
      ok &= optional && named && callback;
    } else if (test.name == "unnamed-library") {
      ok &= unit && unit->libraryName && unit->libraryName->parts.empty() &&
            unit->libraryName->annotations.size() == 1 &&
            unit->libraryName->annotations[0]->name == "deprecated" && unit->declarations.size() == 1;
    } else if (test.name == "external-fields") {
      const auto values = nodes<VarSpec>(result);
      bool staticField = false;
      ok &= values.size() == 3;
      for (const auto* value : values) {
        ok &= value->isExternal && value->type && value->vars.size() == 1;
        staticField |= value->isStatic;
      }
      ok &= staticField;
    } else if (test.name == "unsigned-shift") {
      bool found = false;
      for (const auto* value : nodes<BinaryExpr>(result)) found |= value->op == ">>>";
      ok &= found;
    } else if (test.name == "string-dollar-boundary") {
      bool foo = false, bar = false;
      for (const auto* value : nodes<Ident>(result)) {
        foo |= value->name == "foo";
        bar |= value->name == "bar";
      }
      ok &= foo && bar;
    } else if (test.name == "empty-unit") {
      ok &= unit && unit->declarations.empty() && !unit->exprEntry && !unit->stmtEntry;
    }
    if (!ok) {
      std::fprintf(stderr, "dart2 audit: %.*s failed (accepted=%d recovered=%d diagnostics=%zu)\n",
                   static_cast<int>(test.name.size()), test.name.data(), result.accepted,
                   result.cst().recovered(), result.diagnostics.size());
      all = false;
    }
  }
  return all;
}
} // namespace dart2_audit
