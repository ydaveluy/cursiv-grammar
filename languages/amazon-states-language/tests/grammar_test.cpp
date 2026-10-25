// The grammar's smoke test: every corpus example parses clean with no
// diagnostics, plus one document of this port's own for fields the shipped
// corpus never reaches. A few field values are read back -- a converter
// that silently gave the wrong value would still pass a structural check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>

#include "amazonstateslanguage/core/CoreServices.hpp"
#include "amazonstateslanguage/core/ast.hpp"
#include "amazonstateslanguage/generated/AmazonStatesLanguage/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace {

namespace ast = amazonstateslanguage::ast;
namespace generated =
    amazonstateslanguage::generated::AmazonStatesLanguage;

int failures = 0;

void check(bool ok, const char* what) {
  if (ok) return;
  ++failures;
  std::fprintf(stderr, "amazonstateslanguage: FAILED: %s\n", what);
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p = generated::create_parser();
  return *p;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

template <class T>
bool only(const cursiv::vector<T>& list, std::type_identity_t<T> value) {
  return list.size() == 1 && list[0] == value;
}

template <class T>
T* single(const cursiv::vector<T*>& list) {
  return list.size() == 1 ? list[0] : nullptr;
}

const ast::Binding* find(const ast::Object* obj, std::string_view key) {
  if (obj == nullptr) return nullptr;
  for (const ast::Binding* member : obj->members)
    if (member != nullptr && member->key == key) return member;
  return nullptr;
}

const ast::State* find(const ast::Program* program, std::string_view name) {
  if (program == nullptr) return nullptr;
  for (const ast::State* state : program->states)
    if (state != nullptr && state->name == name) return state;
  return nullptr;
}

void every_corpus_example_parses_clean() {
  static constexpr const char* files[] = {
      "choice_state.asl",
      "dynamodb_put_update_get_item.asl",
      "fail_state.asl",
      "lambda_catch.asl",
      "lambda_retry.asl",
      "map_state.asl",
      "map_state_legacy.asl",
      "parallel_state.asl",
      "pass_state.asl",
  };
  for (const char* name : files) {
    const std::string text =
        read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
    check(!text.empty(), name);
    const cursiv::ParseResult r = engine().parse(text);
    check(r.accepted && !r.cst().recovered(), name);
    check(r.diagnostics.empty(), name);
    check(r.value_as<ast::Program>() != nullptr, name);
  }
}

// dynamodb_put_update_get_item.asl: three Task states chained by Next, each
// with a Parameters object whose entries bind a JSONPath through a `.$` key.
void dynamodb_example_reads_back() {
  const std::string text = read_file(std::string(CURSIV_EXAMPLE_DIR) +
                                     "/dynamodb_put_update_get_item.asl");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::Program* const program = r.value_as<ast::Program>();
  check(program != nullptr, "dynamodb: root");
  if (program == nullptr) return;
  check(only(program->comment, "DYNAMODB_PUT_UPDATE_GET_ITEM"), "dynamodb: comment");
  check(only(program->start_at, "PutItem"), "dynamodb: start_at");
  check(program->states.size() == 3, "dynamodb: state count");

  const ast::State* const put = find(program, "PutItem");
  check(put != nullptr, "dynamodb: PutItem exists");
  if (put == nullptr) return;
  check(only(put->types, "Task"), "dynamodb: PutItem type");
  check(only(put->resources, "arn:aws:states:::dynamodb:putItem"),
       "dynamodb: PutItem resource");
  check(only(put->result_paths, "$.putItemOutput"),
       "dynamodb: PutItem result_path");
  check(only(put->next, "UpdateItem"), "dynamodb: PutItem next");
  const ast::Object* const parameters = single(put->parameters);
  check(parameters != nullptr, "dynamodb: PutItem parameters");
  if (parameters == nullptr) return;
  const ast::Binding* const table_name = find(parameters, "TableName.$");
  check(table_name != nullptr, "dynamodb: TableName.$ binding");
  if (table_name != nullptr)
    check(std::get<std::string_view>(table_name->value) == "$.TableName",
         "dynamodb: TableName.$ value");
}

// choice_state.asl: the first Choices entry is `{"Not": {...}, "Next": ...}`
// -- a CompositeRule wrapping one ComparisonRule.
void choice_state_composite_reads_back() {
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/choice_state.asl");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::Program* const program = r.value_as<ast::Program>();
  check(program != nullptr, "choice_state: root");
  if (program == nullptr) return;
  const ast::State* const choice = find(program, "ChoiceStateX");
  check(choice != nullptr, "choice_state: ChoiceStateX exists");
  if (choice == nullptr) return;
  check(choice->choices.size() == 3, "choice_state: rule count");
  check(only(choice->default_states, "DefaultState"), "choice_state: default");
  if (choice->choices.empty()) return;

  const ast::CompositeRule* const first =
      as<ast::CompositeRule>(choice->choices[0]);
  check(first != nullptr, "choice_state: first rule is composite");
  if (first == nullptr) return;
  check(only(first->logical_op, "Not"), "choice_state: logical_op");
  check(only(first->next, "Public"), "choice_state: composite next");
  check(first->operands.size() == 1, "choice_state: one operand");
  if (first->operands.empty()) return;
  const ast::ComparisonRule* const inner =
      as<ast::ComparisonRule>(first->operands[0]);
  check(inner != nullptr, "choice_state: operand is comparison");
  if (inner == nullptr) return;
  check(only(inner->variable, "$.type"), "choice_state: operand variable");
  check(only(inner->comparison_op, "StringEquals"), "choice_state: operand op");
}

void uncommon_fields_example_parses_clean_and_reads_back() {
  const std::string text =
      read_file(std::string(CURSIV_TEST_DIR) + "/uncommon_fields.asl");
  check(!text.empty(), "uncommon_fields: readable");
  const cursiv::ParseResult r = engine().parse(text);
  check(r.accepted && !r.cst().recovered(), "uncommon_fields: parses clean");
  check(r.diagnostics.empty(), "uncommon_fields: no diagnostics");
  const ast::Program* const program = r.value_as<ast::Program>();
  check(program != nullptr, "uncommon_fields: root");
  if (program == nullptr) return;
  check(only(program->version, "1.0"), "uncommon_fields: version");
  check(only(program->timeout_seconds, 300),
       "uncommon_fields: top-level timeout_seconds");

  const ast::State* const work = find(program, "Work");
  check(work != nullptr, "uncommon_fields: Work exists");
  if (work == nullptr) return;
  check(only(work->max_concurrencies, 2), "uncommon_fields: max_concurrency");
  const ast::Object* const selector = single(work->item_selectors);
  check(selector != nullptr, "uncommon_fields: item_selector");
  if (selector != nullptr)
    check(find(selector, "item.$") != nullptr,
         "uncommon_fields: item_selector binding");
  const ast::ItemReader* const reader = single(work->item_readers);
  check(reader != nullptr, "uncommon_fields: item_reader");
  if (reader != nullptr) {
    const ast::ReaderConfig* const rc = single(reader->reader_config);
    check(rc != nullptr, "uncommon_fields: reader_config");
    if (rc != nullptr) {
      check(only(rc->max_items, 100),
           "uncommon_fields: max_items");
      check(only(rc->max_items_path, "$.limit"), "uncommon_fields: max_items_path");
    }
  }
  const ast::Program* const processor = single(work->item_processors);
  check(processor != nullptr, "uncommon_fields: item_processor");
  if (processor != nullptr) {
    check(single(processor->processor_config) != nullptr,
         "uncommon_fields: processor_config");
    if (single(processor->processor_config) != nullptr)
      check(only(single(processor->processor_config)->mode, "INLINE"),
           "uncommon_fields: mode");
    const ast::State* const item = find(processor, "Item");
    check(item != nullptr, "uncommon_fields: Item state exists");
    if (item != nullptr) {
      check(only(item->timeout_seconds_paths, "$.timeout"),
           "uncommon_fields: timeout_seconds_path");
      check(only(item->heartbeat_seconds, 5),
           "uncommon_fields: heartbeat_seconds");
      check(only(item->heartbeat_seconds_paths, "$.heartbeat"),
           "uncommon_fields: heartbeat_seconds_path");
      check(single(item->result_selectors) != nullptr,
           "uncommon_fields: result_selector");
      check(item->catch_.size() == 1, "uncommon_fields: catch count");
      if (item->catch_.size() == 1)
        check(only(item->catch_[0]->comment, "swallow everything"),
             "uncommon_fields: catcher comment");
    }
  }

  const ast::State* const done = find(program, "Done");
  check(done != nullptr, "uncommon_fields: Done exists");
  if (done != nullptr)
    check(only(done->error_paths, "$.lastError"),
         "uncommon_fields: error_path");
}

void state_keys_written_twice_keep_both() {
  const cursiv::ParseResult r = engine().parse(
      R"({"StartAt": "S", "States": {"S": {"Type": "Task", "Resource": "a",)"
      R"( "Parameters": {"x": 1}, "Resource": "b", "Parameters": {"y": 2},)"
      R"( "Retry": [{"MaxAttempts": 1, "MaxAttempts": 2}], "End": true}}})");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
       "twice: parses clean");
  const ast::State* const state = find(r.value_as<ast::Program>(), "S");
  check(state != nullptr, "twice: S exists");
  if (state == nullptr) return;
  check(state->resources.size() == 2 && state->resources[0] == "a" &&
           state->resources[1] == "b",
       "twice: both resources, in order");
  check(state->parameters.size() == 2 &&
           find(state->parameters[1], "y") != nullptr,
       "twice: both parameters objects");
  check(state->retry.size() == 1 && state->retry[0]->max_attempts.size() == 2 &&
           state->retry[0]->max_attempts[1] == 2,
       "twice: both retrier max_attempts");
}

}  // namespace

