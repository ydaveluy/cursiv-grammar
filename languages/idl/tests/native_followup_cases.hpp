#pragma once
#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "idl/core/ast.hpp"

namespace idl_native_followup {
inline bool run(const cursiv::Parser& parser) {
  const std::string source = R"idl(
const fixed PRICE = 12.50D;
const fixed INTEGER_PRICE = 12D;
const fixed FRACTION = .5d;
const char HEX = '\xA';
const char ALERT = '\a';
const char VERTICAL = '\v';
const char QUESTION = '\?';
const wchar SHORT_UNICODE = L'\u3BC';
const string PARTS = "\xA" "B";
const wstring WIDE_PARTS = L"\u3BC" L"x";
interface Anonymous {
  sequence<long> list(in map<string,long> values);
  readonly attribute sequence<octet> bytes;
  void contextOp() context ("first" "second", "third");
};
bitset EmptyBits {};
bitset NamedBits { bitfield<1> low high; };
bitmask Positioned { @position(0) one, @position(4) flag4 };
module External { const long VALUE = 1; };
@late import ::External;
module Inside {
  @nested import ::External;
  interface I { @inside import ::External; };
};
)idl";
  const auto result = parser.parse(source);
  const auto* root = result.value_as<idl::ast::Specification>();
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !root) {
    std::fprintf(stderr, "idl native: clean standard source expected\n"); return false;
  }
  unsigned fixed = 0;
  bool shortUnicode=false, hex=false, strings=false, wideStrings=false, anonymousReturn=false;
  bool anonymousParam=false, anonymousAttribute=false, context=false;
  bool emptyBits=false, names=false, positions=false;
  unsigned imports=0, annotatedImports=0;
  for (const auto* node : result.nodes) {
    if (cursiv::ast_cast<idl::ast::FixedLit>(node)) ++fixed;
    if (const auto* literal = cursiv::ast_cast<idl::ast::WCharLit>(node))
      shortUnicode |= literal->text == "L'\\u3BC'";
    if (const auto* literal = cursiv::ast_cast<idl::ast::CharLit>(node)) hex |= literal->text == "'\\xA'";
    if (const auto* literal = cursiv::ast_cast<idl::ast::StringLit>(node))
      strings |= literal->text == "\"\\xA\"" && literal->continuations.size() == 1 &&
                 literal->continuations[0] == "\"B\"";
    if (const auto* literal = cursiv::ast_cast<idl::ast::WStringLit>(node))
      wideStrings |= literal->text == "L\"\\u3BC\"" && literal->continuations.size() == 1 &&
                     literal->continuations[0] == "L\"x\"";
    if (const auto* operation = cursiv::ast_cast<idl::ast::OpDecl>(node)) {
      anonymousReturn |= cursiv::ast_cast<idl::ast::SequenceTypeRef>(operation->returnType) != nullptr;
      anonymousParam |= operation->params.size() == 1 &&
                        cursiv::ast_cast<idl::ast::MapTypeRef>(operation->params[0]->type);
      context |= operation->context.size() == 2 &&
                 operation->context[0].find("first") != std::string_view::npos &&
                 operation->context[0].find("second") != std::string_view::npos;
    }
    if (const auto* attribute = cursiv::ast_cast<idl::ast::AttrDecl>(node))
      anonymousAttribute |= cursiv::ast_cast<idl::ast::SequenceTypeRef>(attribute->type) != nullptr;
    if (const auto* bits = cursiv::ast_cast<idl::ast::BitsetDecl>(node)) {
      emptyBits |= bits->name == "EmptyBits" && bits->fields.empty();
      names |= bits->name == "NamedBits" && bits->fields.size() == 1 &&
               bits->fields[0]->names.size() == 2 && bits->fields[0]->names[0] == "low" &&
               bits->fields[0]->names[1] == "high";
    }
    if (const auto* mask = cursiv::ast_cast<idl::ast::BitmaskDecl>(node))
      positions |= mask->values.size() == 2 && mask->values[0]->name == "one" &&
                   mask->values[1]->name == "flag4" && mask->values[0]->annotations.size() == 1 &&
                   mask->values[1]->annotations.size() == 1 &&
                   mask->values[1]->annotations[0]->arguments.size() == 1 &&
                   mask->values[1]->parent == mask;
    if (const auto* imported = cursiv::ast_cast<idl::ast::ImportDecl>(node)) {
      ++imports;
      annotatedImports += imported->annotations.size() == 1 &&
          imported->annotations[0]->parent == imported &&
          (imported->annotations[0]->name == "late" ||
           imported->annotations[0]->name == "nested" ||
           imported->annotations[0]->name == "inside");
    }
  }
  const bool ok = fixed == 3 && shortUnicode && hex && strings && wideStrings && anonymousReturn &&
                  anonymousParam && anonymousAttribute && context && emptyBits && names &&
                  positions && imports == 3 && annotatedImports == 3;
  if (!ok) std::fprintf(stderr, "idl native: fixed/string/anonymous-type AST fields lost\n");
  const auto onlyImport = parser.parse("@lead import Legacy;");
  const auto* imported = onlyImport.value_as<idl::ast::Specification>();
  return ok && onlyImport.accepted && !onlyImport.cst().recovered() &&
         onlyImport.diagnostics.empty() && imported && imported->imports.size() == 1 &&
         imported->imports[0]->annotations.size() == 1 &&
         imported->imports[0]->annotations[0]->name == "lead" &&
         imported->imports[0]->annotations[0]->parent == imported->imports[0];
}
} // namespace idl_native_followup
