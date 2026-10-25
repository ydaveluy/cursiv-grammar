#include "toml/core/validation/TomlValidator.hpp"

#include <charconv>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "toml/core/CoreServices.hpp"
#include "toml/core/TomlString.hpp"

namespace toml {
namespace {
struct Source {
  std::string_view text;
  cursiv::TextRange span;
};
Source source(const cursiv::AstNode &owner, std::string_view property,
              std::size_t index, cursiv::ValidationAcceptor &accept) {
  const auto *document = owner.getDocument();
  if (!document)
    return {};
  const auto span = accept.featureSpan(owner, property, index);
  const auto text = document->textDocument().getText();
  if (!span || span->end < span->begin || span->end > text.size())
    return {};
  return {text.substr(span->begin, span->end - span->begin), *span};
}
void string(const Source &raw, cursiv::ValidationAcceptor &accept) {
  if (raw.text.empty() || (raw.text[0] != '"' && raw.text[0] != '\''))
    return;
  const auto value = decode_string(raw.text);
  if (value.error.empty())
    return;
  auto at =
      static_cast<cursiv::TextOffset>(raw.span.begin + value.error_offset);
  accept.error(cursiv::TextRange{at, at + 1}, value.error, "toml.string");
}
bool integer_in_range(std::string_view raw) {
  std::string text;
  for (char c : raw)
    if (c != '_')
      text += c;
  std::string_view s = text;
  if (s.starts_with('+'))
    s.remove_prefix(1);
  if (s.size() > 2 && s[0] == '0' &&
      (s[1] == 'x' || s[1] == 'o' || s[1] == 'b')) {
    const int base = s[1] == 'x' ? 16 : s[1] == 'o' ? 8 : 2;
    s.remove_prefix(2);
    std::uint64_t value = 0;
    const auto [end, ec] =
        std::from_chars(s.data(), s.data() + s.size(), value, base);
    return ec == std::errc{} && end == s.data() + s.size() &&
           value <= static_cast<std::uint64_t>(
                        std::numeric_limits<std::int64_t>::max());
  }
  std::int64_t value = 0;
  const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  return ec == std::errc{} && end == s.data() + s.size();
}
bool valid_date_time(const ast::DateTime &value) {
  const auto text = value.text;
  const auto number = [&](std::size_t at, std::size_t count) {
    unsigned n = 0;
    for (std::size_t i = 0; i < count; ++i) {
      if (at + i >= text.size() || text[at + i] < '0' || text[at + i] > '9')
        return 99999u;
      n = n * 10 + text[at + i] - '0';
    }
    return n;
  };
  const bool date = value.kind != ast::DateTimeKind::LocalTime;
  if (date) {
    const unsigned year = number(0, 4), month = number(5, 2),
                   day = number(8, 2);
    if (month < 1 || month > 12 || day < 1)
      return false;
    static constexpr unsigned days[] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    if (day > days[month - 1] + (month == 2 && leap))
      return false;
    if (value.kind == ast::DateTimeKind::LocalDate)
      return true;
  }
  const std::size_t time = date ? 11 : 0;
  if (number(time, 2) > 23 || number(time + 3, 2) > 59 ||
      number(time + 6, 2) > 59)
    return false;
  if (value.kind != ast::DateTimeKind::OffsetDateTime)
    return true;
  const auto offset = text.find_first_of("+-", time + 8);
  return offset == std::string_view::npos ||
         (number(offset + 1, 2) <= 23 && number(offset + 4, 2) <= 59);
}
void scalar(const cursiv::AstNode &owner, const ast::Val &value,
            std::string_view property, std::size_t index,
            cursiv::ValidationAcceptor &accept) {
  const auto raw = source(owner, property, index, accept);
  if (std::holds_alternative<std::string_view>(value))
    string(raw, accept);
  else if (std::holds_alternative<std::int64_t>(value) && !raw.text.empty() &&
           !integer_in_range(raw.text))
    accept
        .error(owner,
               "The integer is outside the supported signed 64-bit range.",
               "toml.integer.range")
        .range(raw.span);
  else if (const auto *date = std::get_if<ast::DateTime>(&value);
           date && !valid_date_time(*date))
    accept
        .error(owner, "Invalid calendar date, time or UTC offset.",
               "toml.datetime")
        .range(raw.span);
}

// A namespace trie models decoded components, never strings joined with dots.
// Array-table headers descend into their most recently appended element.
struct Namespace {
  enum Kind { Table, ArrayTable, Value, Frozen } kind = Table;
  bool declared = false;
  bool dotted = false;
  std::unordered_map<std::string, std::unique_ptr<Namespace>> fields;
  std::vector<std::unique_ptr<Namespace>> elements;
};
struct Paths {
  cursiv::ValidationAcceptor &accept;
  void conflict(const ast::Key &key, std::string_view message) {
    accept.error(key, message, "toml.key.conflict");
  }
  Namespace *child(Namespace &parent, std::string_view name) {
    auto &entry = parent.fields[std::string(name)];
    if (!entry)
      entry = std::make_unique<Namespace>();
    return entry.get();
  }
  Namespace *header(Namespace &root, const ast::Table &table) {
    if (!table.key || table.key->parts.empty())
      return &root;
    Namespace *at = &root;
    const auto &parts = table.key->parts;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      auto *next = child(*at, parts[i]);
      const bool last = i + 1 == parts.size();
      if (last && table.is_array) {
        if (next->kind == Namespace::Value || next->kind == Namespace::Frozen ||
            (next->kind == Namespace::Table &&
             (next->declared || next->dotted || !next->fields.empty()))) {
          conflict(*table.key,
                   "An array-table cannot replace an existing value or table.");
          return nullptr;
        }
        next->kind = Namespace::ArrayTable;
        next->elements.push_back(std::make_unique<Namespace>());
        next->elements.back()->declared = true;
        return next->elements.back().get();
      }
      if (next->kind == Namespace::ArrayTable && !last) {
        if (next->elements.empty())
          return nullptr;
        at = next->elements.back().get();
        continue;
      }
      if (next->kind != Namespace::Table ||
          (last && (next->declared || next->dotted))) {
        conflict(*table.key,
                 "The table path is already defined or extends a value.");
        return nullptr;
      }
      if (last)
        next->declared = true;
      at = next;
    }
    return at;
  }
  void entries(Namespace &table,
               const cursiv::vector<ast::KeyValue *> &entries) {
    for (const auto *kv : entries) {
      if (!kv || !kv->key || kv->key->parts.empty())
        continue;
      Namespace *at = &table;
      const auto &parts = kv->key->parts;
      bool invalid = false;
      for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        auto *next = child(*at, parts[i]);
        if (next->kind != Namespace::Table || next->declared) {
          conflict(
              *kv->key,
              "The key path extends a value or an already declared table.");
          invalid = true;
          break;
        }
        next->dotted = true;
        at = next;
      }
      if (invalid)
        continue;
      const std::string name(parts[parts.size() - 1]);
      if (at->fields.contains(name)) {
        conflict(*kv->key, "The key is already defined.");
        continue;
      }
      auto value = std::make_unique<Namespace>();
      value->kind = std::holds_alternative<ast::InlineTable *>(kv->value)
                        ? Namespace::Frozen
                        : Namespace::Value;
      at->fields.emplace(name, std::move(value));
    }
  }
};
} // namespace

