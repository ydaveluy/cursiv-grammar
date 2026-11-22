// The grammar's end to end check: every document under example/ parses
// clean, with no recovery, and builds a Document root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "bencoding/CoreServices.hpp"
#include "bencoding/ast.hpp"
#include "bencoding/generated/Bencoding/Module.hpp"
#include "cursiv/parser/Parser.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "cursiv/syntax-tree/AstNode.hpp"
#include "cursiv/ValidationHelper.hpp"
#include "cursiv/ValidationResult.hpp"
#include "code_audit.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "bencoding: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  {
    for (const std::string_view malformed : {"li1e", "d1:ae", "i-e", "4:abc", "999999999999999999999999:x"}) {
      const auto rejected =
          bencoding::generated::Bencoding::createParser()->parse(malformed);
      if (!rejected.cst().recovered() || rejected.diagnostics.empty())
        return fail("malformed framing was silently accepted");
    }
    // Deep collections use an explicit stack, with no syntax depth cutoff.
    std::string deep(2048, 'l');
    deep.append(2048, 'e');
    const auto nested =
        bencoding::generated::Bencoding::createParser()->parse(deep);
    if (!nested.accepted || nested.cst().recovered() ||
        !nested.diagnostics.empty())
      return fail("deep valid lists did not parse");
    cursiv::CancellationTokenSource cancellation;
    cancellation.request_stop();
    bool cancelled = false;
    try {
      (void)bencoding::generated::Bencoding::createParser()->parse(
          "0:", cancellation.get_token());
    } catch (const cursiv::OperationCancelled &) {
      cancelled = true;
    }
    if (!cancelled)
      return fail("parser ignored cancellation");
  }

  namespace fs = std::filesystem;

  const auto language =
      cursiv::createLanguage<bencoding::BencodingCoreServices,
                              bencoding::BencodingSharedCoreServices,
                              bencoding::install_core_module>();

  if (check_native_audit(language.language)) return 1;

  const auto invalid_integer =
      cursiv::test::validationHelper(language.language, "i-0e");
  if (invalid_integer.diagnostics.size() != 1 ||
      invalid_integer.diagnostics[0].range.begin != 1 ||
      invalid_integer.diagnostics[0].range.end != 3)
    return fail("negative-zero diagnostic did not target its original bytes");
  {
    const std::string source("l0:5:d\0:e\xff" "i42ee", 15);
    const auto parsed = language.language.parser->parse(source);
    const auto *doc = parsed.valueAs<bencoding::ast::Document>();
    if (!doc || !parsed.diagnostics.empty() || doc->values.size() != 1)
      return fail("binary list framing");
    const auto list = std::get_if<bencoding::ast::List *>(&doc->values[0]);
    if (!list || (*list)->items.size() != 3) return fail("binary list items");
    const auto empty = std::get_if<bencoding::ast::ByteString *>(&(*list)->items[0]);
    const auto bytes = std::get_if<bencoding::ast::ByteString *>(&(*list)->items[1]);
    const auto integer = std::get_if<bencoding::ast::Integer *>(&(*list)->items[2]);
    if (!empty || !(*empty)->value().empty() || !bytes || (*bytes)->length() != "5" ||
        (*bytes)->value() != std::string_view("d\0:e\xff", 5) || !integer || (*integer)->text != "42")
      return fail("byte strings must stop at their declared length");
  }

  const std::string padded = std::string(4096, '0') + "3:abc";
  const auto paddedResult = cursiv::test::validationHelper(language.language, padded);
  if (paddedResult.diagnostics.size() != 1 || paddedResult.diagnostics[0].code != "bencoding.string-length" ||
      paddedResult.diagnostics[0].range.end != 4097)
    return fail("leading zeros must preserve the complete count in the diagnostic");

  const auto duplicates = cursiv::test::validationHelper(language.language, "d1:ai0e1:ai1e1:ai2ee");
  if (duplicates.diagnostics.size() != 3 ||
      duplicates.diagnostics[0].code != "bencoding.duplicate-key" ||
      duplicates.diagnostics[1].code != "bencoding.duplicate-key" ||
      duplicates.diagnostics[2].code != "bencoding.duplicate-key")
    return fail("each repeated key must be diagnosed once");
  const auto unordered = cursiv::test::validationHelper(language.language, "d1:zi0e2:aai1ee");
  if (unordered.diagnostics.size() != 1 || unordered.diagnostics[0].code != "bencoding.key-order")
    return fail("key ordering must compare payload bytes");

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
          root->type != &cursiv::kAstTypeTag<bencoding::ast::Document>)
        return fail(path.filename().string() + " built no document");
      ++count;
    }
  if (count == 0)
    return fail("no example documents found");

  std::printf("bencoding: %d examples parse\n", count);
  return 0;
}
