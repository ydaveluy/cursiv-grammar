#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "prolog/core/CoreServices.hpp"
#include "prolog/core/ast.hpp"
#include "reader_cases.hpp"
#include <bit>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>

namespace {
int failures = 0;
int checks = 0;
void check(bool ok, const char *message) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "prolog: %s\n", message);
  }
}
std::string read(const char *path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), {}};
}
bool ownsAll(const cursiv::ParseResult &result,
             const cursiv::AstReflection &reflection,
             const cursiv::AstNode *parent) {
  std::unordered_set<const cursiv::AstNode *> reached;
  bool parents = true;
  std::function<void(const cursiv::AstNode &, const cursiv::AstNode *)> visit =
      [&](const auto &node, const auto *expected) {
        parents &= node.parent == expected;
        if (!reached.insert(&node).second) {
          parents = false;
          return;
        }
        reflection.visitMembers(
            node, [&](auto, auto, auto &child) { visit(child, &node); });
      };
  if (result.value)
    visit(*result.value, parent);
  return parents && reached.size() == result.nodes.size();
}
} // namespace
int main() {
  namespace a = prolog::ast;
  auto language = cursiv::create_language<prolog::PrologCoreServices,
                                          prolog::PrologSharedCoreServices,
                                          prolog::install_core_module>();
  for (auto path : {CURSIV_EXAMPLE1, CURSIV_CONSTRUCTS}) {
    auto source = read(path);
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          path);
  }
  // SWI read_term/3 distinguishes adjacent functor parentheses from the
  // parenthesised operand of a prefix operator, including after comments.
  for (std::string_view op : {
           "-", "+", "\\", "\\+", ":-", "?-", "$", "discontiguous", "dynamic",
           "volatile", "thread_local", "initialization", "thread_initialization",
           "module_transparent", "multifile", "meta_predicate", "public",
           "table"}) {
    const std::string source = std::string(op) + "(a,b). " + std::string(op) +
                               " (a,b). " + std::string(op) +
                               " /*comment*/(a,b).";
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///functor.pl", "prolog", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    check(document.parseSucceeded() && !document.parseRecovered() &&
              document.parseResult.diagnostics.empty(),
          "prefix/functor cases parse without recovery");
    auto root = cursiv::ast_cast<a::PText>(document.parseResult.value);
    check(root && root->items.size() == 3, "prefix/functor item count");
    if (root && root->items.size() == 3) {
      auto call = cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term);
      check(call && call->functor == op && call->args.size() == 2,
            "adjacent parentheses give a two-argument compound");
      if (call && call->args.size() == 2) {
        const auto &cst = document.parseResult.cst();
        const auto &span = cst[call->cstNodeId];
        check(span.begin == 0 && span.end == op.size() + 5,
              "compound span keeps its initial functor and closing parenthesis");
        for (unsigned i = 0; i < 2; ++i) {
          const auto &arg = cst[call->args[i]->cstNodeId];
          check(arg.begin == op.size() + 1 + 2 * i && arg.end == arg.begin + 1,
                "compound argument spans keep their source positions");
        }
      }
      for (unsigned i = 1; i < 3; ++i) {
        auto prefix = cursiv::ast_cast<a::UnaryTerm>(root->items[i]->term);
        auto group = prefix
                         ? cursiv::ast_cast<a::BracedTerm>(prefix->operand)
                         : nullptr;
        auto comma = group ? cursiv::ast_cast<a::BinaryTerm>(group->inner)
                           : nullptr;
        check(prefix && prefix->op == op && comma && comma->op == ",",
              "layout keeps the prefix's single comma-expression operand");
      }
    }
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    check(ownsAll(document.parseResult, *language.language.astReflection,
                  &document),
          "prefix/functor AST parents and ownership");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(diagnostics.empty(), "valid prefix/functor forms need no diagnostic");
  }
  {
    const std::string source =
        "f('a·b', 'A·b', Àname, àname, ªname, µname, ºname, λname, Αname, a·name).";
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "Latin1 names, quoted punctuation and wide names parse");
    auto root = cursiv::ast_cast<a::PText>(result.value);
    auto call = root && root->items.size() == 1
                    ? cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term)
                    : nullptr;
    check(call && call->args.size() == 10, "identifier argument count");
    if (call && call->args.size() == 10) {
      for (unsigned i : {0u, 1u, 3u, 4u, 5u, 6u, 7u, 9u})
        check(cursiv::ast_cast<a::AtomTerm>(call->args[i]) != nullptr,
              "native atom-start class and quoted atoms stay atoms");
      for (unsigned i : {2u, 8u})
        check(cursiv::ast_cast<a::VariableTerm>(call->args[i]) != nullptr,
              "native variable-start class stays a variable");
      auto first = cursiv::ast_cast<a::AtomTerm>(call->args[0]);
      const auto &span = result.cst()[call->args[0]->cstNodeId];
      check(first && first->value == "a·b" && span.begin == 2 && span.end == 8,
            "quoted middle dot keeps its value and original quoted span");
    }
    check(ownsAll(result, *language.language.astReflection, nullptr),
          "identifier AST parents and ownership");
  }
  for (const std::string source : {"f(a·b).", "f(A·b).", "f(\u0085name)."}) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///identifier.pl", "prolog", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(!diagnostics.empty(),
          "invalid Latin1 text cannot silently become an identifier or layout");
  }
  {
    const std::string source =
        "f(a;b,c,a->b,c:-d,:- e,dynamic a,a|b). "
        "[a;b,c|d;e]. tag{k:a|b}. $a.b. $a**b.";
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///argument-operators.pl", "prolog", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    check(document.parseSucceeded() && !document.parseRecovered() &&
              document.parseResult.diagnostics.empty(),
          "operators remain available inside arguments and list tails");
    auto root = cursiv::ast_cast<a::PText>(document.parseResult.value);
    check(root && root->items.size() == 5, "argument operator item count");
    if (root && root->items.size() == 5) {
      auto call = cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term);
      check(call && call->args.size() == 7, "comma separates compound arguments");
      if (call && call->args.size() == 7) {
        for (auto [at, op] : {std::pair{0u, ";"}, {2u, "->"}, {3u, ":-"}, {6u, "|"}}) {
          auto binary = cursiv::ast_cast<a::BinaryTerm>(call->args[at]);
          check(binary && binary->op == op, "argument binary operator preserved");
        }
        for (auto [at, op] : {std::pair{4u, ":-"}, {5u, "dynamic"}}) {
          auto unary = cursiv::ast_cast<a::UnaryTerm>(call->args[at]);
          check(unary && unary->op == op, "argument prefix operator preserved");
        }
      }
      auto list = cursiv::ast_cast<a::ListTerm>(root->items[1]->term);
      auto tail = list ? cursiv::ast_cast<a::BinaryTerm>(list->tail) : nullptr;
      check(list && list->items.size() == 2 && tail && tail->op == ";",
            "list comma and bar separate items and tail, not their operators");
      auto dict = cursiv::ast_cast<a::DictTerm>(root->items[2]->term);
      auto value = dict && dict->entries.size() == 1
                       ? cursiv::ast_cast<a::BinaryTerm>(dict->entries[0]->value)
                       : nullptr;
      check(value && value->op == "|", "bar remains an operator in a dict value");
      for (unsigned i : {3u, 4u}) {
        auto binary = cursiv::ast_cast<a::BinaryTerm>(root->items[i]->term);
        auto dollar = binary ? cursiv::ast_cast<a::UnaryTerm>(binary->left)
                             : nullptr;
        check(binary && dollar && dollar->op == "$" &&
                  binary->op == (i == 3 ? "." : "**"),
              "dollar prefix binds before dot and power");
      }
    }
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    check(ownsAll(document.parseResult, *language.language.astReflection,
                  &document),
          "argument operator AST parents and ownership");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(diagnostics.empty(), "valid argument operators need no diagnostic");
  }
  {
    const std::string source = "-a^b. -2^2. - 2^2. f().";
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "priority cases parse");
    auto root = cursiv::ast_cast<a::PText>(result.value);
    check(root && root->items.size() == 4, "priority item count");
    if (root && root->items.size() == 4) {
      auto sign = cursiv::ast_cast<a::UnaryTerm>(root->items[0]->term);
      auto power =
          sign ? cursiv::ast_cast<a::BinaryTerm>(sign->operand) : nullptr;
      check(sign && sign->op == "-" && power && power->op == "^",
            "-a^b groups below prefix");
      auto negativePower =
          cursiv::ast_cast<a::BinaryTerm>(root->items[1]->term);
      auto number = negativePower
                        ? cursiv::ast_cast<a::IntegerTerm>(negativePower->left)
                        : nullptr;
      check(number && number->value == "-2",
            "contiguous minus belongs to the integer");
      auto spaced = cursiv::ast_cast<a::UnaryTerm>(root->items[2]->term);
      check(spaced && cursiv::ast_cast<a::BinaryTerm>(spaced->operand),
            "spaced minus remains a prefix");
      auto zero = cursiv::ast_cast<a::CompoundTerm>(root->items[3]->term);
      check(zero && zero->functor == "f" && zero->args.empty(),
            "zero arity compound");
    }
  }
  {
    const std::string source =
        R"(f('a', a, "x\u03bb\x0\z", `ab`, 16'ff, 0'λ, 1e3, 1.2e-3, 1r2).)";
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "literal cases parse");
    auto root = cursiv::ast_cast<a::PText>(result.value);
    auto call = root && root->items.size() == 1
                    ? cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term)
                    : nullptr;
    check(call && call->args.size() == 9, "literal arguments preserved");
    if (call && call->args.size() == 9) {
      auto quoted = cursiv::ast_cast<a::AtomTerm>(call->args[0]);
      auto bare = cursiv::ast_cast<a::AtomTerm>(call->args[1]);
      check(quoted && bare && quoted->value == "a" && bare->value == "a",
            "atom spellings share their decoded value");
      auto string = cursiv::ast_cast<a::StringTerm>(call->args[2]);
      check(string && string->value == std::string_view("xλ\0z", 5),
            "string Unicode and NUL decoded");
      auto hex = cursiv::ast_cast<a::IntegerTerm>(call->args[4]);
      auto character = cursiv::ast_cast<a::IntegerTerm>(call->args[5]);
      check(hex && hex->value == "255" && character &&
                character->value == "955",
            "radix and character integer values");
      auto codes = cursiv::ast_cast<a::CodeListTerm>(call->args[3]);
      check(codes && codes->value == "ab", "backquoted code-list text");
      auto rational = cursiv::ast_cast<a::RationalTerm>(call->args[8]);
      check(rational && rational->text == "1r2", "exact rational spelling");
      auto exponent = cursiv::ast_cast<a::FloatTerm>(call->args[6]);
      check(exponent && exponent->value == 1000.0, "exponent-only float value");
    }
  }

  {
    const std::string source = "f(1.25NaN, 1.5NaN, -1.75NaN).";
    auto result = language.language.parser->parse(source);
    auto root = cursiv::ast_cast<a::PText>(result.value);
    auto call = root && root->items.size() == 1
                    ? cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term)
                    : nullptr;
    check(call && call->args.size() == 3, "NaN payload arguments");
    if (call && call->args.size() == 3) {
      const std::uint64_t bits[] = {
          0x7ff8000000000000ULL, 0x7ff8000000000000ULL, 0x7ff8000000000000ULL};
      for (unsigned i = 0; i < 3; ++i) {
        auto value = cursiv::ast_cast<a::FloatTerm>(call->args[i]);
        check(value && std::bit_cast<std::uint64_t>(value->value) == bits[i],
              "NaN values use the native canonical representation");
      }
    }
  }
  {
    const std::string source = "f([](), '[]'()).";
    auto result = language.language.parser->parse(source);
    auto root = cursiv::ast_cast<a::PText>(result.value);
    auto call = root && root->items.size() == 1
                    ? cursiv::ast_cast<a::CompoundTerm>(root->items[0]->term)
                    : nullptr;
    check(call && call->args.size() == 2, "nil and quoted atom functors");
    if (call && call->args.size() == 2) {
      auto nil = cursiv::ast_cast<a::CompoundTerm>(call->args[0]);
      auto atom = cursiv::ast_cast<a::CompoundTerm>(call->args[1]);
      check(nil && atom && nil->nilFunctor && !atom->nilFunctor &&
                nil->functor == "[]" && atom->functor == "[]",
            "the empty-list functor differs from the quoted atom");
    }
  }
  for (const auto &fixture : readerCases) {
    const std::string source = fixture.source;
    cursiv::Document document(
        cursiv::TextDocument::create("file:///reader.pl", "prolog", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    check(document.parseSucceeded() && !document.parseRecovered() &&
              document.parseResult.diagnostics.empty(),
          fixture.name);
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    check(ownsAll(document.parseResult, *language.language.astReflection,
                  &document),
          fixture.name);
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(diagnostics.empty(), fixture.name);
  }
  struct InvalidCase {
    const char *source;
    const char *code;
  };
  const InvalidCase invalid[] = {
      {"f(1.0NaN).", "prolog.float-nan"},
      {"a**b**c.", "prolog.operator-clash"},
      {"a** -b.", "prolog.operator-clash"},
      {"a=b=c.", "prolog.operator-clash"},
      {"dynamic dynamic a.", "prolog.operator-clash"},
      {R"(f('\q').)", "prolog.escape"},
      {R"(f('\uD800').)", "prolog.escape"},
      {"f(1r0).", "prolog.rational"},
      {"f(37'z).", "prolog.integer"},
      {"f([a|b,c]).", "prolog.list-tail"},
      {"f([a|b|c]).", "prolog.list-tail"},
      {"f([a|b;c,d]).", "prolog.list-tail"},
      {"tag{a:1,a:2}.", "prolog.dict-key"},
      {"tag{X:1}.", "prolog.dict-key"},
      {"f (a).", "prolog.functor-gap"},
      {"f(1e309).", "prolog.float-range"},
      {"f(١2).", "prolog.integer"},
      {"f(0b102).", "prolog.integer"},
  };
  for (const auto &fixture : invalid) {
    const std::string source = fixture.source;
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///invalid.pl", "prolog", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    check(document.parseSucceeded() && !document.parseRecovered() &&
              document.parseResult.diagnostics.empty(),
          fixture.source);
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    bool found = false;
    for (const auto &d : diagnostics)
      if (d.code && d.code->isString() && d.code->string() == fixture.code &&
          d.range.begin < d.range.end && d.range.end <= source.size())
        found = true;
    check(found, fixture.code);
  }
  std::printf("prolog: %d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
