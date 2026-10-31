#pragma once

#include <cstdio>
#include <string_view>

#include "clu/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace clu_representation_cases {

inline bool run(const cursiv::Parser& parser) {
  const auto fail=[](const char* message) {
    std::fprintf(stderr,"clu representation: %s\n",message);return false;
  };
  for(const bool represented:{false,true}) {
    const std::string_view source=represented
        ? "utility = cluster is ping before = int rep = int after = bool own n: int := 1 ping = proc () end ping end utility"
        : "utility = cluster is ping before = int own n: int := 1 ping = proc () end ping end utility";
    const auto result=parser.parse(source);
    const auto* module=cursiv::ast_cast<clu::ast::Module>(result.value);
    const auto* cluster=module?cursiv::ast_cast<clu::ast::Cluster>(module->routine):nullptr;
    if(!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
       !cluster || cluster->name!="utility" || cluster->parent!=module ||
       cluster->isNames.size()!=1 || cluster->isNames[0]!="ping" ||
       cluster->preRepEquates.size()!=1 || cluster->preRepEquates[0]->name!="before" ||
       cluster->preRepEquates[0]->parent!=cluster ||
       cluster->postRepEquates.size()!=(represented?1u:0u) ||
       cluster->ownVars.size()!=1 || cluster->ownVars[0]->parent!=cluster ||
       cluster->routines.size()!=1 || cluster->routines[0]->name!="ping" ||
       cluster->routines[0]->parent!=cluster || cluster->endName!="utility")
      return fail("optional representation changes equates/body/order/ownership");
    const auto* rep=cursiv::ast_cast<clu::ast::BuiltinTypeSpec>(cluster->rep);
    if(represented ? !rep || rep->kind!=clu::ast::Builtin::int_ || rep->parent!=cluster ||
                     cluster->postRepEquates[0]->name!="after" || cluster->postRepEquates[0]->parent!=cluster
                   : cluster->rep!=nullptr)
      return fail("absent representation must remain null; explicit rep type must be retained");
  }
  for(const std::string_view source:{
      "utility = cluster is ping rep int ping = proc () end ping end utility",
      "utility = cluster is ping rep = ping = proc () end ping end utility",
      "utility = cluster is ping ping = proc () end ping rep = int end utility",
      "utility = cluster is ping end utility",
      "utility = cluster is ping rep = int end utility"}) {
    const auto result=parser.parse(source);
    if(result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("malformed or late representation unexpectedly parses clean");
  }
  return true;
}

} // namespace clu_representation_cases
