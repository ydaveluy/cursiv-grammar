#include "code_audit.hpp"
// The grammar end to end: every document under `example/`, subdirectories
// included, parses clean -- no recovery, no diagnostics -- and builds a
// Document at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "capnproto/core/CoreServices.hpp"
#include "capnproto/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "capnproto: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if(!code_audit::run())return 1;
  const auto language = cursiv::create_language<
      capnproto::CapnprotoCoreServices, capnproto::CapnprotoSharedCoreServices,
      capnproto::install_core_module>();
  {
    const auto result = language.language.parser->parse(
        "@0x934efea7f017fff0; const big :UInt64 = 1844674407370955161512345; "
        "const empty :List(UInt8) = []; struct Foo {} const obj :Foo = ();");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    bool exact = false, emptyList = false, emptyStruct = false;
    for (const auto* node : result.nodes) {
      if (const auto* number = cursiv::ast_cast<capnproto::ast::IntegerLiteral>(node)) exact |= number->text == "1844674407370955161512345";
      if (const auto* list = cursiv::ast_cast<capnproto::ast::ConstList>(node)) emptyList |= list->items.empty();
      if (const auto* value = cursiv::ast_cast<capnproto::ast::ConstStruct>(node)) emptyStruct |= value->fields.empty();
    }
    if (!exact || !emptyList || !emptyStruct) return 1;
  }


  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".capnp")
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
      if (result.cst().recovered()) {
        failures += fail(entry.path(), "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(entry.path(), "the document carries diagnostics");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<capnproto::ast::Document>) {
        failures += fail(entry.path(), "the parse built no Document");
        continue;
      }
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "capnproto: no .capnp file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("capnproto: %d example documents parse clean\n", count);
  return 0;
}
