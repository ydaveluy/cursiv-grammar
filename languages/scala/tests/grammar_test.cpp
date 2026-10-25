
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "modern_cases.hpp"
#include "scala/core/CoreServices.hpp"
#include "scala/core/ast.hpp"
// The smoke test of the language: every document under `example/` and
// `tests/` parses clean (no recovery) and builds a CompilationUnit.

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"

namespace {

namespace ast = scala::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "scala: %s\n", what.c_str());
  return 1;
}

template <class T>
T* as(const cursiv::AstReflection& reflection, cursiv::AstNode* n) {
  return (n != nullptr && reflection.isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

// A parse kept alive with its text: the AST holds views into it.
struct Parsed {
  std::string text;
  cursiv::ParseResult result;
  Parsed(const cursiv::Parser& parser, std::string source)
      : text(std::move(source)), result(parser.parse(text)) {}
  Parsed(Parsed&&) = delete;
};

// The members of `object T { ... }`, or null when the parse needed repair.
ast::ClassTemplate* object_body(const Parsed& p,
                                const cursiv::AstReflection& reflection) {
  if (!p.result.accepted || p.result.cst().recovered()) return nullptr;
  ast::CompilationUnit* const u = p.result.value_as<ast::CompilationUnit>();
  if (u == nullptr || u->items.size() != 1) return nullptr;
  ast::ObjectDef* const obj = as<ast::ObjectDef>(reflection, u->items[0]);
  return obj != nullptr ? obj->templateOpt : nullptr;
}

ast::Expr* value_of(const Parsed& p, const cursiv::AstReflection& reflection) {
  ast::ClassTemplate* const body = object_body(p, reflection);
  if (body == nullptr || body->members.size() != 1) return nullptr;
  ast::PatDef* const def = as<ast::PatDef>(reflection, body->members[0]);
  return def != nullptr ? def->value : nullptr;
}

ast::Block* def_block(const Parsed& p,
                      const cursiv::AstReflection& reflection) {
  ast::ClassTemplate* const body = object_body(p, reflection);
  ast::DefDef* const def = body == nullptr || body->members.size() != 1
                               ? nullptr
                               : as<ast::DefDef>(reflection, body->members[0]);
  return def == nullptr ? nullptr : as<ast::Block>(reflection, def->value);
}

std::size_t statement_count(const ast::Block& block) {
  return block.stats.size() + (block.result != nullptr ? 1 : 0);
}

struct Grouping {
  const char* expr;
  const char* op;
  bool nestedRight;
  const char* nested;
};

// SLS §6.12.3: the first character selects the class, the last one the
// associativity. §1.2: an operator ending a line takes the next line as its
// operand; a name opening a line or following a `;` starts a new statement.
int check_precedence(const cursiv::Parser& parser,
                     const cursiv::AstReflection& reflection) {
  for (const Grouping& g : {Grouping{"a + b * c", "+", true, "*"},
                            Grouping{"a * b + c", "+", false, "*"},
                            Grouping{"a :: b :: c", "::", true, "::"},
                            Grouping{"a max b + c", "max", true, "+"},
                            Grouping{"xs :+ a :+ b", ":+", false, ":+"},
                            Grouping{"a +: b +: xs", "+:", true, "+:"},
                            Grouping{"1 #:: 2 #:: s", "#::", true, "#::"},
                            Grouping{"a +\n  b * c", "+", true, "*"}}) {
    const Parsed p(parser, std::string("object T { val x = ") + g.expr + " }");
    ast::InfixExpr* const top =
        as<ast::InfixExpr>(reflection, value_of(p, reflection));
    ast::InfixExpr* const nested =
        top == nullptr ? nullptr
                       : as<ast::InfixExpr>(reflection, g.nestedRight
                                                            ? top->right
                                                            : top->left);
    if (nested == nullptr || top->op != g.op || nested->op != g.nested)
      return fail(std::string(g.expr) + " does not group as the language does");
  }

  const Parsed trailing(parser, "object T { val x = a +\n }");
  ast::PostfixExpr* const postfix =
      as<ast::PostfixExpr>(reflection, value_of(trailing, reflection));
  if (postfix == nullptr || postfix->postfixOp != "+")
    return fail("a + at the end of a block does not read as postfix");

  const Parsed twoLines(parser, "object T {\n  val x = a\n  b\n}");
  ast::ClassTemplate* const body = object_body(twoLines, reflection);
  if (body == nullptr || body->members.size() != 2)
    return fail("a name opening a line is read as a postfix operator");

  const Parsed inBlock(parser,
                       "object T {\n  def f = {\n    g(x)\n    done\n  }\n}");
  ast::Block* const block = def_block(inBlock, reflection);
  if (block == nullptr || statement_count(*block) != 2)
    return fail("a name opening a line of a block is read as a postfix operator");

  const Parsed semis(parser, "object T { def f = { var q = 1; q = 2; q } }");
  ast::Block* const semiBlock = def_block(semis, reflection);
  if (semiBlock == nullptr || statement_count(*semiBlock) != 3)
    return fail("a name after a ; is read as a postfix operator");

  const Parsed forSemi(
      parser, "object T { val x = for { x <- xs; y = x - 1 } yield y }");
  ast::ForExpr* const loop =
      as<ast::ForExpr>(reflection, value_of(forSemi, reflection));
  ast::Generator* const gen = loop == nullptr || loop->enumerators.size() != 1
                                  ? nullptr
                                  : loop->enumerators[0];
  if (gen == nullptr || as<ast::PathExpr>(reflection, gen->source) == nullptr ||
      gen->refinements.size() != 1 ||
      as<ast::ValueDef>(reflection, gen->refinements[0]) == nullptr)
    return fail(
        "a value definition after a ; in an enumerator is read as an "
        "assignment to a postfix operation");

  // SLS 6.23: a function literal opening a block takes the rest of it.
  const Parsed lambda(
      parser, "object T { val x = xs.foreach { y =>\n a(y)\n b(y)\n } }");
  ast::CallExpr* const call =
      as<ast::CallExpr>(reflection, value_of(lambda, reflection));
  ast::Lambda* const literal = call == nullptr || call->args.size() != 1
                                   ? nullptr
                                   : as<ast::Lambda>(reflection, call->args[0]);
  ast::Block* const literalBody =
      literal == nullptr ? nullptr : as<ast::Block>(reflection, literal->body);
  if (as<ast::PathExpr>(reflection, call == nullptr ? nullptr : call->callee) ==
          nullptr ||
      literalBody == nullptr || statement_count(*literalBody) != 2)
    return fail("a function literal's body stops before the end of its block");

  const Parsed cases(parser, "object T { val x = xs.map { case 1 => 2 } }");
  ast::CallExpr* const mapped =
      as<ast::CallExpr>(reflection, value_of(cases, reflection));
  if (mapped == nullptr || mapped->args.size() != 1 ||
      as<ast::PathExpr>(reflection, mapped->callee) == nullptr ||
      as<ast::MatchBlock>(reflection, mapped->args[0]) == nullptr)
    return fail("a block argument is not the call's argument");

  const Parsed applied(parser, "object T { val x: Map[Int, String] = y }");
  ast::ClassTemplate* const typed = object_body(applied, reflection);
  ast::PatDef* const typedDef =
      typed == nullptr || typed->members.size() != 1
          ? nullptr
          : as<ast::PatDef>(reflection, typed->members[0]);
  ast::AppliedType* const map =
      typedDef == nullptr ? nullptr
                          : as<ast::AppliedType>(reflection, typedDef->type);
  if (map == nullptr || map->args.size() != 2)
    return fail("a type's arguments are lost");

  // SLS §1.1: `type` is reserved, so `a.type` is a singleton type.
  const Parsed singleton(parser, "object T { val x: a.type = a }");
  ast::ClassTemplate* const holder = object_body(singleton, reflection);
  ast::PatDef* const singletonDef =
      holder == nullptr || holder->members.size() != 1
          ? nullptr
          : as<ast::PatDef>(reflection, holder->members[0]);
  ast::NamedType* const single =
      singletonDef == nullptr
          ? nullptr
          : as<ast::NamedType>(reflection, singletonDef->type);
  if (single == nullptr || !single->isSingleton ||
      !single->path->segments.empty())
    return fail("a.type is not a singleton type");

  const Parsed macro(parser, "object T { def f: Int = macro Impl.f }");
  ast::ClassTemplate* const macroBody = object_body(macro, reflection);
  ast::DefDef* const macroDef =
      macroBody == nullptr || macroBody->members.size() != 1
          ? nullptr
          : as<ast::DefDef>(reflection, macroBody->members[0]);
  if (macroDef == nullptr || !macroDef->isMacro ||
      as<ast::PathExpr>(reflection, macroDef->value) == nullptr)
    return fail("a macro definition is not read as one");

  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<scala::ScalaCoreServices,
                                                scala::ScalaSharedCoreServices,
                                                scala::install_core_module>();
  {
    const auto r = language.language.parser->parse("object X { val a = 0xA }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* unit = r.value_as<scala::ast::CompilationUnit>();
    const auto* object =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<scala::ast::ObjectDef>(unit->items[0])
            : nullptr;
    const auto* def = object && object->templateOpt &&
                              object->templateOpt->members.size() == 1
                          ? cursiv::ast_cast<scala::ast::PatDef>(
                                object->templateOpt->members[0])
                          : nullptr;
    const auto* integer =
        def ? cursiv::ast_cast<scala::ast::IntLit>(def->value) : nullptr;
    if (!integer || integer->text != "0xA") return 1;
  }
  {
    const auto r = language.language.parser->parse("object X { val a = 12e3 }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* unit = r.value_as<scala::ast::CompilationUnit>();
    const auto* object =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<scala::ast::ObjectDef>(unit->items[0])
            : nullptr;
    const auto* def = object && object->templateOpt &&
                              object->templateOpt->members.size() == 1
                          ? cursiv::ast_cast<scala::ast::PatDef>(
                                object->templateOpt->members[0])
                          : nullptr;
    const auto* number =
        def ? cursiv::ast_cast<scala::ast::FloatLit>(def->value) : nullptr;
    if (!number || number->text != "12e3") return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "object X { val y = x match { case a :: b :: c => 1 } }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* unit = r.value_as<scala::ast::CompilationUnit>();
    const auto* object =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<scala::ast::ObjectDef>(unit->items[0])
            : nullptr;
    const auto* def = object && object->templateOpt &&
                              object->templateOpt->members.size() == 1
                          ? cursiv::ast_cast<scala::ast::PatDef>(
                                object->templateOpt->members[0])
                          : nullptr;
    const auto* match =
        def ? cursiv::ast_cast<scala::ast::MatchExpr>(def->value) : nullptr;
    const auto* first = match && match->cases.size() == 1
                            ? cursiv::ast_cast<scala::ast::InfixPattern>(
                                  match->cases[0]->pattern)
                            : nullptr;
    const auto* second =
        first ? cursiv::ast_cast<scala::ast::InfixPattern>(first->right)
              : nullptr;
    if (!first || first->op != "::" || !second || second->op != "::" ||
        cursiv::ast_cast<scala::ast::InfixPattern>(first->left))
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      scala::ScalaCoreServices,
      scala::ScalaSharedCoreServices,
      scala::install_core_module>();

  if (!scala_audit::modern_cases(*language.language.parser))
    return 1;

  int documents = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".scala")
      continue;
    ++documents;

    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": the document does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<scala::ast::CompilationUnit>)
      return fail(entry.path().string() + ": the parse built no compilation unit");
  }

  if (documents == 0) return fail("no .scala document found under example/");

  if (const int precedence = check_precedence(
          *language.language.parser, *language.language.astReflection);
      precedence != 0)
    return precedence;

  std::printf("scala: %d document(s) parse clean\n", documents);
  return 0;
}
