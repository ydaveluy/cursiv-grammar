#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "swiftfin/core/CoreServices.hpp"
#include "swiftfin/core/ast.hpp"
#include "swiftfin/generated/SwiftFin/Module.hpp"

namespace {
namespace ast = swiftfin::ast;
const cursiv::Parser& engine() {
  static const auto parser = swiftfin::generated::SwiftFin::create_parser();
  return *parser;
}
cursiv::ParseResult build(std::string_view text) {
  auto result = engine().parse(text);
  CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
  return result;
}
std::string load(const char* dir, const char* name) {
  return cursiv::test::read_file(std::string(dir) + "/" + name);
}
const ast::Block* block(const ast::Message* message, std::string_view id) {
  if (message) for (const auto* item : message->blocks) if (item && item->id == id) return item;
  return nullptr;
}
std::string_view header(const ast::Message* message, std::string_view id) {
  const auto* value = cursiv::ast_cast<ast::Header>(block(message, id));
  return value ? value->value : std::string_view{};
}
const auto& entries(const ast::Message* message, std::string_view id) {
  static const decltype(ast::TagBlock::tags) empty;
  const auto* value = cursiv::ast_cast<ast::TagBlock>(block(message, id));
  return value ? value->tags : empty;
}
const ast::TextBlock* text(const ast::Message* message) {
  return cursiv::ast_cast<ast::TextBlock>(block(message, "4"));
}
void every_example_parses() {
  for (const char* name : {"test1.swiftfin", "test2.swiftfin", "complex1.swiftfin", "complex2.swiftfin"}) {
    const auto source = load(CURSIV_EXAMPLE_DIR, name);
    const auto result = build(source);
    CHECK(cursiv::ast_cast<ast::Document>(result.value) != nullptr);
  }
  const auto source = load(CURSIV_TEST_DIR, "minimal.swiftfin");
  const auto result = build(source);
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(doc && doc->messages.size() == 1);
  if (!doc || doc->messages.size() != 1) return;
  const auto* message = doc->messages[0];
  CHECK(header(message, "1") == "F01AAAABBCCDDD");
  CHECK(!block(message, "2") && !block(message, "3") && !block(message, "4") && !block(message, "5"));
}
void map_and_text_fields() {
  const auto source = load(CURSIV_EXAMPLE_DIR, "test1.swiftfin");
  const auto result = build(source);
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(doc && doc->messages.size() == 1);
  if (!doc || doc->messages.size() != 1) return;
  const auto* message = doc->messages[0];
  CHECK(header(message, "1") == "AAA" && header(message, "2") == "BBB");
  const auto& user = entries(message, "3");
  CHECK(user.size() == 2);
  if (user.size() == 2) {
    CHECK(user[0]->key == "C" && user[0]->value.empty());
    CHECK(user[1]->key == "D" && user[1]->value == "E");
  }
  const auto* body = text(message);
  CHECK(body && entries(message, "4").empty());
  CHECK(body && body->fields.size() == 1);
  if (body && body->fields.size() == 1) {
    CHECK(body->fields[0]->tag == "FOO");
    CHECK(body->fields[0]->value == "BAR\n:XXX:YYY");
  }
  const auto& trailer = entries(message, "5");
  CHECK(trailer.size() == 2);
  if (trailer.size() == 2) {
    CHECK(trailer[0]->key == "F" && trailer[0]->value == "G");
    CHECK(trailer[1]->key == "H" && trailer[1]->value == "123");
  }
}
void two_messages_both_block4_forms() {
  const auto source = load(CURSIV_EXAMPLE_DIR, "complex1.swiftfin");
  const auto result = build(source);
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(doc && doc->messages.size() == 2);
  if (!doc || doc->messages.size() != 2) return;
  const auto* first = text(doc->messages[0]);
  CHECK(first && !first->fields.empty() && entries(doc->messages[0], "4").empty());
  bool found = false;
  if (first) for (const auto* field : first->fields) if (field->tag == "70E") {
    found = true; CHECK(std::count(field->value.begin(), field->value.end(), '\n') == 1);
  }
  CHECK(found);
  const auto* second = doc->messages[1];
  CHECK(header(second, "1") == "F21" && header(second, "2") == "AAA");
  CHECK(block(second, "4") && !text(second));
  const auto& values = entries(second, "4");
  CHECK(values.size() == 2);
  if (values.size() == 2) {
    CHECK(values[0]->key == "AA" && values[0]->value.empty());
    CHECK(values[1]->key == "BB" && values[1]->value == "CC");
  }
  CHECK(entries(second, "5").size() == 2);
}
void subfield_colon_is_one_line() {
  const auto source = load(CURSIV_EXAMPLE_DIR, "test2.swiftfin");
  const auto result = build(source);
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(doc && doc->messages.size() == 1);
  const auto* body = doc && doc->messages.size() == 1 ? text(doc->messages[0]) : nullptr;
  CHECK(body && !body->fields.empty());
  bool found = false;
  if (body) for (const auto* field : body->fields) if (field->tag == "20") {
    found = true; CHECK(field->value == "55094429070-0713");
  }
  CHECK(found);
}
void native_regressions() {
  const std::string basic = "{1:F01BANKBEBBAXXX2222123456}";
  const auto nativeSource = basic + "{3:{270:{1:A}{4:{x:y}}}}{4:\n:20:\n:21:A\r\n\n:NOTTAG:B\n-}{S:{T01:x}}{5:}";
  const auto parsed = build(nativeSource);
  const auto* doc = cursiv::ast_cast<ast::Document>(parsed.value);
  const auto* message = doc && doc->messages.size() == 1 ? doc->messages[0] : nullptr;
  CHECK(message && message->blocks.size() == 5);
  const auto& user = entries(message, "3");
  CHECK(user.size() == 1 && user[0]->key == "270" && user[0]->value == "{1:A}{4:{x:y}}");
  CHECK(entries(message, "S").size() == 1 && block(message, "5"));
  const auto* body = text(message);
  CHECK(body && body->fields.size() == 2);
  if (body && body->fields.size() == 2) {
    CHECK(body->fields[0]->tag == "20" && body->fields[0]->value.empty());
    CHECK(body->fields[1]->tag == "21" && body->fields[1]->value == "A\r\n\n:NOTTAG:B");
  }
  auto language = cursiv::create_language<swiftfin::SwiftFinCoreServices, swiftfin::SwiftFinSharedCoreServices, swiftfin::install_core_module>();
  const auto tagMode = cursiv::test::validationHelper(language.language, basic + "{3:{4:{ABC:\n-}}}");
  CHECK(tagMode.diagnostics.empty());
  for (const auto& [source, code] : {
      std::pair{std::string("{1:SHORT}"), "swiftfin.header.length"},
      std::pair{basic + "{2:X103BANKDEFFXXXXN}", "swiftfin.header.direction"},
      std::pair{std::string("{3:{108:A}}"), "swiftfin.header.missing"},
      std::pair{basic + "{3:{108:A}}{3:{108:B}}", "swiftfin.block.duplicate"},
      std::pair{basic + "{5:}{3:}", "swiftfin.block.order"},
      std::pair{basic + "{4:\n:FOO:A\n-}", "swiftfin.tag"},
      std::pair{basic + "{4:{4:\n:20:{unbalanced\n-}}}", "swiftfin.tag.end"}}) {
    const auto result = build(source);
    const auto validation = cursiv::test::validationHelper(language.language, source);
    bool found = false;
    for (const auto& d : validation.diagnostics) if (d.code && d.code->isString() && d.code->string() == code) found = true;
    CHECK(found);
  }
}
}  // namespace
int main() {
  every_example_parses(); map_and_text_fields(); two_messages_both_block4_forms();
  subfield_colon_is_one_line(); native_regressions();
  return cursiv::test::report("swiftfin_grammar_test");
}
