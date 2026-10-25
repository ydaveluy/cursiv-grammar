#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "memcachedprotocol/core/CoreServices.hpp"
#include "memcachedprotocol/core/ast.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace audit_regressions {
using Fields = std::vector<std::vector<std::string>>;
inline std::string text(const auto& value) {
  return value ? std::string(*value) : "<absent>";
}
inline void collect(const cursiv::AstNode* value, Fields& out) {
  namespace ast = memcachedprotocol::ast;
  if (const auto* n = cursiv::ast_cast<ast::CommandLine>(value)) {
    for (const auto* c : n->commands) collect(c, out);
  } else if (const auto* n = cursiv::ast_cast<ast::StorageCommand>(value)) {
    const char* names[] = {"set", "add", "replace", "append", "prepend", "cas"};
    out.push_back({"storage", names[int(n->op)], std::string(n->key), std::string(n->flags),
                   std::string(n->exptime), std::string(n->bytes), text(n->casUnique), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::RetrievalCommand>(value)) {
    out.push_back({"retrieval", std::string(n->operation), text(n->exptime), std::to_string(n->keys.size())});
    for (const auto key : n->keys) out.push_back({"key", std::string(key)});
  } else if (const auto* n = cursiv::ast_cast<ast::TouchCommand>(value)) {
    out.push_back({"touch", std::string(n->key), std::string(n->exptime), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::DeleteCommand>(value)) {
    out.push_back({"delete", std::string(n->key), text(n->time), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::IncrementCommand>(value)) {
    out.push_back({"incr", std::string(n->key), std::string(n->amount), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::DecrementCommand>(value)) {
    out.push_back({"decr", std::string(n->key), std::string(n->amount), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::StatisticsCommand>(value)) {
    out.push_back({"stats", text(n->option), std::to_string(n->arguments.size())});
    for (const auto argument : n->arguments) out.push_back({"argument", std::string(argument)});
  } else if (const auto* n = cursiv::ast_cast<ast::MetaCommand>(value)) {
    out.push_back({"meta", std::string(n->operation), text(n->key), text(n->bytes), std::to_string(n->flags.size())});
    for (const auto flag : n->flags) out.push_back({"flag", std::string(flag)});
  } else if (const auto* n = cursiv::ast_cast<ast::FlushCommand>(value)) {
    out.push_back({"flush_all", text(n->delay), n->noreply ? "1" : "0"});
  } else if (const auto* n = cursiv::ast_cast<ast::VerbosityCommand>(value)) {
    out.push_back({"verbosity", std::string(n->level), n->noreply ? "1" : "0"});
  } else if (cursiv::ast_cast<ast::VersionCommand>(value)) out.push_back({"version"});
  else out.push_back({"unknown"});
}
inline bool run(const memcachedprotocol::MemcachedProtocolCoreServices& services,
                const memcachedprotocol::MemcachedProtocolSharedCoreServices& shared) {
  struct Case { const char* file; Fields fields; };
  const Case cases[] = {
    {"00.memcachedprotocol",{{"retrieval","get","<absent>","2"},{"key","a"},{"key","b"}}},
    {"01.memcachedprotocol",{{"retrieval","gets","<absent>","1"},{"key","café"}}},
    {"02.memcachedprotocol",{{"retrieval","get","<absent>","1"},{"key","a\tb"}}},
    {"03.memcachedprotocol",{{"retrieval","gat","-1","2"},{"key","a"},{"key","b"}}},
    {"04.memcachedprotocol",{{"retrieval","gats","+30","1"},{"key","key"}}},
    {"05.memcachedprotocol",{{"touch","café","-1","0"}}},
    {"06.memcachedprotocol",{{"touch","key","+00030","1"}}},
    {"07.memcachedprotocol",{{"stats","settings","0"}}},
    {"08.memcachedprotocol",{{"stats","cachedump","2"},{"argument","1"},{"argument","0"}}},
    {"09.memcachedprotocol",{{"stats","detail","1"},{"argument","on"}}},
    {"10.memcachedprotocol",{{"stats","reset","0"}}},
    {"11.memcachedprotocol",{{"verbosity","+0","1"}}},
    {"12.memcachedprotocol",{{"flush_all","-1","0"}}},
    {"13.memcachedprotocol",{{"incr","café","+2","0"}}},
    {"14.memcachedprotocol",{{"decr","key","18446744073709551615","0"}}},
    {"15.memcachedprotocol",{{"delete","key","0","1"}}},
    {"16.memcachedprotocol",{{"version"}}},
    {"17.memcachedprotocol",{{"retrieval","bget","<absent>","1"},{"key","key"}}},
    {"18.memcachedprotocol",{{"meta","mg","key","<absent>","1"},{"flag","v"}}},
    {"19.memcachedprotocol",{{"meta","md","key","<absent>","0"}}},
    {"20.memcachedprotocol",{{"meta","mn","<absent>","<absent>","0"}}},
    {"21.memcachedprotocol",{{"storage","set","key","00042","-1","3","<absent>","0"}}},
    {"22.memcachedprotocol",{{"storage","set","é","5","+30","0","<absent>","0"}}},
    {"23.memcachedprotocol",{{"storage","set","line","0","0","4","<absent>","0"}}},
    {"29.memcachedprotocol",{{"meta","ma","absent","<absent>","1"},{"flag","D5"}}},
    {"30.memcachedprotocol",{{"meta","me","absent","<absent>","0"}}},
    {"31.memcachedprotocol",{{"meta","ms","meta","3","2"},{"flag","F7"},{"flag","T30"}}},
    {"32.memcachedprotocol",{{"meta","mg","meta","<absent>","2"},{"flag","v"},{"flag","f"}}},
    {"33.memcachedprotocol",{{"touch","wrap","4294967296","0"}}},
    {"34.memcachedprotocol",{{"verbosity","4294967296","0"}}},
    {"35.memcachedprotocol",{{"storage","set","wrap","4294967296","4294967296","4294967296","<absent>","0"}}},
  };
  for (const auto& test : cases) {
    std::ifstream input(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(input)), {});
    const auto result = services.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !result.value || result.value->parent) return false;
    Fields actual; collect(result.value, actual);
    if (actual != test.fields) return false;
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
    auto document = shared.documentFactory->fromString(source, "file:///audit.memcachedprotocol");
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(*document, diagnostics);
    if (!diagnostics.empty()) return false;
  }
  struct Invalid { const char* source; const char* code; };
  for (const auto& test : {
      Invalid{"set bad 0 0 -1\r\n", "memcachedprotocol.bytes"},
      Invalid{"incr key -1\r\n", "memcachedprotocol.delta"},
      Invalid{"delete key 30\r\n", "memcachedprotocol.delete-delay"},
      Invalid{"stats rubbish\r\n", "memcachedprotocol.stats-option"}}) {
    const std::string source(test.source);
    auto document = shared.documentFactory->fromString(source, "file:///audit.memcachedprotocol");
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
}  // namespace audit_regressions
