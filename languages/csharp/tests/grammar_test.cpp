// The smoke test of the language: parse every file under example/, the way a
// workspace would.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "csharp/core/CoreServices.hpp"
#include "csharp/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "modern_cases.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "csharp: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      csharp::CSharpCoreServices, csharp::CSharpSharedCoreServices,
      csharp::install_core_module>();

  {
    const auto result = language.language.parser->parse("class C { int value = 8 >> 1 >> 1 < 5; }");
    const csharp::ast::BinaryExpr* comparison = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* binary = cursiv::ast_cast<csharp::ast::BinaryExpr>(node);
          binary && binary->op == "<") comparison = binary;
    const auto* shift = comparison
        ? cursiv::ast_cast<csharp::ast::BinaryExpr>(comparison->left) : nullptr;
    const auto* left = shift ? cursiv::ast_cast<csharp::ast::BinaryExpr>(shift->left) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !shift || shift->op != ">>" || !left || left->op != ">>")
      return fail("shift associativity or precedence changed");
  }
  for (const char* op : {">>", ">>>", "<<"}) {
    const std::string source = std::string("class C { int value = 8 ") + op + " 1; }";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("adjacent shift operator was rejected");
  }
  for (const char* op : {"> >", ">/* gap */>", ">> >", "< <"}) {
    const std::string source = std::string("class C { int value = 8 ") + op + " 1; }";
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("separated angle brackets were accepted as a shift");
  }
  for (const char* op : {"> >=", ">/* gap */>=", ">> >="}) {
    const std::string source = std::string("class C { void M() { int x = 8; x ") + op + " 1; } }";
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("separated angle brackets were accepted as shift assignment");
  }

  if (!csharp_audit::modern_cases(*language.language.parser))
    return 1;

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "ref struct S { A<X>.B<Y> x; int?[] y; int[]? z; string a = $\"{1}\"; "
        "string b = $@\"{2}\"; }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    bool generic = false, nullableArray = false, arrayNullable = false;
    bool regular = false, verbatim = false, refStruct = false;
    for (const auto *node : result.nodes) {
      if (const auto *type = cursiv::ast_cast<csharp::ast::Type>(node)) {
        if (type->segments.size() == 2 && type->segments[0]->name == "A" &&
            type->segments[1]->name == "B")
          generic = type->segments[0]->arguments.size() == 1 &&
                    type->segments[1]->arguments.size() == 1;
        if (type->suffixes.size() == 2) {
          nullableArray |=
              type->suffixes[0] == "?" && type->suffixes[1] == "[]";
          arrayNullable |=
              type->suffixes[0] == "[]" && type->suffixes[1] == "?";
        }
      }
      if (const auto *str =
              cursiv::ast_cast<csharp::ast::InterpolatedStringExpr>(node)) {
        regular |= !str->verbatim;
        verbatim |= str->verbatim;
      }
      if (const auto *decl = cursiv::ast_cast<csharp::ast::StructDecl>(node))
        for (auto modifier : decl->modifiers)
          refStruct |= modifier == "ref";
    }
    if (!generic || !nullableArray || !arrayNullable || !regular || !verbatim ||
        !refStruct)
      return 1;
  }

  {
    const auto result = language.language.parser->parse(
        "class List<T> {} class C { int value; int[] items; "
        "ref int P => ref value; ref int this[int i] => ref items[i]; "
        "string closed = nameof(List<int>); string simple = nameof(C.value); }");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native generic nameof or ref expression bodies");
    bool property = false, indexer = false, closed = false, simple = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = cursiv::ast_cast<csharp::ast::PropertyDecl>(node))
        property |= value->isRefBody && value->refKind == "ref" && value->exprBody;
      if (const auto* value = cursiv::ast_cast<csharp::ast::IndexerDecl>(node))
        indexer |= value->isRefBody && value->refKind == "ref" && value->exprBody;
      if (const auto* value = cursiv::ast_cast<csharp::ast::NameofExpr>(node)) {
        const auto* target = cursiv::ast_cast<csharp::ast::NameExpr>(value->target);
        closed |= target && target->name == "List" && target->typeArguments.size() == 1;
        simple |= value->text == "C.value";
      }
    }
    if (!property || !indexer || !closed || !simple)
      return fail("nameof generic arguments or ref body metadata lost");
  }
  for (const auto source : {
           R"CS(class C { int a = 1__2; int b = 0x__F; int c = 0b__11; double d = 1.2__3e4__5; })CS",
           R"CS(class C { System.ReadOnlySpan<byte> A() => @"text"u8; System.ReadOnlySpan<byte> B() => @"text"U8; })CS",
           R"CS(class C { System.ReadOnlySpan<byte> A() => "text"U8; System.ReadOnlySpan<byte> B() => """text"""U8; char esc = '\e'; })CS",
           R"CS(class C { void M() { System.Span<int> a = stackalloc int[] {1,2}; System.Span<int> b = stackalloc[] {1,2}; } })CS"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native separator, UTF-8, escape or stackalloc syntax");
  }
  for (const auto literal : {"1_", "0x_", "0xF_", "0b_", "0b1_", "1_e2", "1e2_", "1.2_", "1.2_e3"}) {
    const auto result = language.language.parser->parse(
        std::string("class C { double value = ") + literal + "; }");
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail(std::string("invalid trailing numeric separator accepted: ") + literal);
  }
  {
    const auto result = language.language.parser->parse(
        R"CS(class C { string value = """"""; })CS");
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("a quote-only run is not an empty raw string");
  }

  {
    const auto result = language.language.parser->parse(
        "ref struct S { public ref int value; } "
        "class C { void M((int,int)[] pairs) { var (a, (b,c)) = (1,(2,3)); "
        "foreach ((int x, int y) in pairs) { } scoped System.Span<int> local = default; "
        "[System.Obsolete] static void Local() {} "
        "System.Action action = static delegate { }; } } "
        "class Generic<T> where T : allows ref struct {}");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native deconstruction, scoped, local function or ref field syntax");
    bool deconstruct = false, typed_foreach = false, scoped = false;
    bool local_attribute = false, static_method = false, ref_field = false, constraint = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = cursiv::ast_cast<csharp::ast::DeconstructionExpr>(node))
        deconstruct |= value->designation && value->designation->items.size() == 2 &&
                       value->designation->items[1]->items.size() == 2;
      if (const auto* value = cursiv::ast_cast<csharp::ast::ForeachStmt>(node))
        typed_foreach |= cursiv::ast_cast<csharp::ast::TupleExpr>(value->deconstructExpression) != nullptr;
      if (const auto* value = cursiv::ast_cast<csharp::ast::LocalVarDeclStmt>(node))
        scoped |= value->scoped;
      if (const auto* value = cursiv::ast_cast<csharp::ast::LocalFunctionDeclStmt>(node))
        local_attribute |= value->name == "Local" && value->attributes.size() == 1;
      if (const auto* value = cursiv::ast_cast<csharp::ast::AnonymousMethodExpr>(node))
        static_method |= value->isStatic;
      if (const auto* value = cursiv::ast_cast<csharp::ast::FieldDecl>(node))
        ref_field |= value->refKind == "ref";
      if (const auto* value = cursiv::ast_cast<csharp::ast::TypeParamConstraintClause>(node))
        for (const auto keyword : value->keywordConstraints)
          constraint |= keyword == "allowsrefstruct";
    }
    if (!deconstruct || !typed_foreach || !scoped || !local_attribute ||
        !static_method || !ref_field || !constraint)
      return fail("native deconstruction or later syntax metadata lost");
  }

  {
    const auto valid = cursiv::test::validationHelper(language.language,
        R"CS(class C { int @namespace; string value = """"a """ b""""; })CS");
    const auto invalid = cursiv::test::validationHelper(language.language,
        R"CS(class C { string value = """a""""; })CS");
    if (!valid.diagnostics.empty() || invalid.diagnostics.size() != 1)
      return fail("raw delimiter runtime validation");
    const auto reserved = language.language.parser->parse("class C { int namespace; }");
    if (reserved.accepted && !reserved.cst().recovered() && reserved.diagnostics.empty())
      return fail("unescaped reserved word accepted as identifier");
  }

  for (const auto source : {
           "interface I; enum E;",
           "ref struct S { public ref int value; } class C { void M(ref int v) { S s = new S {value = ref v}; } }",
           "[assembly: global::System.Obsolete] class C { void M() { Target(arg: global::System.String.Empty); } }"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native empty type, ref initializer or target/alias syntax");
    if (std::string_view(source).find("value = ref") != std::string_view::npos) {
      bool retained = false;
      for (const auto* node : result.nodes)
        if (const auto* member = cursiv::ast_cast<csharp::ast::ObjectInitMember>(node))
          retained |= member->name == "value" && member->isRef && member->value;
      if (!retained) return fail("ref object initializer metadata lost");
    }
  }

  int count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cs")
      continue;

    std::ifstream file(entry.path());
    if (!file) return fail("cannot read " + entry.path().string());
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().filename().string() + " does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<csharp::ast::CompilationUnit>)
      return fail(entry.path().filename().string() +
                   " built no compilation unit");
    ++count;
  }
  if (count == 0) return fail("no example files found");

  std::printf("csharp: %d examples parse clean\n", count);
  return 0;
}
