#include "audit_regressions.hpp"
#include "value_regressions.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Document at the root. One
// document's values are also read back, so a converter that silently drops
// or mis-converts a value would still fail this test.

#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "toml/core/CoreServices.hpp"
#include "toml/core/ast.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = toml::ast;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "toml: %s: %s\n", file.string().c_str(), what);
  return 1;
}

const ast::KeyValue* find(const ast::Table& table, std::string_view key) {
  for (const ast::KeyValue* const kv : table.entries)
    if (kv != nullptr && kv->key != nullptr && kv->key->parts.size() == 1 && kv->key->parts[0] == key) return kv;
  return nullptr;
}

const ast::Table* find(const ast::Document& doc, std::string_view key) {
  for (const ast::Table* const t : doc.tables)
    if (t != nullptr && t->key != nullptr && t->key->parts.size() == 1 && t->key->parts[0] == key) return t;
  return nullptr;
}

// The values `test.toml` declares, read back through the AST rather than
// merely trusted to have parsed: a converter that silently yields the wrong
// value, or a converter that refuses, leaves the structural checks above
// none the wiser.
int check_test_toml_values(const fs::path& path,
                           const cursiv::ParseResult& result) {
  const auto& doc = *static_cast<const ast::Document*>(result.value);

  const ast::Table* const root = doc.tables.empty() ? nullptr : doc.tables[0];
  if (root == nullptr) return fail(path, "no root table");
  const ast::KeyValue* const title = find(*root, "title");
  if (title == nullptr ||
      std::get<std::string_view>(title->value) != "TOML Example")
    return fail(path, "title did not read back");

  const ast::Table* const owner = find(doc, "owner");
  if (owner == nullptr) return fail(path, "no [owner] table");
  const ast::KeyValue* const name = find(*owner, "name");
  if (name == nullptr ||
      std::get<std::string_view>(name->value) != "Tom Preston-Werner")
    return fail(path, "owner.name did not read back");
  const ast::KeyValue* const dob = find(*owner, "dob");
  if (dob == nullptr || !std::holds_alternative<ast::DateTime>(dob->value) ||
      std::get<ast::DateTime>(dob->value).kind !=
          ast::DateTimeKind::OffsetDateTime)
    return fail(path, "owner.dob did not read back as an offset date-time");

  const ast::Table* const database = find(doc, "database");
  if (database == nullptr) return fail(path, "no [database] table");
  const ast::KeyValue* const enabled = find(*database, "enabled");
  if (enabled == nullptr || std::get<bool>(enabled->value) != true)
    return fail(path, "database.enabled did not read back");
  const ast::KeyValue* const max = find(*database, "connection_max");
  if (max == nullptr || std::get<std::int64_t>(max->value) != 5000)
    return fail(path, "database.connection_max did not read back");
  const ast::KeyValue* const ports = find(*database, "ports");
  if (ports == nullptr || !std::holds_alternative<ast::Array*>(ports->value))
    return fail(path, "database.ports is not an array");
  const ast::Array& ports_array = *std::get<ast::Array*>(ports->value);
  if (ports_array.items.size() != 3 ||
      std::get<std::int64_t>(ports_array.items[0]) != 8001 ||
      std::get<std::int64_t>(ports_array.items[2]) != 8002)
    return fail(path, "database.ports elements did not read back");

  return 0;
}

int check_extra_toml_values(const fs::path& path,
                            const cursiv::ParseResult& result) {
  const auto& doc = *static_cast<const ast::Document*>(result.value);

  const ast::Table* const numbers = find(doc, "numbers");
  if (numbers == nullptr) return fail(path, "no [numbers] table");
  const ast::KeyValue* const hex1 = find(*numbers, "hex1");
  if (hex1 == nullptr ||
      std::get<std::int64_t>(hex1->value) != 0xDEADBEEF)
    return fail(path, "numbers.hex1 did not read back");
  const ast::KeyValue* const oct2 = find(*numbers, "oct2");
  if (oct2 == nullptr || std::get<std::int64_t>(oct2->value) != 0755)
    return fail(path, "numbers.oct2 did not read back");
  const ast::KeyValue* const bin1 = find(*numbers, "bin1");
  if (bin1 == nullptr || std::get<std::int64_t>(bin1->value) != 0xD6)
    return fail(path, "numbers.bin1 did not read back");

  const ast::Table* const floats = find(doc, "special-floats");
  if (floats == nullptr) return fail(path, "no [special-floats] table");
  const ast::KeyValue* const plus_inf = find(*floats, "plus_inf");
  if (plus_inf == nullptr || std::get<double>(plus_inf->value) <= 0)
    return fail(path, "special-floats.plus_inf did not read back");
  const ast::KeyValue* const minus_inf = find(*floats, "minus_inf");
  if (minus_inf == nullptr || std::get<double>(minus_inf->value) >= 0)
    return fail(path, "special-floats.minus_inf did not read back");

  const ast::Table* const dates = find(doc, "dates");
  if (dates == nullptr) return fail(path, "no [dates] table");
  const ast::KeyValue* const ld = find(*dates, "local_date");
  if (ld == nullptr || !std::holds_alternative<ast::DateTime>(ld->value) ||
      std::get<ast::DateTime>(ld->value).kind != ast::DateTimeKind::LocalDate)
    return fail(path, "dates.local_date did not read back");

  return 0;
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !value_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      toml::TomlCoreServices, toml::TomlSharedCoreServices,
      toml::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".toml")
        continue;
      ++count;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file) {
        failures += fail(path, "not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(path, "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(path, "the document carries diagnostics");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Document>) {
        failures += fail(path, "the parse built no Document");
        continue;
      }

      if (path.filename() == "test.toml")
        failures += check_test_toml_values(path, result);
      if (path.filename() == "extra.toml")
        failures += check_extra_toml_values(path, result);
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "toml: no .toml file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("toml: %d example documents parse clean\n", count);
  return 0;
}
