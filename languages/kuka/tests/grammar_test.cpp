// The grammar's smoke test: every document under example/ (the upstream
// corpus) parses clean, so do the documents of my own under tests/ that
// exercise constructs the one-file corpus never reaches, and a few real
// field values read back correctly -- a converter that silently gave the
// wrong value would still pass a structural-only check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "kuka/core/CoreServices.hpp"
#include "kuka/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "kuka: %s\n", what.c_str());
  return 1;
}

std::string read(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

int check_clean(const cursiv::Parser& parser, const std::filesystem::path& path,
                std::string* text_out) {
  const std::string text = read(path);
  const cursiv::ParseResult result = parser.parse(text);
  if (result.cst().recovered())
    return fail(path.filename().string() + " does not parse clean");
  if (!result.diagnostics.empty())
    return fail(path.filename().string() + " carries a diagnostic");
  if (result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<kuka::ast::Module>)
    return fail(path.filename().string() + " built no module");
  if (text_out != nullptr) *text_out = text;
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      kuka::KukaCoreServices, kuka::KukaSharedCoreServices,
      kuka::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  int count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".kuka")
      continue;
    if (const int rc = check_clean(parser, entry.path(), nullptr); rc != 0)
      return rc;
    ++count;
  }
  if (count == 0) return fail("no example files found");

  for (const char* name : {"datamodule.kuka", "motions.kuka",
                           "control_flow.kuka", "interrupts.kuka",
                           "functions.kuka"}) {
    if (const int rc = check_clean(
            parser, std::filesystem::path(CURSIV_TEST_DIR) / name, nullptr);
        rc != 0)
      return rc;
  }

  // declarationsText kept alive in this scope: every string_view in the tree
  // built from it points into this buffer.
  std::string declarationsText;
  if (const int rc = check_clean(parser,
                                 std::filesystem::path(CURSIV_TEST_DIR) /
                                     "declarations.kuka",
                                 &declarationsText);
      rc != 0)
    return rc;

  // Read real values back out of it.
  const cursiv::ParseResult result = parser.parse(declarationsText);
  const auto* module = as<kuka::ast::Module>(result.value);
  if (module == nullptr) return fail("declarations.kuka built no module");
  if (module->data != nullptr) return fail("declarations.kuka built a data module");
  if (module->routines == nullptr)
    return fail("declarations.kuka built no routine module");

  const kuka::ast::Routine* routine = module->routines->main;
  if (routine == nullptr) return fail("declarations.kuka has no main routine");
  if (routine->name != "decls") return fail("wrong routine name");
  if (routine->isFunction) return fail("decls should be a procedure");
  if (routine->data.size() != 12)
    return fail("decls' data section has the wrong item count");
  if (routine->body.size() != 8)
    return fail("decls' body has the wrong statement count");

  const auto assign_value = [&](std::size_t i) -> const kuka::ast::Expr* {
    const auto* stmt = as<kuka::ast::ExprStmt>(routine->body[i]);
    if (stmt == nullptr) return nullptr;
    const auto* assign = as<kuka::ast::Assign>(stmt->value);
    if (assign == nullptr) return nullptr;
    return assign->value;
  };

  const auto* first = as<kuka::ast::ExprStmt>(routine->body[0]);
  if (first == nullptr) return fail("body[0] is not an expression statement");
  const auto* firstAssign = as<kuka::ast::Assign>(first->value);
  if (firstAssign == nullptr) return fail("body[0] is not an assignment");
  const auto* target = as<kuka::ast::VarRef>(firstAssign->target);
  if (target == nullptr || target->path.size() != 1 ||
      target->path[0]->id != "i")
    return fail("body[0]'s target is not the name 'i'");

  const auto* hexLit = as<kuka::ast::IntLit>(assign_value(5));
  if (hexLit == nullptr || hexLit->value != 31)
    return fail("'H1F' did not decode to 31");

  const auto* binLit = as<kuka::ast::IntLit>(assign_value(6));
  if (binLit == nullptr || binLit->value != 5)
    return fail("'B101' did not decode to 5");

  const auto* escLit = as<kuka::ast::CharLit>(assign_value(7));
  if (escLit == nullptr || escLit->value != "\n")
    return fail("'\\n' did not decode to a newline byte");

  // A motion target climbs only its own operator (`:`) bare, but a
  // parenthesized operand still reaches the full expression grammar -- and a
  // bare `+` at that position is refused.
  const auto motion_target = [&](const std::string& text) -> const kuka::ast::Expr* {
    const cursiv::ParseResult r = parser.parse(text);
    if (r.cst().recovered() || !r.diagnostics.empty()) return nullptr;
    const auto* mod = as<kuka::ast::Module>(r.value);
    if (mod == nullptr || mod->routines == nullptr) return nullptr;
    const auto* motion = as<kuka::ast::MotionStmt>(mod->routines->main->body[0]);
    return motion != nullptr ? motion->target : nullptr;
  };

  const auto* geomTarget =
      as<kuka::ast::BinaryExpr>(motion_target("DEF t()\nDECL INT a\nDECL INT b\n"
                                              "PTP a:b\nEND\n"));
  if (geomTarget == nullptr || geomTarget->op != kuka::ast::Op::geom)
    return fail("'PTP a:b' did not build a ':' target");

  const auto* parenTarget =
      as<kuka::ast::BinaryExpr>(motion_target("DEF t()\nDECL INT a\nDECL INT b\n"
                                              "PTP (a + b)\nEND\n"));
  if (parenTarget == nullptr || parenTarget->op != kuka::ast::Op::add)
    return fail("'PTP (a + b)' did not build a '+' inside the parens");

  {
    const cursiv::ParseResult r =
        parser.parse("DEF t()\nDECL INT a\nDECL INT b\nPTP a + b\nEND\n");
    if (!r.cst().recovered() && r.diagnostics.empty())
      return fail("'PTP a + b' should not parse clean (bare '+' at a motion "
                 "target)");
  }

  std::printf("kuka: %d examples parse clean\n", count);
  return 0;
}
