// The grammar's smoke test: the upstream sample and this port's own
// features document both parse clean, and a few real field values read
// back -- a converter that silently gave the wrong value would still pass
// a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "limbo/core/CoreServices.hpp"
#include "limbo/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

namespace ast = limbo::ast;

int fail(const char* what) {
  std::fprintf(stderr, "limbo: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      limbo::LimboCoreServices, limbo::LimboSharedCoreServices,
      limbo::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  // example1.b: the upstream corpus's own sample.
  {
    const std::string text = read(CURSIV_EXAMPLE_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("example1.b does not parse");
    if (!result.diagnostics.empty())
      return fail("example1.b carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("example1.b built no program");
    if (program->name != "Command")
      return fail("example1.b: the implement name did not read back");
    if (program->decls.size() != 5)
      return fail("example1.b: wrong top-level declaration count");
    const auto* init = as<ast::FunctionDef>(program->decls[4]);
    if (init == nullptr || init->nameParts.size() != 1 ||
        init->nameParts[0] != "init")
      return fail("example1.b: the function name did not read back");
    if (init->signature == nullptr || init->signature->params.size() != 2)
      return fail("example1.b: wrong parameter count");
    const auto* ref = as<ast::RefType>(init->signature->params[0]->type);
    const auto* named = ref ? as<ast::NamedType>(ref->target) : nullptr;
    if (named == nullptr || named->id1 != "Draw" || named->id2 != "Context")
      return fail("example1.b: the ref/module-qualified type did not read back");
  }

  // features.b: this port's own document, one of each remaining construct.
  {
    const std::string text = read(CURSIV_TEST_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("features.b does not parse");
    if (!result.diagnostics.empty())
      return fail("features.b carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("features.b built no program");
    if (program->name != "Features")
      return fail("features.b: the implement name did not read back");

    const auto* mod = as<ast::ModuleDecl>(program->decls[1]);
    if (mod == nullptr || mod->name != "Math" || mod->members.size() != 2)
      return fail("features.b: the module declaration did not read back");
    const auto* con = as<ast::ConDecl>(mod->members[0]);
    if (con == nullptr || con->names.size() != 1 || con->names[0] != "PI")
      return fail("features.b: the module's con member did not read back");

    const auto* adt = as<ast::AdtDecl>(program->decls[2]);
    if (adt == nullptr || adt->name != "Shape" || adt->members.size() != 4)
      return fail("features.b: the adt declaration did not read back");
    const auto* selfFn = as<ast::AdtField>(adt->members[2]);
    const auto* fnType =
        selfFn != nullptr ? as<ast::FnType>(selfFn->type) : nullptr;
    if (fnType == nullptr || fnType->signature->params.size() != 1 ||
        !fnType->signature->params[0]->isSelf ||
        !fnType->signature->params[0]->refopt)
      return fail("features.b: the refopt self parameter did not read back");
    const auto* pick = as<ast::PickDecl>(adt->members[3]);
    if (pick == nullptr || pick->arms.size() != 2 ||
        pick->arms[0]->tags.size() != 2 ||
        pick->arms[0]->tags[1] != "Square")
      return fail("features.b: the pick declaration did not read back");

    const auto* mainFn = as<ast::FunctionDef>(program->decls.back());
    if (mainFn == nullptr || mainFn->nameParts[0] != "main")
      return fail("features.b: did not find the main function");

    bool foundCast = false, foundCase = false, foundCons = false,
         foundSlice = false;
    for (const ast::Item* item : mainFn->body) {
      const auto* stmt = as<ast::ExprStmt>(item);
      const auto* decl =
          stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
      const auto* target =
          decl != nullptr ? as<ast::Ident>(decl->target) : nullptr;
      if (target != nullptr && target->name == "n") {
        const auto* cast = as<ast::CastExpr>(decl->value);
        const auto* prim =
            cast != nullptr ? as<ast::PrimType>(cast->type) : nullptr;
        if (prim != nullptr && prim->keyword == "big" &&
            as<ast::IntLit>(cast->operand) != nullptr)
          foundCast = true;
      }
      if (target != nullptr && target->name == "s") {
        const auto* cons = as<ast::BinaryExpr>(decl->value);
        if (cons != nullptr && cons->op == "::") foundCons = true;
      }
      if (target != nullptr && target->name == "p") {
        const auto* idx = as<ast::IndexExpr>(decl->value);
        if (idx != nullptr && idx->isSlice && idx->high != nullptr)
          foundSlice = true;
      }
      if (const auto* c = as<ast::CaseStmt>(item); c != nullptr) {
        if (c->arms.size() == 3 && c->arms[1]->labels[0]->b != nullptr)
          foundCase = true;
      }
    }
    if (!foundCast) return fail("features.b: the cast expression did not read back");
    if (!foundCase) return fail("features.b: the case range label did not read back");
    if (!foundCons) return fail("features.b: the '::' operator did not read back");
    if (!foundSlice) return fail("features.b: the slice expression did not read back");
  }

  // The language manual's table (Section 8.3): `::` sits between `|` and
  // `&&`/`||`, and is right-associative.
  {
    const cursiv::ParseResult result = parser.parse(
        "implement P;\ninit()\n{\n  x := a | b :: c || d;\n}\n");
    if (result.cst().recovered()) return fail("a|b::c||d does not parse");
    if (!result.diagnostics.empty())
      return fail("a|b::c||d carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    const auto* init = program != nullptr
                            ? as<ast::FunctionDef>(program->decls.back())
                            : nullptr;
    const auto* stmt =
        init != nullptr && !init->body.empty() ? as<ast::ExprStmt>(init->body[0]) : nullptr;
    const auto* decl = stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
    const auto* orExpr = decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
    if (orExpr == nullptr || orExpr->op != "||")
      return fail("a|b::c||d: || is not the loosest operator");
    const auto* consExpr = as<ast::BinaryExpr>(orExpr->left);
    if (consExpr == nullptr || consExpr->op != "::")
      return fail("a|b::c||d: :: does not sit tighter than ||");
    if (as<ast::BinaryExpr>(consExpr->left) == nullptr)
      return fail("a|b::c||d: :: does not sit looser than |");
  }
  {
    const cursiv::ParseResult result =
        parser.parse("implement P;\ninit()\n{\n  x := a :: b :: c;\n}\n");
    if (result.cst().recovered()) return fail("a::b::c does not parse");
    if (!result.diagnostics.empty())
      return fail("a::b::c carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    const auto* init = program != nullptr
                            ? as<ast::FunctionDef>(program->decls.back())
                            : nullptr;
    const auto* stmt =
        init != nullptr && !init->body.empty() ? as<ast::ExprStmt>(init->body[0]) : nullptr;
    const auto* decl = stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
    const auto* cons = decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
    if (cons == nullptr || cons->op != "::" ||
        as<ast::BinaryExpr>(cons->left) != nullptr ||
        as<ast::BinaryExpr>(cons->right) == nullptr)
      return fail("a::b::c: :: is not right-associative");
  }

  {
    const auto result = parser.parse(R"LIMBO(
Thing: adt[T] { value: T; pick { Some => more: T; None => } }
    for { T => copy: fn(nil: self T): T; };
Oops: exception(string);
Alias: type M[T]->Inner[U].Node;
F: type fn[T](x: T): T for { T => copy: fn(nil: self T): T; } raises(Oops);
f[T](x: T): T raises(Oops) {
  { raise Oops("bad"); } exception e { * => return x; }
}
(a, (b, c)) := f[int](1);
init() {
  s := "quoted\" value";
  raw := `raw
value`;
  ch := '\n';
  r := 16RFF;
  realnum := .5e-2;
  based := 16rA.F;
  queue := chan[2] of int;
  queue <- 1;
  queue <-= 2;
  power := 2**3**2;
  power **= 2;
  values := array[] of {1, 2,};
  indexed := array[] of {1 to 3 or 5 => 10, * => 0};
  sequence := list of {1, 2,};
  slice := values[:2];
  other := values[:];
  call := f[int, array of byte](1);
  ops: import (moduleValue);
  first, second: while(1) { break second; }
  spawn (f[int](1));
}
)LIMBO");
    const auto* root = result.value_as<ast::Program>();
    bool generic = false, constraints = false, raises = false, handler = false;
    bool character = false, based = false, real = false, channel = false, send = false;
    bool power = false, slice = false, exports = false, spawn = false, labels = false, initializerKeys = false;
    for (const auto* node : result.nodes) {
      if (const auto* x = as<ast::AdtDecl>(node)) {
        generic |= x->parameters && x->parameters->names.size() == 1;
        constraints |= x->constraints.size() == 1 && x->constraints[0]->fields.size() == 1;
      }
      if (const auto* x = as<ast::FunctionDef>(node)) raises |= x->raises && !x->typeParameters.empty();
      if (const auto* x = as<ast::TryStmt>(node)) handler |= x->body && x->variable == "e" && x->handlers.size() == 1;
      if (const auto* x = as<ast::IntLit>(node)) { character |= x->text == "'\\n'"; based |= x->text == "16RFF"; }
      if (const auto* x = as<ast::RealLit>(node)) real |= x->text == ".5e-2";
      if (const auto* x = as<ast::ChanOfExpr>(node)) channel |= x->size && x->elemType;
      if (const auto* x = as<ast::SendExpr>(node)) send |= !x->equals && x->target && x->value;
      if (const auto* x = as<ast::BinaryExpr>(node)) power |= x->op == "**" && as<ast::BinaryExpr>(x->right);
      if (const auto* x = as<ast::IndexExpr>(node)) slice |= x->isSlice && !x->index;
      if (const auto* x = as<ast::ExportSequence>(node)) exports |= x->values.size() == 2;
      if (const auto* x = as<ast::SpawnStmt>(node)) spawn |= x->expression != nullptr;
      if (const auto* x = as<ast::InitElem>(node)) initializerKeys |= x->keys.size() == 2 && x->keys[0]->b != nullptr;
      if (const auto* x = as<ast::WhileStmt>(node)) labels |= x->label == "first" && x->additionalLabels.size() == 1;
    }
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !root ||
        !root->name.empty() || !generic || !constraints || !raises || !handler || !character ||
        !based || !real || !channel || !send || !power || !slice || !exports || !spawn || !labels || !initializerKeys)
      return fail("native syntax families or structured AST were lost");
    std::unordered_set<const cursiv::AstNode*> reached;
    bool owned = true;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!reached.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node, [&](auto, auto, auto& child) {
        owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    walk(walk, *root);
    if (!owned || root->parent || reached.size() != result.nodes.size())
      return fail("native AST has missing, orphaned or multiply-owned nodes");
  }
  for (const auto source : {"implement A,B; x: int;", "_name: int;", "λ: int;", "R5: int;"}) {
    const auto result = parser.parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native implementation list/identifier form rejected");
  }
  for (const auto source : {"implement if; x:int;", "init(){s:=\"a\nb\";}",
                            "init(){x:=\"bad\\q\";}", "a,b := f();"}) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("non-native reserved/string/top-assignment form accepted");
  }
  for (const auto source : {"init(){x:=1r0;}", "init(){x:=2r2;}", "init(){x:=37rZ;}",
                            "init(){a:=array[] of int;}", "f(*,x:int){}", "M:module{x:int=1;};"}) {
    const auto validated = cursiv::test::validationHelper(language.language, source);
    if (validated.diagnostics.empty()) return fail("native invalid form needs runtime diagnostic");
  }
  std::puts("limbo: the samples parse");
  return 0;
}
