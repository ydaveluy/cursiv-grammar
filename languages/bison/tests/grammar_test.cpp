// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of two documents' ASTs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "bison/core/CoreServices.hpp"
#include "bison/core/ast.hpp"
#include "code_audit.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "bison: %s\n", what.c_str());
  return 1;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const bison::ast::Production* find_production(const bison::ast::File& file,
                                              std::string_view name) {
  for (const bison::ast::GrammarItem& item : file.grammarSection)
    if (const auto* const* p = std::get_if<bison::ast::Production*>(&item))
      if (*p != nullptr && (*p)->name == name) return *p;
  return nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<bison::BisonCoreServices,
                                                bison::BisonSharedCoreServices,
                                                bison::install_core_module>();
  if (!code_audit::run(language)) return fail("native syntax and AST checks");
  {
    constexpr auto text = R"(%{ namespace example { %}
%%
a: 'a' { f({1}); } %?{1};
%%
}
)";
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("code islands");
    bool action = false, predicate = false;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<bison::ast::Action>(node)) {
        action |= value->code == " f({1}); ";
      } else if (const auto* value =
                     cursiv::ast_cast<bison::ast::Predicate>(node))
        predicate |= value->code == "1";
    if (!action || !predicate) return fail("nested or predicate code lost");
  }

  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".y")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path calc_doc, irc_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bison::ast::File>)
      return fail(doc.string() + ": the parse built no file");
    if (doc.filename() == "calc.y") calc_doc = doc;
    if (doc.filename() == "irc.y") irc_doc = doc;
  }
  if (calc_doc.empty() || irc_doc.empty())
    return fail("calc.y or irc.y was not found among the examples");

  // calc.y: a %token/%left/%right declaration, and a production with seven
  // alternatives whose last one is a parenthesised expression.
  {
    const std::string text = read_file(calc_doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& file = *static_cast<const bison::ast::File*>(result.value);

    bool found_num_token = false;
    for (const bison::ast::SectionItem& item : file.prologue)
      if (const auto* const* d = std::get_if<bison::ast::SymbolDecl*>(&item))
        if (*d != nullptr && (*d)->kind == "%token")
          for (const bison::ast::TokenGroup* const group : (*d)->groups)
            for (const bison::ast::TokenSymbol* const sym : group->symbols)
              if (sym->name == "NUM") found_num_token = true;
    if (!found_num_token) return fail("calc.y: %token NUM not found");

    const bison::ast::Production* const exp = find_production(file, "exp");
    if (exp == nullptr || exp->alternatives.size() != 8)
      return fail("calc.y: wrong number of alternatives for 'exp'");
  }

  // irc.y: a %union, and the 'tagged_message' production, whose first
  // alternative names a symbol with a bracketed named reference.
  {
    const std::string text = read_file(irc_doc);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& file = *static_cast<const bison::ast::File*>(result.value);

    bool found_union = false;
    for (const bison::ast::SectionItem& item : file.prologue)
      if (const auto* const* d = std::get_if<bison::ast::Directive*>(&item))
        if (*d != nullptr && (*d)->kind == "%union") found_union = true;
    if (!found_union) return fail("irc.y: %union not found");

    const bison::ast::Production* const tm =
        find_production(file, "tagged_message");
    if (tm == nullptr || tm->alternatives.empty())
      return fail("irc.y: 'tagged_message' production not found");
    bool found_named_ref = false;
    for (const bison::ast::RhsElement& el : tm->alternatives[0]->elements)
      if (const auto* const* s = std::get_if<bison::ast::SymbolRef*>(&el))
        if (*s != nullptr && (*s)->namedRef == "ts") found_named_ref = true;
    if (!found_named_ref)
      return fail("irc.y: named reference [ts] not found on 'tags'");
  }

  std::printf("bison: %zu documents parse clean\n", docs.size());
  return 0;
}
