
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "rcs/core/CoreServices.hpp"
#include "rcs/core/ast.hpp"
// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a File root; a
// dedicated check on full.rcs reads the fields back, since a converter that
// refuses (out of range, unparsable) would still leave recovered() false and
// the root type right.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "rcs: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<rcs::RcsCoreServices, rcs::RcsSharedCoreServices,
                              rcs::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "head;\naccess;\nsymbols;\nlocks user:1.1;\ndesc\n@@\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("head; access; symbols; desc @@");
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  namespace fs = std::filesystem;

  const auto language = cursiv::create_language<
      rcs::RcsCoreServices, rcs::RcsSharedCoreServices,
      rcs::install_core_module>();

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".rcs")
        continue;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file) return fail(path.filename().string() + " is not readable");
      const std::string text = read_file(path);

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");
      if (!result.diagnostics.empty())
        return fail(path.filename().string() + " left diagnostics");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<rcs::ast::File>)
        return fail(path.filename().string() + " built no file");
      ++count;
    }
  if (count == 0) return fail("no example documents found");

  // full.rcs exercises every field: read them back to prove the STRING
  // converter unescapes `@@` and the record shape holds together.
  const std::string text =
      read_file(fs::path(CURSIV_TEST_DIR) / "full.rcs");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto* const file = result.value_as<rcs::ast::File>();
  if (file == nullptr) return fail("full.rcs built no File");

  const rcs::ast::Admin* const admin = file->admin;
  if (admin == nullptr) return fail("full.rcs built no Admin");
  if (admin->head != "1.3") return fail("full.rcs: wrong head");
  if (!admin->branch.has_value() || *admin->branch != "1.1.1")
    return fail("full.rcs: wrong branch");
  if (admin->access.size() != 2 || admin->access[0] != "joe" ||
      admin->access[1] != "mary")
    return fail("full.rcs: wrong access list");
  if (admin->symbols.size() != 2 || admin->symbols[0]->name != "release-2_0" ||
      admin->symbols[0]->revision != "1.3" ||
      admin->symbols[1]->name != "release-1_0" ||
      admin->symbols[1]->revision != "1.1")
    return fail("full.rcs: wrong symbols");
  if (admin->locks.size() != 2) return fail("full.rcs: wrong locks");
  if (!admin->strict) return fail("full.rcs: strict not set");
  if (!admin->comment.has_value() || *admin->comment != "# ")
    return fail("full.rcs: wrong comment");
  if (!admin->expand.has_value() || *admin->expand != "o")
    return fail("full.rcs: wrong expand");
  if (admin->newphrases.size() != 2 ||
      admin->newphrases[0]->words.size() != 2 ||
      admin->newphrases[0]->words[0] != "vendor-tag" ||
      admin->newphrases[0]->words[1] != "SUN")
    return fail("full.rcs: wrong admin newphrases");

  if (file->deltas.size() != 3) return fail("full.rcs: wrong delta count");
  const rcs::ast::Delta& d13 = *file->deltas[0];
  if (d13.revision != "1.3" || d13.author != "joe" || d13.state != "Exp")
    return fail("full.rcs: wrong delta 1.3");
  if (!d13.branches.empty()) return fail("full.rcs: 1.3 should have no branches");
  if (!d13.next.has_value() || *d13.next != "1.2")
    return fail("full.rcs: wrong delta 1.3 next");
  const rcs::ast::Delta& d12 = *file->deltas[1];
  if (d12.branches.size() != 1 || d12.branches[0] != "1.1.1.1")
    return fail("full.rcs: wrong delta 1.2 branches");
  if (d12.newphrases.size() != 1 ||
      d12.newphrases[0]->words[0] != "merge-source")
    return fail("full.rcs: wrong delta 1.2 newphrase");
  const rcs::ast::Delta& d11 = *file->deltas[2];
  if (d11.next.has_value()) return fail("full.rcs: 1.1 should have no next");

  if (file->desc.empty()) return fail("full.rcs: empty desc");

  if (file->deltatexts.size() != 3)
    return fail("full.rcs: wrong deltatext count");
  const rcs::ast::DeltaText& t13 = *file->deltatexts[0];
  if (t13.log != "Fixed the off-by-one bug in the @ handler.")
    return fail("full.rcs: STRING escape `@@` not unescaped to `@`");
  if (t13.newphrases.size() != 1 ||
      t13.newphrases[0]->words[0] != "review-status")
    return fail("full.rcs: wrong deltatext newphrase");
  if (t13.text.find("return 0;") == std::string_view::npos)
    return fail("full.rcs: wrong deltatext text");

  std::printf("rcs: %d examples parse, full.rcs fields verified\n", count);
  return 0;
}
