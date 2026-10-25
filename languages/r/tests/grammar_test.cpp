
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "r/core/CoreServices.hpp"
#include "r/core/ast.hpp"
// The smoke test of the language: parse every file under example/, the way a
// workspace would build one, and read a few real field values back out of
// one document's AST.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

#include "port_audit_cases.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "r: %s\n", what.c_str());
  return 1;
}

}  // namespace

namespace {

std::vector<std::filesystem::path> rFiles(const char* dir) {
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry : fs::directory_iterator(dir))
    if (entry.path().extension() == ".R") files.push_back(entry.path());
  return files;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<r::RCoreServices, r::RSharedCoreServices,
                              r::install_core_module>();
  {
    const auto r = language.language.parser->parse("Inf");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  namespace fs = std::filesystem;
  const std::vector<fs::path> files = rFiles(CURSIV_EXAMPLE_DIR);
  if (files.empty()) return fail("no example files found");
  // Constructs the upstream corpus itself does not exercise (exponent,
  // `$`/`@`, `~`, every assignment arrow, hex/float/complex literals, the
  // `...`/`.` placeholders...): own documents, kept out of example/ so the
  // campaign's count of the upstream corpus stays exact.
  const std::vector<fs::path> fixtures = rFiles(CURSIV_TEST_DIR);
  if (fixtures.empty()) return fail("no fixture files found");

  const auto language = cursiv::create_language<
      r::RCoreServices, r::RSharedCoreServices, r::install_core_module>();
  if (!port_audit::run(*language.language.parser))
    return fail("native syntax and AST regression");

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse reported diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<r::ast::Program>)
      return fail(path.string() + ": the parse built no program");

    if (path.filename() == "Complete.R") {
      const auto& program = *static_cast<const r::ast::Program*>(root);
      if (program.body.empty())
        return fail(path.string() + ": the program holds no statements");

      // Read real field values back: the first top-level statement is
      // `main <- function() {...}`, an assignment of a function definition.
      const r::ast::Expr* const first = program.body[0];
      if (first == nullptr ||
          first->type != &cursiv::ast_type_tag<r::ast::AssignExpr>)
        return fail(path.string() + ": the first statement is not an assignment");
      const auto& assign = *static_cast<const r::ast::AssignExpr*>(first);
      if (assign.op != "<-")
        return fail(path.string() + ": the assignment operator was misread");
      if (assign.target == nullptr ||
          assign.target->type != &cursiv::ast_type_tag<r::ast::Ident>)
        return fail(path.string() + ": the assignment target is not a name");
      const auto& target = *static_cast<const r::ast::Ident*>(assign.target);
      if (target.name != "main")
        return fail(path.string() + ": the assignment target was misread");
      if (assign.value == nullptr ||
          assign.value->type != &cursiv::ast_type_tag<r::ast::FunctionDef>)
        return fail(path.string() + ": the assigned value is not a function");
    }
  }

  for (const fs::path& path : fixtures) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse reported diagnostics");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<r::ast::Program>)
      return fail(path.string() + ": the parse built no program");
  }

  // `?Syntax`'s three assignment tiers, tightest to loosest: `->`/`->>`
  // (left to right), `<-`/`<<-`/`:=` (right to left), `=` (right to left).
  // `a -> b <- c` reads `(a -> b) <- c`: `->` binds tighter, so it groups
  // its own operands before the looser `<-` sees them.
  {
    const std::string text = "a -> b <- c\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("precedence probe: does not parse clean");
    const auto& program = *static_cast<const r::ast::Program*>(result.value);
    if (program.body.empty() ||
        program.body[0]->type != &cursiv::ast_type_tag<r::ast::AssignExpr>)
      return fail("precedence probe: not an assignment");
    const auto& root = *static_cast<const r::ast::AssignExpr*>(program.body[0]);
    if (root.op != "<-") return fail("precedence probe: <- is not the root");
    if (root.target == nullptr ||
        root.target->type != &cursiv::ast_type_tag<r::ast::AssignExpr>)
      return fail("precedence probe: -> does not nest under <-");
    if (static_cast<const r::ast::AssignExpr*>(root.target)->op != "->")
      return fail("precedence probe: the nested operator is not ->");
  }

  // `f()` passes no argument (`gram.y`, `xxfuncall`); an omitted slot needs
  // a comma beside it, so `f(a,)` and `f(,)` hold two.
  {
    const auto argCount = [&](const std::string& text) -> int {
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered() || !result.diagnostics.empty()) return -1;
      const auto& program = *static_cast<const r::ast::Program*>(result.value);
      if (program.body.empty() ||
          program.body[0]->type != &cursiv::ast_type_tag<r::ast::CallExpr>)
        return -1;
      return static_cast<int>(
          static_cast<const r::ast::CallExpr*>(program.body[0])->args.size());
    };
    if (argCount("f()\n") != 0) return fail("call probe: f() holds an argument");
    if (argCount("f(a)\n") != 1) return fail("call probe: f(a) is not one");
    if (argCount("f(a,)\n") != 2) return fail("call probe: f(a,) is not two");
    if (argCount("f(,)\n") != 2) return fail("call probe: f(,) is not two");
  }

  std::printf("r: %zu example(s), %zu fixture(s) parse clean\n", files.size(),
              fixtures.size());
  return 0;
}
