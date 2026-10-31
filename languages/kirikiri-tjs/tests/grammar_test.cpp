// The smoke test of the language: parse every file under example/ (the
// upstream corpus), the way a workspace would build one, and read a few real
// field values back out of one document so a converter that quietly drops a
// value would still be caught.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "kirikiritjs/core/CoreServices.hpp"
#include "kirikiritjs/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lexical_cases.hpp"
#include "octet_regexp_cases.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "kirikiritjs: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path &path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<kirikiritjs::KirikiriTjsCoreServices,
                                kirikiritjs::KirikiriTjsSharedCoreServices,
                                kirikiritjs::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "f(,x); g(,); h(x,); var a=[,,]; var b=[];");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool args = false, array = false, trailing = false, empty = false;
    for (const auto *node : audit_result.nodes) {
      if (const auto *a = cursiv::ast_cast<kirikiritjs::ast::ArrayLit>(node)) {
        array = array || (a->elements.size() == 3 && !a->elements[0]->value &&
                          !a->elements[1]->value && !a->elements[2]->value);
        empty = empty || a->elements.empty();
      }
      if (const auto *a = cursiv::ast_cast<kirikiritjs::ast::CallExpr>(node)) {
        args = args ||
               (a->args.size() == 2 && !a->args[0]->value && a->args[1]->value);
        trailing = trailing || (a->args.size() == 2 && a->args[0]->value &&
                                !a->args[1]->value);
      }
    }
    if (!args || !array || !trailing || !empty)
      return 1;
  }

  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry &entry :
       fs::recursive_directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".tjs")
      files.push_back(entry.path());
  if (files.empty())
    return fail("no example files found");
  const std::size_t example_count = files.size();

  // Constructs the corpus alone does not exercise (see the file's header).
  for (const fs::directory_entry &entry :
       fs::directory_iterator(CURSIV_TEST_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".tjs")
      files.push_back(entry.path());

  const auto language =
      cursiv::create_language<kirikiritjs::KirikiriTjsCoreServices,
                              kirikiritjs::KirikiriTjsSharedCoreServices,
                              kirikiritjs::install_core_module>();
  if (!tjs_lexical_cases::run(*language.language.parser))
    return fail("native lexical contract failed");
  if (!tjs_octet_regexp_cases::run(language.language))
    return fail("native octet/regexp lexical contract failed");

  for (const auto source : {"var x:int=1; function f(a:real=2,rest:string*):void{return;} ", "try f(); catch(e) g();", "property p{getter:int{return 1;} setter(v:int){x=v;}}", "var ́wide=1;", "var v=global; var n=NaN; var i=Infinity;", "#! /usr/bin/tjs\nvar x=1;", "var x=$65;", "switch(x){var y=1;case 1:y=2;}", "var x=(int)-a;", "var x=0x.8p2; var y=0b1.p+ 2; var z=1e+ 3;"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return fail(std::string("native form rejected: ") + source);
  }
  for (const auto source : {"var if=1;", "var property=1;", "function getter(){}", "var $name=1;", "var \\u0061=1;", "var x=0o12;", "var x=1p2;", "var x=a if b;"}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) return fail(std::string("native-invalid form accepted: ") + source);
  }
  {
    const auto r = language.language.parser->parse("a <-> b = c; -f incontextof x incontextof y; a instanceof b * c; var x=$65; var g=global;");
    const auto* root = r.value_as<kirikiritjs::ast::Program>();
    namespace a = kirikiritjs::ast;
    const auto expr = [&](unsigned i) -> const a::Expr* { const auto* s = root && root->body.size() > i ? cursiv::ast_cast<a::ExprStmt>(root->body[i]) : nullptr; return s && s->expressions.size() == 1 ? s->expressions[0] : nullptr; };
    const auto* swap = cursiv::ast_cast<a::AssignExpr>(expr(0));
    const auto* unary = cursiv::ast_cast<a::UnaryExpr>(expr(1));
    const auto* context = unary ? cursiv::ast_cast<a::BinaryExpr>(unary->operand) : nullptr;
    const auto* mul = cursiv::ast_cast<a::BinaryExpr>(expr(2));
    bool dollar = false, global = false;
    for (const auto* n : r.nodes) { dollar |= cursiv::ast_cast<a::CharFromCodeExpr>(n) != nullptr; global |= cursiv::ast_cast<a::GlobalExpr>(n) != nullptr; }
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !swap || swap->op != "<->" || !cursiv::ast_cast<a::AssignExpr>(swap->value) ||
        !context || context->op != "incontextof" || !cursiv::ast_cast<a::BinaryExpr>(context->right) ||
        !mul || mul->op != "*" || !cursiv::ast_cast<a::BinaryExpr>(mul->left) || !dollar || !global)
      return fail("native swap/context/instanceof/dollar/global AST lost");
  }
  for (const auto source : {"function f(a*,b){}", "var f=function(a*,b){};", "property p{}", "property p{getter{} getter{}}", "property p{setter(v=1){}}"}) {
    const auto r = cursiv::test::validationHelper(language.language, source);
    if (r.diagnostics.empty()) return fail("native parameter/property constraint needs diagnostic");
  }
  {
    const auto r = language.language.parser->parse("var x=%[bare:1,dynamic,2];");
    unsigned fixed = 0, dynamic = 0;
    for (const auto* n : r.nodes) if (const auto* p = cursiv::ast_cast<kirikiritjs::ast::Prop>(n)) { fixed += p->identifierKey; dynamic += !p->identifierKey; }
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || fixed != 1 || dynamic != 1) return fail("dictionary literal versus computed key semantics lost");
  }
  bool readback_done = false;
  for (const fs::path &path : files) {
    const std::string text = read_file(path);
    if (text.empty())
      return fail(path.string() + ": not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      for (const auto& d : result.diagnostics) std::fprintf(stderr, "%s: repair kind=%u at=%u:%u\n", path.string().c_str(), static_cast<unsigned>(d.kind), d.begin, d.end);
      return fail(path.string() + ": the parse needed repair");
    }
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse left diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<kirikiritjs::ast::Program>)
      return fail(path.string() + ": the parse built no program");

    const auto &program = *static_cast<const kirikiritjs::ast::Program *>(root);
    if (path.filename() == "test_class.tjs") {
      readback_done = true;
      bool sawFunctionDecl = false;
      bool sawClassDecl = false;
      for (const kirikiritjs::ast::Stmt *const stmt : program.body) {
        if (stmt == nullptr)
          continue;
        if (stmt->type ==
            &cursiv::ast_type_tag<kirikiritjs::ast::FunctionDecl>) {
          const auto &fn =
              *static_cast<const kirikiritjs::ast::FunctionDecl *>(stmt);
          if (!fn.name.empty())
            sawFunctionDecl = true;
        }
        if (stmt->type == &cursiv::ast_type_tag<kirikiritjs::ast::ClassDecl>) {
          const auto &cls =
              *static_cast<const kirikiritjs::ast::ClassDecl *>(stmt);
          if (!cls.name.empty())
            sawClassDecl = true;
        }
      }
      if (!sawFunctionDecl || !sawClassDecl)
        return fail(
            path.string() +
            ": expected a named function and a named class at top level");
    }
  }
  if (!readback_done)
    return fail("test_class.tjs was not found to read back");

  // Two precedence findings from TJS2's own documentation,
  // not visible from a structural parse-clean check alone.
  {
    const std::string text = "a = b = c;";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'a = b = c;' did not parse clean");
    const auto &program =
        *static_cast<const kirikiritjs::ast::Program *>(result.value);
    const auto &stmt =
        *static_cast<const kirikiritjs::ast::ExprStmt *>(program.body[0]);
    const cursiv::AstNode *const outer = stmt.expressions[0];
    if (outer->type != &cursiv::ast_type_tag<kirikiritjs::ast::AssignExpr>)
      return fail("'a = b = c;' did not build an assignment");
    const auto &outerAssign =
        *static_cast<const kirikiritjs::ast::AssignExpr *>(outer);
    if (outerAssign.value->type !=
        &cursiv::ast_type_tag<kirikiritjs::ast::AssignExpr>)
      return fail("'a = b = c;' is not right-associative: value is not itself "
                  "an assignment");
    const auto &innerAssign =
        *static_cast<const kirikiritjs::ast::AssignExpr *>(outerAssign.value);
    const auto &innerTarget =
        *static_cast<const kirikiritjs::ast::Ident *>(innerAssign.target);
    if (innerTarget.name != "b")
      return fail("'a = b = c;' is not right-associative: inner target is not "
                  "'b'");
  }
  {
    const std::string text = "w = (int) a + 1;";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'w = (int) a + 1;' did not parse clean");
    const auto &program =
        *static_cast<const kirikiritjs::ast::Program *>(result.value);
    const auto &stmt =
        *static_cast<const kirikiritjs::ast::ExprStmt *>(program.body[0]);
    const auto &assign =
        *static_cast<const kirikiritjs::ast::AssignExpr *>(stmt.expressions[0]);
    if (assign.value->type !=
        &cursiv::ast_type_tag<kirikiritjs::ast::BinaryExpr>)
      return fail("'(int) a + 1' is not '((int) a) + 1': value is not a "
                  "top-level '+'");
    const auto &plus =
        *static_cast<const kirikiritjs::ast::BinaryExpr *>(assign.value);
    if (plus.left->type !=
        &cursiv::ast_type_tag<kirikiritjs::ast::TypeConvertExpr>)
      return fail("'(int) a + 1': left of '+' is not the cast -- it binds "
                  "looser than documented");
    const auto &cast =
        *static_cast<const kirikiritjs::ast::TypeConvertExpr *>(plus.left);
    if (cast.operand->type != &cursiv::ast_type_tag<kirikiritjs::ast::Ident>)
      return fail("'(int) a + 1': cast operand is not the plain name 'a'");
  }

  std::printf(
      "kirikiritjs: %zu example(s) and %zu own construct file(s) parse clean\n",
      example_count, files.size() - example_count);
  return 0;
}
