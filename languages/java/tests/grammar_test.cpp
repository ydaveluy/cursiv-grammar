// The smoke test of the language: parse every file under example/, the way a
// workspace would.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "java/core/CoreServices.hpp"
#include "java/core/UnicodeEscapes.hpp"
#include "java/core/ast.hpp"
#include "java/generated/Java/Module.hpp"
#include "unicode_input_cases.hpp"
#include "literal_dimension_cases.hpp"
#include "native_followup_cases.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "java: %s\n", what.c_str());
  return 1;
}

// Observe the adapter's input independently of the token grammar. Expectations
// come from the real javac UnicodeReader, including parity and malformed input.
class InputSpy final : public cursiv::Parser {
public:
  explicit InputSpy(std::string &seen)
      : seen(seen), lexical(java::generated::Java::create_parser()) {}
  const cursiv::Grammar &getGrammar() const noexcept override {
    return lexical->getGrammar();
  }
  cursiv::ParseResult parse(std::string_view text,
                            const cursiv::CancellationToken &) const override {
    seen.assign(text);
    cursiv::ParseResult result{cursiv::Cst(text)};
    result.bind_grammar(getGrammar());
    return result;
  }

private:
  std::string &seen;
  std::unique_ptr<cursiv::Parser> lexical;
};

bool clean(const cursiv::ParseResult &result) {
  return result.accepted && !result.cst().recovered() &&
         result.diagnostics.empty() && result.value;
}

int unicode_regressions() {
  std::string seen;
  const auto spy = cursiv::translating_parser(std::make_unique<InputSpy>(seen),
                                              java::translate_unicode_escapes);
  for (const auto &test : unicode_input_cases) {
    const auto result = spy->parse(test.input);
    if (seen != test.output || result.diagnostics.size() != test.diagnostics ||
        result.cst().text() != test.input)
      return fail(std::string("UnicodeReader translation: ") + test.name);
  }
  const auto language = cursiv::create_language<java::JavaCoreServices,
                                                java::JavaSharedCoreServices,
                                                java::install_core_module>();
  const auto &parser = *language.language.parser;
  const auto lexical = java::generated::Java::create_parser();
  const struct {
    std::string_view source, name;
  } classes[] = {
      {R"(cl\u0061ss T {})", "T"},
      {R"(\uu0063lass \u0054 {})", "T"},
      {R"(\u002f\u002a comment \u002a\u002f class T {})", "T"},
      {R"(// comment\u000aclass T {})", "T"},
      {R"(class \u00E9 {})", "é"},
      {R"(class \uD835\uDC9C {})", "𝒜"},
      {"class \\u٠٠٤١ {}", "A"},
      {"class \\u００４１ {}", "A"},
  };
  for (const auto &test : classes) {
    const auto result = parser.parse(test.source);
    const java::ast::ClassDecl *decl = nullptr;
    for (auto *node : result.nodes)
      if (const auto *c = cursiv::ast_cast<java::ast::ClassDecl>(node))
        decl = c;
    if (!clean(result) || !decl || decl->name != test.name)
      return fail("Unicode keyword/comment/identifier AST");
    if (result.cst().text() != test.source || !result.cst().engine_stamped())
      return fail("Unicode CST must retain original source and engine stamps");
    const auto observed = spy->parse(test.source);
    const auto decoded = lexical->parse(seen);
    if (result.cst().size() != decoded.cst().size() ||
        result.cst().entry_root() != decoded.cst().entry_root())
      return fail("Unicode CST ids must match the lexical tree");
    bool name = false;
    for (cursiv::NodeId id = 0; id < result.cst().size(); ++id) {
      const auto &n = result.cst()[id];
      if (n.engine != decoded.cst()[id].engine ||
          n.subtreeSize != decoded.cst()[id].subtreeSize ||
          result.cst().grammar()->recordRule(n) !=
              decoded.cst().grammar()->recordRule(decoded.cst()[id]) ||
          result.cst().grammar()->recordFeature(n) !=
              decoded.cst().grammar()->recordFeature(decoded.cst()[id]))
        return fail("Unicode CST stamps/subtrees/grammar delegation");
      if (n.begin > n.end || n.end > test.source.size() ||
          (id && n.end < result.cst()[id - 1].end))
        return fail("Unicode CST ranges/order");
      if (result.cst().grammar()->recordWrites(n, "name") &&
          result.cst().text(id).find('\\') != std::string_view::npos)
        name = true;
    }
    if (test.source.find("class \\") != std::string_view::npos && !name)
      return fail("Unicode CST feature delegation");
  }
  const struct {
    std::string_view source, text;
    bool character;
  } literals[] = {
      {R"(class T { String s="\u0041"; })", "\"A\"", false},
      {R"(class T { String s="\\u0041"; })", R"("\\u0041")", false},
      {R"(class T { String s="\u005cn"; })", R"("\n")", false},
      {R"(class T { String s="\uD835\uDC9C"; })", "\"𝒜\"", false},
      {R"(class T { String s="\uD800"; })", R"("\uD800")", false},
      {R"(class T { char c='\uDC00'; })", R"('\uDC00')", true},
      {R"(class T { char c='\s'; })", R"('\s')", true},
      {R"(class T { String s="\s"; })", R"("\s")", false},
  };
  for (const auto &test : literals) {
    const auto result = parser.parse(test.source);
    bool found = false;
    for (auto *node : result.nodes) {
      if (const auto *c = cursiv::ast_cast<java::ast::CharLit>(node))
        found |= test.character && c->text == test.text;
      if (const auto *s = cursiv::ast_cast<java::ast::StringLit>(node))
        found |= !test.character && s->text == test.text;
    }
    if (!clean(result) || !found)
      return fail("Unicode literal AST");
  }
  // Results own their translated buffers, including SSO, after both a second
  // parse and destruction of the service/parser, and across move assignment.
  const std::string shortSource = R"(class \u0054{})";
  cursiv::ParseResult retained = [&] {
    const auto temporary = cursiv::create_language<java::JavaCoreServices,
                                                   java::JavaSharedCoreServices,
                                                   java::install_core_module>();
    auto first = temporary.language.parser->parse(shortSource);
    const auto second = temporary.language.parser->parse(R"(class \u0055{})");
    return first;
  }();
  cursiv::ParseResult moved;
  retained.arena.carve_bytes(17, alignof(std::uint64_t));
  auto *extra = retained.arena.make<java::ast::StringLit>();
  extra->text = "alive";
  moved = std::move(retained);
  if (extra->text != "alive" || !moved.arena.carve_bytes(11, 1))
    return fail("Unicode result arena lifetime");
  bool alive = false;
  for (auto *node : moved.nodes)
    if (const auto *c = cursiv::ast_cast<java::ast::ClassDecl>(node))
      alive = c->name == "T";
  if (!clean(moved) || !alive ||
      moved.cst().owned().size() != moved.nodes.size())
    return fail("Unicode result lifetime");
  if (moved.references) {
    moved.references->reset();
    if (moved.references->anyUnresolved())
      return fail("Unicode reference ownership");
  }

  const std::string plain = "class T {  }";
  const std::string escaped = R"(cl\u0061ss T {  })";
  const auto a = parser.parse(plain), b = parser.parse(escaped);
  const auto atA = cursiv::TextOffset(plain.find("  ") + 1);
  const auto atB = cursiv::TextOffset(escaped.find("  ") + 1);
  std::vector<cursiv::NextFeature> expected, actual, partial;
  parser.nextFeaturesAt(a.cst(), atA, atA, expected);
  parser.nextFeaturesAt(b.cst(), atB, atB, actual);
  const auto equal = [](const auto &x, const auto &y) {
    return x.feature == y.feature && x.property == y.property &&
           x.narrow == y.narrow && x.type == y.type;
  };
  if (expected.empty() || expected.size() != actual.size() ||
      !std::equal(expected.begin(), expected.end(), actual.begin(), equal))
    return fail("Unicode completion must match the ASCII document");
  parser.nextFeaturesAt(b.cst(), 4, 4, partial);
  if (!partial.empty())
    return fail("Unicode completion inside an escape");

  const std::string broken = R"(class \u00E9 { int x = ; })";
  const auto repaired = parser.parse(broken);
  if (repaired.diagnostics.empty())
    return fail("Unicode repaired diagnostic expected");
  for (const auto &d : repaired.diagnostics)
    if (d.begin > d.end || d.end > broken.size() || d.begin < broken.find('{'))
      return fail("Unicode repaired diagnostic source offsets");
  const auto malformed = parser.parse(R"(class \u00g1 {})");
  if (std::none_of(malformed.diagnostics.begin(), malformed.diagnostics.end(),
                   [](const auto &d) {
                     return d.message == "Malformed Java Unicode escape" &&
                            d.begin == 6 && d.end == 10;
                   }))
    return fail("Malformed Unicode escape diagnostic");
  cursiv::CancellationTokenSource cancelled;
  cancelled.request_stop();
  try {
    const auto ignored =
        parser.parse(R"(class \u0054 {})", cancelled.get_token());
    return fail("Unicode cancellation expected");
  } catch (const cursiv::OperationCancelled &) {
  }
  std::printf("java: 39 UnicodeReader cases, 16 AST cases, "
              "ownership/CST/completion/diagnostics pass\n");
  return 0;
}

} // namespace

