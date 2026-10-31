#include "converter_cases.hpp"
#include "representation_cases.hpp"
// The smoke test of the language: parse every file in example/, check each
// tree, and read a few real field values back out of one of them.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "clu/core/CoreServices.hpp"
#include "clu/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "clu: %s: %s\n", file.string().c_str(), what);
  return 1;
}

int check_fields(const std::filesystem::path& dir) {
  const auto language = cursiv::create_language<
      clu::CluCoreServices, clu::CluSharedCoreServices,
      clu::install_core_module>();

  const std::filesystem::path file = dir / "bottles-133.clu";
  std::ifstream in(file);
  if (!in) return fail(file, "the document is not readable");
  const std::string text((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(file, "the document does not parse clean");

  const auto* const module = result.value_as<clu::ast::Module>();
  if (module == nullptr) return fail(file, "the parse built no Module");
  if (module->routine == nullptr ||
      module->routine->type != &cursiv::ast_type_tag<clu::ast::Routine>)
    return fail(file, "the module's routine is not a Routine");

  const auto& routine = *static_cast<const clu::ast::Routine*>(module->routine);
  if (routine.name != "start_up" || routine.endName != "start_up")
    return fail(file, "the routine's name does not read back");
  if (routine.ownVars.size() != 1 || routine.ownVars[0]->decls.empty() ||
      routine.ownVars[0]->decls[0]->names.empty() ||
      routine.ownVars[0]->decls[0]->names[0] != "po")
    return fail(file, "the own variable's name does not read back");
  if (routine.statements.empty() ||
      routine.statements[0]->type != &cursiv::ast_type_tag<clu::ast::ForStmt>)
    return fail(file, "the first statement is not the for loop");

  const auto& loop =
      *static_cast<const clu::ast::ForStmt*>(routine.statements[0]);
  if (loop.decls.size() != 1 || loop.decls[0]->names.empty() ||
      loop.decls[0]->names[0] != "i")
    return fail(file, "the loop variable's name does not read back");

  return 0;
}

// `**` is right-associative (CLU Reference Manual, A.6.11, p.356): the only
// operator so marked, and the port's own infix table had it left.
int check_power_right_assoc() {
  const auto language = cursiv::create_language<
      clu::CluCoreServices, clu::CluSharedCoreServices,
      clu::install_core_module>();
  const cursiv::ParseResult r = language.language.parser->parse(
      "p = proc ()\n    x: int := a + b ** c ** d\nend p\n");
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail("probe", "a + b ** c ** d does not parse clean");

  const auto* const module = r.value_as<clu::ast::Module>();
  const auto& routine = *static_cast<const clu::ast::Routine*>(module->routine);
  const auto& outer =
      *static_cast<const clu::ast::VarInitStmt*>(routine.statements[0]);
  const auto* const add = static_cast<const clu::ast::BinaryExpr*>(outer.value);
  if (add->type != &cursiv::ast_type_tag<clu::ast::BinaryExpr> ||
      add->op != clu::ast::BinOp::add)
    return fail("probe", "a + (...) is not the outermost node");
  const auto* const rhs = static_cast<const clu::ast::BinaryExpr*>(add->right);
  if (rhs->type != &cursiv::ast_type_tag<clu::ast::BinaryExpr> ||
      rhs->op != clu::ast::BinOp::power)
    return fail("probe", "b ** c ** d did not group under +");
  const auto* const rhsRight =
      static_cast<const clu::ast::BinaryExpr*>(rhs->right);
  if (rhsRight->type != &cursiv::ast_type_tag<clu::ast::BinaryExpr> ||
      rhsRight->op != clu::ast::BinOp::power)
    return fail("probe", "b ** c ** d read left-associative: (b ** c) ** d");
  return 0;
}

// An equate's bare name (`Q = foo`) is an expression: `constant: expression |
// type_spec` lists it first, and the reference compiler reads every right side
// but a type set with `p_expr` (PCLU, cmpclu/pdefn1.clu, `p_equate`). Whether
// `foo` names a constant or a type is left to name resolution; a type keyword
// still opens a type_spec (`R = int`).
int check_equate_bare_name_reading() {
  const auto language = cursiv::create_language<
      clu::CluCoreServices, clu::CluSharedCoreServices,
      clu::install_core_module>();
  const cursiv::ParseResult r = language.language.parser->parse(
      "p = proc ()\n    Q = foo\n    R = int\nend p\n");
  if (r.cst().recovered() || !r.diagnostics.empty())
    return fail("probe", "Q = foo does not parse clean");
  const auto* const module = r.value_as<clu::ast::Module>();
  const auto& routine = *static_cast<const clu::ast::Routine*>(module->routine);
  if (routine.bodyEquates.size() != 2)
    return fail("probe", "Q = foo, R = int did not read as two equates");

  const auto& bare = *routine.bodyEquates[0];
  if (!std::holds_alternative<clu::ast::Expression*>(bare.value))
    return fail("probe", "Q = foo no longer reads as an Expression");
  const auto* const name = static_cast<const clu::ast::NameRef*>(
      std::get<clu::ast::Expression*>(bare.value));
  if (name->type != &cursiv::ast_type_tag<clu::ast::NameRef> ||
      name->name != "foo")
    return fail("probe", "Q = foo is not the name foo");

  if (!std::holds_alternative<clu::ast::TypeSpec*>(
          routine.bodyEquates[1]->value))
    return fail("probe", "R = int no longer reads as a TypeSpec");
  return 0;
}

int check_native_shapes() {
  namespace ast = clu::ast;
  const auto language = cursiv::create_language<
      clu::CluCoreServices, clu::CluSharedCoreServices, clu::install_core_module>();
  const std::string source = R"clu(
_P = PROC [T: TYPE] (a: ARRAY[T], s: SEQUENCE[INT]);
  kind: TYPE;
  c: CHAR := '\101';
  text: STRING := "a\n\"b";
  r: REAL := .5E+2;
  f: ANY := FORCE[INT];
  ping();
  WHILE TRUE DO CONTINUE; END;
  ping() EXCEPT WHEN oops (why: STRING): RETURN; END;
END _p;
)clu";
  const auto result = language.language.parser->parse(source);
  const auto* module = result.value_as<ast::Module>();
  const auto* routine = module ? cursiv::ast_cast<ast::Routine>(module->routine) : nullptr;
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !routine || routine->parms.size() != 1 || !routine->parms[0]->isTypeParam ||
      routine->args.size() != 2 || routine->statements.size() != 8)
    return fail("native-shapes", "native syntax did not build its complete routine");
  const auto* array = cursiv::ast_cast<ast::ArrayTypeSpec>(routine->args[0]->type);
  const auto* seq = cursiv::ast_cast<ast::SequenceTypeSpec>(routine->args[1]->type);
  if (!array || !array->element || !seq || !seq->element)
    return fail("native-shapes", "array or sequence element type was lost");
  bool ch = false, str = false, real = false, force = false, zero_call = false,
       continued = false, handler = false;
  for (const auto* node : result.nodes) {
    if (const auto* v = cursiv::ast_cast<ast::CharLit>(node)) ch |= v->value == "A";
    if (const auto* v = cursiv::ast_cast<ast::StringLit>(node)) str |= v->value == "a\n\"b";
    if (const auto* v = cursiv::ast_cast<ast::RealLit>(node)) real |= v->value == 50.;
    if (const auto* v = cursiv::ast_cast<ast::ForceExpr>(node)) force |= v->type != nullptr;
    if (const auto* v = cursiv::ast_cast<ast::Call>(node)) zero_call |= v->args.empty();
    continued |= cursiv::ast_cast<ast::ContinueStmt>(node) != nullptr;
    if (const auto* v = cursiv::ast_cast<ast::WhenHandler>(node))
      handler |= v->decls.size() == 1 && v->decls[0]->names.size() == 1 &&
                 v->decls[0]->names[0] == "why";
  }
  if (!ch || !str || !real || !force || !zero_call || !continued || !handler)
    return fail("native-shapes", "native literal, call or handler capture was lost");
  const auto validation = cursiv::test::validationHelper(language.language, source);
  if (!validation.diagnostics.empty())
    return fail("native-shapes", "case-insensitive END name was rejected");
  const std::string set_source =
      "S = {t | t has equal: proctype(t, t) returns(bool)}; "
      "p = proc [u: type]() where u in S; end p;";
  const auto set_result = language.language.parser->parse(set_source);
  const auto* set_module = set_result.value_as<ast::Module>();
  const auto* set_routine = set_module
      ? cursiv::ast_cast<ast::Routine>(set_module->routine) : nullptr;
  const auto* set = set_module && set_module->equates.size() == 1
      ? std::get_if<ast::TypeSet*>(&set_module->equates[0]->value) : nullptr;
  if (!set_result.accepted || set_result.cst().recovered() ||
      !set_result.diagnostics.empty() || !set || !*set || (*set)->name != "t" ||
      (*set)->items.size() != 1 || (*set)->items[0]->name != "t" ||
      (*set)->items[0]->ops.size() != 1 || !set_routine ||
      set_routine->where.size() != 1 || set_routine->where[0]->inName != "S")
    return fail("native-shapes", "type-set binder, operation or named restriction was lost");
  for (const char* invalid : {
           "p = proc t: type() end p", "p = proc(a: array) end p",
           "p = proc(a: sequence int) end p", "p = proc() x: any := force int end p",
           "p = proc() x: char := 'ab' end p", "p = proc() x: string := \"a\\q\" end p",
           "p = proc() int: int end p"}) {
    const auto bad = language.language.parser->parse(invalid);
    if (bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty())
      return fail("native-shapes", "old non-native spelling was still accepted");
  }
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      clu::CluCoreServices, clu::CluSharedCoreServices,
      clu::install_core_module>();

  if (!clu_converter_cases::run(*language.language.parser)) return 1;
  if (!clu_representation_cases::run(*language.language.parser)) return 1;

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".clu")
      continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false. Every file here is meant to
    // parse cleanly, so a diagnostic is also a failure.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(file, "the document carries a diagnostic");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<clu::ast::Module>)
      failures += fail(file, "the parse built no Module");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  failures += check_fields(CURSIV_EXAMPLE_DIR);
  failures += check_power_right_assoc();
  failures += check_equate_bare_name_reading();
  failures += check_native_shapes();
  if (failures > 0) return 1;

  std::printf("clu: %d example(s) parse\n", total);
  return 0;
}
