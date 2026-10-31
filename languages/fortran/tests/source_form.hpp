#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "fortran/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "source_form_cases.hpp"

namespace fortran_source {
namespace ast = fortran::ast;

inline bool fail(std::string_view what, std::string_view source) {
  std::fprintf(stderr, "Fortran source form: %.*s\n%.*s\n",
               static_cast<int>(what.size()), what.data(),
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool clean(const cursiv::ParseResult& result, std::string_view source) {
  if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) return true;
  std::fprintf(stderr, "accepted=%d recovered=%d diagnostics=%zu\n", result.accepted,
               result.cst().recovered(), result.diagnostics.size());
  for (const auto& diagnostic : result.diagnostics)
    std::fprintf(stderr, "kind=%d begin=%u end=%u\n", static_cast<int>(diagnostic.kind),
                 diagnostic.begin, diagnostic.end);
  return fail("clean parse required", source);
}
inline bool parents(const auto& language, const cursiv::ParseResult& result) {
  if (!result.value || result.value->parent) return false;
  std::unordered_set<const cursiv::AstNode*> seen;
  bool valid = true;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *result.value);
  return valid && seen.size() == result.nodes.size();
}

inline bool run(const auto& language) {
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = language.parser->parse(source);
    if (test.diagnostic == "parse") {
      if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
        return fail(std::string(test.id) + ": invalid source form accepted", source);
      continue;
    }
    if (!clean(result, source) || !parents(language, result)) return fail(test.id, source);
    const auto diagnostics = cursiv::test::validationHelper(language, source).diagnostics;
    if (test.diagnostic.empty()) {
      if (!diagnostics.empty()) return fail(std::string(test.id) + ": unexpected validation", source);
    } else {
      bool found = false;
      for (const auto& diagnostic : diagnostics)
        found |= diagnostic.code && diagnostic.code->isString() &&
                 diagnostic.code->string() == test.diagnostic;
      if (!found) return fail(std::string(test.id) + ": expected " +
                              std::string(test.diagnostic), source);
    }
  }

