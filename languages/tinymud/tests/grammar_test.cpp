// Parses every corpus document plus the hand-written ones under example/,
// then reads a few field values back to pin the converters and the STRING
// terminal (its class includes a space, so a description spans words).

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "tinymud/core/CoreServices.hpp"
#include "tinymud/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "tinymud: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      tinymud::TinymudCoreServices, tinymud::TinymudSharedCoreServices,
      tinymud::install_core_module>();

  for (const char* path : {CURSIV_EXAMPLE1, CURSIV_ADMIN, CURSIV_ACTIONS}) {
    const std::string text = read(path);
    if (text.empty()) return fail("an example document is not readable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("an example does not parse");
    if (!result.diagnostics.empty())
      return fail("an example carries a diagnostic");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<tinymud::ast::Session>)
      return fail("the parse built no session");
  }

  // example1: the four lines Igor's own transcript declares.
  {
    const std::string text = read(CURSIV_EXAMPLE1);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& session = *result.value_as<tinymud::ast::Session>();
    if (session.lines.size() != 4) return fail("example1 has the wrong shape");

    const auto* desc = as<tinymud::ast::DescribeCommand>(session.lines[0]);
    if (desc == nullptr || desc->object != "me" ||
        desc->description !=
            "Igor is a ferret with an evil glint in his eye.")
      return fail("@desc did not read back");

    const auto* lock = as<tinymud::ast::LockCommand>(session.lines[1]);
    if (lock == nullptr || lock->object != "me" || lock->key != "me")
      return fail("@lock did not read back");

    const auto* okfail = as<tinymud::ast::OfailCommand>(session.lines[3]);
    if (okfail == nullptr ||
        okfail->message != "howls in pain as Igor bites them.")
      return fail("@ofail did not read back");
  }

  // admin: a numeric converter, a variant target, a negated flag.
  {
    const std::string text = read(CURSIV_ADMIN);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& session = *result.value_as<tinymud::ast::Session>();

    const auto* create = as<tinymud::ast::CreateCommand>(session.lines[2]);
    if (create == nullptr || !create->cost.has_value() ||
        *create->cost != 10)
      return fail("@create's cost did not convert");

    bool sawNumber = false, sawText = false;
    for (const tinymud::ast::Line* line : session.lines) {
      const auto* link = as<tinymud::ast::LinkCommand>(line);
      if (link == nullptr) continue;
      if (std::holds_alternative<std::int64_t>(link->target)) sawNumber = true;
      if (std::holds_alternative<std::string_view>(link->target)) sawText = true;
    }
    if (!sawNumber || !sawText)
      return fail("@link's target did not cover both alternatives");

    bool sawNegate = false;
    for (const tinymud::ast::Line* line : session.lines) {
      const auto* set = as<tinymud::ast::SetCommand>(line);
      if (set != nullptr && set->negate) sawNegate = true;
    }
    if (!sawNegate) return fail("@set's '!' did not set the flag");
  }

  // A blank line mid-document is not a member: `line+` cannot resync past it.
  {
    const std::string text = read(CURSIV_NEGATIVE_BLANK_LINE);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("a mid-document blank line should not parse clean");
  }

  std::puts("tinymud: the corpus parses");
  return 0;
}
