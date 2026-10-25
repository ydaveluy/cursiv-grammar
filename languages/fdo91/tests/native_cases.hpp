#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/grammar/Conversion.hpp"
#include "fdo91/core/CoreServices.hpp"
#include "fdo91/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace native_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline void atoms(const cursiv::AstNode::vector<fdo91::ast::Atom*>&, Fields&);
inline void argument(const fdo91::ast::ArgValue& value, Fields& fields) {
  namespace ast = fdo91::ast;
  std::visit([&](auto x) {
    using T = decltype(x);
    if constexpr (std::is_same_v<T, std::string_view>) fields.push_back({"string", std::string(x)});
    else if constexpr (std::is_same_v<T, ast::Ident*>) fields.push_back({"ident", x ? std::string(x->text) : "<null>"});
    else if constexpr (std::is_same_v<T, ast::Gid*>) fields.push_back({"gid", x ? std::string(x->text) : "<null>"});
    else if constexpr (std::is_same_v<T, ast::Hex*>) fields.push_back({"hex", x ? std::string(x->text) : "<null>"});
    else if constexpr (std::is_same_v<T, ast::Number*>) {
      const auto number = x ? cursiv::default_convert<std::int64_t>(x->text) : cursiv::ConversionResult<std::int64_t>{};
      fields.push_back({"number", number ? std::to_string(*number.value) : "<invalid>"});
    } else if constexpr (std::is_same_v<T, ast::Piped*>) {
      fields.push_back({"pipe", x ? std::to_string(x->parts.size()) : "<null>"});
      if (x) for (const auto& part : x->parts) argument(part, fields);
    } else if constexpr (std::is_same_v<T, ast::AtomGroup*>) {
      fields.push_back({"group", "0"});
      if (x) atoms(x->atoms, fields);
    }
  }, value);
}
inline void atoms(const cursiv::AstNode::vector<fdo91::ast::Atom*>& values, Fields& fields) {
  fields.push_back({"atoms", std::to_string(values.size())});
  for (const auto* atom : values) {
    fields.push_back({"atom", std::string(atom->command), std::to_string(atom->args.size())});
    for (const auto& arg : atom->args) argument(arg, fields);
  }
}
inline bool run(const fdo91::Fdo91CoreServices& services,
                const fdo91::Fdo91SharedCoreServices& shared) {
  struct Case { const char* file; Fields native; Fields unresolvedSource; std::vector<std::string> numbers; };
  const Case cases[] = {
    {"00.fdo91",{{"atoms","2"},{"atom","uni_start_stream","0"},{"atom","uni_end_stream","0"}},{},{}},
    {"01.fdo91",{{"atoms","2"},{"atom","uni_start_stream","0"},{"atom","uni_end_stream","0"}},{},{}},
    {"02.fdo91",{{"atoms","1"},{"atom","mat_title","1"},{"string","Hello"}},{},{}},
    {"03.fdo91",{{"atoms","1"},{"atom","mat_title","1"},{"string","Joe\"Q\""}},{},{}},
    {"04.fdo91",{{"atoms","1"},{"atom","mat_title","1"},{"string","line\nend"}},{},{}},
    {"05.fdo91",{{"atoms","1"},{"atom","mat_title","1"},{"string","a\\\\b"}},{},{}},
    {"06.fdo91",{{"atoms","1"},{"atom","mat_title","1"},{"string","literal\\x00"}},{},{}},
    {"07.fdo91",{{"atoms","1"},{"atom","mat_precise_width","1"},{"number","-1"}},{},{"-1"}},
    {"08.fdo91",{{"atoms","1"},{"atom","mat_precise_width","0"}},{},{}},
    {"09.fdo91",{{"atoms","1"},{"atom","mat_color_face","3"},{"number","0"},{"number","102"},{"number","153"}},{},{"0","102","153"}},
    {"10.fdo91",{{"atoms","1"},{"atom","man_start_object","2"},{"ident","ind_group"},{"string","Title"}},{},{}},
    {"11.fdo91",{{"atoms","1"},{"atom","mat_orientation","1"},{"pipe","3"},{"ident","left"},{"ident","center"},{"number","4"}},{},{"4"}},
    {"12.fdo91",{{"atoms","1"},{"atom","foo2","1"},{"number","3"}},{},{"3"}},
    {"13.fdo91",{{"atoms","2"},{"atom","uni_start_stream","0"},{"atom","uni_end_stream","0"}},{},{}},
    {"14.fdo91",{{"atoms","1"},{"atom","sm_send_token_arg","2"},{"string","U1"},{"gid","16-14610"}},{},{}},
    {"15.fdo91",{{"atoms","1"},{"atom","mat_art_id","1"},{"gid","1-1-40898"}},{},{}},
    {"16.fdo91",{{"atoms","1"},{"atom","atom_unknown","5"},{"hex","00x"},{"hex","0ax"},{"hex","4bx"},{"hex","ABx"},{"hex","00X"}},{},{}},
    {"17.fdo91",{{"atoms","1"},{"atom","act_replace_select_action","1"},{"group","0"},{"atoms","2"},{"atom","uni_start_stream","0"},{"atom","uni_end_stream","0"}},{},{}},
    {"18.fdo91",{{"atoms","1"},{"atom","act_replace_select_action","1"},{"group","0"},{"atoms","1"},{"atom","uni_start_stream","0"}},{{"atoms","1"},{"atom","act_replace_select_action","1"},{"ident","uni_start_stream"}},{}},
    {"19.fdo91",{{"atoms","1"},{"atom","mat_orientation","1"},{"ident","ind_group"}},{},{}},
    {"20.fdo91",{{"atoms","1"},{"atom","act_replace_select_action","1"},{"group","0"},{"atoms","2"},{"atom","uni_start_stream","0"},{"atom","uni_end_stream","0"}},{},{}},
    {"21.fdo91",{{"atoms","1"},{"atom","act_replace_select_action","1"},{"group","2"},{"atoms","1"},{"atom","uni_start_stream","0"},{"hex","00x"},{"hex","07x"}},{{"atoms","1"},{"atom","act_replace_select_action","3"},{"ident","uni_start_stream"},{"hex","00x"},{"hex","07x"}},{}},
    {"22.fdo91",{{"atoms","1"},{"atom","unknown_outer","1"},{"group","0"},{"atoms","2"},{"atom","foo","0"},{"atom","bar","0"}},{},{}},
    {"23.fdo91",{{"atoms","1"},{"atom","mat_art_id","1"},{"gid","1-2-3-4"}},{},{}},
    {"30.fdo91",{{"atoms","0"}},{},{}},
    {"31.fdo91",{{"atoms","0"}},{},{}},
    {"34.fdo91",{{"atoms","1"},{"atom","mat_precise_width","1"},{"number","123"}},{},{"000123"}},
    {"35.fdo91",{{"atoms","1"},{"atom","act_replace_select_action","1"},{"group","0"},{"atoms","0"}},{},{}},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<fdo91::ast::Doc>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !root || root->parent) return false;
    Fields actual; atoms(root->atoms, actual);
    if (test.unresolvedSource.empty()) { if (actual != test.native) return false; }
    else if (actual == test.native || actual != test.unresolvedSource) return false;
    bool parents = true;
    std::vector<std::string> numbers;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      if (const auto* number = cursiv::ast_cast<fdo91::ast::Number>(&node)) numbers.emplace_back(number->text);
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    visit(visit, *root);
    if (!parents || seen.size() != result.nodes.size() || numbers != test.numbers) return false;
    auto document = shared.documentFactory->fromString(source, "file:///audit.fdo91");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return false;
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& test : {
      Invalid{"mat_precise_width<9223372036854775808>", "fdo91.number-range"},
      Invalid{"mat_orientation<4|center>", "fdo91.pipe-first-name"},
      Invalid{"mat_orientation<left|\"center\">", "fdo91.pipe-part"}}) {
    const std::string source(test.source);
    auto document = shared.documentFactory->fromString(source, "file:///audit.fdo91");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered()) return false;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& d : diagnostics) found |= d.code && d.code->isString() && d.code->string() == test.code;
    if (!found) return false;
  }
  const std::string legacy = "atom$mat_title \"legacy\"";
  const auto result = services.parser->parse(legacy);
  const auto* root = cursiv::ast_cast<fdo91::ast::Doc>(result.value);
  if (!result.accepted || result.cst().recovered() || !root || root->atoms.size() != 1 ||
      !root->atoms[0]->atomPrefix || root->atoms[0]->bracketed || root->atoms[0]->command != "mat_title") return false;
  return true;
}
}  // namespace native_regressions
