#include "audit_regressions.hpp"
// The smoke test of the language: parse every document under tests/ (no
// upstream corpus exists for this language), and check the tree each one
// built.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "stacktrace/core/CoreServices.hpp"
#include "stacktrace/core/ast.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "stacktrace: %s: %s\n", file, what);
  return 1;
}

std::string read(const std::string& path) {
  std::ifstream in(path);
  return std::string((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      stacktrace::StacktraceCoreServices, stacktrace::StacktraceSharedCoreServices,
      stacktrace::install_core_module>();

  static constexpr const char* kExamples[] = {
      "simple.stacktrace",
      "nested.stacktrace",
      "multi_message.stacktrace",
  };

  int failures = 0;
  for (const char* name : kExamples) {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
    const std::string text = read(path);
    if (text.empty()) {
      failures += fail(name, "the document is not readable");
      continue;
    }

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(name, "the document does not parse");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<stacktrace::ast::StackTrace>) {
      failures += fail(name, "the parse built no StackTrace");
      continue;
    }

    const auto& trace = *static_cast<const stacktrace::ast::StackTrace*>(root);
    if (trace.messages.empty()) {
      failures += fail(name, "the trace holds no header line");
      continue;
    }
  }

  if (failures != 0) return 1;
  std::puts("stacktrace: every example parses");
  return 0;
}
