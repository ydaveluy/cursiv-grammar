
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "protobuf/core/CoreServices.hpp"
#include "protobuf/core/Numbers.hpp"
#include "protobuf/core/ast.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "protobuf/generated/Protobuf/Module.hpp"

namespace {

namespace ast = protobuf::ast;
namespace fs = std::filesystem;

const cursiv::Parser &engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      protobuf::generated::Protobuf::create_parser();
  return *p;
}

bool is_clean(const cursiv::ParseResult &r) {
  return r.accepted && r.diagnostics.empty() && !r.cst().recovered();
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

// A keyword doubles as a name wherever the state reads one, spelled once.
void soft_keyword_message_name() {
  const cursiv::ParseResult r =
      build("syntax = \"proto3\";\nmessage message { string body = 1; }\n");
  ast::Document *const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->messages.size() == 1);
  if (doc == nullptr || doc->messages.size() != 1)
    return;
  CHECK(doc->messages[0]->name == "message");
  CHECK(doc->messages[0]->fields.size() == 1);
  if (doc->messages[0]->fields.size() == 1)
    CHECK(doc->messages[0]->fields[0]->name == "body");
}

// `true`/`false` read as the boolean literal in a value position, and as a
// plain field name elsewhere -- a soft keyword, spelled with a local guard
// rather than a reservation.
void true_false_guard() {
  const cursiv::ParseResult r1 =
      build("syntax = \"proto3\";\noption allow = true;\n");
  ast::Document *const doc1 = r1.value_as<ast::Document>();
  CHECK(doc1 != nullptr && doc1->options.size() == 1);
  if (doc1 != nullptr && doc1->options.size() == 1)
    CHECK(std::holds_alternative<bool>(doc1->options[0]->value) &&
          std::get<bool>(doc1->options[0]->value));

  const cursiv::ParseResult r2 =
      build("syntax = \"proto3\";\nmessage M { bool true = 1; }\n");
  ast::Document *const doc2 = r2.value_as<ast::Document>();
  CHECK(doc2 != nullptr && doc2->messages.size() == 1);
  if (doc2 != nullptr && doc2->messages.size() == 1 &&
      doc2->messages[0]->fields.size() == 1)
    CHECK(doc2->messages[0]->fields[0]->name == "true");
}

// Hex and octal field numbers, a leading-dot qualified type, `import weak`,
// `extend`, and `reserved N to max` -- constructs the shipped corpus never
// exercises.
void gap_constructs() {
  const std::string src = cursiv::test::read_file(
      (fs::path(CURSIV_TEST_DIR) / "gap_constructs.proto").string());
  const cursiv::ParseResult r = build(src);
  CHECK(is_clean(r));
  ast::Document *const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  CHECK(doc->imports.size() == 1 && doc->imports[0]->isWeak);
  CHECK(doc->extends_.size() == 1 &&
        doc->extends_[0]->target == "google.protobuf.FileOptions");
  CHECK(doc->messages.size() == 2);
  if (doc->messages.size() == 2) {
    ast::MessageDef *const widget = doc->messages[1];
    CHECK(widget->name == "Widget");
    CHECK(widget->fields.size() == 2);
    if (widget->fields.size() == 2) {
      CHECK(widget->fields[0]->type == ".Outer.Inner");
      CHECK(protobuf::integer_value(widget->fields[1]->number) == 0x2A);
    }
    CHECK(widget->reserved.size() == 1 &&
          widget->reserved[0]->ranges.size() == 1);
    if (widget->reserved.size() == 1 && widget->reserved[0]->ranges.size() == 1)
      CHECK(widget->reserved[0]->ranges[0]->toMax);
  }
}

