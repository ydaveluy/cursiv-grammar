#pragma once

#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include <vector>
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "yini/core/CoreServices.hpp"
#include "yini/core/Literals.hpp"
#include "yini/core/ast.hpp"

namespace native_regressions {
inline void run() {
  namespace a = yini::ast;
  const auto language = cursiv::create_language<yini::YiniCoreServices,
      yini::YiniSharedCoreServices, yini::install_core_module>();
  const auto noErrors = [&](const std::string& text) {
    const auto validation = cursiv::test::validationHelper(language.language, text);
    for (const auto& d : validation.diagnostics)
      if (d.severity == cursiv::DiagnosticSeverity::Error) return false;
    return true;
  };
  for (const std::string literal : {"9223372036854775808", "-9223372036854775808",
      "18446744073709551616", "9007199254740993", "1_000", "hex:_FF_AA",
      "0b_1010_1100", "%_1010", "0o_755_644", "0z_2E_X9", "0z2AX9",
      "1.23_45e+2_0", "1e-9999", "0e99999"}) {
    const std::string text = "value=" + literal + "\n";
    const auto parsed = language.language.parser->parse(text);
    const auto* doc = cursiv::ast_cast<a::Document>(parsed.value);
    const auto* member = doc && doc->entries.size() == 1 ? cursiv::ast_cast<a::Member>(doc->entries[0]) : nullptr;
    const auto* value = member ? std::get_if<a::Value*>(&member->value) : nullptr;
    const auto* number = value ? cursiv::ast_cast<a::Number>(*value) : nullptr;
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
    CHECK(number && number->literal == literal);
    CHECK(noErrors(text));
  }
  for (const std::string literal : {std::string(70, '9'), "0z" + std::string(65, 'B')}) {
    const std::string text = "value=" + literal + "\n";
    const auto parsed = language.language.parser->parse(text);
    const auto* doc = cursiv::ast_cast<a::Document>(parsed.value);
    const auto* member = doc && doc->entries.size() == 1 ? cursiv::ast_cast<a::Member>(doc->entries[0]) : nullptr;
    const auto* value = member ? std::get_if<a::Value*>(&member->value) : nullptr;
    const auto* number = value ? cursiv::ast_cast<a::Number>(*value) : nullptr;
    CHECK(parsed.accepted && !parsed.cst().recovered() && number && number->literal == literal);
  }
  CHECK(!yini::valid_number(std::string(10000, '9')));
  CHECK(!yini::valid_number("0z" + std::string(10000, 'B')));
  struct StringCase { const char* literal; std::string value; };
  for (const StringCase c : {StringCase{R"(C"\xFF\u00E9\U0001F642\o377")", "ÿé🙂ÿ"},
      {R"(C"\o1\o12\o123")", std::string("\1\12S", 3)},
      {R"(C"a\0b")", std::string("a\0b", 3)},
      {R"(R"raw\n")", "raw\\n"}, {"\"\"\"a\r\nb\rc\"\"\"", "a\nb\nc"}}) {
    const auto decoded = yini::decode_string(c.literal);
    CHECK(decoded.error.empty() && decoded.value == c.value);
    const auto text = std::string("value=") + c.literal + "\n";
    const auto parsed = language.language.parser->parse(text);
    CHECK(parsed.accepted && !parsed.cst().recovered() && noErrors(text));
  }
  {
    const std::string text = "\xef\xbb\xbf> Root\nvalue=\"port-\" +\n8080 + true + null";
    const auto parsed = language.language.parser->parse(text);
    const auto* doc = cursiv::ast_cast<a::Document>(parsed.value);
    CHECK(parsed.accepted && !parsed.cst().recovered() && doc && doc->bom && noErrors(text));
    const auto* section = doc && doc->entries.size() == 2 ? cursiv::ast_cast<a::Section>(doc->entries[0]) : nullptr;
    const auto* member = doc && doc->entries.size() == 2 ? cursiv::ast_cast<a::Member>(doc->entries[1]) : nullptr;
    const auto* value = member ? std::get_if<a::Value*>(&member->value) : nullptr;
    const auto* concat = value ? cursiv::ast_cast<a::Concat>(*value) : nullptr;
    CHECK(section && section->markerText == ">" && section->name == "Root");
    CHECK(concat && concat->rest.size() == 3);
  }
  {
    const std::string text = "a=1 #abc\nb=#abc\n";
    const auto parsed = language.language.parser->parse(text);
    const auto* doc = cursiv::ast_cast<a::Document>(parsed.value);
    const auto* a = doc && doc->entries.size() == 2 ? cursiv::ast_cast<a::Member>(doc->entries[0]) : nullptr;
    const auto* b = doc && doc->entries.size() == 2 ? cursiv::ast_cast<a::Member>(doc->entries[1]) : nullptr;
    CHECK(a && a->hasValue && b && !b->hasValue && noErrors(text));
  }
  for (const auto& [text, code] : {
      std::pair{R"(value=C"\uD800")" "\n", "yini.string"},
      {R"(value=C"\U00110000")" "\n", "yini.string"},
      {R"(value=C"\o400")" "\n", "yini.string"},
      {"value=1e9999\n", "yini.number"}, {"1bad=1\n", "yini.key"},
      {"^^ A\n", "yini.section-depth"}, {"^256 A\n", "yini.section-marker"},
      {"a=1\n/END\nb=2\n", "yini.end"},
      {"@yini strict\n^ A\nx=[1,]\n/END\n", "yini.trailing-comma"},
      {"@yini strict\n^ A\nx={a=1}\n/END\n", "yini.object-separator"},
      {"@yini strict\n^ A\nx=\"a\"+1\n/END\n", "yini.concat"}}) {
    const auto parsed = language.language.parser->parse(text);
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
    const auto validation = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& d : validation.diagnostics)
      if (d.code && d.code->isString() && d.code->string() == code) found = true;
    CHECK(found);
  }
}
}  // namespace native_regressions
