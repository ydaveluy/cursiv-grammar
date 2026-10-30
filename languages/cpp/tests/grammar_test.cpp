// The smoke test of the language: every file in example/ parses without
// recovery and builds a TranslationUnit.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cpp/core/CoreServices.hpp"
#include "cpp/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "cpp: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      cpp::CppCoreServices,
      cpp::CppSharedCoreServices,
      cpp::install_core_module>();
  {
    const auto result = language.language.parser->parse("int value = 8 >> 1 >> 1 < 5;");
    const cpp::ast::BinaryExpr* comparison = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* binary = cursiv::ast_cast<cpp::ast::BinaryExpr>(node);
          binary && binary->op == "<") comparison = binary;
    const auto* shift = comparison
        ? cursiv::ast_cast<cpp::ast::BinaryExpr>(comparison->left) : nullptr;
    const auto* left = shift ? cursiv::ast_cast<cpp::ast::BinaryExpr>(shift->left) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !shift || shift->op != ">>" || !left || left->op != ">>")
      return fail("shift associativity or precedence changed");
  }
  for (const char* op : {">>", "<<"}) {
    const std::string source = std::string("int value = 8 ") + op + " 1;";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("adjacent shift operator was rejected");
  }
  for (const char* op : {"> >", ">/* gap */>", "< <"}) {
    const std::string source = std::string("int value = 8 ") + op + " 1;";
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("separated angle brackets were accepted as a shift");
  }
  {
    const auto result = language.language.parser->parse(
        "int f(){ MyClass *test= new MyClass; test->foo(); }");
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* function = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::FunctionDef>(unit->items[0]) : nullptr;
    const auto* declaration = function && function->body && function->body->items.size() == 2
        ? cursiv::ast_cast<cpp::ast::Declaration>(function->body->items[0]) : nullptr;
    const auto* declarator = declaration && declaration->declarators.size() == 1
        ? declaration->declarators[0]->declarator : nullptr;
    const auto* name = declarator
        ? cursiv::ast_cast<cpp::ast::Ident>(declarator->name) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !name || name->name != "test" || declarator->pointers.size() != 1 ||
        declarator->pointers[0]->kind != "*" || declarator->pointers[0]->memberOf)
      return fail("pointer declaration or deferred lookahead context");
  }
  {
    const auto result = language.language.parser->parse(
        "double totalArea(const Shape& a, const Shape& b) { return 0; }");
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* function = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::FunctionDef>(unit->items[0]) : nullptr;
    const auto* suffix = function && function->declarator &&
        function->declarator->suffixes.size() == 1
        ? cursiv::ast_cast<cpp::ast::FunctionSuffix>(function->declarator->suffixes[0])
        : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !suffix || suffix->params.size() != 2)
      return fail("reference parameters or deferred lookahead context");
    for (std::size_t i = 0; i < suffix->params.size(); ++i) {
      const auto* parameter = suffix->params[i];
      const auto* declarator = parameter->declarator;
      const auto* name = declarator
          ? cursiv::ast_cast<cpp::ast::Ident>(declarator->name) : nullptr;
      if (!name || name->name != (i == 0 ? "a" : "b") ||
          declarator->pointers.size() != 1 ||
          declarator->pointers[0]->kind != "&" ||
          declarator->pointers[0]->memberOf || !parameter->specifiers ||
          parameter->specifiers->qualifiers.size() != 1 ||
          parameter->specifiers->qualifiers[0] != "const")
        return fail("reference parameter AST");
    }
  }
  for (const auto source : {"int while;", "struct if { int value; };",
                           "enum E { for };", "int xor;"}) {
    const auto invalid = cursiv::test::validationHelper(language.language, source);
    if (invalid.diagnostics.size() != 1)
      return fail("reserved word accepted as a name");
  }
  const auto validNames = cursiv::test::validationHelper(language.language,
      "int final; int override; [[gnu::const]] int function();");
  if (!validNames.diagnostics.empty())
    return fail("contextual keyword or attribute token refused as a name");
  struct KeywordCase {
    const char* source;
    const cursiv::AstTypeTag* expected;
  };
  const KeywordCase keywordCases[]{
      {"auto value = int(1);", &cursiv::ast_type_tag<cpp::ast::FunctionalCastExpr>},
      {"auto value = static_cast<int>(1);", &cursiv::ast_type_tag<cpp::ast::NamedCastExpr>},
      {"auto value = typeid(int);", &cursiv::ast_type_tag<cpp::ast::TypeidExpr>}};
  for (const auto& test : keywordCases) {
    const auto source = test.source;
    const auto result = language.language.parser->parse(source);
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* declaration = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
    const auto* value = declaration && declaration->declarators.size() == 1
        ? declaration->declarators[0]->init : nullptr;
    const auto* typeidExpr = cursiv::ast_cast<cpp::ast::TypeidExpr>(value);
    const bool correctType = value && value->type == test.expected &&
        (!typeidExpr || (typeidExpr->type && !typeidExpr->expr));
    const auto valid = cursiv::test::validationHelper(language.language, source);
    if (!result.accepted || result.cst().recovered() || !correctType ||
        !valid.diagnostics.empty())
      return fail("keyword expression parsed as an identifier");
  }
  for (const auto source : {"int value;", "unsigned const long int value;",
                           "Widget value;", "const Widget value;"}) {
    const auto result = language.language.parser->parse(source);
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* declaration = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
    const auto* declarator = declaration && declaration->declarators.size() == 1
        ? declaration->declarators[0]->declarator : nullptr;
    const auto* name = declarator
        ? cursiv::ast_cast<cpp::ast::Ident>(declarator->name) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !name || name->name != "value")
      return fail("declaration name consumed as a type specifier");
  }
  for (const auto text : {"0x1p2", "0x.8p+1", "0x1.p-2", "0xA'B.C'Dp1'0f"}) {
    const auto source = std::string("double value = ") + text + ';';
    const auto result = language.language.parser->parse(source);
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* declaration = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
    const auto* literal = declaration && declaration->declarators.size() == 1
        ? cursiv::ast_cast<cpp::ast::FloatLit>(declaration->declarators[0]->init)
        : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !literal || literal->text != text)
      return fail("hexadecimal floating literal spelling or AST type");
  }
  {
    const auto result = language.language.parser->parse("union U : Base { int x; };");
    const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
    const auto* declaration = unit && unit->items.size() == 1
        ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
    const auto* type = declaration && declaration->specifiers &&
        declaration->specifiers->types.size() == 1
        ? cursiv::ast_cast<cpp::ast::ClassType>(declaration->specifiers->types[0])
        : nullptr;
    const auto invalid = cursiv::test::validationHelper(language.language,
        "struct Base {}; union U : Base { int x; };");
    const auto valid = cursiv::test::validationHelper(language.language,
        "struct Base {}; struct Derived : Base {}; union U { int x; };");
    if (!result.accepted || result.cst().recovered() || !type ||
        type->keyword != "union" || type->bases.size() != 1 ||
        invalid.diagnostics.size() != 1 || !valid.diagnostics.empty())
      return fail("union base retained for runtime validation");
  }
  {
    constexpr auto source = R"CPP(auto f = [x = 1] { return x; };
const char* s = R"!@(raw )not" text)!@";
struct A { virtual void f() = 1; };
)CPP";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return fail("lambda or raw string regression");
    const auto invalid = cursiv::test::validationHelper(language.language, "struct A { virtual void f() = 1; };");
    const auto valid = cursiv::test::validationHelper(language.language, "struct A { virtual void f() = 0; };");
    if (invalid.diagnostics.size() != 1 || !valid.diagnostics.empty()) return fail("pure specifier runtime check");
    const auto longRaw = cursiv::test::validationHelper(language.language,
        R"CPP(const char* s = R"abcdefghijklmnopq(x)abcdefghijklmnopq";)CPP");
    if (longRaw.diagnostics.size() != 1) return fail("raw delimiter runtime length");
  }


  {
    int rawCases = 0;
    const auto rawStrings = [&](const std::vector<std::string>& pieces,
                                std::string_view separator = {}) {
      std::string source = "auto value = ";
      for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i != 0) source += separator;
        source += pieces[i];
      }
      source += ';';
      const auto result = language.language.parser->parse(source);
      if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
        return false;
      const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
      const auto* declaration = unit && unit->items.size() == 1
          ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
      const auto* literal = declaration && declaration->declarators.size() == 1
          ? cursiv::ast_cast<cpp::ast::StringLit>(declaration->declarators[0]->init)
          : nullptr;
      if (!literal || literal->pieces.size() != pieces.size()) return false;
      for (std::size_t i = 0; i < pieces.size(); ++i)
        if (literal->pieces[i] != pieces[i]) return false;
      ++rawCases;
      return true;
    };
    for (const auto* text : {
             R"CPP(R""(abc)"")CPP",
             R"CPP(R"!@#$%^&*+,-./:;=(text)!@#$%^&*+,-./:;=")CPP",
             R"CPP(R"-./:;=[]{|}~?(punctuation)-./:;=[]{|}~?")CPP",
             R"CPP(R"(plain)")CPP", R"CPP(R""(##body)"")CPP",
             R"CPP(R"(###)")CPP", R"CPP(R""()"")CPP",
             R"CPP(R"() extension)")CPP", R"CPP(u8R""(abc)"")CPP"})
      if (!rawStrings({text})) return fail("raw string spelling or AST attachment");
    if (!rawStrings({R"CPP(R""(first)"")CPP", R"CPP(R""(second)"")CPP"}, " ") ||
        !rawStrings({R"CPP(R"a(##first)a")CPP", R"CPP(R"b(##second)b")CPP"}, " "))
      return fail("adjacent raw string boundaries or AST pieces");
    for (const auto& body : {std::string(100000, 'x'), std::string(8192, '#'),
                             std::string(100000, '#')})
      if (!rawStrings({std::string("R\"\"(") + body + ")\"\""}))
        return fail("long raw body or hash prefix");
    for (const auto* text : {R"CPP(R"(plain)"_unit)CPP",
                             R"CPP(R""(with quote)""_unit)CPP"}) {
      const std::string source = std::string("auto value = ") + text + ';';
      const auto result = language.language.parser->parse(source);
      const auto* unit = cursiv::ast_cast<cpp::ast::TranslationUnit>(result.value);
      const auto* declaration = unit && unit->items.size() == 1
          ? cursiv::ast_cast<cpp::ast::Declaration>(unit->items[0]) : nullptr;
      const auto* literal = declaration && declaration->declarators.size() == 1
          ? cursiv::ast_cast<cpp::ast::UserDefinedLit>(declaration->declarators[0]->init)
          : nullptr;
      if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
          !literal || literal->text != text)
        return fail("user-defined raw string suffix or AST spelling");
      ++rawCases;
    }
    std::printf("cpp: %d raw string AST regressions pass\n", rawCases);
  }


  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    // tests/ also holds this file itself and CMakeLists.txt: the language's
    // own extension collides with the test driver's, so the driver is
    // excluded by name, not just by extension.
    if (!entry.is_regular_file() || entry.path().extension() != ".cpp" ||
        entry.path().filename() == "grammar_test.cpp")
      continue;
    ++seen;
    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<cpp::ast::TranslationUnit>)
      return fail(entry.path().string() + ": built no translation unit");
  }
  if (seen == 0) return fail("no examples found");

  {
    const auto result = language.language.parser->parse(
        "auto a = new int; auto b = new int(); auto c = new int{};");
    const auto* unit = result.value_as<cpp::ast::TranslationUnit>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !unit || unit->items.size() != 3) return fail("native new-initializer syntax");
    for (std::size_t i = 0; i < 3; ++i) {
      const auto* declaration = cursiv::ast_cast<cpp::ast::Declaration>(unit->items[i]);
      const auto* value = declaration && declaration->declarators.size() == 1
                              ? cursiv::ast_cast<cpp::ast::NewExpr>(declaration->declarators[0]->init)
                              : nullptr;
      if (!value || value->hasInit != (i != 0) || value->braced != (i == 2) ||
          !value->type || value->type->declarator)
        return fail("new int and new int() must preserve different initialization");
    }
  }
  {
    const auto result = language.language.parser->parse(
        "struct List { List(int,int); }; void consume(List); void consume(...); "
        "template<class... Ts> struct A {}; "
        "template<class... Ts> struct B : A<Ts...> {}; "
        "template<class... Ts> void unnamed(Ts*...); "
        "template<class... Ts> void f(Ts... ts) { "
        "auto l = [ts...](...) { return 0; }; consume(ts...); consume({1,2}); } "
        "template<class... Ts> void g() throw(Ts...); "
        "template<class... Bs> struct D : Bs... { D() : Bs()... {} }; "
        "int increment(int x) { return ++(int&)x; } "
        "void generic() { auto g = [](auto v) { return v; }; g.operator()<int>(1); }");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !result.value) return fail("native C++14 pack and initializer-list forms");
    unsigned expressions = 0, types = 0, captures = 0, initializers = 0, variadic = 0;
    bool cast_operand = false, brace_argument = false, operator_template = false;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (cursiv::ast_cast<cpp::ast::PackExpansionExpr>(&node)) ++expressions;
      if (const auto* type = cursiv::ast_cast<cpp::ast::TypeId>(&node)) types += type->packExpansion;
      if (const auto* capture = cursiv::ast_cast<cpp::ast::Capture>(&node)) captures += capture->pack;
      if (const auto* init = cursiv::ast_cast<cpp::ast::MemInit>(&node)) initializers += init->pack;
      if (const auto* lambda = cursiv::ast_cast<cpp::ast::LambdaExpr>(&node)) variadic += lambda->variadic;
      if (const auto* prefix = cursiv::ast_cast<cpp::ast::PrefixExpr>(&node))
        cast_operand |= prefix->op == "++" && cursiv::ast_cast<cpp::ast::CastExpr>(prefix->operand);
      if (const auto* call = cursiv::ast_cast<cpp::ast::CallExpr>(&node))
        for (const auto* argument : call->args)
          brace_argument |= cursiv::ast_cast<cpp::ast::BracedInitList>(argument) != nullptr;
      if (const auto* id = cursiv::ast_cast<cpp::ast::TemplateId>(&node))
        operator_template |= cursiv::ast_cast<cpp::ast::OperatorFunctionName>(id->templateName) != nullptr;
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) { self(self, child); });
    };
    walk(walk, *result.value);
    if (!expressions || types < 2 || !captures || !initializers || !variadic ||
        !cast_operand || !brace_argument || !operator_template)
      return fail("native pack/initializer metadata was lost");
  }
  for (const auto source : {
           "int a<:2:>; void f()<% a<:0:> = 1; %>",
           "struct N {}; template<class T> struct X {}; X<::N> x;",
           "int [[vendor::meta]] x; int y [[vendor::meta]]; "
           "void f() { [[vendor::tokens(int[2]{1,2}/2)]] return; }",
           "int \u203F; int int\u20AC;",
           "struct Bits { unsigned value [[vendor::meta]] : 3; };"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native digraph, attribute or C++14 identifier syntax");
  }

  std::printf("cpp: %d examples parse\n", seen);
  return 0;
}