void TomlValidator::checkKey(const ast::Key &key,
                             cursiv::ValidationAcceptor &accept) const {
  for (std::size_t i = 0; i < key.parts.size(); ++i)
    string(source(key, "parts", i, accept), accept);
}
void TomlValidator::checkValue(const ast::KeyValue &value,
                               cursiv::ValidationAcceptor &accept) const {
  scalar(value, value.value, "value", 0, accept);
}
void TomlValidator::checkArray(const ast::Array &array,
                               cursiv::ValidationAcceptor &accept) const {
  for (std::size_t i = 0; i < array.items.size(); ++i)
    scalar(array, array.items[i], "items", i, accept);
}
void TomlValidator::checkDocument(const ast::Document &document,
                                  cursiv::ValidationAcceptor &accept) const {
  Namespace root;
  Paths paths{accept};
  for (const auto *table : document.tables) {
    if (!table)
      continue;
    if (auto *scope = paths.header(root, *table))
      paths.entries(*scope, table->entries);
  }
}
void TomlValidator::checkInlineTable(const ast::InlineTable &table,
                                     cursiv::ValidationAcceptor &accept) const {
  Namespace root;
  Paths{accept}.entries(root, table.entries);
}
void register_validation_checks(TomlCoreServices &services) {
  using V = TomlValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkKey>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkValue>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArray>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInlineTable>(
           validator)});
}
} // namespace toml