// Real field values read back from one corpus document, so a converter that
// silently mismaps its value does not pass a structural check alone.
void field_values() {
  const fs::path dir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  const std::string src =
      cursiv::test::read_file((dir / "example.proto").string());
  const cursiv::ParseResult r = build(src);
  ast::Document *const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;

  CHECK(doc->enums.size() == 1);
  if (doc->enums.size() == 1) {
    CHECK(doc->enums[0]->name == "EnumAllowingAlias");
    CHECK(doc->enums[0]->fields.size() == 3);
    if (doc->enums[0]->fields.size() == 3) {
      CHECK(doc->enums[0]->fields[1]->name == "STARTED");
      CHECK(protobuf::integer_value(doc->enums[0]->fields[1]->value) == 1);
    }
  }

  CHECK(doc->messages.size() == 2);
  if (doc->messages.size() != 2)
    return;
  ast::MessageDef *const outer = doc->messages[0];
  CHECK(outer->name == "outer");
  CHECK(outer->messages.size() == 1 && outer->messages[0]->name == "inner");
  CHECK(outer->options.size() == 1);
  if (outer->options.size() == 1) {
    CHECK(outer->options[0]->name->isExtension);
    CHECK(outer->options[0]->name->name == "my_option");
    CHECK(outer->options[0]->name->suffix.has_value() &&
          *outer->options[0]->name->suffix == "a");
    CHECK(std::holds_alternative<bool>(outer->options[0]->value) &&
          std::get<bool>(outer->options[0]->value));
  }
  CHECK(outer->fields.size() == 3);
  if (outer->fields.size() == 3) {
    CHECK(outer->fields[0]->name == "inner_message" &&
          outer->fields[0]->type == "inner");
    CHECK(outer->fields[0]->label.has_value() &&
          *outer->fields[0]->label == ast::FieldLabel::repeated_);
    CHECK(outer->fields[2]->name == "optional_field" &&
          protobuf::integer_value(outer->fields[2]->number) == 5);
    CHECK(outer->fields[2]->label.has_value() &&
          *outer->fields[2]->label == ast::FieldLabel::optional_);
  }
  CHECK(outer->mapFields.size() == 1);
  if (outer->mapFields.size() == 1) {
    CHECK(outer->mapFields[0]->keyType == "int32");
    CHECK(outer->mapFields[0]->valueType == "string");
    CHECK(outer->mapFields[0]->name == "my_map");
  }

  ast::MessageDef *const foo = doc->messages[1];
  CHECK(foo->name == "Foo");
  CHECK(foo->reserved.size() == 2);
  if (foo->reserved.size() == 2) {
    CHECK(foo->reserved[0]->ranges.size() == 3);
    if (foo->reserved[0]->ranges.size() == 3) {
      CHECK(protobuf::integer_value(foo->reserved[0]->ranges[0]->from) == 2);
      CHECK(protobuf::integer_value(foo->reserved[0]->ranges[2]->from) == 9);
      CHECK(foo->reserved[0]->ranges[2]->to.has_value() &&
            protobuf::integer_value(*foo->reserved[0]->ranges[2]->to) == 11);
    }
    CHECK(foo->reserved[1]->fieldNames.size() == 2);
    if (foo->reserved[1]->fieldNames.size() == 2) {
      CHECK(foo->reserved[1]->fieldNames[0] == "foo");
      CHECK(foo->reserved[1]->fieldNames[1] == "bar");
    }
  }

  CHECK(doc->services.size() == 1);
  if (doc->services.size() == 1) {
    CHECK(doc->services[0]->name == "SearchService");
    CHECK(doc->services[0]->rpcs.size() == 1);
    if (doc->services[0]->rpcs.size() == 1) {
      CHECK(doc->services[0]->rpcs[0]->name == "Search");
      CHECK(doc->services[0]->rpcs[0]->requestType == "SearchRequest");
      CHECK(doc->services[0]->rpcs[0]->responseType == "SearchResponse");
    }
  }
}

// A field option's quoted pattern, hex/octal escapes decoded.
void field_option_unescape() {
  const fs::path dir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  const std::string src =
      cursiv::test::read_file((dir / "helloworldreserved.proto").string());
  const cursiv::ParseResult r = build(src);
  ast::Document *const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->messages.size() == 2);
  if (doc == nullptr || doc->messages.size() != 2)
    return;
  ast::MessageDef *const req = doc->messages[0];
  CHECK(req->name == "HelloRequest");
  CHECK(req->fields.size() == 1 && req->fields[0]->options.size() == 1);
  if (req->fields.size() == 1 && req->fields[0]->options.size() == 1) {
    ast::FieldOption *const opt = req->fields[0]->options[0];
    CHECK(opt->name->name == "validate.rules");
    CHECK(opt->name->suffix.has_value() &&
          *opt->name->suffix == "string.pattern");
    CHECK(std::holds_alternative<std::string_view>(opt->value) &&
          std::get<std::string_view>(opt->value) == "^\\w+( +\\w+)*$");
  }
}

