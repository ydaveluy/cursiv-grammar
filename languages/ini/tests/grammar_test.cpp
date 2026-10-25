#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "ini/core/CoreServices.hpp"
#include "ini/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

int main() {
  const auto language = cursiv::create_language<ini::IniCoreServices,
      ini::IniSharedCoreServices, ini::install_core_module>();
  int failures = 0;
  const auto require = [&](bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "ini: %s\n", message); ++failures; }
  };
  const auto inspect = [&](const std::string& source, auto check) {
    const auto parsed = language.language.parser->parse(source);
    const auto* document = parsed.template value_as<ini::ast::Document>();
    require(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && document,
            "physical INI lines must parse without repairs");
    if (!document) return;
    std::string spelling;
    for (const auto* line : document->lines) {
      require(line->parent == document, "line parent");
      spelling += ini::source_text(*line); spelling += line->ending;
    }
    require(spelling == source, "AST preserves source lines and line endings");
    check(language.language.reader->read(*document), *document);
  };
  inspect("# comment\n [a]suffix]tail\n  key = value\n\nflag", [&](const auto&, const auto& d) {
    require(d.lines.size() == 5, "typed source lines");
    if (d.lines.size() != 5) return;
    const auto* header = cursiv::ast_cast<ini::ast::SectionHeader>(d.lines[1]);
    const auto* option = cursiv::ast_cast<ini::ast::Option>(d.lines[2]);
    require(cursiv::ast_cast<ini::ast::Comment>(d.lines[0]) &&
            cursiv::ast_cast<ini::ast::Blank>(d.lines[3]), "comment and blank nodes");
    require(header && header->name == "a]suffix" && header->suffix == "tail" &&
            header->indentation == " ", "grammar owns header recognition");
    require(option && option->key == "key " && option->separator == "=" &&
            option->value == " value" && option->indentation == "  ",
            "grammar owns key, separator and value recognition");
  });
  inspect("root = 001\n[ service . name ]\nkey with spaces: bare value\nempty=\nflag\nquoted=\"text\"\ntruth=true\n", [&](const auto& r, const auto&) {
    require(r.issues.empty() && r.sections.size() == 2, "unnamed and named sections");
    if (r.sections.size() != 2) return;
    require(r.sections[0].entries[0].value == "001", "leading zeroes remain text");
    const auto& s = r.sections[1];
    require(s.name == " service . name " && s.entries.size() == 5, "section and option names");
    if (s.entries.size() != 5) return;
    require(s.entries[0].key == "key with spaces" && s.entries[0].value == "bare value", "colon separator");
    require(s.entries[1].hasValue && s.entries[1].value.empty(), "empty value");
    require(!s.entries[2].hasValue && s.entries[2].key == "flag", "valueless option");
    require(s.entries[3].value == "\"text\"" && s.entries[4].value == "true", "quotes and booleans remain text");
  });
  inspect("[main]\nx=first\n  second\n  [not a section]\n  embedded=value\n\n  last\n; ignored\nnext: value # literal ; literal\n", [&](const auto& r, const auto&) {
    require(r.issues.empty() && r.sections.size() == 1, "continuation lines stay in their section");
    if (r.sections.size() != 1 || r.sections[0].entries.size() != 2) { require(false, "two grouped options"); return; }
    require(r.sections[0].entries[0].value == "first\nsecond\n[not a section]\nembedded=value\n\nlast", "multiline values with a blank line");
    require(r.sections[0].entries[1].value == "value # literal ; literal", "inline punctuation remains value text");
  });
  inspect("x=\n  first\ny=last\n\n# trailing comment\n", [&](const auto& r, const auto&) {
    require(r.sections[0].entries[0].value == "\nfirst", "empty first line retained in multiline value");
    require(r.sections[0].entries[1].value == "last", "trailing blank value lines trimmed");
  });
  inspect("  first=one\n  second=two\n   continuation\nthird=three\n", [&](const auto& r, const auto&) {
    require(r.issues.empty() && r.sections[0].entries.size() == 3, "relative indentation decides continuation");
    require(r.sections[0].entries[1].value == "two\ncontinuation", "indented options may be siblings");
  });
  inspect("[a]suffix\r\nx=one\r;y comment\n[b # c]\ny:two", [&](const auto& r, const auto& d) {
    require(r.issues.empty() && r.sections.size() == 2 && r.sections[1].name == "b # c", "permissive header suffix and punctuation");
    require(d.lines.size() == 5 && d.lines[0]->ending == "\r\n" && d.lines[1]->ending == "\r", "mixed newline forms");
  });
  inspect("x=one\nx=two\n[a]\ny=1\n[a]\ny=2\n", [&](const auto& r, const auto&) {
    require(r.sections.size() == 3 && r.sections[0].entries.size() == 2, "duplicate names retained in source order");
  });
  inspect("\u00a0x\u2003=\u2003value\u00a0\n\u00a0\u2003more\n", [&](const auto& r, const auto&) {
    require(r.issues.empty() && r.sections[0].entries[0].key == "x" && r.sections[0].entries[0].value == "value\nmore", "Unicode whitespace and indentation");
  });
  inspect("flag\n  continuation\n=value\n", [&](const auto& r, const auto&) {
    require(r.issues.size() == 2, "runtime diagnoses valueless continuation and missing key");
  });
  inspect("", [&](const auto& r, const auto& d) { require(r.sections.empty() && d.lines.empty(), "empty document"); });
  const auto invalid = cursiv::test::validationHelper(language.language, "flag\n  continuation\n=value\n");
  const auto valid = cursiv::test::validationHelper(language.language, "x=bare value\n  continuation\n");
  require(invalid.diagnostics.size() == 2 && valid.diagnostics.empty(), "registered runtime validation checks");
  return failures != 0;
}