int main() {
  {
    const auto language = cursiv::create_language<
        amazonstateslanguage::AmazonStatesLanguageCoreServices,
        amazonstateslanguage::AmazonStatesLanguageSharedCoreServices,
        amazonstateslanguage::install_core_module>();
    constexpr auto source =
        R"({"StartAt":"A","Comment":"first","Comment":"second","States":{"A":{"Type":"Succeed"}}})";
    const auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "duplicate object members parse without recovery");
    const auto *program = result.value_as<ast::Program>();
    check(program && program->members.size() == 4 &&
              program->members[1] == "Comment" &&
              program->members[2] == "Comment" && program->comment.size() == 2 &&
              program->comment[0] == "first" && program->comment[1] == "second",
          "duplicate member occurrences retained");
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    check(validation.diagnostics.size() == 1,
          "duplicate member reported at runtime");
    const auto valid = cursiv::test::validationHelper(
        language.language,
        R"({"States":{"A":{"Type":"Succeed"}},"StartAt":"A"})");
    check(valid.diagnostics.empty(), "field order is unrestricted");
  }
  every_corpus_example_parses_clean();
  dynamodb_example_reads_back();
  choice_state_composite_reads_back();
  uncommon_fields_example_parses_clean_and_reads_back();
  state_keys_written_twice_keep_both();
  if (failures != 0) {
    std::fprintf(stderr, "amazonstateslanguage: %d check(s) failed\n",
                failures);
    return 1;
  }
  std::puts("amazonstateslanguage: the corpus parses");
  return 0;
}
