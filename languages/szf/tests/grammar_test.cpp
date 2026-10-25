// The smoke test of the language: build the container the way a workspace
// does, parse every file of example/, and check each tree.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "szf/core/CoreServices.hpp"
#include "szf/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "szf: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  namespace fs = std::filesystem;

  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  if (files.empty()) return fail("no example to parse");

  const auto language =
      cursiv::create_language<szf::SzfCoreServices, szf::SzfSharedCoreServices,
                              szf::install_core_module>();

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": the example is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path.string() + ": does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<szf::ast::File>)
      return fail(path.string() + ": the parse built no File");
  }

  {
    const std::string source =
        "#define $DIR my\n#include \"$DIR/part.sfz\"\n"
        "<control> default_path=My samples/\n"
        "<region> sample=Kick big.wav key=-12 gain=-1.25 // trailing comment\n"
        "<region> sample=*sine\n"
        "<sample> base64data=ab==\n//payload\n<effect> type=eq";
    const auto result = language.language.parser->parse(source);
    const auto* file = cursiv::ast_cast<szf::ast::File>(result.value);
    if (!result.accepted || result.cst().recovered() || !file ||
        file->elements.size() != 14 ||
        result.nodes.size() != file->elements.size() + 1)
      return fail("flat headers, directives and opcodes");
    for (const auto* element : file->elements)
      if (element->parent != file) return fail("element parent");
    const auto* definition =
        cursiv::ast_cast<szf::ast::Definition>(file->elements[0]);
    const auto* include =
        cursiv::ast_cast<szf::ast::Include>(file->elements[1]);
    const auto* sample = cursiv::ast_cast<szf::ast::Opcode>(file->elements[5]);
    const auto* key = cursiv::ast_cast<szf::ast::Opcode>(file->elements[6]);
    const auto* payload =
        cursiv::ast_cast<szf::ast::Opcode>(file->elements[11]);
    if (!definition || definition->name != "DIR" || definition->value != "my" ||
        !include || include->path != "$DIR/part.sfz" || !sample ||
        sample->key != "sample" || sample->value != "Kick big.wav" || !key ||
        key->key != "key" || key->value != "-12" || !payload ||
        payload->key != "base64data" || payload->value != "ab==\n//payload\n")
      return fail("raw opcode, definition, include or base64 value");
  }
  {
    const std::string source =
        "#define $T base64data\n<sample> $T=ab==\n//notcomment\n"
        "<region> sample=*sine";
    const auto result = language.language.parser->parse(source);
    bool preserved = false;
    for (cursiv::NodeId id = 0; id < result.cst().size(); ++id)
      preserved |=
          result.cst()[id].hidden && result.cst().text(id) == "//notcomment";
    if (!result.accepted || result.cst().recovered() || !preserved)
      return fail("macro-sensitive source comment remains in CST");
  }
  for (const auto& [source, code] :
       std::vector<std::pair<std::string, std::string>>{
           {"<bad name> key=1", "sfz.header"},
           {"#unknown value", "sfz.directive"},
           {"key=1", "sfz.orphan-opcode"}}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:sfz", "szf", 0, source));
    document.language = &language.language;
    document.parseResult =
        language.language.parser->parse(document.textDocument().getText());
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    if (!document.parseResult.accepted ||
        document.parseResult.cst().recovered() || diagnostics.size() != 1 ||
        !diagnostics[0].code || diagnostics[0].code->string() != code ||
        diagnostics[0].range.begin > diagnostics[0].range.end ||
        diagnostics[0].range.end > source.size())
      return fail("runtime diagnostic " + code);
  }
  std::puts("szf: every example parses");
  return 0;
}
