// The toml language: its AST. A document is a sequence of tables (the first
// may be the implicit, nameless root table); each table holds key/value
// entries whose value is a variant reaching every TOML value kind.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace toml::ast {

enum class DateTimeKind : std::uint8_t {
  OffsetDateTime,
  LocalDateTime,
  LocalDate,
  LocalTime,
};

// No member owns text, so a date/time value stays its own written spelling
// rather than being decomposed into fields.
struct DateTime {
  DateTimeKind kind;
  std::string_view text;
};

struct Array;
struct InlineTable;

using Val = std::variant<std::monostate, bool, std::int64_t, double,
                          std::string_view, DateTime, Array*, InlineTable*>;

// Key components are separate, decoded strings: quoted dots stay inside
// one component, while separators produce separate components.
struct Key : cursiv::Extends<cursiv::AstNode> { vector<string_view> parts; };

struct KeyValue : cursiv::Extends<cursiv::AstNode> {
  Key* key;
  Val value;
};

struct Array : cursiv::Extends<cursiv::AstNode> {
  vector<Val> items;
};

struct InlineTable : cursiv::Extends<cursiv::AstNode> {
  vector<KeyValue*> entries;
};

// One class for `[key]` and `[[key]]`: `is_array` is the only difference.
// The implicit root table (entries written before any header) has an empty
// `key` and `is_array` false.
struct Table : cursiv::Extends<cursiv::AstNode> {
  Key* key;
  boolean is_array;
  vector<KeyValue*> entries;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Table*> tables;
};

}  // namespace toml::ast
