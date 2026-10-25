#include "ini/core/IniReader.hpp"

#include <algorithm>
#include <memory>

#include "cursiv/core/text/scalar_at.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ini/core/CoreServices.hpp"

namespace ini {
namespace {

bool space(char32_t c) {
  return (c >= 9 && c <= 13) || (c >= 0x1c && c <= 0x20) ||
         c == 0x85 || c == 0xa0 || c == 0x1680 ||
         (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 ||
         c == 0x202f || c == 0x205f || c == 0x3000;
}

std::size_t leading(std::string_view text, std::size_t& count) {
  std::size_t offset = 0;
  count = 0;
  while (offset < text.size()) {
    std::size_t length = 0;
    const char32_t c = cursiv::scalar_at(text.data() + offset, text.size() - offset, length);
    if (!space(c)) break;
    offset += length;
    ++count;
  }
  return offset;
}

std::string_view rtrim(std::string_view text) {
  std::size_t last = 0;
  for (std::size_t offset = 0; offset < text.size();) {
    std::size_t length = 0;
    const char32_t c = cursiv::scalar_at(text.data() + offset, text.size() - offset, length);
    offset += std::max(length, std::size_t{1});
    if (!space(c)) last = offset;
  }
  return text.substr(0, last);
}

std::string_view trim(std::string_view text) {
  std::size_t ignored;
  text.remove_prefix(leading(text, ignored));
  return rtrim(text);
}

}  // namespace

std::string source_text(const ast::Line& line) {
  std::string text(line.indentation);
  if (const auto* h = cursiv::ast_cast<ast::SectionHeader>(&line)) {
    text += h->open; text += h->name; text += h->close; text += h->suffix;
  } else if (const auto* o = cursiv::ast_cast<ast::Option>(&line)) {
    text += o->key; text += o->separator; text += o->value;
  } else if (const auto* c = cursiv::ast_cast<ast::Comment>(&line)) {
    text += c->text;
  }
  return text;
}

ReadResult IniReader::read(const ast::Document& document) const {
  ReadResult result;
  Section* section = nullptr;
  Entry* entry = nullptr;
  std::size_t optionIndent = 0;
  for (const ast::Line* line : document.lines) {
    if (!line || cursiv::ast_cast<ast::Comment>(line)) continue;
    if (cursiv::ast_cast<ast::Blank>(line)) {
      if (entry && entry->hasValue) {
        entry->value += '\n';
        entry->continuations.push_back(line);
      }
      continue;
    }
    std::size_t indent;
    leading(line->indentation, indent);
    if (entry && indent > optionIndent) {
      if (!entry->hasValue) {
        result.issues.push_back({line, "A valueless option cannot have continuation lines."});
      } else {
        entry->value += '\n';
        entry->value += trim(source_text(*line));
        entry->continuations.push_back(line);
      }
      continue;
    }
    if (const auto* header = cursiv::ast_cast<ast::SectionHeader>(line)) {
      result.sections.push_back({header->name, line, {}});
      section = &result.sections.back();
      entry = nullptr;
      continue;
    }
    const auto* option = cursiv::ast_cast<ast::Option>(line);
    if (!option) continue;
    if (!section) {
      result.sections.push_back({});
      section = &result.sections.back();
    }
    const std::string_view key = trim(option->key);
    if (key.empty()) {
      result.issues.push_back({line, "An option must have a name before its separator."});
      entry = nullptr;
      continue;
    }
    section->entries.push_back({key, {}, !option->separator.empty(), line, {}});
    entry = &section->entries.back();
    if (entry->hasValue) entry->value = trim(option->value);
    optionIndent = indent;
  }
  for (Section& item : result.sections)
    for (Entry& option : item.entries)
      option.value = rtrim(option.value);
  return result;
}

void IniReader::checkDocument(const ast::Document& document,
                              cursiv::ValidationAcceptor& accept) const {
  for (const ReadIssue& issue : read(document).issues)
    accept.error(*issue.source, issue.message, "ini-invalid-option");
}

void install_reader(IniCoreServices& services) {
  services.reader = std::make_unique<const IniReader>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&IniReader::checkDocument>(*services.reader)});
}

}  // namespace ini
