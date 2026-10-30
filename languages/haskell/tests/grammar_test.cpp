// The smoke test of the language: every file in example/ parses clean, with no
// diagnostics, and builds a Module.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "haskell/core/CoreServices.hpp"
#include "haskell/core/ast.hpp"
#include "port_audit_cases.hpp"
#include "native_followup_cases.hpp"

namespace {

int fail(const std::string &path, const char *what) {
  std::fprintf(stderr, "haskell: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string &s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty())
      parts.push_back(item);
  return parts;
}

} // namespace

int main() {
  {
    const auto language = cursiv::create_language<haskell::HaskellCoreServices,
        haskell::HaskellSharedCoreServices, haskell::install_core_module>();
    if (!port_audit_cases::run(*language.language.parser) ||
        !haskell_native_followup::run(*language.language.parser)) return 1;
  }
  {
    const auto audit_language =
        cursiv::create_language<haskell::HaskellCoreServices,
                                haskell::HaskellSharedCoreServices,
                                haskell::install_core_module>();
    bool app = false, quote = false, binary = false;
    for (const char *source :
         {"f = let x = 1 in x\n", "g = h @Int 1\n", "q = [sql| select x |]\n",
          "app = f x y\n", "module Audit where x = 1; y = 2\n",
          "f = let x = 1 in x\ng = h @Int 1\nq = [sql| select x |]\n"}) {
      const auto result = audit_language.language.parser->parse(source);
      if (!result.accepted || result.cst().recovered() ||
          !result.diagnostics.empty() || !result.value) {
        std::fprintf(stderr, "haskell audit: clean parse expected for %s",
                     source);
        for (const auto &diagnostic : result.diagnostics)
          std::fprintf(stderr, "  kind=%u at %u..%u %s\n",
                       static_cast<unsigned>(diagnostic.kind), diagnostic.begin,
                       diagnostic.end, diagnostic.message.c_str());
        return 1;
      }
      for (const auto *node : result.nodes) {
        if (const auto *t = cursiv::ast_cast<haskell::ast::TypeApp>(node))
          app = app || (t->func && t->arg);
        if (const auto *q = cursiv::ast_cast<haskell::ast::QuasiQuote>(node))
          quote = quote || (q->quoter == "sql" && q->body == " select x ");
        if (const auto *a = cursiv::ast_cast<haskell::ast::App>(node)) {
          const auto *f = cursiv::ast_cast<haskell::ast::App>(a->func);
          if (f)
            binary = binary || (f->func && f->arg && a->arg);
        }
      }
    }
    if (!app || !quote || !binary)
      return 1;
  }

  const auto language =
      cursiv::create_language<haskell::HaskellCoreServices,
                              haskell::HaskellSharedCoreServices,
                              haskell::install_core_module>();

  for (const std::string &path : split(CURSIV_SAMPLES, ',')) {
    std::ifstream file(path);
    if (!file)
      return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the sample parses with diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<haskell::ast::Module>)
      return fail(path, "the parse built no Module");

    std::printf("haskell: %s parses\n", path.c_str());
  }

  return 0;
}
