
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "dart2/core/CoreServices.hpp"
#include "dart2/core/ast.hpp"
#include "modern_cases.hpp"
// Dart2 end to end: every document under example/ and tests/ parses clean
// (no repair, no diagnostics) into a CompilationUnit.

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "dart2/generated/Dart2/Module.hpp"

namespace {

namespace ast = dart2::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      dart2::generated::Dart2::create_parser();
  return *p;
}

void corpus() {
  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".dart")
        files.push_back(entry.path());
  CHECK(!files.empty());

  for (const std::filesystem::path& file : files) {
    const std::string text = cursiv::test::read_file(file.string());
    CHECK(!text.empty());

    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    CHECK(r.diagnostics.empty());
    ast::CompilationUnit* const unit = r.value_as<ast::CompilationUnit>();
    CHECK(unit != nullptr);
  }
}

// Each declaration carries the annotations written before it, wherever it
// stands: at the top level, in a class, in a mixin, in an extension.
template <class List>
std::string names(const List& list) {
  std::string out;
  for (const ast::Metadata* const m : list) {
    if (!out.empty()) out += ',';
    out += std::string(m->name);
  }
  return out;
}

void metadata() {
  const cursiv::ParseResult r = engine().parse(
      "@a1 class A {\n  @m1 int x = 0;\n  @m2 @m3 void f() {}\n}\n"
      "@b1 @b2 mixin M {\n  @mm1 void g() {}\n  void h() {}\n}\n"
      "@c1 extension E on int {\n  @em1 int get k => 1;\n}\n"
      "@d1 enum En { v }\n"
      "@e1 void top() {}\n"
      "@f1 int y = 0;\n");
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  ast::CompilationUnit* const unit = r.value_as<ast::CompilationUnit>();
  CHECK(unit != nullptr);
  CHECK(unit->declarations.size() == 6);
  CHECK(names(unit->declarations[0]->annotations) == "a1");
  CHECK(names(unit->declarations[1]->annotations) == "b1,b2");
  CHECK(names(unit->declarations[2]->annotations) == "c1");
  CHECK(names(unit->declarations[3]->annotations) == "d1");
  CHECK(names(unit->declarations[4]->annotations) == "e1");
  CHECK(names(unit->declarations[5]->annotations) == "f1");

  const auto* const cls =
      cursiv::ast_cast<ast::ClassDecl>(unit->declarations[0]);
  CHECK(cls != nullptr && cls->members.size() == 2);
  CHECK(names(cls->members[0]->annotations) == "m1");
  CHECK(names(cls->members[1]->annotations) == "m2,m3");
  const auto* const mix =
      cursiv::ast_cast<ast::MixinDecl>(unit->declarations[1]);
  CHECK(mix != nullptr && mix->members.size() == 2);
  CHECK(names(mix->members[0]->annotations) == "mm1");
  CHECK(mix->members[1]->annotations.empty());
  const auto* const ext =
      cursiv::ast_cast<ast::ExtensionDecl>(unit->declarations[2]);
  CHECK(ext != nullptr && ext->members.size() == 1);
  CHECK(names(ext->members[0]->annotations) == "em1");
}

// A local declaration ends on its own `;` (`localVariableDeclaration`), and
// a document holding one declaration is a library.
void local_declarations() {
  const cursiv::ParseResult r =
      engine().parse("void f() {\n  const int limit = 80;\n}\n");
  CHECK(r.accepted && r.diagnostics.empty());
  ast::CompilationUnit* const unit = r.value_as<ast::CompilationUnit>();
  CHECK(unit != nullptr && unit->declarations.size() == 1);
  const auto* const fn =
      cursiv::ast_cast<ast::TopLevelFunction>(unit->declarations[0]);
  const auto* const body =
      fn != nullptr ? cursiv::ast_cast<ast::BlockStmt>(fn->body) : nullptr;
  CHECK(body != nullptr && body->statements.size() == 1);
  const auto* const decl =
      cursiv::ast_cast<ast::VarDeclStmt>(body->statements[0]);
  CHECK(decl != nullptr && decl->spec->isConst && decl->spec->type != nullptr);
  CHECK(decl->spec->vars.size() == 1 && decl->spec->vars[0]->name == "limit");

  CHECK(!engine().parse("void f() { int x int y; }\n").diagnostics.empty());

  const cursiv::ParseResult top = engine().parse("List<int> l;\n");
  ast::CompilationUnit* const lib = top.value_as<ast::CompilationUnit>();
  CHECK(lib != nullptr && lib->declarations.size() == 1 &&
        lib->stmtEntry == nullptr);
}

