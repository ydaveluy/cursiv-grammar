#include "native_regressions.hpp"

#include <cstdint>
#include <cstdlib>
#include <set>
#include <vector>
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "yini/core/CoreServices.hpp"
#include "yini/core/Literals.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "yini/core/ast.hpp"
#include "yini/generated/Yini/Module.hpp"

namespace {

namespace ast = yini::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      yini::generated::Yini::create_parser();
  return *p;
}

cursiv::ParseResult parse(const std::string& text) {
  return engine().parse(text);
}

struct SectionView {
  bool hasHeader = false;
  std::string_view markerText, name;
  std::vector<const ast::Member*> members;
};
struct DocumentView {
  bool shebang = false;
  std::vector<std::unique_ptr<SectionView>> sections;
};
std::unique_ptr<DocumentView> document(cursiv::ParseResult& r) {
  const auto* root = cursiv::ast_cast<ast::Document>(r.value);
  if (!root) return {};
  auto view = std::make_unique<DocumentView>();
  view->sections.push_back(std::make_unique<SectionView>());
  for (auto* entry : root->entries) {
    if (cursiv::ast_cast<ast::Shebang>(entry)) view->shebang = true;
    if (auto* section = cursiv::ast_cast<ast::Section>(entry)) {
      auto current = std::make_unique<SectionView>();
      current->hasHeader = true; current->name = section->name; current->markerText = section->markerText;
      view->sections.push_back(std::move(current));
    }
    if (auto* member = cursiv::ast_cast<ast::Member>(entry)) view->sections.back()->members.push_back(member);
  }
  return view;
}
std::string str(const ast::Val& v) {
  const auto* value = std::get_if<ast::Value*>(&v);
  if (!value) return "<not a string>";
  if (const auto* text = cursiv::ast_cast<ast::String>(*value)) return yini::decode_string(text->literal).value;
  if (const auto* concat = cursiv::ast_cast<ast::Concat>(*value)) {
    auto result = str(concat->first);
    for (const auto& part : concat->rest) result += str(part);
    return result;
  }
  return "<not a string>";
}
std::int64_t integer(const ast::Val& v) {
  const auto* value = std::get_if<ast::Value*>(&v);
  const auto* number = value ? cursiv::ast_cast<ast::Number>(*value) : nullptr;
  if (!number) return -1;
  std::string text(number->literal);
  if (text.starts_with("0z")) {
    std::int64_t result = 0;
    for (char c : text.substr(2)) result = result * 12 + (c == 'E' ? 11 : c - '0');
    return result;
  }
  if (text.starts_with('%')) return std::strtoll(text.c_str() + 1, nullptr, 2);
  if (text.starts_with("0b")) return std::strtoll(text.c_str() + 2, nullptr, 2);
  if (text.starts_with("0o")) return std::strtoll(text.c_str() + 2, nullptr, 8);
  return std::strtoll(text.c_str(), nullptr, 0);
}
bool boolean(const ast::Val& v, bool want) {
  return std::holds_alternative<bool>(v) && std::get<bool>(v) == want;
}
bool is_null(const ast::Val& v) {
  return std::holds_alternative<std::monostate>(v);
}

const ast::Member* find_member(SectionView* s, std::string_view key) {
  if (s == nullptr) return nullptr;
  for (const ast::Member* const m : s->members)
    if (m != nullptr && m->key == key) return m;
  return nullptr;
}
SectionView* find_section(DocumentView* d, std::string_view name) {
  if (d == nullptr) return nullptr;
  for (const auto& s : d->sections)
    if (s != nullptr && s->hasHeader && s->name == name) return s.get();
  return nullptr;
}

