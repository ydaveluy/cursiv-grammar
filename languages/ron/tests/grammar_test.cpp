
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "ron/core/CoreServices.hpp"
#include "ron/core/ast.hpp"
// The grammar's end to end check: every document under example/ and tests/
// parses clean, with no recovery, and builds a Document root.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "ron: %s\n", what.c_str());
  return 1;
}

int native_shapes(const cursiv::Parser &parser) {
  namespace ast = ron::ast;
  const std::string source = R"ron(#![enable(implicit_some)]
/* outer /* inner */ end */
Config2(élément2: "a\"b\n\u{1F600}", raw: r###"a"##b"###,
  character: '\u{1F600}', mode: r#type,
  numbers: [+1,-0xff,0o755,0b10_10,9_007_199_254_740_993,1_0.5e+1,2u8,inff64,NaNf32],
  keys: {true: 1, (1,2):3, [4]:5},
  bytes: [b"\x41\x00", b'A', br#"raw"#])
)ron";
  const auto parsed = parser.parse(source);
  if (!parsed.accepted || parsed.cst().recovered() ||
      !parsed.diagnostics.empty())
    return fail("native forms did not parse clean");
  const auto *doc = parsed.value_as<ast::Document>();
  if (!doc || doc->values.size() != 1 || doc->attributes.size() != 1 ||
      doc->values[0]->classname != "Config2")
    return fail("native forms lost their document attributes or struct name");
  const auto *held = std::get_if<ast::StructLit *>(&doc->values[0]->payload);
  const auto *body = held ? *held : nullptr;
  if (!body || body->items.size() != 7 || body->items[0]->name != "élément2")
    return fail("native forms lost struct fields");
  const auto *text =
      std::get_if<std::string_view>(&body->items[0]->value->payload);
  const auto *raw =
      std::get_if<std::string_view>(&body->items[1]->value->payload);
  const auto *character =
      std::get_if<ast::CharLit *>(&body->items[2]->value->payload);
  const auto *mode = std::get_if<ast::Ident *>(&body->items[3]->value->payload);
  if (!text || *text != "a\"b\n😀" || !raw || *raw != "a\"##b" || !character ||
      !*character || (*character)->text != "😀" || !mode || !*mode ||
      (*mode)->name != "type")
    return fail("native forms lost decoded literals or raw identifiers");
  const auto *numbers =
      std::get_if<ast::ListV *>(&body->items[4]->value->payload);
  const char *expected[] = {
      "+1",       "-0xff", "0o755",  "0b10_10", "9_007_199_254_740_993",
      "1_0.5e+1", "2u8",   "inff64", "NaNf32"};
  if (!numbers || !*numbers || (*numbers)->items.size() != std::size(expected))
    return fail("native forms lost numeric literals");
  for (std::size_t i = 0; i < std::size(expected); ++i) {
    const auto *number =
        std::get_if<ast::Number *>(&(*numbers)->items[i]->payload);
    if (!number || !*number || (*number)->text != expected[i])
      return fail("native forms changed an exact numeric spelling");
  }
  const auto *keys =
      std::get_if<ast::MapLit *>(&body->items[5]->value->payload);
  if (!keys || !*keys || (*keys)->items.size() != 3 ||
      !std::holds_alternative<bool>((*keys)->items[0]->key->payload) ||
      !std::holds_alternative<ast::Tuple *>((*keys)->items[1]->key->payload) ||
      !std::holds_alternative<ast::ListV *>((*keys)->items[2]->key->payload))
    return fail("native forms lost composite map keys");
  const auto *bytes =
      std::get_if<ast::ListV *>(&body->items[6]->value->payload);
  if (!bytes || !*bytes || (*bytes)->items.size() != 3)
    return fail("native forms lost byte literals");
  const auto *escaped = std::get_if<ast::Bytes *>(&(*bytes)->items[0]->payload);
  const auto *rawBytes =
      std::get_if<ast::Bytes *>(&(*bytes)->items[2]->payload);
  if (!escaped || !*escaped || (*escaped)->text != std::string_view("A\0", 2) ||
      !rawBytes || !*rawBytes || (*rawBytes)->text != "raw")
    return fail("native forms lost byte string values");
  const std::string comment = "// text\rignored\ntrue";
  const auto skipped = parser.parse(comment);
  const auto *result = skipped.value_as<ast::Document>();
  return result && result->values.size() == 1 &&
                 std::holds_alternative<bool>(result->values[0]->payload)
             ? 0
             : fail("a CR inside a line comment exposed comment text");
}

} // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<ron::RonCoreServices, ron::RonSharedCoreServices,
                              ron::install_core_module>();
  {
    const auto r = language.language.parser->parse("[]");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("{}");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("()");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("-1");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("0xff");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto *doc = r.value_as<ron::ast::Document>();
    if (!doc || doc->values.size() != 1 ||
        !std::holds_alternative<ron::ast::Number *>(doc->values[0]->payload) ||
        std::get<ron::ast::Number *>(doc->values[0]->payload)->text != "0xff")
      return 1;
  }
  {
    const auto r = language.language.parser->parse("(x: 1, y: 2)");
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
  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<ron::RonCoreServices, ron::RonSharedCoreServices,
                              ron::install_core_module>();
  if (native_shapes(*language.language.parser))
    return 1;

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".ron")
        continue;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file)
        return fail(path.filename().string() + " is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<ron::ast::Document>)
        return fail(path.filename().string() + " built no document");
      ++count;
    }
  }
  if (count == 0)
    return fail("no example documents found");

  std::printf("ron: %d examples parse\n", count);
  return 0;
}