// A `=>` body is a whole expression (`functionExpressionBody`): the `is`
// after it belongs to the body, not to an operand around the function.
void arrow_body() {
  const cursiv::ParseResult r =
      engine().parse("void f() {\n  var g = (x) => x is E && y;\n}\n");
  CHECK(r.accepted && r.diagnostics.empty());
  ast::CompilationUnit* const unit = r.value_as<ast::CompilationUnit>();
  CHECK(unit != nullptr && unit->declarations.size() == 1);
  const auto* const fn =
      unit != nullptr
          ? cursiv::ast_cast<ast::TopLevelFunction>(unit->declarations[0])
          : nullptr;
  const auto* const body =
      fn != nullptr ? cursiv::ast_cast<ast::BlockStmt>(fn->body) : nullptr;
  CHECK(body != nullptr && body->statements.size() == 1);
  const auto* const decl =
      body != nullptr ? cursiv::ast_cast<ast::VarDeclStmt>(body->statements[0])
                      : nullptr;
  CHECK(decl != nullptr && decl->spec->vars.size() == 1);
  const auto* const g =
      decl != nullptr
          ? cursiv::ast_cast<ast::FunctionExpr>(decl->spec->vars[0]->init)
          : nullptr;
  const auto* const both =
      g != nullptr ? cursiv::ast_cast<ast::BinaryExpr>(g->bodyExpr) : nullptr;
  CHECK(both != nullptr && cursiv::ast_cast<ast::IsTest>(both->left) != nullptr);
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<dart2::Dart2CoreServices,
                                                dart2::Dart2SharedCoreServices,
                                                dart2::install_core_module>();
  {
    const auto r = language.language.parser->parse("'''x $name ${1+2} y'''");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;

    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    if (!unit) return __LINE__;
    const auto* adjacent =
        cursiv::ast_cast<dart2::ast::AdjacentStrings>(unit->exprEntry);
    if (!adjacent || adjacent->parts.size() != 1) return __LINE__;
    const auto* str = adjacent->parts[0];
    if (!str || !str->multiline || str->raw || str->parts.size() != 5 ||
        !std::holds_alternative<std::string_view>(str->parts[0]) ||
        std::get<std::string_view>(str->parts[0]) != "x " ||
        !std::holds_alternative<dart2::ast::Expr*>(str->parts[1]))
      return __LINE__;
    const auto* ident = cursiv::ast_cast<dart2::ast::Ident>(
        std::get<dart2::ast::Expr*>(str->parts[1]));
    if (!ident || ident->name != "name" ||
        std::get<std::string_view>(str->parts[2]) != " " ||
        !std::holds_alternative<dart2::ast::Expr*>(str->parts[3]) ||
        std::get<std::string_view>(str->parts[4]) != " y")
      return __LINE__;
  }
  for (const char* source :
       {R"dart("""a" b"" c $name""")dart", R"dart('''a' b'' c $name''')dart"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return __LINE__;
    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    const auto* adjacent =
        unit ? cursiv::ast_cast<dart2::ast::AdjacentStrings>(unit->exprEntry)
             : nullptr;
    const auto* str =
        adjacent && adjacent->parts.size() == 1 ? adjacent->parts[0] : nullptr;
    if (!str || !str->multiline) return __LINE__;
    std::string text;
    for (const auto& part : str->parts) {
      if (const auto* chunk = std::get_if<std::string_view>(&part))
        text += *chunk;
      else
        text += "{}";
    }
    if (text != (source[0] == '\"' ? "a\" b\"\" c {}" : "a' b'' c {}"))
      return __LINE__;
  }
  for (const char* text : {"\"// text\"", "\"/* text */\"", "\" \""}) {
    const auto r = language.language.parser->parse(text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return __LINE__;
    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    const auto* adjacent =
        unit ? cursiv::ast_cast<dart2::ast::AdjacentStrings>(unit->exprEntry)
             : nullptr;
    const auto* str =
        adjacent && adjacent->parts.size() == 1 ? adjacent->parts[0] : nullptr;
    const std::string_view expected(text + 1,
                                    std::string_view(text).size() - 2);
    if (!str || str->parts.size() != 1 ||
        !std::holds_alternative<std::string_view>(str->parts[0]) ||
        std::get<std::string_view>(str->parts[0]) != expected)
      return __LINE__;
  }
  {
    const auto r =
        language.language.parser->parse("\"${ 1 /* comment */ + 2 }\"");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return __LINE__;
  }
  {
    const auto r = language.language.parser->parse("\"$name\"");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
  }
  {
    const auto r = language.language.parser->parse("r'$name'");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
  }
  struct RawCase {
    const char* source;
    std::string_view body;
    bool multiline;
  };
  for (const RawCase& raw :
       {RawCase{R"dart(r"""a "" $b""")dart", "a \"\" $b", true},
        RawCase{R"dart(r'''a '' $b''')dart", "a '' $b", true},
        RawCase{R"dart(r"a $b")dart", "a $b", false},
        RawCase{R"dart(r'a $b')dart", "a $b", false}}) {
    const auto r = language.language.parser->parse(raw.source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.cst().leaf_count() != 1)
      return __LINE__;
    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    const auto* adjacent =
        unit ? cursiv::ast_cast<dart2::ast::AdjacentStrings>(unit->exprEntry)
             : nullptr;
    const auto* str =
        adjacent && adjacent->parts.size() == 1 ? adjacent->parts[0] : nullptr;
    if (!str || !str->raw || str->multiline != raw.multiline ||
        str->parts.size() != 1 ||
        !std::holds_alternative<std::string_view>(str->parts[0]) ||
        std::get<std::string_view>(str->parts[0]) != raw.body)
      return __LINE__;
  }
  {
    const auto r = language.language.parser->parse("\"\"\"$name ${1+2}\"\"\"");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
  }
  {
    const auto r = language.language.parser->parse("[...?other]");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    const auto* list =
        unit ? cursiv::ast_cast<dart2::ast::ListLit>(unit->exprEntry) : nullptr;
    const auto* spread =
        list && list->elements.size() == 1
            ? cursiv::ast_cast<dart2::ast::SpreadElement>(list->elements[0])
            : nullptr;
    if (!spread || !spread->nullAware) return __LINE__;
  }
  {
    const auto r = language.language.parser->parse("a?..foo()");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
    const auto* unit = r.value_as<dart2::ast::CompilationUnit>();
    const auto* cascade =
        unit ? cursiv::ast_cast<dart2::ast::CascadeExpr>(unit->exprEntry)
             : nullptr;
    if (!cascade || !cascade->nullAware || cascade->sections.size() != 1)
      return __LINE__;
  }
  {
    const auto r =
        language.language.parser->parse("/* a /* b */ c */ void main() {}");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return __LINE__;
  }
  return 0;
}

int main() {
  CHECK(dart2_audit::modern_cases(engine()));
  if (const int line = audit_regression(); line != 0) {
    std::fprintf(stderr, "audit regression failed at line %d\n", line);
    return 1;
  }
  corpus();
  metadata();
  local_declarations();
  arrow_body();
  return cursiv::test::report("dart2_grammar_test");
}