void corpus_parses_clean() {
  const auto language = cursiv::create_language<yini::YiniCoreServices,
      yini::YiniSharedCoreServices, yini::install_core_module>();
  const std::set<std::string> nativeInvalid = {
      "example/AppConfig.yini",
      "example/Basic Configuration.yini",
      "example/Compact.yini",
      "example/Complex-Structure-with-Multiple-Sections.yini",
      "example/Lists.yini",
      "example/Package-Example.yini",
      "example/String-Literals-with-Different-Types.yini",
      "example/Strings.yini",
      "example/myPackage-2.yini",
      "tests/framework-coverage.yini",
      "tests/invalid-key.yini",
      "tests/only-in-strict-mode/Package-Example.yini",
  };
  int seen = 0;
  for (const auto& [dir, prefix] : {std::pair{fs::path(CURSIV_EXAMPLE_DIR) / "example", "example/"},
                                  std::pair{fs::path(CURSIV_TEST_DIR), "tests/"}}) {
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".yini") continue;
      ++seen;
      const auto text = cursiv::test::read_file(entry.path().string());
      const auto parsed = language.language.parser->parse(text);
      CHECK(parsed.cst().text() == text);
      CHECK(cursiv::ast_cast<ast::Document>(parsed.value) != nullptr);
      const auto validation = cursiv::test::validationHelper(language.language, text);
      bool errors = false;
      for (const auto& d : validation.diagnostics)
        errors |= d.severity == cursiv::DiagnosticSeverity::Error;
      const auto relative = std::string(prefix) + fs::relative(entry.path(), dir).generic_string();
      if (nativeInvalid.contains(relative)) CHECK(errors);
      else {
        CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
        CHECK(!errors);
      }
    }
  }
  CHECK(seen == 44);
}

void invalid_key_names_the_defect() {
  const std::string text = cursiv::test::read_file(CURSIV_INVALID_KEY);
  const auto language = cursiv::create_language<yini::YiniCoreServices,
      yini::YiniSharedCoreServices, yini::install_core_module>();
  const auto r = language.language.parser->parse(text);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  const auto validation = cursiv::test::validationHelper(language.language, text);
  CHECK(validation.diagnostics.size() == 1);
  if (validation.diagnostics.size() == 1) {
    const auto& diagnostic = validation.diagnostics[0];
    CHECK(diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == "yini.key");
  }
}

void framework_coverage_values() {
  const std::string text = cursiv::test::read_file(
      (fs::path(CURSIV_TEST_DIR) / "framework-coverage.yini").string());
  cursiv::ParseResult r = parse(text);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  auto d = document(r);
  CHECK(d != nullptr);
  if (d == nullptr) return;

  CHECK(d->shebang);

  SectionView* root = d->sections.empty() ? nullptr : d->sections[0].get();
  CHECK(root != nullptr && !root->hasHeader);
  CHECK(find_member(root, "name") != nullptr &&
        str(find_member(root, "name")->value) == "Root Table");
  CHECK(find_member(root, "build") != nullptr &&
        integer(find_member(root, "build")->value) == 42);

  SectionView* bases = find_section(d.get(), "Bases");
  CHECK(bases != nullptr);
  CHECK(integer(find_member(bases, "decimal")->value) == 42);
  CHECK(integer(find_member(bases, "negative")->value) == -7);
  CHECK(is_null(find_member(bases, "hexHash")->value));
  CHECK(integer(find_member(bases, "hexPrefix")->value) == 0x1F);
  CHECK(integer(find_member(bases, "binPercent")->value) == 0b1010);
  CHECK(integer(find_member(bases, "binPrefix")->value) == 0b1010);
  CHECK(integer(find_member(bases, "octal")->value) == 017);
  CHECK(integer(find_member(bases, "duodecimal")->value) ==
        1 * 12 + 11);  // "1E" base 12, E = 11

  SectionView* alt1 = find_section(d.get(), "Alternate");
  CHECK(alt1 != nullptr && alt1->markerText == "\xc2\xa7");
  CHECK(boolean(find_member(alt1, "euroMarker")->value, true));

  SectionView* alt2 = find_section(d.get(), "AnotherAlternate");
  CHECK(alt2 != nullptr && alt2->markerText == "\xe2\x82\xac");

  SectionView* strings = find_section(d.get(), "Strings");
  CHECK(strings != nullptr);
  CHECK(str(find_member(strings, "raw")->value) ==
        "No escapes here: \\n stays two chars.");
  CHECK(str(find_member(strings, "classic")->value) == "Line one\nLine two");
  CHECK(str(find_member(strings, "hyper")->value) ==
        "Padding collapses across lines too");
  CHECK(std::holds_alternative<ast::Value*>(
      find_member(strings, "empty")->value));

  SectionView* shorthand = find_section(d.get(), "ShorthandLevel");
  CHECK(shorthand != nullptr && shorthand->members.empty());
  SectionView* deep = find_section(d.get(), "DeepShorthand");
  CHECK(deep != nullptr && deep->markerText == "^7");
  CHECK(str(find_member(deep, "value")->value) == "reached level seven");
}

