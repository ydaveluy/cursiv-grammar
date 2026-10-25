#include "native_regressions.hpp"
#include "audit_regressions.hpp"
// The examples retain the cluster boundaries produced by ICU.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "graphemes/core/CoreServices.hpp"
#include "graphemes/core/ast.hpp"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
  if (ok) return;
  std::fprintf(stderr, "graphemes: FAILED: %s\n", what);
  ++failures;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

graphemes::GraphemesCoreServices& language() {
  static const auto lang = cursiv::create_language<
      graphemes::GraphemesCoreServices, graphemes::GraphemesSharedCoreServices,
      graphemes::install_core_module>();
  return lang.language;
}

// A clean parse: not recovered, no diagnostics -- a value converter that
// refuses leaves the root type right but the diagnostics list non-empty, so
// both are checked, not only `recovered()`.
cursiv::ParseResult parse_clean(const std::string& text, const char* label) {
  cursiv::ParseResult r = language().parser->parse(text);
  check(!r.cst().recovered(), label);
  check(r.diagnostics.empty(), label);
  return r;
}

graphemes::ast::Document* document(cursiv::ParseResult& r) {
  const cursiv::AstNode* const root = r.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<graphemes::ast::Document>)
    return nullptr;
  return static_cast<graphemes::ast::Document*>(
      const_cast<cursiv::AstNode*>(root));
}

void ascii_example() {
  const std::string src =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/ascii.graphemes");
  cursiv::ParseResult r = parse_clean(src, "example/ascii.graphemes");
  graphemes::ast::Document* d = document(r);
  check(d != nullptr, "ascii: no document");
  if (d == nullptr) return;
  check(d->clusters.size() == 6, "ascii: cluster count");
  if (d->clusters.size() != 6) return;
  const char* want[] = {"A", "B", "C", "D", "E", "\n"};
  for (int i = 0; i < 6; ++i)
    check(d->clusters[i] != nullptr && d->clusters[i]->text == want[i],
          "ascii: cluster text");
}

void emoji_example() {
  const std::string src =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/emoji.graphemes");
  cursiv::ParseResult r = parse_clean(src, "example/emoji.graphemes");
  graphemes::ast::Document* d = document(r);
  check(d != nullptr, "emoji: no document");
  if (d == nullptr) return;
  check(d->clusters.size() == 50, "emoji: cluster count");

  int zwj_gender = 0, regional_flag = 0, tag_flag = 0;
  for (const graphemes::ast::GraphemeCluster* c : d->clusters) {
    if (c == nullptr) continue;
    if (c->text == "👮‍♀️") ++zwj_gender;      // base + ZWJ + gender + VS16
    if (c->text == "🇨🇨") ++regional_flag;          // two regional indicators
    if (c->text.size() == 28 && c->text.substr(0, 4) == "🏴")
      ++tag_flag;  // an `EmojiTagSequence`: base, six tag chars, terminator
  }
  check(zwj_gender == 1, "emoji: ZWJ gender sequence");
  check(regional_flag == 1, "emoji: regional indicator flag");
  check(tag_flag == 2, "emoji: tag sequence flags");
}

void udhr_example() {
  const std::string src =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/udhr.graphemes");
  cursiv::ParseResult r = parse_clean(src, "example/udhr.graphemes");
  graphemes::ast::Document* d = document(r);
  check(d != nullptr, "udhr: no document");
  if (d == nullptr) return;
  check(d->clusters.size() == 561, "udhr: cluster count");
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !native_regressions::run()) return 1;
  ascii_example();
  emoji_example();
  udhr_example();
  if (failures == 0) std::puts("graphemes: the examples parse");
  return failures == 0 ? 0 : 1;
}
