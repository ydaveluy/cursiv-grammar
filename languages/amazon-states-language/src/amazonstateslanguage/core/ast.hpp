// The Amazon States Language AST: a JSON-shaped `Value` tree for the
// payload-template positions (Result, Parameters, ItemSelector,
// ResultSelector -- the spec leaves these as arbitrary JSON), and typed
// nodes for the state-machine shape a tool actually walks: Program, State,
// ChoiceRule, Retrier, Catcher. A State or Retrier key written again keeps
// every value, in order, in its list.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace amazonstateslanguage::ast {

struct Value : cursiv::Extends<cursiv::AstNode> {};
struct Binding;
using Val = std::variant<std::monostate, bool, std::int64_t, double,
                          std::string_view, Value*, Binding*>;

struct Object : cursiv::Extends<Value> {
  vector<Binding*> members;
};

struct Array : cursiv::Extends<Value> {
  vector<Val> items;
};

// A payload-template `"foo.$"` key keeps its `.$` suffix as written -- that
// suffix, not a separate flag, is what marks `value` as a JSONPath or
// intrinsic-call expression rather than a literal.
struct Binding : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Val value;
};

struct Retrier : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> error_equals;
  vector<int64> interval_seconds;
  vector<int64> max_attempts;
  vector<double> backoff_rates;
  vector<int64> max_delay_seconds;
  vector<string_view> jitter_strategies;
  vector<string_view> comments;
};

struct Catcher : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> error_equals;
  vector<string_view> result_path;
  bool result_path_null = false;
  vector<string_view> next;
  vector<string_view> comment;
  vector<Object*> assignments;
  vector<Val> outputs;
};

// The two Choices shapes share this base, so `State::choices` reads both
// uniformly; `type->derivesFrom` tells a ComparisonRule from a CompositeRule.
struct ChoiceRule : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> comment;
  vector<string_view> next;
  vector<Object*> assignments;
  vector<Val> outputs;
};

struct JsonataChoiceRule : cursiv::Extends<ChoiceRule> {
  vector<Val> conditions;
};

// `{ "Variable": ..., "<Op>": <value>, "Next": ... }`.
struct ComparisonRule : cursiv::Extends<ChoiceRule> {
  vector<string_view> variable;
  vector<string_view> comparison_op;
  vector<Val> comparison_value;
};

// `{ "And"/"Or"/"Not": <rule or [rules]>, "Next": ... }`.
struct CompositeRule : cursiv::Extends<ChoiceRule> {
  vector<string_view> logical_op;
  vector<ChoiceRule*> operands;
};

struct ProcessorConfig : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> mode;
  vector<string_view> execution_type;
  vector<Binding*> extensions;
};

struct ReaderConfig : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> input_type;
  vector<string_view> csv_header_location;
  vector<string_view> csv_headers;
  vector<int64> max_items;
  vector<string_view> max_items_path;
  vector<Binding*> extensions;
};

struct ItemReader : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> resource;
  vector<Object*> parameters;
  vector<ReaderConfig*> reader_config;
  vector<Val> arguments;
};

struct State;

// The body shared by a state machine, an ItemProcessor, an Iterator and a
// Parallel branch. Which fields a given context allows is the grammar's
// affair: an Iterator's Program never fills `version`.
struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> members;
  vector<string_view> comment;
  vector<string_view> version;
  vector<string_view> start_at;
  vector<int64> timeout_seconds;
  vector<ProcessorConfig*> processor_config;
  vector<State*> states;
  vector<string_view> query_languages;
};

struct State : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> comments;
  vector<string_view> types;
  vector<string_view> resources;
  vector<string_view> input_paths;
  bool input_path_null = false;
  vector<string_view> output_paths;
  bool output_path_null = false;
  vector<string_view> result_paths;
  bool result_path_null = false;
  vector<Val> results;
  vector<string_view> next;
  vector<bool> end;
  vector<string_view> default_states;
  vector<ChoiceRule*> choices;
  vector<string_view> errors;
  vector<string_view> error_paths;
  vector<string_view> causes;
  vector<string_view> cause_paths;
  vector<int64> seconds;
  vector<string_view> seconds_paths;
  vector<string_view> timestamps;
  vector<string_view> timestamp_paths;
  vector<string_view> items_paths;
  vector<Program*> item_processors;
  vector<Program*> iterators;
  vector<Object*> item_selectors;
  vector<ItemReader*> item_readers;
  vector<int64> max_concurrencies;
  vector<int64> timeout_seconds;
  vector<string_view> timeout_seconds_paths;
  vector<int64> heartbeat_seconds;
  vector<string_view> heartbeat_seconds_paths;
  vector<Program*> branches;
  vector<Object*> parameters;
  vector<Retrier*> retry;
  vector<Catcher*> catch_;
  vector<Object*> result_selectors;
  vector<string_view> query_languages;
  vector<Object*> assignments;
  vector<Val> arguments;
  vector<Val> outputs;
  vector<Object*> credentials;
  // Map-specific options retain their names and arbitrary JSON values.
  vector<Binding*> map_options;
  vector<Binding*> numeric_expressions;
  vector<Val> item_selector_values;
};

}  // namespace amazonstateslanguage::ast
