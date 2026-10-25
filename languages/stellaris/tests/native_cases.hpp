#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "stellaris/core/CoreServices.hpp"
#include "stellaris/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace native_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline std::string hex(std::string_view text) {
  std::string result; const char* digits = "0123456789abcdef";
  for (const unsigned char c : text) { result += digits[c >> 4]; result += digits[c & 15]; }
  return result;
}
inline void collect(const cursiv::AstNode* value, Fields& tree, Fields& tokens) {
  namespace ast = stellaris::ast;
  if (const auto* n = cursiv::ast_cast<ast::Document>(value)) {
    tree.push_back({"document", std::to_string(n->items.size())});
    for (const auto* child : n->items) collect(child, tree, tokens);
  } else if (const auto* n = cursiv::ast_cast<ast::Scalar>(value)) {
    tree.push_back({"scalar", std::string(n->text), n->quoted ? "1" : "0"});
    tokens.push_back({"scalar", hex(n->text), n->quoted ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::KeyValue>(value)) {
    tree.push_back({"pair", n->op.empty() ? "=" : std::string(n->op)});
    collect(n->key, tree, tokens);
    if (!n->op.empty()) tokens.push_back({"op", std::string(n->op)});
    collect(n->value, tree, tokens);
  } else if (const auto* n = cursiv::ast_cast<ast::Group>(value)) {
    tree.push_back({"group", n->header ? "header" : "", std::to_string(n->items.size())});
    if (n->header) collect(n->header, tree, tokens);
    tokens.push_back({"open"});
    for (const auto* child : n->items) collect(child, tree, tokens);
    if (n->closed) tokens.push_back({"close"});
  } else tree.push_back({"unknown"});
}
inline bool run(const stellaris::StellarisCoreServices& services,
                const stellaris::StellarisSharedCoreServices& shared) {
  struct Case { const char* file; Fields tree; Fields tokens; bool bom; bool unclosed; bool streamMatchesTape; };
  const Case cases[] = {
    {"00.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","3"},{"scalar","a","0"},{"scalar","b","0"},{"scalar","c","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"scalar","62","0"},{"scalar","63","0"},{"close"}},false,false,true},
    {"01.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"close"}},false,false,true},
    {"02.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","2"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"},{"pair","="},{"scalar","b","0"},{"scalar","2","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"scalar","62","0"},{"op","="},{"scalar","32","0"},{"close"}},false,false,true},
    {"03.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","4"},{"scalar","a","0"},{"pair","="},{"scalar","b","0"},{"scalar","2","0"},{"scalar","c","0"},{"pair","="},{"scalar","d","0"},{"scalar","4","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"scalar","62","0"},{"op","="},{"scalar","32","0"},{"scalar","63","0"},{"scalar","64","0"},{"op","="},{"scalar","34","0"},{"close"}},false,false,true},
    {"04.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","3"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"},{"scalar","b","0"},{"pair","="},{"scalar","c","0"},{"scalar","3","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"scalar","62","0"},{"scalar","63","0"},{"op","="},{"scalar","33","0"},{"close"}},false,false,true},
    {"05.stellaris",{{"document","1"},{"pair","="},{"scalar","color","0"},{"group","header","3"},{"scalar","rgb","0"},{"scalar","1","0"},{"scalar","2","0"},{"scalar","3","0"}},{{"scalar","636f6c6f72","0"},{"op","="},{"scalar","726762","0"},{"open"},{"scalar","31","0"},{"scalar","32","0"},{"scalar","33","0"},{"close"}},false,false,true},
    {"06.stellaris",{{"document","1"},{"pair","="},{"scalar","color","0"},{"group","header","3"},{"scalar","hsv","0"},{"scalar","0.1","0"},{"scalar",".5","0"},{"scalar","1.0","0"}},{{"scalar","636f6c6f72","0"},{"op","="},{"scalar","687376","0"},{"open"},{"scalar","302e31","0"},{"scalar","2e35","0"},{"scalar","312e30","0"},{"close"}},false,false,true},
    {"07.stellaris",{{"document","2"},{"pair","="},{"scalar","@var","0"},{"scalar","10","0"},{"pair","="},{"scalar","key","0"},{"scalar","from.owner@scope:age","0"}},{{"scalar","40766172","0"},{"op","="},{"scalar","3130","0"},{"scalar","6b6579","0"},{"op","="},{"scalar","66726f6d2e6f776e65724073636f70653a616765","0"}},false,false,true},
    {"08.stellaris",{{"document","1"},{"pair","="},{"scalar","name","0"},{"scalar","Joe \\\"Q\\\"","1"}},{{"scalar","6e616d65","0"},{"op","="},{"scalar","4a6f65205c22515c22","1"}},false,false,true},
    {"09.stellaris",{{"document","1"},{"pair","="},{"scalar","msg","0"},{"scalar","line\nnext","1"}},{{"scalar","6d7367","0"},{"op","="},{"scalar","6c696e650a6e657874","1"}},false,false,true},
    {"10.stellaris",{{"document","1"},{"pair","="},{"scalar","quote","0"},{"scalar","a\\\\b\\n\\q","1"}},{{"scalar","71756f7465","0"},{"op","="},{"scalar","615c5c625c6e5c71","1"}},false,false,true},
    {"11.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","é","0"}},{{"scalar","78","0"},{"op","="},{"scalar","c3a9","0"}},true,false,true},
    {"12.stellaris",{{"document","1"},{"pair","="},{"scalar","file","0"},{"scalar","gfx/interface/foo-bar.dds","0"}},{{"scalar","66696c65","0"},{"op","="},{"scalar","6766782f696e746572666163652f666f6f2d6261722e646473","0"}},false,false,true},
    {"13.stellaris",{{"document","1"},{"pair","="},{"scalar","arr","0"},{"group","","2"},{"group","","1"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"},{"group","","1"},{"pair","="},{"scalar","b","0"},{"scalar","2","0"}},{{"scalar","617272","0"},{"op","="},{"open"},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"close"},{"open"},{"scalar","62","0"},{"op","="},{"scalar","32","0"},{"close"},{"close"}},false,false,true},
    {"14.stellaris",{{"document","5"},{"pair","!="},{"scalar","a","0"},{"scalar","b","0"},{"pair","?="},{"scalar","c","0"},{"scalar","d","0"},{"pair","=="},{"scalar","e","0"},{"scalar","f","0"},{"pair","<="},{"scalar","g","0"},{"scalar","h","0"},{"pair",">="},{"scalar","i","0"},{"scalar","j","0"}},{{"scalar","61","0"},{"op","!="},{"scalar","62","0"},{"scalar","63","0"},{"op","?="},{"scalar","64","0"},{"scalar","65","0"},{"op","=="},{"scalar","66","0"},{"scalar","67","0"},{"op","<="},{"scalar","68","0"},{"scalar","69","0"},{"op",">="},{"scalar","6a","0"}},false,false,true},
    {"15.stellaris",{{"document","1"},{"pair","="},{"scalar","count","0"},{"scalar","123456789012345678901234567890","0"}},{{"scalar","636f756e74","0"},{"op","="},{"scalar","313233343536373839303132333435363738393031323334353637383930","0"}},false,false,true},
    {"16.stellaris",{{"document","1"},{"pair","="},{"scalar","arr","0"},{"group","","4"},{"scalar","1.2.3","0"},{"scalar","-1.5","0"},{"scalar","+0001","0"},{"scalar","1e999","0"}},{{"scalar","617272","0"},{"op","="},{"open"},{"scalar","312e322e33","0"},{"scalar","2d312e35","0"},{"scalar","2b30303031","0"},{"scalar","3165393939","0"},{"close"}},false,false,true},
    {"17.stellaris",{{"document","2"},{"pair","="},{"scalar","x","0"},{"scalar","y","0"},{"pair","="},{"scalar","z","0"},{"scalar"," padded ","1"}},{{"scalar","78","0"},{"op","="},{"scalar","79","0"},{"scalar","7a","0"},{"op","="},{"scalar","2070616464656420","1"}},false,false,true},
    {"18.stellaris",{{"document","0"}},{},false,false,true},
    {"19.stellaris",{{"document","0"}},{},false,false,true},
    {"20.stellaris",{{"document","1"},{"pair","="},{"scalar","root","0"},{"group","","1"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"}},{{"scalar","726f6f74","0"},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"close"}},false,false,true},
    {"21.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","2"},{"pair","="},{"scalar","p","0"},{"scalar","1","0"},{"pair","="},{"scalar","implicit","0"},{"group","","1"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","70","0"},{"op","="},{"scalar","31","0"},{"scalar","696d706c69636974","0"},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"close"},{"close"}},false,false,true},
    {"22.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","3"},{"scalar","a","0"},{"scalar","b","0"},{"group","","1"},{"scalar","c","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"scalar","62","0"},{"open"},{"scalar","63","0"},{"close"},{"close"}},false,false,true},
    {"23.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","1"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"}},false,true,true},
    {"26.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","2"},{"scalar","a","0"},{"scalar","]","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"scalar","5d","0"},{"close"}},false,false,true},
    {"27.stellaris",{{"document","2"},{"pair","="},{"scalar","@v","0"},{"scalar","10","0"},{"pair","="},{"scalar","x","0"},{"scalar","@[foo bar]","0"}},{{"scalar","4076","0"},{"op","="},{"scalar","3130","0"},{"scalar","78","0"},{"op","="},{"scalar","405b666f6f206261725d","0"}},false,false,true},
    {"28.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","trailing \\\"","1"}},{{"scalar","78","0"},{"op","="},{"scalar","747261696c696e67205c22","1"}},false,false,true},
    {"29.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","5"},{"pair","="},{"scalar","first","0"},{"scalar","1","0"},{"pair","="},{"scalar","second","0"},{"group","","1"},{"pair","="},{"scalar","z","0"},{"scalar","2","0"},{"scalar","tail","0"},{"scalar","third","0"},{"group","","1"},{"pair","="},{"scalar","q","0"},{"scalar","4","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","6669727374","0"},{"op","="},{"scalar","31","0"},{"scalar","7365636f6e64","0"},{"open"},{"scalar","7a","0"},{"op","="},{"scalar","32","0"},{"close"},{"scalar","7461696c","0"},{"scalar","7468697264","0"},{"open"},{"scalar","71","0"},{"op","="},{"scalar","34","0"},{"close"},{"close"}},false,false,true},
    {"30.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","7"},{"pair","="},{"scalar","a","0"},{"scalar","1","0"},{"scalar","b","0"},{"scalar","c","0"},{"group","","1"},{"scalar","d","0"},{"pair","="},{"scalar","e","0"},{"scalar","5","0"},{"scalar","f","0"},{"group","","1"},{"scalar","g","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","61","0"},{"op","="},{"scalar","31","0"},{"scalar","62","0"},{"scalar","63","0"},{"open"},{"scalar","64","0"},{"close"},{"scalar","65","0"},{"op","="},{"scalar","35","0"},{"scalar","66","0"},{"open"},{"scalar","67","0"},{"close"},{"close"}},false,false,true},
    {"31.stellaris",{{"document","1"},{"pair","="},{"scalar","y","0"},{"scalar","2","0"}},{{"scalar","79","0"},{"op","="},{"scalar","32","0"}},false,false,true},
    {"32.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","a\"b","0"}},{{"scalar","78","0"},{"op","="},{"scalar","612262","0"}},false,false,true},
    {"33.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","﻿key","0"}},{{"scalar","78","0"},{"op","="},{"scalar","6b6579","0"}},false,false,false},
    {"34.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","﻿","0"}},{{"scalar","78","0"},{"op","="}},false,false,false},
    {"35.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"scalar","pre﻿post","0"}},{{"scalar","78","0"},{"op","="},{"scalar","707265efbbbf706f7374","0"}},false,false,true},
    {"36.stellaris",{{"document","1"},{"pair","="},{"scalar","﻿x","0"},{"scalar","y","0"}},{{"scalar","efbbbf78","0"},{"op","="},{"scalar","79","0"}},true,false,true},
    {"37.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","2"},{"scalar","﻿","0"},{"scalar","bar","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","626172","0"},{"close"}},false,false,false},
    {"38.stellaris",{{"document","1"},{"pair","="},{"scalar","x","0"},{"group","","2"},{"scalar","﻿","0"},{"scalar","﻿bar","0"}},{{"scalar","78","0"},{"op","="},{"open"},{"scalar","efbbbf626172","0"},{"close"}},false,false,false},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    const auto* document = cursiv::ast_cast<stellaris::ast::Document>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !document || document->parent || document->bom != test.bom) return false;
    Fields tree, tokens; collect(result.value, tree, tokens);
    if (tree != test.tree || ((tokens == test.tokens) != test.streamMatchesTape)) return false;
    bool parents = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return false;
    auto sourceDocument = shared.documentFactory->fromString(source, "file:///audit.stellaris");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*sourceDocument, diagnostics);
    if (test.unclosed) {
      if (diagnostics.size() != 1 || !diagnostics[0].code ||
          !diagnostics[0].code->isString() || diagnostics[0].code->string() != "stellaris.unclosed-group") return false;
    } else if (!diagnostics.empty()) return false;
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& test : {Invalid{"x >< y", "stellaris.operator"},
                           Invalid{"bare", "stellaris.document-item"}}) {
    const std::string source(test.source);
    auto document = shared.documentFactory->fromString(source, "file:///audit.stellaris");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered()) return false;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    bool found = false;
    for (const auto& d : diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.code;
    if (!found) return false;
  }
  return true;
}
}  // namespace native_regressions
