#include "code_audit.hpp"
// The smoke test of the language: every corpus document parses clean, a
// document of ours exercises constructs the corpus never uses, and one
// corpus document is read back field by field to catch a converter that
// silently mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "langium/core/CoreServices.hpp"
#include "langium/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "langium: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "sql.langium",
    "langium-grammar.langium",
};

}  // namespace

int main() {
  if (!code_audit::run()) return 1;
  {
    const auto audit_language =
        cursiv::create_language<langium::LangiumCoreServices,
                                langium::LangiumSharedCoreServices,
                                langium::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "grammar Test\nentry Model: =>\"a\" ->\"b\";\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool full = false, first = false;
    for (const auto* node : audit_result.nodes)
      if (const auto* k = cursiv::ast_cast<langium::ast::Keyword>(node)) {
        full = full || k->predicate == "=>";
        first = first || k->predicate == "->";
      }
    if (!full || !first) return 1;
  }

  const auto language =
      cursiv::create_language<langium::LangiumCoreServices,
                              langium::LangiumSharedCoreServices,
                              langium::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<langium::ast::Grammar>)
      return fail((std::string(name) + ": built no Grammar").c_str());

    const auto& grammar = *static_cast<const langium::ast::Grammar*>(root);
    if (grammar.rules.empty() && grammar.interfaces.empty() &&
        grammar.types.empty())
      return fail((std::string(name) + ": Grammar holds no member").c_str());
  }

  // features.langium covers constructs the two corpus documents never use:
  // interface, import, the grammar/rule 'hidden(...)' clause, '&', named
  // arguments, predicated forms, and the terminal-rule EBNF shapes.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/features.langium");
    if (text.empty()) return fail("features.langium: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("features.langium: does not parse clean");
    if (!result.diagnostics.empty())
      return fail("features.langium: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<langium::ast::Grammar>)
      return fail("features.langium: built no Grammar");
  }

  // sql.langium, read back field by field.
  const std::string sqlText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/sql.langium");
  const cursiv::ParseResult result = language.language.parser->parse(sqlText);
  const auto& sql = *static_cast<const langium::ast::Grammar*>(result.value);
  if (sql.name != "Sql") return fail("sql.langium: wrong grammar name");

  if (sql.rules.empty()) return fail("sql.langium: no rules");
  const auto* firstRule =
      cursiv::ast_cast<langium::ast::ParserRule>(sql.rules[0]);
  if (firstRule == nullptr)
    return fail("sql.langium: first rule is not a ParserRule");
  const langium::ast::ParserRule& sqlFile = *firstRule;
  if (sqlFile.name != "SqlFile")
    return fail("sql.langium: wrong first rule name");
  if (!sqlFile.entry)
    return fail("sql.langium: SqlFile should be the entry rule");

  bool foundCatalog = false;
  for (const auto& rule : sql.rules) {
    const auto* pr = cursiv::ast_cast<langium::ast::ParserRule>(rule);
    if (pr != nullptr && pr->name == "CatalogDefinition") {
      foundCatalog = true;
      break;
    }
  }
  if (!foundCatalog) return fail("sql.langium: CatalogDefinition rule missing");

  if (sql.types.empty()) return fail("sql.langium: no type aliases");
  if (sql.types[0]->name != "Definition")
    return fail("sql.langium: wrong first type alias name");

  // A converter readback the two corpus documents never exercise: NUMBER,
  // and a default-value BooleanLiteral written `?=`-style on an interface
  // attribute (`isOptional`).
  {
    const std::string text =
        "grammar T\ninterface X {\n a: number = 3.14\n b?: string\n}\n"
        "entry R:\n 'x'\n;\n";
    const cursiv::ParseResult r = language.language.parser->parse(text);
    if (r.cst().recovered() || !r.diagnostics.empty())
      return fail("inline interface: does not parse clean");
    const auto& g = *static_cast<const langium::ast::Grammar*>(r.value);
    if (g.interfaces.size() != 1) return fail("inline interface: wrong count");
    const langium::ast::Interface& iface = *g.interfaces[0];
    if (iface.attributes.size() != 2)
      return fail("inline interface: wrong attribute count");
    const langium::ast::TypeAttribute& a = *iface.attributes[0];
    if (a.isOptional)
      return fail("inline interface: 'a' should not be optional");
    if (a.defaultValue == nullptr ||
        a.defaultValue->type !=
            &cursiv::ast_type_tag<langium::ast::NumberLiteral>)
      return fail("inline interface: 'a' default is not a NumberLiteral");
    const auto& num =
        *static_cast<const langium::ast::NumberLiteral*>(a.defaultValue);
    if (num.value != 3.14) return fail("inline interface: wrong NUMBER value");
    const langium::ast::TypeAttribute& b = *iface.attributes[1];
    if (!b.isOptional) return fail("inline interface: 'b' should be optional");
  }

  {
    auto guard_condition =
        [](const cursiv::ParseResult& r) -> const langium::ast::Condition* {
      if (r.cst().recovered() || !r.diagnostics.empty()) return nullptr;
      const auto* g = r.value_as<langium::ast::Grammar>();
      if (!g || g->rules.empty()) return nullptr;
      const auto* pr = cursiv::ast_cast<langium::ast::ParserRule>(g->rules[0]);
      const auto* group =
          pr ? cursiv::ast_cast<langium::ast::Group>(pr->definition) : nullptr;
      return group ? group->guardCondition : nullptr;
    };
    auto param_named = [](const langium::ast::Condition* c,
                          std::string_view name) {
      const auto* p = cursiv::ast_cast<langium::ast::ParameterReference>(c);
      return p != nullptr && p->parameter == name;
    };
    const std::string chainText = "entry Model: <a & b & c> 'x';";
    const auto chainResult = language.language.parser->parse(chainText);
    const auto* chain = cursiv::ast_cast<langium::ast::Conjunction>(
        guard_condition(chainResult));
    if (!chain || !param_named(chain->right, "c"))
      return fail("guard 'a & b & c': wrong top node");
    const auto* chainLeft =
        cursiv::ast_cast<langium::ast::Conjunction>(chain->left);
    if (!chainLeft || !param_named(chainLeft->left, "a") ||
        !param_named(chainLeft->right, "b"))
      return fail("guard 'a & b & c': middle operand lost");
    const std::string mixText = "entry Model: <a & b | c> 'x';";
    const auto mixResult = language.language.parser->parse(mixText);
    const auto* mix =
        cursiv::ast_cast<langium::ast::Disjunction>(guard_condition(mixResult));
    if (!mix || !param_named(mix->right, "c") ||
        !cursiv::ast_cast<langium::ast::Conjunction>(mix->left))
      return fail("guard 'a & b | c': wrong precedence");
  }

  std::puts("langium: the corpus parses");
  return 0;
}
