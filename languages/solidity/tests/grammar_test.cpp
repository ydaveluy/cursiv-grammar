#include "native_followup.hpp"
#include "audit_regressions.hpp"
#include "code_audit.hpp"
// The smoke test of the language: parse every file under example/, the way
// a workspace would, and read a few real field values back out of one of
// them so a converter that silently misreads a value does not pass.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "solidity/core/CoreServices.hpp"
#include "solidity/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "solidity: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  if (!native_followup::run() || !audit_regressions::run()) return 1;
  if (!code_audit::run()) return 1;
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry : fs::directory_iterator(dir))
      if (entry.path().extension() == ".sol") files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<
      solidity::SolidityCoreServices, solidity::SoliditySharedCoreServices,
      solidity::install_core_module>();

  for (const fs::path& path : files) {
    const std::string text = read_file(path);
    if (text.empty()) return fail(path.string() + ": not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse leaves diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<solidity::ast::SourceUnit>)
      return fail(path.string() + ": the parse built no source unit");
  }

  // small_test.sol names one contract, one external function taking one
  // `address` parameter: read those fields back rather than trust the
  // structural check alone.
  const std::string small = read_file(fs::path(CURSIV_EXAMPLE_DIR) / "small_test.sol");
  const cursiv::ParseResult result = language.language.parser->parse(small);
  const auto& unit = *static_cast<const solidity::ast::SourceUnit*>(result.value);

  const solidity::ast::ContractDefinition* contract = nullptr;
  for (const solidity::ast::Item* const item : unit.items)
    if (item != nullptr &&
        item->type == &cursiv::ast_type_tag<solidity::ast::ContractDefinition>) {
      contract = static_cast<const solidity::ast::ContractDefinition*>(item);
      break;
    }
  if (contract == nullptr || contract->name != "Ballot")
    return fail("small_test.sol: contract name misread");

  const solidity::ast::FunctionDefinition* function = nullptr;
  for (const solidity::ast::Item* const item : contract->body)
    if (item != nullptr &&
        item->type == &cursiv::ast_type_tag<solidity::ast::FunctionDefinition>) {
      function = static_cast<const solidity::ast::FunctionDefinition*>(item);
      break;
    }
  if (function == nullptr || function->name != "x")
    return fail("small_test.sol: function name misread");
  if (function->parameters.size() != 1 || function->parameters[0] == nullptr ||
      function->parameters[0]->name != "to")
    return fail("small_test.sol: parameter name misread");
  const solidity::ast::TypeName* const paramType = function->parameters[0]->type;
  if (paramType == nullptr ||
      paramType->type != &cursiv::ast_type_tag<solidity::ast::ElementaryType>)
    return fail("small_test.sol: parameter type misread");
  const auto& elementary = *static_cast<const solidity::ast::ElementaryType*>(paramType);
  if (elementary.text != "address")
    return fail("small_test.sol: parameter type text misread");

  std::printf("solidity: %zu example(s) parse clean\n", files.size());
  return 0;
}
