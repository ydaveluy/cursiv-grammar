#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "rcs/core/CoreServices.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

int main() {
  const auto language =
      cursiv::create_language<rcs::RcsCoreServices, rcs::RcsSharedCoreServices,
                              rcs::install_core_module>();
  unsigned equal = 0;
  for (const auto &c : nativeCases) {
    auto fail = [&](const char *message) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), message);
      return 1;
    };
    std::ifstream in(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                         std::string(c.file),
                     std::ios::binary);
    std::string source((std::istreambuf_iterator<char>(in)), {});
    const auto result = language.language.parser->parse(source);
    const auto *file = cursiv::ast_cast<rcs::ast::File>(result.value);
    if (!file || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      ++equal;
      if (file_view(*file).toJsonString({.pretty = false}) != c.native)
        return fail("native full repository fields");
    }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() &&
        (validation.diagnostics[0].code != c.error ||
         validation.diagnostics[0].range.begin >=
             validation.diagnostics[0].range.end ||
         validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    if (c.name.starts_with("unknown-")) {
      const rcs::ast::NewPhrase *phrase =
          c.name == "unknown-admin"   ? file->admin->newphrases[0]
          : c.name == "unknown-delta" ? file->deltas[0]->newphrases[0]
                                      : file->deltatexts[0]->newphrases[0];
      if (phrase->words.size() != 5 || phrase->words[2] != "1.2" ||
          phrase->words[3] != ":" ||
          phrase->words[1] != (c.name == "unknown-admin" ? "hello@world" : "x"))
        return fail("extension string, number and colon fields");
    }
    bool owned = file->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *file);
    if (!owned || seen.size() != result.nodes.size())
      return fail("ownership");
  }
  std::printf("RCS: %zu files, %u complete native repositories\n",
              std::size(nativeCases), equal);
  return equal == 28 ? 0 : 1;
}
