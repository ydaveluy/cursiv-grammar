#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

// Native answers are pinned in audit/verification/resume-20261024/objc.
// These cases distinguish token spelling from decoded character/code-unit limits.
namespace objc_resume {
inline bool run(const objc::ObjcCoreServices& services) {
  bool ok = true;
  const auto check = [&](bool value, std::string_view literal) {
    if (!value) {
      std::fprintf(stderr, "objc literal regression: %.*s\n",
                   static_cast<int>(literal.size()), literal.data());
      ok = false;
    }
  };
  const auto tree = [&](const cursiv::ParseResult& result,
                        std::string_view source) {
    bool valid = result.accepted && !result.cst().recovered() &&
                 result.diagnostics.empty() && result.value &&
                 result.cst().text() == source;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      if (node.cstNodeId == cursiv::kNoNode ||
          node.cstNodeId >= result.cst().size()) {
        valid = false;
      } else {
        const auto& span = result.cst()[node.cstNodeId];
        valid &= span.begin <= span.end && span.end <= source.size();
      }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node;
            self(self, child);
          });
    };
    if (result.value) walk(walk, *result.value);
    return valid && seen.size() == result.nodes.size();
  };
  const auto hasCode = [](const auto& validation, std::string_view code) {
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && *diagnostic.code == code) return true;
    return false;
  };
  struct Literal { std::string_view text; bool nativeValid; };
  const Literal numbers[] = {
      {"1lL", false},
      {"1Ll", false},
      {"1ulL", false},
      {"1uLl", false},
      {"1lLu", false},
      {"1Llu", false},
      {"1wB", false},
      {"1Wb", false},
      {"1uWb", false},
      {"1UwB", false},
      {"1wbU", true},
      {"1WBU", true},
      {"1uWB", true},
      {"1Uwb", true},
      {"1wb", true},
      {"1WB", true},
      {"1U", true},
      {"1UL", true},
      {"1Lu", true},
      {"1LLU", true},
      {"1.f32", false},
      {"1.f64", false},
      {"1.f128", false},
      {"1.q", true},
      {"1.Q", true},
      {"1.f16", true},
      {"1.F16", true},
      {"1.f128x", false},
      {"1.i", true},
      {"1.fi", true},
      {"1.0if", true},
      {"1.j", true},
      {"1.0f16i", true},
      {"1lLi", false},
      {"1iLL", true},
      {"1i", true},
      {"1ui", true},
      {"1wbj", true},
      {"1LiL", false},
      {"1uil", true},
      {"1Lui", true},
      {"1iU", true},
  };
  for (const auto& literal : numbers) {
    const std::string source = "_Complex double value=" +
                               std::string(literal.text) + ";";
    const auto result = services.parser->parse(source);
    const auto nodes = objc_native::nodes<objc::ast::NumberLit>(result);
    check(tree(result, source) && nodes.size() == 1 &&
              nodes[0]->text == literal.text, literal.text);
    const auto validation = cursiv::test::validationHelper(services, source);
    check(hasCode(validation, "objc.number") == !literal.nativeValid,
          literal.text);
  }
  const Literal characters[] = {
      {"'ab'", true},
      {"L'ab'", false},
      {"u'ab'", false},
      {"U'ab'", false},
      {"u8'ab'", false},
      {"'é'", false},
      {"L'é'", true},
      {"u'é'", true},
      {"U'é'", true},
      {"u8'é'", false},
      {"u'😀'", false},
      {"U'😀'", true},
      {"L'😀'", true},
      {"'\\777'", false},
      {"L'\\777'", true},
      {"u'\\777'", true},
      {"u8'\\777'", false},
      {"'\\x100'", false},
      {"L'\\x100'", true},
      {"u'\\x10000'", false},
      {"U'\\x110000'", true},
      {"'\\400'", false},
      {"'\\377'", true},
      {"u'\\U0001F600'", false},
      {"u8'\\u00E9'", false},
  };
  for (const auto& literal : characters) {
    const std::string source = "unsigned long value=" +
                               std::string(literal.text) + ";";
    const auto result = services.parser->parse(source);
    const auto nodes = objc_native::nodes<objc::ast::CharLit>(result);
    check(tree(result, source) && nodes.size() == 1 &&
              nodes[0]->text == literal.text, literal.text);
    const auto validation = cursiv::test::validationHelper(services, source);
    check(hasCode(validation, "objc.character") == !literal.nativeValid,
          literal.text);
  }
  const std::string_view strings[] = {
      "@/*gap*/\"a\"",
      "@//gap\n\"a\"",
      "@\\\n\"a\"",
      "@/* \"comment\nignored\" */\"a\"",
      "@/* \"\\x\" */\"a\"",
      "@// gap \\\ncontinued\n\"a\"",
  };
  for (const auto literal : strings) {
    const std::string source = "id f(void){return " + std::string(literal) + ";}";
    const auto result = services.parser->parse(source);
    const auto nodes = objc_native::nodes<objc::ast::StringLit>(result);
    check(tree(result, source) && nodes.size() == 1 &&
              nodes[0]->pieces.size() == 1 && nodes[0]->pieces[0] == literal &&
              result.cst().text(nodes[0]->cstNodeId) == literal, literal);
    check(cursiv::test::validationHelper(services, source).diagnostics.empty(),
          literal);
  }
  const std::string byte(1, static_cast<char>(0xFF));
  const std::string byteControls[] = {
      "const char*s=\"" + byte + "\";",
      "int value='" + byte + "';",
      "id f(void){return @\"" + byte + "\";}",
      "const char*s=u8\"" + byte + "\";",
      "int value=u8'" + byte + "';",
      "const int*s=L\"" + byte + "\";",
  };
  for (std::size_t i = 0; i < std::size(byteControls); ++i) {
    const auto& source = byteControls[i];
    const auto result = services.parser->parse(source);
    check(tree(result, source), "raw source byte ownership");
    const auto validation = cursiv::test::validationHelper(services, source);
    check(validation.diagnostics.empty() == (i < 3),
          "ordinary bytes versus Unicode literal encoding");
  }
  return ok;
}
}  // namespace objc_resume
