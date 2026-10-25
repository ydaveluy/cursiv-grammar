// The smoke test of the language: every document under example/ parses clean
// and builds a File at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cmake/core/CoreServices.hpp"
#include "cmake/core/ast.hpp"

namespace {

void report(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "cmake: %s: %s\n", file.string().c_str(), what);
}

bool bracket_arguments(const cursiv::Parser& parser) {
  struct Case {
    std::string_view input, value;
  };
  for (const Case c : {
           Case{"message([[plain]])\n", "plain"},
           Case{"message([=[${var}]=])\n", "${var}"},
           Case{"message([==[body ]=] inside]==])\n", "body ]=] inside"},
       }) {
    const auto result = parser.parse(c.input);
    const auto* file = result.value_as<cmake::ast::File>();
    const auto* arg =
        file && file->commands.size() == 1 &&
                file->commands[0]->arguments.size() == 1
            ? cursiv::ast_cast<cmake::ast::BracketArgument>(
                  file->commands[0]->arguments[0])
            : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !arg || arg->text != c.value) {
      std::fputs("cmake: bracket argument lost its kind or contents\n", stderr);
      return false;
    }
  }

  // A malformed bracket opener can still be represented as unquoted text.
  const auto result = parser.parse("message([==[not closed]=])\n");
  const auto* file = result.value_as<cmake::ast::File>();
  if (!result.accepted || result.cst().recovered() ||
      !result.diagnostics.empty() || !file || file->commands.size() != 1 ||
      file->commands[0]->arguments.size() != 3)
    return false;
  const auto* first = cursiv::ast_cast<cmake::ast::UnquotedArgument>(
      file->commands[0]->arguments[0]);
  const auto* second = cursiv::ast_cast<cmake::ast::UnquotedArgument>(
      file->commands[0]->arguments[2]);
  if (!first || !second || first->text != "[" ||
      second->text != "closed]=]")
    return false;
  return true;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<cmake::CmakeCoreServices,
                                                cmake::CmakeSharedCoreServices,
                                                cmake::install_core_module>();
  if (!bracket_arguments(*language.language.parser)) return 1;

  int count = 0;
  bool ok = true;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cmake")
      continue;
    ++count;
    const std::filesystem::path path = entry.path();

    std::ifstream file(path);
    if (!file) {
      report(path, "not readable");
      ok = false;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      report(path, "did not parse clean");
      ok = false;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<cmake::ast::File>) {
      report(path, "the parse built no File");
      ok = false;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "cmake: no examples found\n");
    return 1;
  }
  if (!ok) return 1;

  std::printf("cmake: %d example(s) parse\n", count);
  return 0;
}
