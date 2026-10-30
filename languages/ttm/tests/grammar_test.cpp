// The ttm conformance suite: every document under example/ must parse without
// recovery and root a Program.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ttm/core/CoreServices.hpp"
#include "ttm/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

bool check(const fs::path& file) {
  std::ifstream stream(file);
  if (!stream) {
    std::fprintf(stderr, "ttm: %s: not readable\n", file.string().c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(stream)),
                         std::istreambuf_iterator<char>());

  const auto language =
      cursiv::create_language<ttm::TtmCoreServices, ttm::TtmSharedCoreServices,
                              ttm::install_core_module>();

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "ttm: %s: does not parse clean\n",
                 file.string().c_str());
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<ttm::ast::Program>) {
    std::fprintf(stderr, "ttm: %s: did not build a Program\n",
                 file.string().c_str());
    return false;
  }
  return true;
}

}  // namespace

int main() {
  bool ok = true;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".ttm")
        continue;
      ++count;
      ok &= check(entry.path());
    }
  }

  {
    const auto language = cursiv::create_language<ttm::TtmCoreServices,
        ttm::TtmSharedCoreServices, ttm::install_core_module>();
    const std::string source = "before;> #<ps;;a#<ad;1;2>b;<##<ps;quoted>>;@;> after";
    const auto r = language.language.parser->parse(source);
    const auto *program = r.value_as<ttm::ast::Program>();
    const auto *held = program && program->parts.size() == 3
        ? std::get_if<ttm::ast::Call *>(&program->parts[1]) : nullptr;
    const auto *call = held ? *held : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        !call || !cursiv::ast_cast<ttm::ast::Active>(call) || !call->name ||
        call->name->parts.size() != 1 ||
        std::get<std::string_view>(call->name->parts[0]) != "ps" ||
        call->args.size() != 4 || !call->args[0]->parts.empty() ||
        call->args[1]->parts.size() != 3 ||
        !std::holds_alternative<ttm::ast::Call *>(call->args[1]->parts[1]) ||
        std::get<std::string_view>(call->args[2]->parts[0]) != "<##<ps;quoted>>" ||
        std::get<std::string_view>(call->args[3]->parts[0]) != "@;" ||
        call->args[1]->parent != call)
      return std::fputs("ttm: native source parts/empty argument/quoting AST lost\n", stderr), 1;
    for (const char *text : {"plain Unicode λ text", "#<ps;>", "#<ps;<>>",
                             "#<ps;<a;<b>;c>>", "#<ps;a@>b>", "#<ps;a@;b>",
                             "##<ps;a>", "a#b###<ps;c>"}) {
      const auto parsed = language.language.parser->parse(text);
      if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
        return std::fprintf(stderr, "ttm: native source did not parse: %s\n", text), 1;
    }
    for (const char *text : {"#<ps;", "#<ps;<a>", "<unbalanced", "#<ps;@"}) {
      const auto parsed = language.language.parser->parse(text);
      if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
        return std::fprintf(stderr, "ttm: unbalanced source accepted: %s\n", text), 1;
    }
  }
  if (count == 0) {
    std::fprintf(stderr, "ttm: no examples found under %s\n",
                 CURSIV_EXAMPLE_DIR);
    return 1;
  }
  if (!ok) return 1;

  std::printf("ttm: %d examples parse clean\n", count);
  return 0;
}
