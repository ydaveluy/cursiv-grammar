#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "less/core/CoreServices.hpp"
#include "less/core/ast.hpp"

namespace less_lookup_cases {
namespace a=less::ast;
inline std::string name(const a::Name* value){
  std::string out;
  if(value)for(const auto& part:value->parts){
    if(const auto* text=std::get_if<std::string_view>(&part))out+=*text;
    else out+="?";
  }
  return out;
}
inline std::string variable(const a::VariableName* value){
  if(!value)return {};
  return value->inner?"@@"+std::string(value->inner->name):"@"+std::string(value->name);
}
inline const a::Expr* atom(const a::CommandStatement* value){
  if(!value||value->terms.size()!=1)return nullptr;
  const auto* add=value->terms[0];
  const auto* mul=add?add->operand:nullptr;
  if(!mul||mul->terms.size()!=1)return nullptr;
  const auto* term=mul->terms[0];
  return term&&term->items.size()==1?term->items[0]:nullptr;
}
inline bool run(const less::LessCoreServices& services){
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message){
    if(!value){std::fprintf(stderr,"less lookup %s: %s\n",name,message);ok=false;}
  };
  struct Case{const char* name;const char* source;bool parses;std::size_t namespaces,calls,lookupCount;const char* lookups;const char* variable;bool hasArguments;std::size_t elements,args;const char* selector;};
  const Case cases[]={
    {"mixin-call-lookup","a{x:.m()[@c];}",true,1,1,1,"@c","",true,1,0,".m"},
    {"mixin-bare-lookup","a{x:.m[@c];}",true,1,1,1,"@c","",false,1,0,".m"},
    {"mixin-id-child-path","a{x:#ns > .m(1)[@c];}",true,1,1,1,"@c","",true,2,1,"#ns>.m"},
    {"lookup-chain","a{x:.m()[@a][$b][];}",true,1,1,3,"@a|$b|","",true,1,0,".m"},
    {"lookup-empty","a{x:.m()[];}",true,1,1,1,"","",true,1,0,".m"},
    {"lookup-numeric","a{x:.m()[123];}",true,1,1,1,"123","",true,1,0,".m"},
    {"lookup-property-indirect","a{x:.m()[$@key];}",true,1,1,1,"$@key","",true,1,0,".m"},
    {"lookup-prefix-at-dollar","a{x:.m()[@$key];}",true,1,1,1,"@$key","",true,1,0,".m"},
    {"lookup-variable-indirect","a{x:.m()[@@key];}",true,1,1,1,"@@key","",true,1,0,".m"},
    {"variable-lookup","a{x:@r[@c];}",true,1,0,1,"@c","@r",false,0,0,""},
    {"variable-indirect-lookup","a{x:@@r[@c];}",true,1,0,1,"@c","@@r",false,0,0,""},
    {"variable-call-value","a{x:@r();}",false,0,0,0,"","",false,0,0,""},
    {"variable-call-then-lookup","a{x:@r()[@c];}",false,0,0,0,"","",false,0,0,""},
    {"variable-gap-before-lookup","a{x:@r [@c];}",false,0,0,0,"","",false,0,0,""},
    {"mixin-gap-before-lookup","a{x:.m() [@c];}",true,1,1,1,"@c","",true,1,0,".m"},
    {"lookup-padding","a{x:.m()[ @c ];}",true,1,1,1,"@c","",true,1,0,".m"},
    {"lookup-internal-name-space","a{x:.m()[@ c];}",false,0,0,0,"","",false,0,0,""},
    {"lookup-quoted","a{x:.m()[\"c\"];}",false,0,0,0,"","",false,0,0,""},
    {"lookup-three-prefixes","a{x:.m()[@@@c];}",false,0,0,0,"","",false,0,0,""},
    {"bare-value-control","a{x:.m;}",false,0,0,0,"","",false,0,0,""},
    {"mixin-call-value","a{x:.m();}",true,0,1,0,"","",true,1,0,".m"},
    {"mixin-comma-arguments","a{x:.m(1,2)[@c];}",true,1,1,1,"@c","",true,1,2,".m"},
    {"mixin-named-semicolon-arguments","a{x:.m(@x:1;@y:2)[@c];}",true,1,1,1,"@c","",true,1,2,".m"},
    {"lookup-in-function-argument","a{x:f(.m()[@c],1);}",true,1,1,1,"@c","",true,1,0,".m"},
    {"mixin-escaped-name","a{x:.m\\69 x()[@c];}",true,1,1,1,"@c","",true,1,0,".m\\69 x"},
    {"hex-id-bare-lookup","a{x:#abc[@c];}",true,1,1,1,"@c","",false,1,0,"#abc"},
    {"hex-id-paren-lookup","a{x:#abc()[@c];}",false,0,0,0,"","",false,0,0,""},
    {"short-id-paren-lookup","a{x:#ab()[@c];}",true,1,1,1,"@c","",true,1,0,"#ab"},
    {"five-hex-bare-lookup","a{x:#abcde[@c];}",true,1,1,1,"@c","",false,1,0,"#abcde"},
    {"five-hex-paren-lookup","a{x:#abcde()[@c];}",true,1,1,1,"@c","",true,1,0,"#abcde"},
    {"six-hex-bare-lookup","a{x:#abcdef[@c];}",true,1,1,1,"@c","",false,1,0,"#abcdef"},
    {"six-hex-paren-lookup","a{x:#abcdef()[@c];}",false,0,0,0,"","",false,0,0,""},
    {"hex-color-control","a{x:#abc;}",true,0,0,0,"","",false,0,0,""},
  };
  for(const auto& test:cases){
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty()&&parsed.value;
    check(clean==test.parses,test.name,"native complete-source parser verdict");
    if(clean!=test.parses)for(const auto& d:parsed.diagnostics)
      std::fprintf(stderr,"kind%u element%u bytes%u..%u\n",unsigned(d.kind),d.element,d.begin,d.end);
    if(!clean)continue;
    check(parsed.cst().text()==test.source,test.name,"complete source text");
    std::vector<const a::NamespaceValue*> namespaces;
    std::vector<const a::MixinRef*> calls;
    for(const auto* node:parsed.nodes){
      if(const auto* value=cursiv::ast_cast<a::NamespaceValue>(node))namespaces.push_back(value);
      if(const auto* value=cursiv::ast_cast<a::MixinRef>(node))calls.push_back(value);
    }
    check(namespaces.size()==test.namespaces&&calls.size()==test.calls,test.name,"native namespace/mixin-call classification");
    if(namespaces.size()==1){
      const auto* ns=namespaces[0];
      std::string keys;
      for(std::size_t i=0;i<ns->lookups.size();++i){if(i)keys+='|';keys+=ns->lookups[i];}
      check(ns->lookups.size()==test.lookupCount&&keys==test.lookups,test.name,"ordered raw lookup names, prefixes and empty []");
      check(bool(ns->call)!=bool(ns->variable),test.name,"exactly one native callee kind");
      if(ns->variable)check(ns->variable->parent==ns&&variable(ns->variable)==test.variable,test.name,"variable/indirect-variable raw spelling and owner");
      else check(ns->call&&ns->call->parent==ns&&test.variable[0]=='\0',test.name,"mixin invocation owned by lookup");
    }
    if(calls.size()==1){
      const auto* call=calls[0];
      const auto* target=call->target;
      check(target&&target->parent==call&&target->elements.size()==test.elements&&call->hasArguments==test.hasArguments&&!call->important,test.name,"native target structure, absent/empty parentheses and important flag");
      std::string selector;
      if(target)for(const auto* element:target->elements){
        check(element&&element->parent==target&&!element->descendant,test.name,"native path element owner/combinator kind");
        if(!element)continue;
        if(element->prefix){check(*element->prefix==a::SelectorPrefix::child,test.name,"only native child combinator in mixin path");selector+='>';}
        if(const auto* named=cursiv::ast_cast<a::NamedElement>(element))selector+=name(named->name);
        else if(const auto* id=cursiv::ast_cast<a::IdElement>(element))selector+="#"+name(id->name);
        else check(false,test.name,"native mixin target contains only class/id elements");
      }
      check(selector==test.selector,test.name,"callee path element source order and exact escaped spelling");
      std::size_t args=call->args.size();
      for(const auto* group:call->argumentGroups)if(group)args+=group->items.size();
      check(args==test.args,test.name,"native argument count retained across comma/semicolon modes");
      const std::string_view testName(test.name);
      if(testName=="mixin-comma-arguments"||testName=="mixin-id-child-path"){
        check(call->argumentGroups.empty()&&call->args.size()==test.args,test.name,"comma argument lane");
        for(std::size_t i=0;i<call->args.size();++i){
          const auto* number=cursiv::ast_cast<a::Measurement>(atom(call->args[i]));
          check(number&&number->value==i+1,test.name,"native positional argument value/order");
        }
      }
      if(testName=="mixin-named-semicolon-arguments"){
        check(call->args.empty()&&call->argumentGroups.size()==2,test.name,"semicolon named-argument lane");
        for(std::size_t i=0;i<call->argumentGroups.size();++i){
          const auto* group=call->argumentGroups[i];
          const auto* named=group&&group->items.size()==1?cursiv::ast_cast<a::NamedArgument>(atom(group->items[0])):nullptr;
          const auto* value=named&&named->value&&named->value->items.size()==1?cursiv::ast_cast<a::Measurement>(atom(named->value->items[0])):nullptr;
          check(named&&variable(named->name)==(i==0?"@x":"@y")&&value&&value->value==i+1,test.name,"native named argument name/value/order");
        }
      }
      if(!test.namespaces)check(cursiv::ast_cast<a::MulTerm>(call->parent)!=nullptr,test.name,"direct invocation remains an expression, not a statement consuming its separator");
    }
    if(std::string_view(test.name)=="hex-color-control"){
      bool color=false;for(const auto* node:parsed.nodes)if(const auto* value=cursiv::ast_cast<a::ColorLit>(node))color|=value->value=="#abc";
      check(color,test.name,"ordinary hexadecimal color kept distinct from namespace call");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while(!work.empty()){
      const auto* node=work.back();work.pop_back();
      if(!seen.insert(node).second){check(false,test.name,"unique AST owner");continue;}
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child){
        check(child.parent==node&&node->cstNodeId<parsed.cst().size()&&child.cstNodeId<parsed.cst().size()&&parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"all typed parents and CST containment");work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size()&&!parsed.value->parent,test.name,"every allocated node reachable once");
  }
  // Existing statement-call API keeps its separator/important ownership;
  // these are primary-backed regressions, not extra fresh native witnesses.
  for(const auto* source:{"a{.m(1);}","a{.m(1)!important;}"}){
    const auto r=services.parser->parse(source);
    check(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty()&&r.value,"statement-regression","old statement syntax preserved");
    bool found=false;for(const auto* node:r.nodes)if(const auto* call=cursiv::ast_cast<a::MixinRef>(node))
      found|=call->hasArguments&&call->important==(std::string_view(source).find("important")!=std::string_view::npos)&&cursiv::ast_cast<a::Block>(call->parent);
    check(found,"statement-regression","old call fields plus explicit-parentheses flag and immediate block owner");
  }
  std::printf("less lookup: %zu native parser/AST cases\n",std::size(cases));
  return ok;
}
} // namespace less_lookup_cases