void real_documents_read_back() {
  {
    const std::string text = "^ String_Concatenation\n"
        "text1 = C\"The boat's name is \" + C\"\\\"Mare Stella\\\"\"\n"
        "text2 = C\"The boat's name is \" +\nC\"\\\"Mare Stella\\\"\"\n";
    cursiv::ParseResult r = parse(text);
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    auto d = document(r);
    CHECK(d != nullptr);
    if (d == nullptr) return;
    SectionView* concat = find_section(d.get(), "String_Concatenation");
    CHECK(concat != nullptr);
    if (concat != nullptr) {
      const std::string_view want = "The boat's name is \"Mare Stella\"";
      CHECK(str(find_member(concat, "text1")->value) == want);
      CHECK(str(find_member(concat, "text2")->value) == want);
    }
  }
  {
    const std::string text = cursiv::test::read_file(
        (fs::path(CURSIV_EXAMPLE_DIR) / "example" / "Lists.yini").string());
    cursiv::ParseResult r = parse(text);
    // The native reader rejects the legacy colon-list syntax in this fixture.
    CHECK(r.accepted && r.cst().text() == text);
    auto d = document(r);
    CHECK(d != nullptr);
    if (d == nullptr) return;
    SectionView* s = find_section(d.get(), "ListWithBrackets");
    CHECK(s != nullptr);
    if (s != nullptr) {
      const ast::Member* list1 = find_member(s, "list1");
      CHECK(list1 != nullptr &&
            std::holds_alternative<ast::Value*>(list1->value));
      auto* items = static_cast<ast::List*>(
          std::get<ast::Value*>(list1->value));
      CHECK(items != nullptr && items->items.size() == 4);
      if (items != nullptr && items->items.size() == 4)
        CHECK(str(items->items[0]) == "Orange" &&
              str(items->items[3]) == "Peach");

      const ast::Member* var2 = find_member(s, "var2");
      CHECK(var2 != nullptr && is_null(var2->value));
    }
  }
  {
    const std::string text = cursiv::test::read_file(
        (fs::path(CURSIV_EXAMPLE_DIR) / "example" / "Nested-Objects.yini")
            .string());
    cursiv::ParseResult r = parse(text);
    CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
    auto d = document(r);
    CHECK(d != nullptr);
    if (d == nullptr) return;
    SectionView* s = find_section(d.get(), "Application");
    CHECK(s != nullptr);
    const ast::Member* database = s != nullptr ? find_member(s, "database") : nullptr;
    CHECK(database != nullptr &&
          std::holds_alternative<ast::Value*>(database->value));
    if (database != nullptr &&
        std::holds_alternative<ast::Value*>(database->value)) {
      auto* obj =
          static_cast<ast::Object*>(std::get<ast::Value*>(database->value));
      CHECK(obj != nullptr && obj->members.size() == 5);
      if (obj != nullptr && obj->members.size() == 5) {
        CHECK(obj->members[0]->key == "host" &&
              str(obj->members[0]->value) == "db.example.com");
        CHECK(obj->members[1]->key == "port" &&
              integer(obj->members[1]->value) == 5432);
      }
    }
  }
}

}  // namespace

int main() {
  native_regressions::run();
  corpus_parses_clean();
  invalid_key_names_the_defect();
  framework_coverage_values();
  real_documents_read_back();
  return cursiv::test::report("yini_grammar_test");
}