void literal_and_aggregate_values() {
  const std::string source = R"proto(syntax="pro" /* version */ "to3";
option java_package = "\1Z\12Q\x4G" /* 'ignored' */ "tail";
option java_outer_classname = "\u0041\U0001F600\uD83D\uDE00\777Z";
enum E { reserved - /* low */ 3 to - 1; Z=0; }
option (cfg) = { names: ["a", "b"]; child < count: - /* sign */ 1 >, count: 2 };
)proto";
  const auto r = build(source);
  CHECK(is_clean(r));
  const auto *doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->options.size() == 3 && doc->enums.size() == 1);
  if (doc == nullptr || doc->options.size() != 3 || doc->enums.size() != 1)
    return;
  CHECK(doc->syntax == "proto3");
  const auto *text = std::get_if<std::string_view>(&doc->options[0]->value);
  CHECK(text != nullptr && *text == std::string_view("\1Z\12Q\4Gtail"));
  const auto *unicode = std::get_if<std::string_view>(&doc->options[1]->value);
  CHECK(unicode != nullptr && *unicode == "A😀😀\xffZ");
  CHECK(doc->enums[0]->reserved.size() == 1);
  if (doc->enums[0]->reserved.size() == 1) {
    const auto &ranges = doc->enums[0]->reserved[0]->ranges;
    CHECK(ranges.size() == 1);
    if (ranges.size() == 1)
      CHECK(protobuf::integer_value(ranges[0]->from) == -3 && ranges[0]->to &&
            protobuf::integer_value(*ranges[0]->to) == -1);
  }
  const auto *aggregate =
      std::get_if<ast::MessageValue *>(&doc->options[2]->value);
  CHECK(aggregate != nullptr && *aggregate != nullptr &&
        (*aggregate)->fields.size() == 3);
  if (aggregate == nullptr || *aggregate == nullptr ||
      (*aggregate)->fields.size() != 3)
    return;
  const auto *names =
      std::get_if<ast::MessageListValue *>(&(*aggregate)->fields[0]->value);
  CHECK(names != nullptr && *names != nullptr && (*names)->items.size() == 2);
  if (names != nullptr && *names != nullptr && (*names)->items.size() == 2)
    CHECK(std::get<std::string_view>((*names)->items[1]) == "b");
  const auto *child =
      std::get_if<ast::MessageValue *>(&(*aggregate)->fields[1]->value);
  CHECK(child != nullptr && *child != nullptr && (*child)->fields.size() == 1);
  if (child != nullptr && *child != nullptr && (*child)->fields.size() == 1)
    CHECK(
        protobuf::integer_value(
            std::get<ast::NumberLiteral *>((*child)->fields[0]->value)->text) ==
        -1);
}

void integer_constant_range() {
  const std::string source = R"proto(syntax="proto3";
option (maximum) = 18446744073709551615;
option (hexadecimal) = + /* sign */ 0xffffffffffffffff;
option (minimum) = - /* sign */ 9223372036854775808;
option (small) = 42;
)proto";
  const auto r = build(source);
  CHECK(is_clean(r));
  const auto *doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->options.size() == 4);
  if (doc == nullptr || doc->options.size() != 4)
    return;
  const auto number = [&](unsigned i) {
    const auto *node = std::get<ast::NumberLiteral *>(doc->options[i]->value);
    return protobuf::number_value(node->text, node->floating);
  };
  CHECK(std::get<std::uint64_t>(*number(0)) == UINT64_MAX);
  CHECK(std::get<std::uint64_t>(*number(1)) == UINT64_MAX);
  CHECK(std::get<std::int64_t>(*number(2)) == INT64_MIN);
  CHECK(std::get<std::uint64_t>(*number(3)) == 42);
}

void corpus() {
  const fs::path dir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  std::vector<fs::path> files;
  for (const fs::directory_entry &e : fs::recursive_directory_iterator(dir))
    if (e.is_regular_file() && e.path().extension() == ".proto")
      files.push_back(e.path());
  CHECK(files.size() == 29);
  std::sort(files.begin(), files.end());

  const std::vector<std::string> expected_error = {"descriptor.proto",
                                                   "5.v2.proto"};

  int clean_count = 0;
  for (const fs::path &file : files) {
    const std::string name = file.filename().string();
    const std::string src = cursiv::test::read_file(file.string());
    const cursiv::ParseResult r = engine().parse(src);
    const bool clean = is_clean(r);

    const bool wants_error =
        std::find(expected_error.begin(), expected_error.end(), name) !=
        expected_error.end();

    if (wants_error) {
      CHECK(!clean);
      if (clean)
        std::printf("FAIL expected a diagnostic, got a clean parse: %s\n",
                    file.string().c_str());
    } else {
      CHECK(clean);
      if (!clean)
        std::printf("FAIL corpus file did not parse clean: %s\n",
                    file.string().c_str());
      else
        ++clean_count;
    }
  }
  CHECK(clean_count == 27);
}

} // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<protobuf::ProtobufCoreServices,
                              protobuf::ProtobufSharedCoreServices,
                              protobuf::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "syntax=\"proto3\"; message M { string s = 1 [deprecated = true]; "
        "repeated int32 n = 2 [packed = true]; } enum E { Z = 0 [deprecated = "
        "true]; }");
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
  soft_keyword_message_name();
  true_false_guard();
  gap_constructs();
  field_values();
  field_option_unescape();
  literal_and_aggregate_values();
  integer_constant_range();
  corpus();
  return cursiv::test::report("protobuf_grammar_test");
}
