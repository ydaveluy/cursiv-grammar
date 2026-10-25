#include "audit_regressions.hpp"
// The smoke test of the language: every corpus document parses clean, this
// port's own documents cover what the corpus never exercises, and one is
// read back field by field to catch a converter that silently mismaps its
// value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "sparql/core/CoreServices.hpp"
#include "sparql/core/ast.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "sparql: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char *const kExamples[] = {
    "caseInsensitiveIdentifiers.rq",
    "example1.rq",
    "example2.rq",
    "example3.rq",
    "example4.rq",
    "example5.rq",
};

const char *const kOwnDocs[] = {
    "select_features.rq", "reduced_star_from.rq", "construct_features.rq",
    "describe_star.rq",   "describe_all.rq",      "ask_features.rq",
};

} // namespace

int main() {
  if (!audit_regressions::run())
    return 1;
  const auto language =
      cursiv::create_language<sparql::SparqlCoreServices,
                              sparql::SparqlSharedCoreServices,
                              sparql::install_core_module>();

  for (const char *const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<sparql::ast::Query>)
      return fail(std::string(name) + ": built no Query");
  }

  for (const char *const name : kOwnDocs) {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail(std::string(name) + ": empty or unreadable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(std::string(name) + ": does not parse clean");
    if (!result.diagnostics.empty())
      return fail(std::string(name) + ": has diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<sparql::ast::Query>)
      return fail(std::string(name) + ": built no Query");
  }

  // select_features.rq, read back field by field: BASE/PREFIX, the SELECT
  // form's flags, the group pattern's five element kinds, a blank node
  // property list, a collection, a long-string escape (the STRING_LITERAL_
  // LONG2 fix) and a datatype/langtag literal.
  {
    const std::string path =
        std::string(CURSIV_TEST_DIR) + "/select_features.rq";
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("select_features.rq: does not parse clean");

    const auto &query = *static_cast<const sparql::ast::Query *>(result.value);
    if (query.prologue == nullptr)
      return fail("select_features.rq: no prologue");
    if (query.prologue->base == nullptr ||
        query.prologue->base->value != "http://example.org/base/")
      return fail("select_features.rq: BASE not read back");
    if (query.prologue->prefixes.size() != 2)
      return fail("select_features.rq: expected 2 PREFIX declarations");

    const auto *const select =
        std::get_if<sparql::ast::SelectQuery *>(&query.form);
    if (select == nullptr || *select == nullptr)
      return fail("select_features.rq: not a SelectQuery");
    const sparql::ast::SelectQuery &sel = **select;
    if (!sel.distinct)
      return fail("select_features.rq: DISTINCT not set");
    if (sel.vars.size() != 2)
      return fail("select_features.rq: expected 2 vars");
    if (sel.vars[1]->type != &cursiv::ast_type_tag<sparql::ast::Var> ||
        static_cast<const sparql::ast::Var *>(sel.vars[1])->name != "y")
      return fail("select_features.rq: $y not read back as 'y'");

    if (sel.where == nullptr)
      return fail("select_features.rq: no WHERE pattern");
    bool sawOptional = false, sawUnion = false, sawGraph = false;
    int filterCount = 0, tripleCount = 0;
    for (const auto &element : sel.where->elements) {
      if (std::get_if<sparql::ast::OptionalGraphPattern *>(&element))
        sawOptional = true;
      else if (std::get_if<sparql::ast::GroupOrUnionGraphPattern *>(&element))
        sawUnion = true;
      else if (std::get_if<sparql::ast::GraphGraphPattern *>(&element))
        sawGraph = true;
      else if (std::get_if<sparql::ast::Filter *>(&element))
        ++filterCount;
      else if (std::get_if<sparql::ast::Triple *>(&element))
        ++tripleCount;
    }
    if (!sawOptional || !sawUnion || !sawGraph)
      return fail("select_features.rq: missing OPTIONAL/UNION/GRAPH element");
    if (filterCount != 5)
      return fail("select_features.rq: expected 5 FILTER elements");
    if (tripleCount != 9)
      return fail("select_features.rq: expected 9 top-level triples");

    // The bio field: a raw '\'' and a decoded '\\' -- the two string
    // defects this port fixes.
    bool sawBio = false, sawBlank = false, sawCollection = false;
    for (const auto &element : sel.where->elements) {
      const auto *const tripleP = std::get_if<sparql::ast::Triple *>(&element);
      if (tripleP == nullptr || *tripleP == nullptr)
        continue;
      const auto *const props = (*tripleP)->properties;
      if (props == nullptr)
        continue;
      for (const auto *const po : props->items) {
        if (po == nullptr)
          continue;
        for (const auto *const obj : po->objects) {
          if (obj == nullptr)
            continue;
          if (obj->type == &cursiv::ast_type_tag<sparql::ast::StringLit>) {
            const auto &s = *static_cast<const sparql::ast::StringLit *>(obj);
            if (s.value.find('\'') != std::string_view::npos &&
                s.value.find('\\') != std::string_view::npos)
              sawBio = true;
          } else if (obj->type == &cursiv::ast_type_tag<
                                      sparql::ast::BlankNodePropertyList>) {
            sawBlank = true;
            const auto &bnode =
                *static_cast<const sparql::ast::BlankNodePropertyList *>(obj);
            if (bnode.properties != nullptr)
              for (const auto *const nestedPo : bnode.properties->items) {
                if (nestedPo == nullptr)
                  continue;
                for (const auto *const nestedObj : nestedPo->objects)
                  if (nestedObj != nullptr &&
                      nestedObj->type ==
                          &cursiv::ast_type_tag<sparql::ast::Collection>)
                    sawCollection = true;
              }
          } else if (obj->type ==
                     &cursiv::ast_type_tag<sparql::ast::Collection>) {
            sawCollection = true;
          }
        }
      }
    }
    if (!sawBio)
      return fail(
          "select_features.rq: the long-string escape did not round-trip");
    if (!sawBlank)
      return fail("select_features.rq: no BlankNodePropertyList object");
    if (!sawCollection)
      return fail(
          "select_features.rq: no Collection object (via the property list)");
  }

  // reduced_star_from.rq: REDUCED, star, two dataset clauses (one NAMED).
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/reduced_star_from.rq");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &query = *static_cast<const sparql::ast::Query *>(result.value);
    const auto *const select =
        std::get_if<sparql::ast::SelectQuery *>(&query.form);
    if (select == nullptr || *select == nullptr)
      return fail("reduced_star_from.rq: not a SelectQuery");
    const sparql::ast::SelectQuery &sel = **select;
    if (!sel.reduced || !sel.star)
      return fail("reduced_star_from.rq: REDUCED/star not set");
    if (sel.datasets.size() != 2 || !sel.datasets[1]->named)
      return fail("reduced_star_from.rq: FROM/FROM NAMED not read back");
    if (!sel.modifier->limit.has_value() || *sel.modifier->limit != "5" ||
        !sel.modifier->offset.has_value() || *sel.modifier->offset != "1")
      return fail("reduced_star_from.rq: LIMIT/OFFSET not read back");
  }

  // construct_features.rq: the template holds one triple.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/construct_features.rq");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &query = *static_cast<const sparql::ast::Query *>(result.value);
    const auto *const construct =
        std::get_if<sparql::ast::ConstructQuery *>(&query.form);
    if (construct == nullptr || *construct == nullptr ||
        (*construct)->templateTriples.size() != 1)
      return fail("construct_features.rq: template not read back");
  }

  // ask_features.rq: an AskQuery, no solution modifier to read.
  {
    const std::string text =
        read_file(std::string(CURSIV_TEST_DIR) + "/ask_features.rq");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &query = *static_cast<const sparql::ast::Query *>(result.value);
    if (!std::holds_alternative<sparql::ast::AskQuery *>(query.form))
      return fail("ask_features.rq: not an AskQuery");
  }

  std::puts("sparql: the corpus and the port's own documents parse clean");
  return 0;
}
