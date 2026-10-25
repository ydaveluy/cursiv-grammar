#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include "bibtex/core/CoreServices.hpp"
#include "bibtex/core/ast.hpp"
#include "bibtex/generated/Bibtex/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace ast = bibtex::ast;
int main() {
  auto language = cursiv::create_language<bibtex::BibtexCoreServices,
      bibtex::BibtexSharedCoreServices, bibtex::install_core_module>();
  const auto parse = [&](std::string_view source) {
    auto result = language.language.parser->parse(source);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    if (!result.value || result.cst().recovered()) std::exit(1);
    CHECK(result.cst().text() == source);
    std::unordered_set<const cursiv::AstNode*> nodes;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      CHECK(nodes.insert(&node).second);
      language.language.astReflection->visitMembers(
          node, [&](auto, auto, cursiv::AstNode& child) {
            CHECK(child.parent == &node &&
                  result.cst().contains(node.cstNodeId, child.cstNodeId));
            self(self, child);
          });
    };
    walk(walk, *result.value);
    CHECK(nodes.size() == result.nodes.size());
    return result;
  };
  for (std::string_view type : {"article", "book", "booklet", "inbook",
       "incollection", "inproceedings", "proceedings", "manual",
       "mastersthesis", "phdthesis", "misc", "techreport", "unpublished",
       "software", "data-set", "string_other", "commentary", "3d"}) {
    const auto source = "@ " + std::string(type) + " (key, title={T})";
    const auto result = parse(source);
    const auto* doc = result.value_as<ast::Document>();
    CHECK(doc && doc->declarations.size() == 1);
    if (!doc || doc->declarations.size() != 1) return 1;
    const auto* entry = cursiv::ast_cast<ast::Entry>(doc->declarations[0]);
    CHECK(entry && entry->type == type && entry->citekey == "key" &&
          entry->hasKeySeparator && entry->open == "(" && entry->close == ")");
    CHECK(cursiv::test::validationHelper(language.language, source)
              .diagnostics.empty());
  }
  constexpr std::string_view mixed = R"bib(ignored words
@ STRING {Prefix = "Hello", another = {world}}
@Software(KeY, TITLE = Prefix # { {nested}
text} # 999999999999999999999999999999, field:name = "100% @literal")
@preamble{Prefix # {end}}
@COMMENT(A (nested) comment)
)bib";
  const auto result = parse(mixed);
  const auto* doc = result.value_as<ast::Document>();
  CHECK(doc && doc->declarations.size() == 4);
  if (!doc || doc->declarations.size() != 4) return 1;
  const auto* strings = cursiv::ast_cast<ast::StringDef>(doc->declarations[0]);
  const auto* entry = cursiv::ast_cast<ast::Entry>(doc->declarations[1]);
  const auto* preamble = cursiv::ast_cast<ast::Preamble>(doc->declarations[2]);
  const auto* comment = cursiv::ast_cast<ast::Comment>(doc->declarations[3]);
  CHECK(strings && strings->fields.size() == 2 &&
        strings->fields[0]->key == "Prefix" &&
        strings->fields[0]->value->parts[0]->text == "Hello");
  CHECK(entry && entry->type == "Software" && entry->citekey == "KeY" &&
        entry->fields.size() == 2);
  if (!entry || entry->fields.size() != 2) return 1;
  const auto* value = entry->fields[0]->value;
  CHECK(entry->fields[0]->key == "TITLE" && value && value->parts.size() == 3);
  if (!value || value->parts.size() != 3) return 1;
  CHECK(cursiv::ast_cast<ast::MacroPart>(value->parts[0]) &&
        value->parts[0]->text == "Prefix");
  CHECK(cursiv::ast_cast<ast::BracedPart>(value->parts[1]) &&
        value->parts[1]->text == " {nested}\ntext");
  CHECK(cursiv::ast_cast<ast::NumberPart>(value->parts[2]) &&
        value->parts[2]->text == "999999999999999999999999999999");
  CHECK(preamble && preamble->value && preamble->value->parts.size() == 2);
  CHECK(comment && comment->raw == "(A (nested) comment)");
  CHECK(cursiv::test::validationHelper(language.language, std::string(mixed))
            .diagnostics.empty());
  for (const auto& [source, code] : {
      std::pair{"@book{}", "bibtex.key"},
      std::pair{"@book{key}", "bibtex.separator"},
      std::pair{"@book{k,title=}", "bibtex.value"},
      std::pair{"@book{k,123field={x}}", "bibtex.field-name"},
      std::pair{"@123{k,title={x}}", "bibtex.entry-type"},
      std::pair{"@book(k,title={x}}", "bibtex.delimiter"},
      std::pair{"@preamble{}", "bibtex.value"},
      std::pair{"@comment(A {unbalanced)", "bibtex.comment-braces"}}) {
    const auto tree = parse(source);
    const auto check = cursiv::test::validationHelper(language.language, source);
    CHECK(check.diagnostics.size() == 1);
    CHECK(!check.diagnostics.empty() && check.diagnostics[0].code &&
          *check.diagnostics[0].code == code);
  }
  for (const auto& [dir, name, count] : {
      std::tuple{CURSIV_EXAMPLE_DIR, "references.bib", std::size_t{3}},
      std::tuple{CURSIV_TEST_DIR, "kinds.bib", std::size_t{11}}}) {
    const auto source = cursiv::test::read_file(std::string(dir) + "/" + name);
    const auto tree = parse(source);
    CHECK(tree.value_as<ast::Document>()->declarations.size() == count);
  }
  for (const auto source : {
      "junk % @misc{fake,title={X}}\n@misc{real,title={Y}}",
      "junk\n% @misc{fake,title={X}}\n@misc{real,title={Y}}",
      "junk % ignored\r@misc{fake,title={X}}\n@misc{real,title={Y}}"}) {
    const auto tree = parse(source);
    const auto* document = tree.value_as<ast::Document>();
    CHECK(document && document->declarations.size() == 1);
    if (!document || document->declarations.size() != 1) return 1;
    const auto* real = cursiv::ast_cast<ast::Entry>(document->declarations[0]);
    CHECK(real && real->citekey == "real" && real->fields.size() == 1 &&
          real->fields[0]->value->parts.size() == 1 &&
          real->fields[0]->value->parts[0]->text == "Y");
  }
  for (const auto source : {
      "junk% ignored @misc{fake,title={X}}\n@misc{real,title={Y}}",
      "\v% ignored @misc{fake,title={X}}\n@misc{real,title={Y}}",
      "\f% ignored @misc{fake,title={X}}\n@misc{real,title={Y}}"}) {
    const auto tree = parse(source);
    CHECK(tree.value_as<ast::Document>()->declarations.size() == 2);
  }
  CHECK(parse("").value_as<ast::Document>()->declarations.empty());
  CHECK(parse("% comment\n").value_as<ast::Document>()->declarations.empty());
  return cursiv::test::report("bibtex_grammar_test");
}
