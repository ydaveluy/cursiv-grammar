#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "awk/core/CoreServices.hpp"
#include "awk/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "awk: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      awk::AwkCoreServices, awk::AwkSharedCoreServices,
      awk::install_core_module>();

  int failures = 0;
  int count = 0;
  const fs::path root = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  for (const fs::directory_entry& entry :
       fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".awk")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    const bool clean =
        result.accepted && !result.cst().recovered() && result.diagnostics.empty();

    if (!clean) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    const cursiv::AstNode* const rootNode = result.value;
    if (rootNode == nullptr ||
        rootNode->type != &cursiv::ast_type_tag<awk::ast::Program>) {
      failures += fail(entry.path(), "the parse built no Program");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "awk: no .awk file found under example/\n");
    return 1;
  }

  {
    const fs::path path = root / "count_words.awk";
    std::ifstream file(path);
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& program =
        *static_cast<const awk::ast::Program*>(result.value);
    auto is = [](const cursiv::AstNode* node, auto tagOf) {
      return node != nullptr && node->type == tagOf;
    };
    if (program.items.size() != 3) {
      failures += fail(path, "expected three top-level items");
    } else {
      const auto* begin = static_cast<const awk::ast::Rule*>(program.items[0]);
      const auto* main = static_cast<const awk::ast::Rule*>(program.items[1]);
      const auto* end = static_cast<const awk::ast::Rule*>(program.items[2]);
      if (!is(begin->pattern, &cursiv::ast_type_tag<awk::ast::BeginPattern>))
        failures += fail(path, "the first rule is not BEGIN");
      if (main->pattern != nullptr)
        failures += fail(path, "the second rule is not pattern-less");
      if (!is(end->pattern, &cursiv::ast_type_tag<awk::ast::EndPattern>))
        failures += fail(path, "the third rule is not END");
      if (main->action == nullptr || main->action->items.empty() ||
          !is(main->action->items[0], &cursiv::ast_type_tag<awk::ast::IfStmt>)) {
        failures += fail(path, "the main rule's first statement is not if");
      } else {
        const auto* ifStmt =
            static_cast<const awk::ast::IfStmt*>(main->action->items[0]);
        if (!is(ifStmt->cond, &cursiv::ast_type_tag<awk::ast::BinaryExpr>) ||
            static_cast<const awk::ast::BinaryExpr*>(ifStmt->cond)->op !=
                awk::ast::BinOp::Gt)
          failures += fail(path, "the if condition is not NF > 0");
      }
    }
  }

  if (failures > 0) return 1;

  std::printf("awk: %d example documents checked\n", count);
  return 0;
}
