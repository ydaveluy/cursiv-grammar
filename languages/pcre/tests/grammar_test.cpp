
#include "modern_cases.hpp"
#include "extended_class_cases.hpp"
#include "converter_cases.hpp"
#include "x_mode_tests.hpp"
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pcre/core/CoreServices.hpp"
#include "pcre/core/ast.hpp"
// The grammar lane: every document under example/ and tests/ parses clean
// (no repair, no diagnostic), and one pattern is read back field by field --
// a structural check alone would miss a converter that silently returns the
// wrong value.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "pcre: %s\n", what.c_str());
  return 1;
}

const pcre::ast::Pcre *parseClean(const cursiv::Parser &parser,
                                  const std::string &text,
                                  cursiv::ParseResult &out) {
  out = parser.parse(text);
  if (!out.accepted || out.cst().recovered() || !out.diagnostics.empty())
    return nullptr;
  const cursiv::AstNode *const root = out.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<pcre::ast::Pcre>)
    return nullptr;
  return static_cast<const pcre::ast::Pcre *>(root);
}

template <class T> const T *nodeAs(const pcre::ast::Node *n) {
  if (n == nullptr || n->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(n);
}

// a(?<year>\d{4})b*|c -- a name capture around a range-quantified class
// shorthand, a star-quantified literal, and a second, one-item branch.
int checkReadback(const cursiv::Parser &parser) {
  const std::string text = "a(?<year>\\d{4})b*|c";
  cursiv::ParseResult result;
  const pcre::ast::Pcre *const doc = parseClean(parser, text, result);
  if (doc == nullptr)
    return fail("the readback pattern does not parse clean");
  if (doc->pattern == nullptr)
    return fail("the readback pattern built no Alternation");

  const pcre::ast::Alternation &alt = *doc->pattern;
  if (alt.branches.size() != 2)
    return fail("expected two branches");

  const pcre::ast::Sequence &branch1 = *alt.branches[0];
  if (branch1.items.size() != 3)
    return fail("expected three items in branch 1");

  const auto *const a = nodeAs<pcre::ast::Literal>(branch1.items[0]);
  if (a == nullptr || a->text != "a")
    return fail("item 0 is not the literal 'a'");

  const auto *const group = nodeAs<pcre::ast::Group>(branch1.items[1]);
  if (group == nullptr)
    return fail("item 1 is not a Group");
  if (group->kind != pcre::ast::GroupKind::named_angle)
    return fail("the group's kind was not read back as named_angle");
  if (group->name != "year")
    return fail("the group's name was not read back as 'year'");
  if (group->body == nullptr || group->body->branches.size() != 1)
    return fail("the group's body is not a one-branch alternation");
  const pcre::ast::Sequence &groupBody = *group->body->branches[0];
  if (groupBody.items.size() != 1)
    return fail("expected one item inside the group");

  const auto *const quantifiedClass =
      nodeAs<pcre::ast::Quantified>(groupBody.items[0]);
  if (quantifiedClass == nullptr)
    return fail("\\d{4} is not a Quantified");
  if (quantifiedClass->kind != pcre::ast::QuantifierKind::range)
    return fail("\\d{4}'s quantifier kind was not read back as range");
  if (quantifiedClass->from != 4)
    return fail("\\d{4}'s from was not read back as 4");
  if (quantifiedClass->hasComma)
    return fail("\\d{4} should not have a comma");
  const auto *const charType =
      nodeAs<pcre::ast::CharacterType>(quantifiedClass->operand);
  if (charType == nullptr || charType->text != "\\d")
    return fail("\\d{4}'s operand was not read back as the \\d shorthand");

  const auto *const bStar = nodeAs<pcre::ast::Quantified>(branch1.items[2]);
  if (bStar == nullptr)
    return fail("item 2 is not a Quantified");
  if (bStar->kind != pcre::ast::QuantifierKind::star)
    return fail("b*'s quantifier kind was not read back as star");
  const auto *const b = nodeAs<pcre::ast::Literal>(bStar->operand);
  if (b == nullptr || b->text != "b")
    return fail("b*'s operand was not read back as 'b'");

  const pcre::ast::Sequence &branch2 = *alt.branches[1];
  if (branch2.items.size() != 1)
    return fail("expected one item in branch 2");
  const auto *const c = nodeAs<pcre::ast::Literal>(branch2.items[0]);
  if (c == nullptr || c->text != "c")
    return fail("branch 2's item is not the literal 'c'");

  return 0;
}

// [.][^]a] -- inside a class `.` is a plain byte, and a `]` first in it is a
// member, after the `^` that negates it.
int checkClasses(const cursiv::Parser &parser) {
  const std::string text = "[.][^]a]";
  cursiv::ParseResult result;
  const pcre::ast::Pcre *const doc = parseClean(parser, text, result);
  if (doc == nullptr || doc->pattern == nullptr ||
      doc->pattern->branches.size() != 1)
    return fail("the class pattern does not parse clean");
  const pcre::ast::Sequence &items = *doc->pattern->branches[0];
  if (items.items.size() != 2)
    return fail("expected two classes");

  const auto *const dot = nodeAs<pcre::ast::CharacterClass>(items.items[0]);
  if (dot == nullptr || dot->negate || dot->atoms.size() != 1)
    return fail("[.] is not a one-member class");
  const auto *const dotLit = nodeAs<pcre::ast::Literal>(dot->atoms[0]);
  if (dotLit == nullptr || dotLit->text != ".")
    return fail("[.]'s member is not the literal '.'");

  const auto *const neg = nodeAs<pcre::ast::CharacterClass>(items.items[1]);
  if (neg == nullptr || !neg->negate || neg->atoms.size() != 2)
    return fail("[^]a] is not a negated two-member class");
  const auto *const bracket = nodeAs<pcre::ast::Literal>(neg->atoms[0]);
  const auto *const a = nodeAs<pcre::ast::Literal>(neg->atoms[1]);
  if (bracket == nullptr || bracket->text != "]" || a == nullptr ||
      a->text != "a")
    return fail("[^]a]'s members are not ']' and 'a'");
  return 0;
}

// [01[:alpha:]%] -- pcre2pattern's own example: `[:alpha:]` inside a class is
// a POSIX class among the literals, not a `[` that the next `]` closes.
int checkPosixClass(const cursiv::Parser &parser) {
  const std::string text = "[01[:alpha:]%]";
  cursiv::ParseResult result;
  const pcre::ast::Pcre *const doc = parseClean(parser, text, result);
  if (doc == nullptr || doc->pattern == nullptr ||
      doc->pattern->branches.size() != 1 ||
      doc->pattern->branches[0]->items.size() != 1)
    return fail("[01[:alpha:]%] is not one item");
  const auto *const cls =
      nodeAs<pcre::ast::CharacterClass>(doc->pattern->branches[0]->items[0]);
  if (cls == nullptr || cls->atoms.size() != 4)
    return fail("[01[:alpha:]%] is not a four-member class");
  const auto *const posix =
      nodeAs<pcre::ast::PosixCharacterClass>(cls->atoms[2]);
  if (posix == nullptr || posix->negate || posix->name != "alpha")
    return fail("[01[:alpha:]%]'s third member is not [:alpha:]");
  return 0;
}

} // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<pcre::PcreCoreServices,
                                                pcre::PcreSharedCoreServices,
                                                pcre::install_core_module>();
  {
    const auto r = language.language.parser->parse("()");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("|a");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("(?:)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<pcre::PcreCoreServices,
                                                pcre::PcreSharedCoreServices,
                                                pcre::install_core_module>();
  if (!pcre_converter_cases::run(*language.language.parser))
    return 1;
  if (!pcre_x_modes::run(language))
    return 1;
  if (!pcre_audit::modern_cases(language))
    return 1;
  if (!pcre_audit::extended_class_cases(language))
    return 1;
  const cursiv::Parser &parser = *language.language.parser;

  std::vector<std::string> examples;
  for (const std::filesystem::directory_entry &entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".pcre")
      examples.push_back(entry.path().string());
  if (examples.empty())
    return fail("no .pcre document found under example/");

  for (const std::string &path : examples) {
    std::ifstream file(path);
    if (!file)
      return fail(path + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    cursiv::ParseResult result;
    if (parseClean(parser, text, result) == nullptr)
      return fail(path + " does not parse clean (recovered, a diagnostic, "
                         "or the wrong root type)");
  }

  // newline_conventions.pcre: the four (*...) newline conventions the
  // corpus under example/ never spells (only (*CR) does).
  {
    const std::string path =
        std::string(CURSIV_TEST_DIR) + "/newline_conventions.pcre";
    std::ifstream file(path);
    if (!file)
      return fail(path + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    cursiv::ParseResult result;
    if (parseClean(parser, text, result) == nullptr)
      return fail(path + " does not parse clean");
  }

  if (const int rc = checkReadback(parser); rc != 0)
    return rc;
  if (const int rc = checkClasses(parser); rc != 0)
    return rc;
  if (const int rc = checkPosixClass(parser); rc != 0)
    return rc;

  std::printf("pcre: %zu examples parse clean\n", examples.size());
  return 0;
}
