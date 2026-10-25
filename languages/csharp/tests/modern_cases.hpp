#pragma once

#include <cstdio>
#include <string_view>
#include <vector>

#include "csharp/core/ast.hpp"

namespace csharp_audit {
namespace ast = csharp::ast;

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
      {"zero-parameter-increment", "struct C { public void operator ++() {} }"},
      {"checked-zero-parameter-increment",
       "struct C { public void operator checked ++() {} }"},
      {"checked-operator",
       "struct C { public static C operator checked +(C a,C b) => a; }"},
      {"checked-conversion",
       "struct C { public static explicit operator checked int(C a) => 0; }"},
      {"compound-operator",
       "struct C { public void operator +=(C other) {} }"},
      {"compound-shift-operator",
       "struct C { public void operator >>>=(int count) {} }"},
      {"operator-no-modifiers", "struct C { C operator +(C a,C b) => a; }"},
      {"extended-property",
       "class C { bool M(object x) => x is { P.Q.R: 1, Other: { Value: 2 } }; }"},
      {"compat-ws", "class\x1a C {}"},
      {"with-precedence",
       "class C { object M(C a) => -a with { value = 2 }; int value; }"},
      {"switch-chain",
       "class C { int M(int x) => x switch { _ => 1 } switch { _ => 2 }; }"},
      {"array-creation", "class C { int[] x = new int[2]; }"},
      {"anonymous-object", "class C { object value = new { X = 1 }; }"},
      {"type-pattern-combination",
       "class C { bool M(object x) => x is int and > 0; }"},
      {"collection-expression",
       R"AUDIT(class C { int[] x = [1, 2, ..other]; int[] other; })AUDIT"},
      {"file-namespace", R"AUDIT(namespace N; class C {})AUDIT"},
      {"function-pointer",
       R"AUDIT(unsafe class C { delegate* unmanaged[Cdecl]<ref int, void> ptr; })AUDIT"},
      {"global-using",
       R"AUDIT(global using static System.Math; class C {})AUDIT"},
      {"identifier-boundary", R"AUDIT(class C { int classé; })AUDIT"},
      {"lambda-return-default",
       R"AUDIT(class C { object f = static int (int x = 1) => x; })AUDIT"},
      {"list-pattern",
       R"AUDIT(class C { bool M(int[] x) => x is [1, .. var tail]; })AUDIT"},
      {"pattern-combination",
       R"AUDIT(class C { bool M(int x) => x is >= 1 and < 10 or not 0; })AUDIT"},
      {"primary-constructor", R"AUDIT(class Point(int x) { int X = x; })AUDIT"},
      {"raw-quote-run",
       R"AUDIT(class C { string s = """"one """ quote""""; })AUDIT"},
      {"raw-string", R"AUDIT(class C { string s = """hello"""; })AUDIT"},
      {"record-struct", R"AUDIT(readonly record struct Point(int X);)AUDIT"},
      {"record", R"AUDIT(public record Point(int X, int Y);)AUDIT"},
      {"required-init",
       R"AUDIT(class C { public required string Name { get; init; } })AUDIT"},
      {"static-lambda",
       R"AUDIT(class C { System.Func<int,int> f = static x => x + 1; })AUDIT"},
      {"target-new", R"AUDIT(class C { C value = new(); })AUDIT"},
      {"top-level", R"AUDIT(System.Console.WriteLine("hello");)AUDIT"},
      {"unicode-char", R"AUDIT(class C { char c = 'é'; })AUDIT"},
      {"unicode-comment", R"AUDIT(// comment class C {})AUDIT"},
      {"unicode-ws", R"AUDIT(class C {})AUDIT"},
      {"unsigned-shift",
       R"AUDIT(class C { int M(int n) { n >>>= 1; return n >>> 2; } })AUDIT"},
      {"utf8-string", R"AUDIT(class C { object s = "abc"u8; })AUDIT"},
      {"with",
       R"AUDIT(class C { C M(C c) => c with { value = 2 }; int value; })AUDIT"},
  };
  for (const auto &test : cases) {
    const auto result = parser.parse(test.source);
    const auto unit = cursiv::ast_cast<ast::CompilationUnit>(result.value);
    bool ok = result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && unit;
    using namespace ast;
    const auto classes = nodes<ClassDecl>(result);
    const auto lambdas = nodes<LambdaExpr>(result);
    if (test.name == "checked-operator" || test.name == "checked-conversion" ||
        test.name == "compound-operator" || test.name == "compound-shift-operator" ||
        test.name == "operator-no-modifiers" ||
        test.name == "zero-parameter-increment" ||
        test.name == "checked-zero-parameter-increment") {
      const auto values = nodes<OperatorDecl>(result);
      ok &= values.size() == 1;
      if (values.size() == 1) {
        const auto *value = values[0];
        ok &= value->returnType && (value->body || value->exprBody);
        if (test.name == "zero-parameter-increment" ||
            test.name == "checked-zero-parameter-increment")
          ok &= value->op == "++" && value->params.empty() &&
                value->isChecked == (test.name == "checked-zero-parameter-increment");
        else if (test.name == "checked-operator")
          ok &= value->isChecked && value->op == "+" && value->params.size() == 2;
        else if (test.name == "checked-conversion")
          ok &= value->isChecked && value->opKind == "explicit" &&
                value->op.empty() && value->params.size() == 1;
        else if (test.name == "compound-operator")
          ok &= !value->isChecked && value->op == "+=" && value->params.size() == 1;
        else if (test.name == "compound-shift-operator")
          ok &= !value->isChecked && value->op == ">>>=" && value->params.size() == 1;
        else
          ok &= value->modifiers.empty() && value->op == "+";
      }
    } else if (test.name == "extended-property") {
      const auto values = nodes<Subpattern>(result);
      bool path = false, other = false, inner = false;
      for (const auto *value : values) {
        if (const auto *member = cursiv::ast_cast<MemberExpr>(value->expression)) {
          const auto *target = cursiv::ast_cast<MemberExpr>(member->target);
          const auto *root = target ? cursiv::ast_cast<NameExpr>(target->target) : nullptr;
          path |= value->name.empty() && member->member == "R" && target &&
                  target->member == "Q" && root && root->name == "P" && value->value;
        }
        other |= value->name == "Other" && !value->expression &&
                 cursiv::ast_cast<PropertyPattern>(value->value);
        inner |= value->name == "Value" && !value->expression && value->value;
      }
      ok &= values.size() == 3 && path && other && inner;
    } else if (test.name == "with-precedence") {
      const auto values = nodes<WithExpr>(result);
      ok &=
          values.size() == 1 && cursiv::ast_cast<PrefixExpr>(values[0]->target);
    } else if (test.name == "switch-chain") {
      const auto values = nodes<SwitchExpr>(result);
      bool nested = false;
      for (const auto *value : values)
        nested |= cursiv::ast_cast<SwitchExpr>(value->value) != nullptr;
      ok &= values.size() == 2 && nested;
    } else if (test.name == "array-creation") {
      const auto values = nodes<ArrayCreationExpr>(result);
      ok &= values.size() == 1 && values[0]->sizes.size() == 1 &&
            nodes<IndexExpr>(result).empty() &&
            nodes<ObjectCreationExpr>(result).empty();
    } else if (test.name == "anonymous-object") {
      const auto values = nodes<AnonymousObjectCreationExpr>(result);
      ok &= values.size() == 1 && values[0]->members.size() == 1 &&
            values[0]->members[0]->name == "X" &&
            nodes<ObjectCreationExpr>(result).empty();
    } else if (test.name == "type-pattern-combination") {
      const auto values = nodes<TypePattern>(result);
      ok &= values.size() == 1 && values[0]->typeRef &&
            nodes<BinaryPattern>(result).size() == 1;
    } else if (test.name == "record")
      ok &= classes.size() == 1 && classes[0]->isRecord &&
            classes[0]->name == "Point" &&
            classes[0]->primaryParams.size() == 2;
    else if (test.name == "record-struct") {
      const auto values = nodes<StructDecl>(result);
      ok &= values.size() == 1 && values[0]->isRecord &&
            values[0]->primaryParams.size() == 1;
    } else if (test.name == "primary-constructor")
      ok &= classes.size() == 1 && classes[0]->primaryParams.size() == 1 &&
            classes[0]->primaryParams[0]->name == "x";
    else if (test.name == "file-namespace") {
      const auto values = nodes<NamespaceDecl>(result);
      ok &= values.size() == 1 && values[0]->fileScoped &&
            values[0]->members.size() == 1;
    } else if (test.name == "global-using") {
      const auto values = nodes<UsingDirective>(result);
      ok &= values.size() == 1 && values[0]->isGlobal &&
            values[0]->kind == "static";
    } else if (test.name == "target-new") {
      const auto values = nodes<ObjectCreationExpr>(result);
      ok &=
          values.size() == 1 && !values[0]->typeRef && values[0]->args.empty();
    } else if (test.name == "required-init") {
      const auto values = nodes<PropertyDecl>(result);
      ok &= values.size() == 1 && values[0]->modifiers.size() == 2 &&
            values[0]->modifiers[1] == "required" &&
            values[0]->accessors.size() == 2 &&
            values[0]->accessors[1]->kind == "init";
    } else if (test.name == "with") {
      const auto values = nodes<WithExpr>(result);
      ok &= values.size() == 1 && values[0]->target && values[0]->init &&
            values[0]->init->members.size() == 1 &&
            values[0]->init->members[0]->name == "value";
    } else if (test.name == "unsigned-shift") {
      const auto binary = nodes<BinaryExpr>(result);
      const auto assign = nodes<AssignExpr>(result);
      ok &= binary.size() == 1 && binary[0]->op == ">>>" &&
            assign.size() == 1 && assign[0]->op == ">>>=";
    } else if (test.name == "pattern-combination") {
      const auto binary = nodes<BinaryPattern>(result);
      const auto unary = nodes<UnaryPattern>(result);
      const auto relational = nodes<RelationalPattern>(result);
      bool precedence = false;
      for (const auto *node : binary)
        if (node->op == "or")
          if (const auto *left = cursiv::ast_cast<BinaryPattern>(node->left))
            precedence = left->op == "and";
      ok &= binary.size() == 2 && unary.size() == 1 && unary[0]->op == "not" &&
            relational.size() == 2 && precedence;
    } else if (test.name == "list-pattern") {
      const auto lists = nodes<ListPattern>(result);
      const auto slices = nodes<SlicePattern>(result);
      const auto vars = nodes<VarPattern>(result);
      ok &= lists.size() == 1 && lists[0]->items.size() == 2 &&
            slices.size() == 1 && slices[0]->pattern && vars.size() == 1 &&
            vars[0]->designation && vars[0]->designation->name == "tail";
    } else if (test.name == "collection-expression") {
      const auto values = nodes<CollectionExpr>(result);
      ok &= values.size() == 1 && values[0]->elements.size() == 3 &&
            !values[0]->elements[0]->spread && values[0]->elements[2]->spread;
    } else if (test.name == "static-lambda")
      ok &= lambdas.size() == 1 && lambdas[0]->isStatic &&
            lambdas[0]->params.size() == 1 &&
            lambdas[0]->params[0]->name == "x";
    else if (test.name == "lambda-return-default")
      ok &= lambdas.size() == 1 && lambdas[0]->isStatic &&
            lambdas[0]->returnType && lambdas[0]->params.size() == 1 &&
            lambdas[0]->params[0]->defaultValue;
    else if (test.name == "function-pointer") {
      bool found = false;
      for (const auto *value : nodes<Type>(result))
        if (value->functionPointer)
          found = value->callingConvention == "unmanaged" &&
                  value->callingConventions.size() == 1 &&
                  value->callingConventions[0] == "Cdecl" &&
                  value->functionParameters.size() == 2 &&
                  value->functionParameters[0]->modifiers.size() == 1 &&
                  value->functionParameters[0]->modifiers[0] == "ref";
      ok &= found;
    } else if (test.name == "unicode-char") {
      const auto values = nodes<LiteralExpr>(result);
      ok &= values.size() == 1 && values[0]->text == "'é'";
    } else if (test.name == "utf8-string" || test.name == "raw-string" ||
               test.name == "raw-quote-run") {
      const auto values = nodes<LiteralExpr>(result);
      ok &= values.size() == 1 &&
            test.source.find(values[0]->text) != std::string_view::npos;
      if (test.name == "utf8-string")
        ok &= values.size() == 1 && values[0]->text.ends_with("u8");
      if (test.name == "raw-quote-run")
        ok &= values.size() == 1 && values[0]->text.starts_with("\"\"\"\"") &&
              values[0]->text.ends_with("\"\"\"\"");
    } else if (test.name == "top-level")
      ok &= unit && unit->statements.size() == 1;
    else if (test.name == "identifier-boundary") {
      const auto values = nodes<Declarator>(result);
      ok &= values.size() == 1 && values[0]->name == "classé";
    } else if (test.name == "unicode-ws" || test.name == "unicode-comment" ||
               test.name == "compat-ws")
      ok &= classes.size() == 1 && classes[0]->name == "C";
    if (!ok) {
      std::fprintf(stderr, "csharp audit: %.*s failed\n",
                   static_cast<int>(test.name.size()), test.name.data());
      return false;
    }
  }
  return true;
}
} // namespace csharp_audit