int main() {
  if (const auto error = unicode_regressions())
    return error;
  {
    const auto audit_language =
        cursiv::create_language<java::JavaCoreServices,
                                java::JavaSharedCoreServices,
                                java::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "class T { Object x = new int[1][][]; }");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *a = cursiv::ast_cast<java::ast::NewArrayExpr>(node))
        found = a->dimensions.size() == 3 && a->dimensions[0]->size &&
                !a->dimensions[1]->size && !a->dimensions[2]->size;
    if (!found)
      return 1;
  }

  const auto language = cursiv::create_language<java::JavaCoreServices,
                                                java::JavaSharedCoreServices,
                                                java::install_core_module>();
  if (!java_native_followup::run(language)) return 1;

  if (!java_literal_dimension_cases(*language.language.parser)) return 1;

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".java")
        continue;

      std::ifstream file(entry.path());
      if (!file)
        return fail("cannot read " + entry.path().string());
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      // The whole language in one call: the defaults, then
      // `install_core_module`.
      const cursiv::ParseResult result = language.language.parser->parse(text);
      // `recovered()` is set when the parser had to repair the input to keep
      // going: a clean document leaves it false.
      if (result.cst().recovered())
        return fail(entry.path().filename().string() + " does not parse clean");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<java::ast::CompilationUnit>)
        return fail(entry.path().filename().string() +
                    " built no compilation unit");
      ++count;
    }
  if (count == 0)
    return fail("no example files found");

  std::printf("java: %d examples parse clean\n", count);
  return 0;
}