  const std::string source = R"f(module MiXeD
type :: T
integer x
end type t
interface Gen
subroutine Proto(x)
integer x
end subroutine proto
end interface gen
contains
real function F(x) result(y)
real x
y=x
end function f
subroutine S
end subroutine s
end module mixed
block data B
integer i
common /c/i
data i/1/
end block data b
program Main
integer i,x
outer: do i=1,2
test: if(i>0) then
x=1
else if(i==0) then test
x=2
else TEST
x=3
end if Test
pick: select case(i)
case(1) pick
x=4
case default PICK
x=5
end select Pick
end do OUTER
if(.true.) x=5
do 010 i=1,2
x=x+1
10 continue
end program main
)f";
  const auto result = language.parser->parse(source);
  if (!clean(result, source) || !parents(language, result)) return false;
  if (!cursiv::test::validationHelper(language, source).diagnostics.empty())
    return fail("case-insensitive end names and labelled block DO", source);
  const auto* model = cursiv::ast_cast<ast::Model>(result.value);
  if (!model || model->units.size() != 3) return fail("three units", source);
  const auto* module = cursiv::ast_cast<ast::ModuleUnit>(model->units[0]);
  const auto* block = cursiv::ast_cast<ast::BlockDataUnit>(model->units[1]);
  const auto* main = cursiv::ast_cast<ast::MainProgramUnit>(model->units[2]);
  if (!module || module->name != "MiXeD" || module->endName != "mixed" ||
      !module->endHasKind || module->body.size() != 2 || module->subprograms.size() != 2 ||
      !block || block->name != "B" || block->endName != "b" || !block->endHasKind ||
      !main || main->name != "Main" || main->endName != "main" ||
      !main->endHasKind || main->body.size() != 4)
    return fail("opening/closing unit names and exact bodies", source);
  const auto* type = cursiv::ast_cast<ast::DerivedTypeDecl>(module->body[0]);
  const auto* interface = cursiv::ast_cast<ast::InterfaceDecl>(module->body[1]);
  const auto* prototype = interface && interface->procedures.size() == 1
      ? cursiv::ast_cast<ast::SubroutineUnit>(interface->procedures[0]) : nullptr;
  const auto* function = cursiv::ast_cast<ast::FunctionUnit>(module->subprograms[0]);
  const auto* subroutine = cursiv::ast_cast<ast::SubroutineUnit>(module->subprograms[1]);
  if (!type || type->name != "T" || type->endName != "t" ||
      !interface || interface->name != "Gen" || interface->endName != "gen" ||
      !prototype || prototype->name != "Proto" || prototype->endName != "proto" ||
      !function || function->name != "F" || function->endName != "f" ||
      !subroutine || subroutine->name != "S" || subroutine->endName != "s")
    return fail("derived type, interface, internal procedure and function names", source);
  const auto* loop = cursiv::ast_cast<ast::DoStmt>(main->body[1]);
  const auto* branch = loop && loop->body.size() == 2
      ? cursiv::ast_cast<ast::BlockIfStmt>(loop->body[0]) : nullptr;
  const auto* select = loop && loop->body.size() == 2
      ? cursiv::ast_cast<ast::SelectCaseStmt>(loop->body[1]) : nullptr;
  const auto* logicalIf = cursiv::ast_cast<ast::IfStmt>(main->body[2]);
  const auto* labelled = cursiv::ast_cast<ast::DoStmt>(main->body[3]);
  if (!loop || loop->name != "outer" || loop->endName != "OUTER" ||
      !branch || branch->name != "test" || branch->endName != "Test" ||
      !branch->hasElse || branch->elseName != "TEST" || branch->elseIfs.size() != 1 ||
      branch->elseIfs[0]->name != "test" || branch->thenBody.size() != 1 ||
      branch->elseBody.size() != 1 || branch->elseIfs[0]->body.size() != 1 ||
      !select || select->name != "pick" || select->endName != "Pick" ||
      select->cases.size() != 2 || select->cases[0]->name != "pick" ||
      select->cases[1]->name != "PICK" || select->cases[0]->body.size() != 1 ||
      select->cases[1]->body.size() != 1 || !logicalIf ||
      !cursiv::ast_cast<ast::AssignStmt>(logicalIf->action) || !labelled ||
      labelled->endLabel != "010" || labelled->terminalLabel != "10" ||
      !labelled->endsWithContinue || labelled->body.size() != 1)
    return fail("construct/branch names, logical IF action and terminal label", source);

  // Neither END IF's name nor a unit's name may eat a statement on the
  // following physical line. One external subroutine precedes the main.
  const std::string following = "subroutine s\nif(.true.)then\nend if\ncontinue\nend subroutine\nstop\nend\n";
  const auto retained = language.parser->parse(following);
  if (!clean(retained, following) || !parents(language, retained)) return false;
  const auto* retainedModel = cursiv::ast_cast<ast::Model>(retained.value);
  const auto* first = retainedModel && retainedModel->units.size() == 2
      ? cursiv::ast_cast<ast::SubroutineUnit>(retainedModel->units[0]) : nullptr;
  const auto* second = retainedModel && retainedModel->units.size() == 2
      ? cursiv::ast_cast<ast::MainProgramUnit>(retainedModel->units[1]) : nullptr;
  if (!first || !second || !first->endName.empty() || !second->endName.empty() ||
      !first->endHasKind || second->endHasKind || first->body.size() != 2 ||
      !cursiv::ast_cast<ast::ContinueStmt>(first->body[1]) || second->body.size() != 1 ||
      !cursiv::ast_cast<ast::StopStmt>(second->body[0]))
    return fail("post-END statements retained instead of closing names", following);
  return true;
}
}  // namespace fortran_source
