// The smoke test of the language: every corpus document parses clean, the
// constructs the corpus never exercises parse clean too, and one document is
// read back field by field to catch a converter that silently mismaps its
// value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "vaxscan/core/CoreServices.hpp"
#include "vaxscan/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "vaxscan: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "caps.scn",   "comment.scn", "compress.scn", "convert.scn",
    "cref.scn",   "crlf.scn",    "crypt.scn",    "dcl.scn",
    "dif.scn",    "global.scn",  "hanoi.scn",    "macro.scn",
    "micro1.scn", "micro2.scn",  "mms.scn",      "record.scn",
    "replace.scn", "time.scn",   "traceback.scn", "trap.scn",
    "trees.scn",
};

const vaxscan::ast::Module* parse_module(const cursiv::CoreLanguage<
                                              vaxscan::VaxscanCoreServices,
                                              vaxscan::VaxscanSharedCoreServices>&
                                              language,
                                          const std::string& text,
                                          const char* what,
                                          cursiv::ParseResult& out) {
  out = language.language.parser->parse(text);
  if (out.cst().recovered()) {
    fail((std::string(what) + ": does not parse clean").c_str());
    return nullptr;
  }
  if (!out.diagnostics.empty()) {
    fail((std::string(what) + ": has diagnostics").c_str());
    return nullptr;
  }
  const cursiv::AstNode* const root = out.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<vaxscan::ast::Module>) {
    fail((std::string(what) + ": built no Module").c_str());
    return nullptr;
  }
  return static_cast<const vaxscan::ast::Module*>(root);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      vaxscan::VaxscanCoreServices, vaxscan::VaxscanSharedCoreServices,
      vaxscan::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());
    cursiv::ParseResult result;
    if (parse_module(language, text, name, result) == nullptr) return 1;
  }

  // features.scn: OVERLAY, every COLLATE form, REDEFINE, EXTERNAL/GLOBAL
  // constants, AUTOMATIC/GLOBAL storage attributes -- none in the corpus.
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty()) return fail("features.scn: empty or unreadable");
    cursiv::ParseResult result;
    if (parse_module(language, text, "features.scn", result) == nullptr)
      return 1;
  }

  // hanoi.scn, read back field by field.
  const std::string hanoiText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/hanoi.scn");
  cursiv::ParseResult hanoiResult;
  const vaxscan::ast::Module* const mod =
      parse_module(language, hanoiText, "hanoi.scn", hanoiResult);
  if (mod == nullptr) return 1;
  if (mod->name != "hanoi") return fail("hanoi.scn: wrong module name");
  if (mod->statements.size() != 3)
    return fail("hanoi.scn: expected a constant and two procedures");

  if (mod->statements[0]->type !=
      &cursiv::ast_type_tag<vaxscan::ast::ConstantDecl>)
    return fail("hanoi.scn: first statement is not a ConstantDecl");
  const auto& disks =
      *static_cast<const vaxscan::ast::ConstantDecl*>(mod->statements[0]);
  if (disks.name != "disks") return fail("hanoi.scn: wrong constant name");
  if (disks.value == nullptr ||
      disks.value->type != &cursiv::ast_type_tag<vaxscan::ast::StrLit>)
    return fail("hanoi.scn: constant value is not a StrLit");
  if (static_cast<const vaxscan::ast::StrLit*>(disks.value)->value != "SDI")
    return fail("hanoi.scn: wrong constant value");

  if (mod->statements[1]->type !=
      &cursiv::ast_type_tag<vaxscan::ast::ProcedureDecl>)
    return fail("hanoi.scn: second statement is not a ProcedureDecl");
  const auto& proc =
      *static_cast<const vaxscan::ast::ProcedureDecl*>(mod->statements[1]);
  if (proc.name != "hanoi") return fail("hanoi.scn: wrong procedure name");
  if (proc.params.size() != 4)
    return fail("hanoi.scn: expected four parameters");
  if (proc.params[0]->name != "number" ||
      proc.params[0]->passingMech != "value")
    return fail("hanoi.scn: wrong first parameter");
  if (proc.body.empty())
    return fail("hanoi.scn: procedure body is empty");

  std::puts("vaxscan: the corpus parses");
  return 0;
}
