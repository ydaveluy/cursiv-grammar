// muddb end to end: both corpus dumps parse clean, and the first room of each
// holds the fields its text declares -- including the blank lines that read
// back as absent rather than as an empty string or a zero. Each check keeps
// its source text alive alongside the ParseResult it feeds: the AST borrows
// the document, so the text must outlive every read of it.

#include <memory>
#include <string>

#include "cursiv/test/Check.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "muddb/core/ast.hpp"
#include "muddb/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "native_cases.hpp"
#include "muddb/generated/Muddb/Module.hpp"

namespace {

namespace ast = muddb::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      muddb::generated::Muddb::create_parser();
  return *p;
}

std::string read_example(const char* name) {
  std::string text =
      cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
  CHECK(!text.empty());
  return text;
}

void every_example_parses_clean() {
  static constexpr const char* files[] = {"minimal.muddb", "small.muddb"};
  for (const char* name : files) {
    const std::string text = read_example(name);
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted && !r.cst().recovered());
    CHECK(r.value_as<ast::Db>() != nullptr);
  }
}

void minimal_dump_holds_its_two_rooms() {
  const std::string text = read_example("minimal.muddb");
  cursiv::ParseResult r = engine().parse(text);
  ast::Db* db = r.value_as<ast::Db>();
  CHECK(db != nullptr && db->rooms.size() == 2);
  if (db == nullptr || db->rooms.size() != 2) return;

  const ast::Room& zero = *db->rooms[0];
  CHECK(zero.roomno == "#0");
  CHECK(zero.name.has_value() && *zero.name == "Room Zero");
  CHECK(zero.location.has_value() && *zero.location == "-1");
  CHECK(zero.description.has_value() &&
        *zero.description == "You are in Room Zero.  It's very dark here.");
  CHECK(!zero.password.has_value());

  const ast::Room& one = *db->rooms[1];
  CHECK(one.roomno == "#1");
  CHECK(one.location.has_value() && *one.location == "0");
  CHECK(one.password.has_value() && *one.password == "potrzebie");
}

// Fields left blank (nothing before the EOL) read back as absent, not as an
// empty string or a zero -- the three message fields of room #0 are blank
// in the source, while its numeric fields (including a real "-1") are all
// present.
void blank_fields_read_as_absent() {
  const std::string text = read_example("minimal.muddb");
  cursiv::ParseResult r = engine().parse(text);
  ast::Db* db = r.value_as<ast::Db>();
  CHECK(db != nullptr && !db->rooms.empty());
  if (db == nullptr || db->rooms.empty()) return;

  const ast::Room& zero = *db->rooms[0];
  CHECK(zero.exits.has_value() && *zero.exits == "-1");
  CHECK(zero.next_room.has_value() && *zero.next_room == "-1");
  const auto* key = cursiv::ast_cast<ast::LegacyKey>(zero.key);
  CHECK(key && key->text == "-1");
  CHECK(!zero.fail_message.has_value());
  CHECK(!zero.succ_message.has_value());
  CHECK(!zero.ofail.has_value());
  CHECK(zero.osuccess.has_value() &&
        *zero.osuccess == "is briefly visible through the mist.");
}

void an_asterisk_after_the_record_is_not_an_extra_field() {
  const std::string original = read_example("minimal.muddb");
  std::string source = original;
  source.insert(source.find("***END OF DUMP***"), "*not an end marker\n");
  const auto result = engine().parse(source);
  CHECK(!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty());
}

void broken_documents_are_refused() {
  const std::string text = "not a room dump at all\n";
  const cursiv::ParseResult r = engine().parse(text);
  CHECK(!r.accepted || r.cst().recovered());
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<muddb::MuddbCoreServices, muddb::MuddbSharedCoreServices, muddb::install_core_module>();
  CHECK(native_regressions::run(language) == 0);
  every_example_parses_clean();
  minimal_dump_holds_its_two_rooms();
  blank_fields_read_as_absent();
  an_asterisk_after_the_record_is_not_an_extra_field();
  broken_documents_are_refused();
  return cursiv::test::report("muddb_grammar_test");
}
