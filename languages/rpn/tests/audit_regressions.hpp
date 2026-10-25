#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "rpn/core/CoreServices.hpp"
#include "rpn/core/ast.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

namespace audit_regressions {
inline std::string_view spelling(rpn::ast::Operator op) {
  switch (op) {
    case rpn::ast::Operator::pow: return "^";
    case rpn::ast::Operator::plus: return "+";
    case rpn::ast::Operator::minus: return "-";
    case rpn::ast::Operator::times: return "*";
    case rpn::ast::Operator::div: return "/";
  }
  return {};
}
inline bool run(const rpn::RpnCoreServices& services) {
  struct Item {
    bool isOperator;
    const char* text;
    std::vector<rpn::ast::Operator> signs;
    bool numeric;
    bool compareBits;
    std::uint64_t bits;
  };
  struct Case { const char* file; std::vector<Item> items; };
  const Case cases[] = {
    {"00.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"",{},true,true,0x4000000000000000ull},{true,"+",{},false,false,0}}},
    {"01.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"cos",{},false,false,0}}},
    {"02.rpn",{{false,"cos",{},false,false,0}}},
    {"03.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"sin",{},false,false,0},{false,"tan",{},false,false,0},{false,"acos",{},false,false,0},{false,"asin",{},false,false,0},{false,"atan",{},false,false,0},{false,"ln",{},false,false,0},{false,"log",{},false,false,0}}},
    {"04.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"cosx",{},false,false,0},{false,"_foo",{},false,false,0},{false,"ABC",{},false,false,0}}},
    {"05.rpn",{{false,"",{rpn::ast::Operator::minus},true,true,0x4000000000000000ull},{false,"",{rpn::ast::Operator::minus,rpn::ast::Operator::minus},true,true,0x4008000000000000ull},{true,"+",{},false,false,0}}},
    {"06.rpn",{{false,"x",{rpn::ast::Operator::plus},false,false,0}}},
    {"07.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"",{rpn::ast::Operator::plus,rpn::ast::Operator::plus},true,true,0x4000000000000000ull}}},
    {"08.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{false,"",{},true,true,0x4000000000000000ull},{true,"-",{},false,false,0}}},
    {"09.rpn",{{false,"",{},true,true,0x3fbf9a6b50b0f27cull}}},
    {"10.rpn",{{false,"",{},true,true,0x0000000000000000ull}}},
    {"11.rpn",{{false,"",{},true,false,0}}},
    {"12.rpn",{{false,"",{},true,true,0x0000000000000000ull}}},
    {"13.rpn",{{false,"",{},true,true,0x3f50624dd2f1a9fcull}}},
    {"14.rpn",{{false,"",{},true,true,0x7ff0000000000000ull}}},
    {"15.rpn",{{false,"",{},true,true,0x0000000000000000ull}}},
    {"16.rpn",{{false,"",{},true,true,0x3ff0000000000000ull}}},
    {"17.rpn",{{false,"",{},true,true,0x4206fee0e1a9e065ull}}},
    {"21.rpn",{{false,"",{},true,true,0x3ff0000000000000ull},{true,"*",{},false,false,0},{false,"",{},true,true,0x4000000000000000ull}}},
    {"22.rpn",{{false,"",{},true,true,0x0000000000000000ull}}},
    {"23.rpn",{{false,"",{},true,true,0x7ff0000000000000ull}}},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<rpn::ast::Expression>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !root || root->parent || root->items.size() != test.items.size()) return false;
    std::size_t nodes = 1;
    for (std::size_t i = 0; i < test.items.size(); ++i) {
      const auto& expected = test.items[i];
      if (expected.isOperator) {
        const auto* op = std::get_if<rpn::ast::Operator>(&root->items[i]);
        if (!op || spelling(*op) != expected.text) return false;
      } else {
        const auto* value = std::get_if<rpn::ast::SignedAtom*>(&root->items[i]);
        if (!value || !*value || (*value)->parent != root || !std::equal((*value)->signs.begin(), (*value)->signs.end(), expected.signs.begin(), expected.signs.end()) ||
            !result.cst().contains(root->cstNodeId, (*value)->cstNodeId)) return false;
        ++nodes;
        if (expected.numeric) {
          const auto* number = std::get_if<double>(&(*value)->value);
          if (!number || (expected.compareBits && std::bit_cast<std::uint64_t>(*number) != expected.bits)) return false;
        } else {
          const auto* text = std::get_if<std::string_view>(&(*value)->value);
          if (!text || *text != expected.text) return false;
        }
      }
    }
    if (result.nodes.size() != nodes) return false;
  }
  return true;
}
}  // namespace audit_regressions
