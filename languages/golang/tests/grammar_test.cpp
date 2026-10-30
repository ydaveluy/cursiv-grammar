// The smoke test of the language: every file of example/ parses the way its
// corpus says it should, and a handful of real field values come back out of
// one document's AST -- a converter that silently drops a value would still
// leave the tree shape and `recovered()` right.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "golang/core/CoreServices.hpp"
#include "golang/core/GolangGrammar.hpp"
#include "golang/core/ast.hpp"
#include "audit_regressions.hpp"
#include "port_audit_cases.hpp"
#include "header_regressions.hpp"

namespace {

int failures = 0;

void fail(const std::string& what) {
  std::fprintf(stderr, "golang: %s\n", what.c_str());
  ++failures;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// examples/4733.go has no `package` clause at all, which Go requires.
bool expects_error(const std::filesystem::path& path) {
  return path.filename() == "4733.go";
}

template <class T>
T* as(golang::ast::Item* item) {
  if (item == nullptr || item->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<T*>(item);
}

}  // namespace

int main() {
  for (std::string_view text : {"", "\"", R"("\")", R"("\u")",
                                R"("\u123")", R"('\U0001')", R"('\x0')"})
    if (golang::checked_literal(text))
      fail("incomplete rune/string conversion unexpectedly succeeded");
  {
    const auto audit_language = cursiv::create_language<
      golang::GolangCoreServices, golang::GolangSharedCoreServices,
      golang::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("package p\nvar x = 0x_FF\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      golang::GolangCoreServices, golang::GolangSharedCoreServices,
      golang::install_core_module>();
  if (!audit_regressions::run(*language.language.parser) ||
      !port_audit_cases::run(*language.language.parser)) return 1;
  if (!go_header_regressions::run(*language.language.parser))
    fail("header regressions failed");

  int total = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".go") continue;
    ++total;
    const std::string text = read_file(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const std::string name = entry.path().filename().string();

    if (expects_error(entry.path())) {
      if (result.diagnostics.empty() && !result.cst().recovered())
        fail(name + ": expected a diagnostic, the document parsed clean");
      continue;
    }

    if (!result.diagnostics.empty()) {
      go_header_regressions::diagnostics(name, text, result);
      fail(name + ": unexpected diagnostics (" +
           std::to_string(result.diagnostics.size()) + ")");
    }
    if (result.cst().recovered()) fail(name + ": the document did not parse clean");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<golang::ast::SourceFile>)
      fail(name + ": the parse built no source file");
  }
  if (total == 0) fail("no .go file found under " + std::string(CURSIV_EXAMPLE_DIR));
  std::printf("golang: %d example files checked\n", total);

  // Constructs the upstream corpus does not happen to exercise: generics and
  // their constraints, select, goto/labels, iota, method expressions and
  // values, all three range forms, local type declarations, and a few
  // multi-line layouts that automatic semicolon insertion must get right.
  int test_total = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".go") continue;
    ++test_total;
    const std::string text = read_file(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const std::string name = entry.path().filename().string();
    if (!result.diagnostics.empty()) {
      go_header_regressions::diagnostics(name, text, result);
      fail(name + ": unexpected diagnostics (" +
           std::to_string(result.diagnostics.size()) + ")");
    }
    if (result.cst().recovered()) fail(name + ": the document did not parse clean");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<golang::ast::SourceFile>)
      fail(name + ": the parse built no source file");
  }
  if (test_total == 0) fail("no .go file found under " + std::string(CURSIV_TEST_DIR));
  std::printf("golang: %d of my own test files checked\n", test_total);

  // -- Field readback, on example.go: the classic prime-sieve program. ----
  const std::string text = read_file(std::filesystem::path(CURSIV_EXAMPLE_DIR) /
                                     "example.go");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<golang::ast::SourceFile>) {
    fail("example.go: the parse built no source file");
  } else {
    const auto& file = *static_cast<const golang::ast::SourceFile*>(result.value);
    if (file.packageName != "main")
      fail("example.go: package name is not 'main'");
    if (file.imports.size() != 1 || file.imports[0]->path != "\"fmt\"")
      fail("example.go: the fmt import did not read back");

    const golang::ast::FuncDecl* sieve = nullptr;
    const golang::ast::FuncDecl* generate = nullptr;
    for (golang::ast::Item* decl : file.decls) {
      if (auto* f = as<golang::ast::FuncDecl>(decl)) {
        if (f->name == "sieve") sieve = f;
        if (f->name == "generate") generate = f;
      }
    }
    if (generate == nullptr || generate->sig == nullptr ||
        generate->sig->params.size() != 1)
      fail("example.go: generate(ch chan<- int) did not read back");
    else {
      const golang::ast::Param& p = *generate->sig->params[0];
      if (p.names.size() != 1 || p.names[0] != "ch")
        fail("example.go: generate's parameter name did not read back");
      const auto* ch = p.type != nullptr &&
                                p.type->type == &cursiv::ast_type_tag<
                                                    golang::ast::ChannelType>
                            ? static_cast<const golang::ast::ChannelType*>(p.type)
                            : nullptr;
      if (ch == nullptr || !ch->sendOnly || ch->recvOnly)
        fail("example.go: generate's `chan<- int` did not read back send-only");
    }

    if (sieve == nullptr || sieve->body == nullptr ||
        sieve->body->stmts.empty())
      fail("example.go: func sieve() did not read back");
    else {
      const auto* first =
          as<golang::ast::ShortVarDecl>(sieve->body->stmts[0]);
      if (first == nullptr || first->names.size() != 1 ||
          first->names[0] != "ch" || first->values.size() != 1)
        fail("example.go: sieve's `ch := make(chan int)` did not read back");
      else {
        const auto* call = cursiv::ast_cast<golang::ast::CallExpr>(first->values[0]);
        const auto* name = call ? cursiv::ast_cast<golang::ast::Ident>(call->callee) : nullptr;
        const auto* type = call && call->args.size() == 1
            ? cursiv::ast_cast<golang::ast::TypeExpr>(call->args[0]) : nullptr;
        if (!name || name->name != "make" || !type ||
            !cursiv::ast_cast<golang::ast::ChannelType>(type->type))
          fail("example.go: make(chan int) lost its callee or type argument");
      }
    }
  }

  if (failures != 0) {
    std::fprintf(stderr, "golang: %d failure(s)\n", failures);
    return 1;
  }
  std::puts("golang: all examples parse as expected");
  return 0;
}
