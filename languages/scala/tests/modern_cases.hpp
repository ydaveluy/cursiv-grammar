#pragma once
#include "scala/core/ast.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace scala_audit {
namespace ast = scala::ast;
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
      {"empty-lambda", R"AUDIT(object X { val f = () => 1; val g = { () => 2 } }
)AUDIT"},
      {"interpolation-trivia", R"AUDIT(object X { val s = s" leading $name tail "; val t = s"${ /* c */ f(1) } suffix " }
)AUDIT"},
      {"multiline-trailing-commas", R"AUDIT(object X { def f[A,
](x: A,
) = x; val p = (1, 2,
); val y = f[Int,
](1,
) }
)AUDIT"},
      {"custom-interpolation", R"AUDIT(object X { val s = raw"hello $name" }
)AUDIT"},
      {"empty-package", R"AUDIT(package p {}
)AUDIT"},
      {"empty-refinement", R"AUDIT(object X { type T = A {} }
)AUDIT"},
      {"empty-unit", R"AUDIT(
)AUDIT"},
      {"existential-separators",
       R"AUDIT(object X { type T = List[A] forSome { type A; val b: Int } }
)AUDIT"},
      {"form-feed", R"AUDIT(object X { val x = 1 }
)AUDIT"},
      {"interpolation-block", R"AUDIT(object X { val s = s"${val x=1; x}" }
)AUDIT"},
      {"interpolation-multiline",
       R"AUDIT(object X { val s = s"""hello $name ${f(1)}"""" }
)AUDIT"},
      {"interpolation-multiple", R"AUDIT(object X { val s = s"$a$b $$ $this" }
)AUDIT"},
      {"interpolation-pattern",
       R"AUDIT(object X { val y = x match { case s"$a${b}" => a } }
)AUDIT"},
      {"interpolation", R"AUDIT(object X { val s = s"hello $name ${f(1)}" }
)AUDIT"},
      {"multiline-quotes", R"AUDIT(object X { val s = """one ""quote"""" }
)AUDIT"},
      {"multiple-u",
       R"AUDIT(object X { val s = "\uuuu0041"; val c = '\uuu0041' }
)AUDIT"},
      {"octal-escape", R"AUDIT(object X { val s = "\123"; val c = '\123' }
)AUDIT"},
      {"operators-comment",
       R"AUDIT(object X { val x = a +/* first */ b; val y = a +// last
 b }
)AUDIT"},
      {"package-only", R"AUDIT(package p

)AUDIT"},
      {"slash-operator", R"AUDIT(object X { def +/(x:Int)=x; val y = a +/ b }
)AUDIT"},
      {"string-escape", R"AUDIT(object X { val s = "\u00e9"; val c = '\u00e9' }
)AUDIT"},
      {"unicode-backtick", R"AUDIT(object X { val `nom é` = 1 }
)AUDIT"},
      {"unicode-identifiers",
       R"AUDIT(object 中文 { val 变量 = 1; val Ⅳ = 2; val á = 3 }
)AUDIT"},
      {"unicode-operators", R"AUDIT(object X { def ⊕(x:Int)=x; val z = a ⊕ b }
)AUDIT"},
      {"unicode-string", R"AUDIT(object X { val s = "été 中文"; val c = 'é' }
)AUDIT"},
      {"uppercase-hex", R"AUDIT(object X { val x = 0Xff }
)AUDIT"},
      {"unicode-arrows", R"AUDIT(object X { val f: Int ⇒ Int = (x: Int) ⇒ x; val xs = for (x ← ys) yield x; def byName(x: ⇒ Int) = x }
)AUDIT"},
      {"varargs", R"AUDIT(object X { def f(xs:Int*) = xs; val z = f(xs: _*) }
)AUDIT"},
  };
  bool all = true;
  for (const auto &test : cases) {
    const auto result = parser.parse(test.source);
    using namespace ast;
    const auto *unit = cursiv::ast_cast<CompilationUnit>(result.value);
    bool ok = result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && unit;
    if (test.name == "empty-unit")
      ok &= unit && unit->items.empty() && unit->packageClauses.empty();
    else if (test.name == "package-only")
      ok &= unit && unit->items.empty() && unit->packageClauses.size() == 1;
    else if (test.name == "empty-package") {
      const auto values = nodes<Packaging>(result);
      ok &= values.size() == 1 && values[0]->items.empty();
    } else if (test.name == "empty-refinement") {
      const auto values = nodes<RefinedType>(result);
      ok &= values.size() == 1 && values[0]->base && values[0]->members.empty();
    } else if (test.name == "unicode-identifiers") {
      const auto objects = nodes<ObjectDef>(result);
      ok &= objects.size() == 1 && objects[0]->name == "中文";
      for (auto name : {"变量", "Ⅳ", "á"}) {
        bool found = false;
        for (const auto *value : nodes<StableId>(result))
          found |= value->root == name;
        for (const auto *value : nodes<VarPattern>(result))
          found |= value->name == name;
        ok &= found;
      }
    } else if (test.name == "unicode-operators" ||
               test.name == "slash-operator") {
      bool function = false, infix = false;
      const auto op = test.name == "unicode-operators" ? "⊕" : "+/";
      for (const auto *value : nodes<DefDef>(result))
        function |= value->name == op;
      for (const auto *value : nodes<InfixExpr>(result))
        infix |= value->op == op;
      ok &= function && infix;
    } else if (test.name == "operators-comment") {
      const auto values = nodes<InfixExpr>(result);
      ok &= values.size() == 2;
      for (const auto *value : values)
        ok &= value->op == "+";
    } else if (test.name == "unicode-string") {
      const auto chars = nodes<CharLit>(result);
      const auto strings = nodes<StringLit>(result);
      ok &= chars.size() == 1 && chars[0]->text == "'é'" &&
            strings.size() == 1 && strings[0]->text == "\"été 中文\"" &&
            !strings[0]->multiline;
    } else if (test.name == "uppercase-hex") {
      const auto values = nodes<IntLit>(result);
      ok &= values.size() == 1 && values[0]->text == "0Xff" &&
            nodes<PostfixExpr>(result).empty();
    } else if (test.name == "multiline-quotes") {
      const auto values = nodes<StringLit>(result);
      ok &= values.size() == 1 && values[0]->multiline &&
            values[0]->text.ends_with("\"\"\"\"");
    } else if (test.name == "multiple-u" || test.name == "octal-escape" ||
               test.name == "string-escape") {
      ok &= nodes<CharLit>(result).size() == 1 &&
            nodes<StringLit>(result).size() == 1;
    } else if (test.name == "interpolation" ||
               test.name == "custom-interpolation" ||
               test.name == "interpolation-multiline" ||
               test.name == "interpolation-multiple" ||
               test.name == "interpolation-block") {
      const auto values = nodes<InterpolatedString>(result);
      ok &= values.size() == 1;
      if (values.size() == 1) {
        ok &= values[0]->prefix ==
              (test.name == "custom-interpolation" ? "raw" : "s");
        ok &= values[0]->multiline == (test.name == "interpolation-multiline");
        std::size_t holes = 0;
        bool first = false, second = false, thisRef = false, block = false;
        for (const auto *part : values[0]->parts)
          if (part->expression) {
            ++holes;
            block |= cursiv::ast_cast<Block>(part->expression) != nullptr;
            if (const auto *path = cursiv::ast_cast<PathExpr>(part->expression))
              if (path->path) {
                first |= path->path->root == "a";
                second |= path->path->root == "b";
                thisRef |= path->path->hasThis;
              }
          }
        ok &= holes == (test.name == "interpolation-multiple" ? 3u
                        : test.name == "custom-interpolation" ||
                                test.name == "interpolation-block"
                            ? 1u
                            : 2u);
        if (test.name == "interpolation-multiple")
          ok &= first && second && thisRef;
        if (test.name == "interpolation-block")
          ok &= block;
      }
    } else if (test.name == "empty-lambda") {
      const auto values = nodes<Lambda>(result);
      ok &= values.size() == 2;
      for (const auto* value : values) ok &= value->params.empty() && value->body;
    } else if (test.name == "interpolation-trivia") {
      const auto values = nodes<InterpolatedString>(result);
      ok &= values.size() == 2;
      bool leading = false, trailing = false, suffix = false;
      for (const auto* value : values)
        for (const auto* part : value->parts) {
          leading |= part->text == " leading ";
          trailing |= part->text == " tail ";
          suffix |= part->text == " suffix ";
        }
      ok &= leading && trailing && suffix;
    } else if (test.name == "multiline-trailing-commas") {
      const auto values = nodes<DefDef>(result);
      ok &= values.size() == 1 && values[0]->typeParams.size() == 1 &&
            values[0]->paramClauses.size() == 1 && values[0]->paramClauses[0]->params.size() == 1;
    } else if (test.name == "unicode-arrows") {
      bool byName = false;
      for (const auto* p : nodes<ParamType>(result)) byName |= p->byName;
      ok &= nodes<Lambda>(result).size() == 1 && nodes<Generator>(result).size() == 1 &&
            nodes<FunctionType>(result).size() == 1 && byName;
    } else if (test.name == "interpolation-pattern") {
      const auto values = nodes<InterpolatedPattern>(result);
      ok &= values.size() == 1 && values[0]->prefix == "s";
      if (values.size() == 1) {
        std::size_t holes = 0;
        for (const auto *part : values[0]->parts)
          holes += part->pattern != nullptr;
        ok &= holes == 2;
      }
    }
    if (!ok) {
      std::fprintf(stderr, "scala audit: %.*s failed\n",
                   static_cast<int>(test.name.size()), test.name.data());
      all = false;
    }
  }
  return all;
}
} // namespace scala_audit
