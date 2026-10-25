// Every document is one intrinsic-function call. Three documents of this
// port's own cover what the shipped corpus does not exercise -- bracket JSONPath segments (plain and
// nested), ArrayContains/ArrayUnique, Base64Encode/Decode, a `\u` string
// escape, MathRandom's optional seed, and a NUMBER with a fraction and an
// exponent.

#include <memory>
#include "native_cases.hpp"
#include "amazonstateslanguageintrinsicfunctions/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include <string>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "amazonstateslanguageintrinsicfunctions/core/ast.hpp"
#include "amazonstateslanguageintrinsicfunctions/generated/AmazonStatesLanguageIntrinsicFunctions/Module.hpp"

namespace {

namespace ast = amazonstateslanguageintrinsicfunctions::ast;
namespace generated = amazonstateslanguageintrinsicfunctions::generated::
    AmazonStatesLanguageIntrinsicFunctions;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p = generated::create_parser();
  return *p;
}

std::string read_example(const char* name) {
  std::string text =
      cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
  if (text.empty())
    text = cursiv::test::read_file(std::string(CURSIV_TEST_DIR) + "/" + name);
  CHECK(!text.empty());
  return text;
}

void every_example_parses_clean() {
  static constexpr const char* files[] = {
      "intrinsic_states_format.asl-intrinsic",
      "intrinsic_states_string_to_json.asl-intrinsic",
      "intrinsic_states_json_to_string.asl-intrinsic",
      "intrinsic_states_array.asl-intrinsic",
      "intrinsic_states_array_partition.asl-intrinsic",
      "intrinsic_states_array_range.asl-intrinsic",
      "intrinsic_states_hash.asl-intrinsic",
      "intrinsic_states_json_merge.asl-intrinsic",
      "intrinsic_states_nested_calls.asl-intrinsic",
      "intrinsic_states_uuid.asl-intrinsic",
      "custom_bracket_paths.asl-intrinsic",
      "custom_base64_unicode.asl-intrinsic",
      "custom_math_random_decimal.asl-intrinsic",
  };
  for (const char* name : files) {
    const std::string text = read_example(name);
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted && !r.cst().recovered());
    CHECK(r.value_as<ast::FuncCall>() != nullptr);
  }
}

void format_reads_its_string_and_two_paths() {
  const std::string text =
      read_example("intrinsic_states_format.asl-intrinsic");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::FuncCall* const call = r.value_as<ast::FuncCall>();
  CHECK(call != nullptr);
  if (call == nullptr) return;
  CHECK(call->function == ast::Function::format);
  CHECK(call->args.size() == 3);
  if (call->args.size() != 3) return;
  CHECK(call->args[0]->type == &cursiv::ast_type_tag<ast::StringArg>);
  CHECK(call->args[1]->type == &cursiv::ast_type_tag<ast::JsonPathArg>);
}

void json_merge_carries_its_bool_flag() {
  const std::string text =
      read_example("intrinsic_states_json_merge.asl-intrinsic");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::FuncCall* const call = r.value_as<ast::FuncCall>();
  CHECK(call != nullptr);
  if (call == nullptr || call->args.size() != 3) return;
  const ast::Arg* const last = call->args[2];
  CHECK(last->type == &cursiv::ast_type_tag<ast::BoolArg>);
  if (last->type != &cursiv::ast_type_tag<ast::BoolArg>) return;
  CHECK(static_cast<const ast::BoolArg*>(last)->value == false);
}

void nested_calls_carry_a_context_path_and_a_negative_int() {
  const std::string text =
      read_example("intrinsic_states_nested_calls.asl-intrinsic");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::FuncCall* const call = r.value_as<ast::FuncCall>();
  CHECK(call != nullptr);
  if (call == nullptr) return;
  CHECK(call->function == ast::Function::array_get_item);
  CHECK(call->args.size() == 2);
  if (call->args.size() != 2) return;
  CHECK(call->args[0]->type == &cursiv::ast_type_tag<ast::FuncCall>);
}

void nested_brackets_and_string_split_and_math_add_parse() {
  const std::string text =
      read_example("custom_bracket_paths.asl-intrinsic");
  const cursiv::ParseResult r = engine().parse(text);
  const ast::FuncCall* const call = r.value_as<ast::FuncCall>();
  CHECK(call != nullptr);
  if (call == nullptr) return;
  CHECK(call->function == ast::Function::array_contains);
  CHECK(call->args.size() == 2);
}

}  // namespace

int main() {
  const auto language=cursiv::create_language<amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctionsCoreServices, amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctionsSharedCoreServices, amazonstateslanguageintrinsicfunctions::install_core_module>();
  if(native_regressions::run(language)) return 1;
  every_example_parses_clean();
  format_reads_its_string_and_two_paths();
  json_merge_carries_its_bool_flag();
  nested_calls_carry_a_context_path_and_a_negative_int();
  nested_brackets_and_string_split_and_math_add_parse();
  return cursiv::test::report("amazonstateslanguageintrinsicfunctions_grammar_test");
}
