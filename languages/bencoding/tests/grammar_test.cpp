// The grammar's end to end check: every document under example/ parses
// clean, with no recovery, and builds a Document root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "bencoding/core/CoreServices.hpp"
#include "bencoding/core/ast.hpp"
#include "bencoding/generated/Bencoding/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "code_audit.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "bencoding: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  {
    for (const std::string_view malformed : {"li1e", "d1:ae", "i-e"}) {
      const auto rejected =
          bencoding::generated::Bencoding::create_parser()->parse(malformed);
      if (!rejected.cst().recovered() || rejected.diagnostics.empty())
        return fail("malformed framing was silently accepted");
    }
    // Deep collections use an explicit stack, with no syntax depth cutoff.
    std::string deep(2048, 'l');
    deep.append(2048, 'e');
    const auto nested =
        bencoding::generated::Bencoding::create_parser()->parse(deep);
    if (!nested.accepted || nested.cst().recovered() ||
        !nested.diagnostics.empty())
      return fail("deep valid lists did not parse");
    cursiv::CancellationTokenSource cancellation;
    cancellation.request_stop();
    bool cancelled = false;
    try {
      (void)bencoding::generated::Bencoding::create_parser()->parse(
          "0:", cancellation.get_token());
    } catch (const cursiv::OperationCancelled &) {
      cancelled = true;
    }
    if (!cancelled)
      return fail("parser ignored cancellation");
  }

  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<bencoding::BencodingCoreServices,
                              bencoding::BencodingSharedCoreServices,
                              bencoding::install_core_module>();

  if (check_native_audit(language.language)) return 1;

  const auto invalid_integer =
      cursiv::test::validationHelper(language.language, "i-0e");
  if (invalid_integer.diagnostics.size() != 1 ||
      invalid_integer.diagnostics[0].range.begin != 1 ||
      invalid_integer.diagnostics[0].range.end != 3)
    return fail("negative-zero diagnostic did not target its original bytes");
  for (const char *const source : {"4:abc", "999999999999999999999999:x"}) {
    const auto mismatch = cursiv::test::validationHelper(language.language, source);
    if (mismatch.diagnostics.size() != 1 ||
        mismatch.diagnostics[0].code != "bencoding.string-size")
      return fail("a byte count that differs from the payload went unreported");
  }

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file())
        continue;
      const fs::path path = entry.path();
      if (path.extension() != ".torrent")
        continue;

      std::ifstream file(path, std::ios::binary);
      if (!file)
        return fail(path.filename().string() + " is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<bencoding::ast::Document>)
        return fail(path.filename().string() + " built no document");
      ++count;
    }
  if (count == 0)
    return fail("no example documents found");

  std::printf("bencoding: %d examples parse\n", count);
  return 0;
}
