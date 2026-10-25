#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "kotlin/core/ast.hpp"

namespace kotlin_audit {
namespace ast = kotlin::ast;

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
      {"nullable-extension", R"AUDIT(fun String?.sizeOrZero() = this?.length
val String?.copy: String? get()=this
)AUDIT"},
      {"annotations-in-bodies", R"AUDIT(fun f() {
 for (@A x in xs) {}
 consume @A { 1 }
}
)AUDIT"},
      {"annotation-group", "class C(@field:[A B] val x:Int)"},
      {"annotation-all", R"AUDIT(@all:Deprecated("x") val x=1)AUDIT"},
      {"annotation-typeargs", R"AUDIT(annotation class A<T>
@A<String> class C)AUDIT"},
      {"context-class", R"AUDIT(context(x:String) class C)AUDIT"},
      {"context-function-type",
       R"AUDIT(val f:context(String) () -> Unit = {})AUDIT"},
      {"context-property",
       R"AUDIT(context(x:String) val size:Int get()=x.length)AUDIT"},
      {"definitely-not-null",
       R"AUDIT(fun <T> f(x: T & Any): T & Any = x)AUDIT"},
      {"dollar-reference", R"AUDIT(val x=1
val s="$x")AUDIT"},
      {"fun-interface",
       R"AUDIT(fun interface Factory { fun create():Int })AUDIT"},
      {"generic-receiver",
       R"AUDIT(fun <T> List<T>.firstCopy():T = this[0])AUDIT"},
      {"not-in-no-space", R"AUDIT(fun f(x:Int)=x !in(1..2))AUDIT"},
      {"not-is-comment", R"AUDIT(fun f(x:Any)=x !is/* comment */String)AUDIT"},
      {"nullable-function-receiver",
       R"AUDIT(val f:String?.() -> Unit = {})AUDIT"},
      {"range-until", R"AUDIT(fun f() = 0..<10)AUDIT"},
      {"raw-trailing-quotes", R"AUDIT(val s="""one ""quote"""")AUDIT"},
      {"receiver", R"AUDIT(fun String.sizePlus(x:Int)=length+x)AUDIT"},
      {"unicode-label", R"AUDIT(fun f() = λ@ { return@λ 1 })AUDIT"},
      {"unsigned", R"AUDIT(val a=1u
val b=1uL
val c=0xFFUL
val d=0b11u)AUDIT"},
      {"value-class", R"AUDIT(@JvmInline value class Value(val x:Int))AUDIT"},
      {"when-guard",
       R"AUDIT(fun f(x:Any)=when(x){is String if x.length>0 -> 1; else -> 0})AUDIT"},
      {"when-subject-var",
       R"AUDIT(fun f(x:Int)=when(val y=x){1 -> y; else -> 0})AUDIT"},
  };
  for (const auto &test : cases) {
    const auto result = parser.parse(test.source);
    bool ok = result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() &&
              cursiv::ast_cast<ast::KotlinFile>(result.value);
    using namespace ast;
    const auto functions = nodes<FunctionDecl>(result);
    const auto properties = nodes<PropertyDecl>(result);
    if (test.name == "nullable-extension") {
      ok &= functions.size() == 1 &&
            cursiv::ast_cast<NullableType>(functions[0]->receiverType) &&
            properties.size() == 1 &&
            cursiv::ast_cast<NullableType>(properties[0]->receiverType);
      const auto values = nodes<MemberExpr>(result);
      ok &= values.size() == 1 && values[0]->safe &&
            values[0]->member == "length";
    } else if (test.name == "annotations-in-bodies") {
      const auto loops = nodes<ForExpr>(result);
      const auto lambdas = nodes<FunctionLiteral>(result);
      ok &= loops.size() == 1 && loops[0]->annotations.size() == 1 &&
            loops[0]->annotations[0]->name == "A" && lambdas.size() == 1 &&
            lambdas[0]->annotations.size() == 1 &&
            lambdas[0]->annotations[0]->name == "A";
    } else if (test.name == "annotation-group") {
      const auto values = nodes<AnnotationGroup>(result);
      ok &= values.size() == 1 && values[0]->useSiteTarget == "field" &&
            values[0]->annotations.size() == 2 &&
            values[0]->annotations[0]->name == "A" &&
            values[0]->annotations[1]->name == "B";
    } else if (test.name == "unsigned") {
      const auto literals = nodes<Literal>(result);
      ok &= literals.size() == 4 && literals[0]->text == "1u" &&
            literals[1]->text == "1uL" && literals[2]->text == "0xFFUL" &&
            literals[3]->text == "0b11u";
    } else if (test.name == "not-in-no-space" || test.name == "range-until") {
      bool found = false;
      for (const auto *value : nodes<BinaryExpr>(result))
        found |= value->op == (test.name == "range-until" ? "..<" : "!in");
      ok &= found;
    } else if (test.name == "not-is-comment") {
      const auto values = nodes<TypeTestExpr>(result);
      ok &= values.size() == 1 && values[0]->negated && values[0]->type;
    } else if (test.name == "receiver" || test.name == "generic-receiver") {
      ok &= functions.size() == 1 && functions[0]->receiverType;
      if (functions.size() == 1) {
        const auto *receiver =
            cursiv::ast_cast<UserType>(functions[0]->receiverType);
        ok &= receiver && receiver->segments.size() == 1 &&
              receiver->segments[0]->name ==
                  (test.name == "receiver" ? "String" : "List");
        if (test.name == "generic-receiver")
          ok &= functions[0]->typeParameters.size() == 1 && receiver &&
                receiver->segments[0]->typeArguments.size() == 1;
      }
    } else if (test.name == "definitely-not-null") {
      const auto values = nodes<IntersectionType>(result);
      ok &= values.size() == 2 && values[0]->left && values[0]->right &&
            values[1]->left && values[1]->right;
    } else if (test.name == "when-subject-var") {
      const auto values = nodes<WhenExpr>(result);
      ok &= values.size() == 1 && !values[0]->subject &&
            values[0]->subjectDeclaration &&
            values[0]->subjectDeclaration->declarators.size() == 1 &&
            values[0]->subjectDeclaration->declarators[0]->name == "y";
    } else if (test.name == "when-guard") {
      const auto values = nodes<WhenExpr>(result);
      ok &= values.size() == 1 && values[0]->entries.size() == 2 &&
            values[0]->entries[0]->guard && !values[0]->entries[1]->guard;
    } else if (test.name == "annotation-typeargs" ||
               test.name == "annotation-all") {
      bool found = false;
      for (const auto *value : nodes<Annotation>(result))
        if (test.name == "annotation-typeargs")
          found |= value->name == "A" && value->typeArguments.size() == 1 &&
                   value->typeArguments[0]->type;
        else
          found |= value->useSiteTarget == "all" && value->name == "Deprecated";
      ok &= found;
    } else if (test.name == "fun-interface" || test.name == "value-class" ||
               test.name == "context-class") {
      const auto values = nodes<ClassDecl>(result);
      ok &= values.size() == 1;
      if (values.size() == 1) {
        if (test.name == "context-class")
          ok &= values[0]->contextParameters.size() == 1 &&
                values[0]->contextParameters[0]->name == "x";
        else {
          bool modifier = false;
          for (auto text : values[0]->modifiers)
            modifier |=
                text == (test.name == "fun-interface" ? "fun" : "value");
          ok &= modifier &&
                (test.name != "fun-interface" || values[0]->isInterface);
        }
      }
    } else if (test.name == "context-property")
      ok &= properties.size() == 1 &&
            properties[0]->contextParameters.size() == 1 &&
            properties[0]->contextParameters[0]->name == "x";
    else if (test.name == "context-function-type" ||
             test.name == "nullable-function-receiver") {
      const auto values = nodes<FunctionType>(result);
      ok &= values.size() == 1;
      if (values.size() == 1) {
        if (test.name == "context-function-type")
          ok &= values[0]->contextParameters.size() == 1 &&
                values[0]->contextParameters[0]->type;
        else
          ok &= cursiv::ast_cast<NullableType>(values[0]->receiver) != nullptr;
      }
    } else if (test.name == "dollar-reference") {
      bool found = false;
      for (const auto *value : nodes<StringPart>(result))
        if (const auto *ref = cursiv::ast_cast<Ident>(value->expr))
          found |= ref->name == "x" && value->text.empty();
      ok &= found;
    } else if (test.name == "raw-trailing-quotes") {
      const auto values = nodes<StringLiteral>(result);
      std::string text;
      if (values.size() == 1)
        for (const auto *part : values[0]->parts)
          text += part->text;
      ok &= values.size() == 1 && values[0]->multiline &&
            text == "one \"\"quote\"";
    } else if (test.name == "unicode-label") {
      const auto values = nodes<ReturnExpr>(result);
      ok &= values.size() == 1 && values[0]->label == "λ";
    }
    if (!ok) {
      std::fprintf(stderr, "kotlin audit: %.*s failed\n",
                   static_cast<int>(test.name.size()), test.name.data());
      return false;
    }
  }
  return true;
}
} // namespace kotlin_audit
