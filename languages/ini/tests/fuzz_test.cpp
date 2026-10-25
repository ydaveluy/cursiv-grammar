#include <cstdio>
#include <random>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "ini/core/CoreServices.hpp"

int main() {
  auto language = cursiv::create_language<ini::IniCoreServices,
      ini::IniSharedCoreServices, ini::install_core_module>();
  std::mt19937 random(0x5eed02);
  constexpr std::string_view values[] = {"001", "true", "bare text", "\"quoted\"", "#; punctuation", "é 你好", ""};
  constexpr std::string_view endings[] = {"\n", "\r", "\r\n"};
  for (int i = 0; i < 400; ++i) {
    const std::string eol(endings[random() % 3]);
    const std::string_view value = values[random() % 7];
    const std::string separator = random() % 2 ? " = " : ": ";
    std::string source = "# comment" + eol + "root" + separator + std::string(value) + eol;
    source += "[ a section ]" + eol + "multi" + separator + std::string(value) + eol + "  continuation" + eol;
    source += "  another=line" + eol + "flag" + eol + "empty=";
    if (random() % 2) source += eol;
    const auto parsed = language.language.parser->parse(source);
    const auto* doc = parsed.value_as<ini::ast::Document>();
    if (!doc || !parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty()) return 1;
    std::string rebuilt;
    for (const auto* line : doc->lines) { rebuilt += ini::source_text(*line); rebuilt += line->ending; }
    const auto result = language.language.reader->read(*doc);
    if (rebuilt != source || !result.issues.empty() || result.sections.size() != 2 || result.sections[0].entries.size() != 1 || result.sections[1].entries.size() != 3) return 1;
    const auto& options = result.sections[1].entries;
    if (result.sections[0].entries[0].value != value || options[0].value != std::string(value) + "\ncontinuation\nanother=line" || options[1].hasValue || !options[2].hasValue || !options[2].value.empty()) return 1;
  }
  std::puts("ini: 400 generated documents preserve text and grouped values");
}
