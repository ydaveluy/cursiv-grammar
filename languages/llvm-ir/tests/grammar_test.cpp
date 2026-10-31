// The grammar end to end: every document in example/ parses clean (no
// recovery, no diagnostics), and a few real field values read back out of
// one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "llvmir/core/CoreServices.hpp"
#include "llvmir/core/ast.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = llvmir::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "llvmir: %s\n", what.c_str());
  return 1;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

}  // namespace

#include "code_audit.hpp"
#include "attribute_group_cases.hpp"

int main() {
  if (!code_audit::run()) return 1;
  {
    const auto audit_language =
        cursiv::create_language<llvmir::LlvmIrCoreServices,
                                llvmir::LlvmIrSharedCoreServices,
                                llvmir::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "define void @f(ptr %p) {\nentry:\n indirectbr ptr %p, [label %a, "
        "label %b, label %c]\na:\n ret void\nb:\n ret void\nc:\n ret "
        "void\n}\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language =
      cursiv::create_language<llvmir::LlvmIrCoreServices,
                              llvmir::LlvmIrSharedCoreServices,
                              llvmir::install_core_module>();
  if (!llvm_attribute_groups::run(language.language)) return 1;

  int clean = 0, total = 0;
  for (const auto& entry : fs::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (entry.path().extension() != ".ll") continue;
    ++total;
    const std::string text = read_file(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": did not parse clean");
    if (!result.diagnostics.empty())
      return fail(entry.path().string() + ": left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Module>)
      return fail(entry.path().string() + ": built no Module");
    ++clean;
  }
  if (total == 0) return fail("no example found");
  if (clean != total) return fail("not every example parsed clean");

  // Read real values back: a target triple, a global's name, and a
  // function's basic blocks. Kept in its own ParseResult, alive for as long
  // as these reads need the arena it owns.
  const std::string text = read_file(fs::path(CURSIV_EXAMPLE_DIR) / "test.ll");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto* const checked = static_cast<const ast::Module*>(result.value);

  bool sawTriple = false;
  const ast::GlobalVar* fmtstr = nullptr;
  const ast::FuncDef* mainFn = nullptr;
  for (const ast::TopLevelEntity* const entity : checked->entities) {
    if (entity->type == &cursiv::ast_type_tag<ast::TargetDef>) {
      const auto* const td = static_cast<const ast::TargetDef*>(entity);
      if (td->kind == "triple" && !td->text.empty()) sawTriple = true;
    }
    if (entity->type == &cursiv::ast_type_tag<ast::GlobalVar>) {
      const auto* const gv = static_cast<const ast::GlobalVar*>(entity);
      if (gv->name == "@.fmtstr") fmtstr = gv;
    }
    if (entity->type == &cursiv::ast_type_tag<ast::FuncDef>) {
      const auto* const fn = static_cast<const ast::FuncDef*>(entity);
      if (fn->header != nullptr && fn->header->name == "@main") mainFn = fn;
    }
  }
  if (!sawTriple) return fail("test.ll: target triple did not read back");
  if (fmtstr == nullptr || fmtstr->immutable != "constant")
    return fail("test.ll: @.fmtstr did not read back as constant");
  if (mainFn == nullptr) return fail("test.ll: @main was not found");
  if (mainFn->body == nullptr || mainFn->body->blocks.empty())
    return fail("test.ll: @main has no basic blocks");
  const ast::BasicBlock* const entryBlock = mainFn->body->blocks[0];
  if (entryBlock->terminator == nullptr)
    return fail("test.ll: @main's entry block has no terminator");

  std::puts("llvmir: every example parses clean");
  return 0;
}
