// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "memcachedprotocol/core/CoreServices.hpp"
#include "memcachedprotocol/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

#include "native_cases.hpp"

namespace {

int fail(const std::filesystem::path &file, const char *what) {
  std::fprintf(stderr, "memcachedprotocol: %s: %s\n", file.string().c_str(),
               what);
  return 1;
}

} // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
        memcachedprotocol::MemcachedProtocolCoreServices,
        memcachedprotocol::MemcachedProtocolSharedCoreServices,
        memcachedprotocol::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("get a\r\nversion\r\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto *commands =
        audit_result.value_as<memcachedprotocol::ast::CommandLine>();
    if (!commands || commands->commands.size() != 2)
      return 1;
    const auto *get =
        cursiv::ast_cast<memcachedprotocol::ast::RetrievalCommand>(
            commands->commands[0]);
    if (!get || get->keys.size() != 1 || get->keys[0] != "a" ||
        !cursiv::ast_cast<memcachedprotocol::ast::VersionCommand>(
            commands->commands[1]))
      return 1;
  }

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      memcachedprotocol::MemcachedProtocolCoreServices,
      memcachedprotocol::MemcachedProtocolSharedCoreServices,
      memcachedprotocol::install_core_module>();

  if (!audit_regressions::run(language.language, *language.shared)) return 1;

  int total = 0;
  int failures = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() ||
          entry.path().extension() != ".memcachedprotocol")
        continue;
      const std::filesystem::path &file = entry.path();
      ++total;

      std::ifstream in(file);
      if (!in) {
        failures += fail(file, "the document is not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      // `recovered()` is set when the parser had to repair the input to keep
      // going: a clean document leaves it false.
      if (result.cst().recovered()) {
        failures += fail(file, "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(file, "the document carries a diagnostic");
        continue;
      }

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type !=
              &cursiv::ast_type_tag<memcachedprotocol::ast::CommandLine>)
        failures += fail(file, "the parse built no CommandLine");
    }
  }

  {
    const auto result = language.language.parser->parse(
        "slabs reassign -1 2\nlru_crawler metadump all\nlru mode segmented\n"
        "watch fetchers mutations\ncache_memlimit 64 noreply\nshutdown graceful\n"
        "version ignored arguments\nquit ignored\n");
    const auto* root = result.value_as<memcachedprotocol::ast::CommandLine>();
    const auto* admin = root && root->commands.size() == 8
        ? cursiv::ast_cast<memcachedprotocol::ast::AdministrationCommand>(root->commands[0]) : nullptr;
    const auto* version = root && root->commands.size() == 8
        ? cursiv::ast_cast<memcachedprotocol::ast::VersionCommand>(root->commands[6]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !admin || admin->operation != "slabs" || admin->arguments.size() != 3 ||
        admin->arguments[1] != "-1" || !version || version->arguments.size() != 2)
      return fail("native-followup", "native administrative/ignored-argument AST");
  }
  for (const auto source : {"get-key\n", "version-extra\n"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-followup", "a command prefix must occupy its whole native token");
  }
  for (const auto source : {"set key 0-1 2\n", "incr key +-1\n", "stats detail invalid\n",
                            "stats cachedump 1\n", "shutdown invalid\n", "watch invalid\n",
                            "slabs reassign 1\n", "lru mode invalid\n"}) {
    const auto validated = cursiv::test::validationHelper(language.language, source);
    if (validated.diagnostics.empty())
      return fail("native-followup", "native malformed command needs a diagnostic");
  }
  for (const auto source : {"incr key -18446744073709551615\n", "touch key \t+30\n",
                            "incr key 12\tignored\n", "cache_memlimit 64 noreply\n",
                            "slabs automove ratio 0.8\n", "lru_crawler enable\n"}) {
    const auto validated = cursiv::test::validationHelper(language.language, source);
    if (!validated.diagnostics.empty())
      return fail("native-followup", "native numeric/admin spelling rejected");
  }
  if (total == 0)
    return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0)
    return 1;

  std::printf("memcachedprotocol: %d example(s) parse\n", total);
  return 0;
}
