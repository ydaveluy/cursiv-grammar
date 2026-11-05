// The Amazon States Language AST: a JSON-shaped `Value` tree for the
// payload-template positions (Result, Parameters, ItemSelector,
// ResultSelector -- the spec leaves these as arbitrary JSON), and typed
// nodes for the state-machine shape a tool actually walks: Program, State,
// ChoiceRule, Retrier, Catcher. A State or Retrier key written again keeps
// every value, in order, in its list.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/syntax-tree/AstNode.hpp"

namespace amazonstateslanguage::ast {

struct Value : cursiv::Extends<cursiv::AstNode> {};
struct Binding;
using Val = std::variant<std::monostate, bool, std::int64_t, double,
                          std::string, Value*, Binding*>;

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
  string key;
  Val value;
};

struct Retrier : cursiv::Extends<cursiv::AstNode> {
  vector<string> error_equals;
  vector<int64> interval_seconds;
  vector<int64> max_attempts;
  vector<double> backoff_rates;
  vector<int64> max_delay_seconds;
  vector<string> jitter_strategies;
  vector<string> comments;
};

struct Catcher : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> error_equals;
  vector<Val> result_path;
  vector<string> next;
  vector<string> comment;
  vector<Object*> assignments;
  vector<Val> outputs;
};

// The two Choices shapes share this base, so `State::choices` reads both
// uniformly; `type->derivesFrom` tells a ComparisonRule from a CompositeRule.
struct ChoiceRule : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> comment;
  vector<string> next;
  vector<Object*> assignments;
  vector<Val> outputs;
};

struct JsonataChoiceRule : cursiv::Extends<ChoiceRule> {
  vector<Val> conditions;
};

// `{ "Variable": ..., "<Op>": <value>, "Next": ... }`.
struct ComparisonRule : cursiv::Extends<ChoiceRule> {
  vector<string> variable;
  vector<string> comparison_op;
  vector<Val> comparison_value;
};

// `{ "And"/"Or"/"Not": <rule or [rules]>, "Next": ... }`.
struct CompositeRule : cursiv::Extends<ChoiceRule> {
  vector<string> logical_op;
  vector<ChoiceRule*> operands;
};

struct ProcessorConfig : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> mode;
  vector<string> execution_type;
  vector<Binding*> extensions;
};

struct ReaderConfig : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> input_type;
  vector<string> csv_header_location;
  vector<string> csv_headers;
  vector<int64> max_items;
  vector<string> max_items_path;
  vector<Binding*> extensions;
};

struct ItemReader : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> resource;
  vector<Object*> parameters;
  vector<ReaderConfig*> reader_config;
  vector<Val> arguments;
};

struct State;

// The body shared by a state machine, an ItemProcessor, an Iterator and a
// Parallel branch. Which fields a given context allows is the grammar's
// affair: an Iterator's Program never fills `version`.
struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<string> members;
  vector<string> comment;
  vector<string> version;
  vector<string> start_at;
  vector<int64> timeout_seconds;
  vector<ProcessorConfig*> processor_config;
  vector<State*> states;
  vector<string> query_languages;
};

struct State : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<string> comments;
  vector<string> types;
  vector<string> resources;
  vector<Val> input_paths;
  vector<Val> output_paths;
  vector<Val> result_paths;
  vector<Val> results;
  vector<string> next;
  vector<bool> end;
  vector<string> default_states;
  vector<ChoiceRule*> choices;
  vector<string> errors;
  vector<string> error_paths;
  vector<string> causes;
  vector<string> cause_paths;
  vector<int64> seconds;
  vector<string> seconds_paths;
  vector<string> timestamps;
  vector<string> timestamp_paths;
  vector<string> items_paths;
  vector<Program*> item_processors;
  vector<Program*> iterators;
  vector<Object*> item_selectors;
  vector<ItemReader*> item_readers;
  vector<int64> max_concurrencies;
  vector<int64> timeout_seconds;
  vector<string> timeout_seconds_paths;
  vector<int64> heartbeat_seconds;
  vector<string> heartbeat_seconds_paths;
  vector<Program*> branches;
  vector<Object*> parameters;
  vector<Retrier*> retry;
  vector<Catcher*> catch_;
  vector<Object*> result_selectors;
  vector<string> query_languages;
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
