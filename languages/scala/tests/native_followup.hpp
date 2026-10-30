#pragma once
#include "modern_cases.hpp"
namespace scala_audit {
template<class Parser> bool native_followup(const Parser& parser) {
  using namespace ast;
  for (const auto source : {
      "object P { f(values: _*: _*) }",
      "object P { f(values: _`*`: _*) }",
      "object P { f(xs: _+) }", "object P { val x=1abc }", "object P { val x=1é }",
      "object P { x match { case Some(xs @ _+) => 1 } }",
      "object P { x match { case _+ => 1 } }", "object P { val _+ = 1 }"}) {
    const auto result=parser.parse(source);
    if(result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr,"scala native: malformed splice accepted: %s\n",source);
      for(const auto* t:nodes<NamedType>(result))if(t->path)std::fprintf(stderr,"  named type root=%.*s\n",int(t->path->root.size()),t->path->root.data());
      for(const auto* t:nodes<AscribeExpr>(result))std::fprintf(stderr,"  ascription splice=%d hasType=%d\n",t->varargsSplice,t->type!=nullptr);
      return false;
    }
  }
  // The native parser represents both spellings as a Typed/WILDCARD_STAR
  // argument, not a second splice marker on the argument list.
  for(const auto* positive:{"object P { f(values: _*) }", "object P { f(values: _`*`) }"}) {
    const auto r=parser.parse(positive);
    const auto calls=nodes<CallExpr>(r);
    const auto typed=nodes<AscribeExpr>(r);
    const auto* call=calls.size()==1?calls[0]:nullptr;
    const auto* arg=typed.size()==1?typed[0]:nullptr;
    const auto* value=arg?cursiv::ast_cast<PathExpr>(arg->value):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!call||!arg||
       call->args.size()!=1||call->args[0]!=arg||call->varargsSplice||
       !arg->varargsSplice||arg->type||!arg->annotations.empty()||
       arg->parent!=call||!value||value->parent!=arg||!value->path||
       value->path->root!="values"||r.cst().text()!=positive) {
      std::fprintf(stderr,"scala native: repeated argument lost canonical Typed AST: %s\n",positive);
      for(const auto& d:r.diagnostics)
        std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
      return false;
    }
  }
  // The type guard excludes the underscore token, not longer identifiers.
  // Both names are ordinary named types in the native scanner/parser.
  struct TypeCase {const char* source; const char* name;};
  for(const auto probe:{
      TypeCase{"object P { f(values: _foo) }", "_foo"},
      TypeCase{"object P { f(values: `_+`) }", "`_+`"}}) {
    const auto r=parser.parse(probe.source);
    const auto calls=nodes<CallExpr>(r);
    const auto typed=nodes<AscribeExpr>(r);
    const auto* call=calls.size()==1?calls[0]:nullptr;
    const auto* arg=typed.size()==1?typed[0]:nullptr;
    const auto* type=arg?cursiv::ast_cast<NamedType>(arg->type):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!call||!arg||
       call->args.size()!=1||call->args[0]!=arg||arg->varargsSplice||
       !type||type->parent!=arg||!type->path||type->path->root!=probe.name||
       r.cst().text()!=probe.source) {
      std::fprintf(stderr,"scala native: underscore-prefixed/quoted ascription type lost: %s\n",probe.source);
      return false;
    }
  }
  const char* const source=
      "object P { x match { case name => 1; case Name => 2; case `name` => 3; "
      "case ns.name => 4; case whole @ Some(inner) => 5; "
      "case Some(xs @ _*) => 6; case _foo => 7; case `_+` => 8; case _+y => 9 }; "
      "f(values: _*); f(values: _`*`); val v=1 abc }";
  const auto result=parser.parse(source);
  bool ok=result.accepted && !result.cst().recovered() && result.diagnostics.empty();
  bool variable=false,stableUpper=false,stableQuoted=false,stableQualified=false,bound=false,splice=false;
  for(const auto* n:nodes<VarPattern>(result))variable|=n->name=="name";
  for(const auto* n:nodes<StablePattern>(result))if(n->path){
    stableUpper|=n->path->root=="Name";
    stableQuoted|=n->path->root=="`name`";
    stableQualified|=n->path->root=="ns" && n->path->segments.size()==1 && n->path->segments[0]=="name";
  }
  for(const auto* n:nodes<BindPattern>(result))bound|=n->name=="whole" && n->inner;
  for(const auto* n:nodes<ExtractorPattern>(result))splice|=n->varargsSplice && n->varargsBindName=="xs";
  if(!ok||!variable||!stableUpper||!stableQuoted||!stableQualified||!bound||!splice){
    std::fprintf(stderr,"scala native: variable/stable/bound patterns or sequence tail lost (%d%d%d%d%d%d%d)\n",ok,variable,stableUpper,stableQuoted,stableQualified,bound,splice);
    for(const auto& d:result.diagnostics)std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
    std::fprintf(stderr,"  original source=%s\n",source);
    // Diagnostic decompositions of the already required native-positive
    // specimen. Keep its original assertions and verdict unchanged.
    struct Probe {const char* label; const char* source;};
    for(const auto probe:{
        Probe{"plain-varargs","object P { f(values: _*) }"},
        Probe{"quoted-varargs","object P { f(values: _`*`) }"},
        Probe{"postfix-value","object P { val v=1 abc }"},
        Probe{"combined-calls","object P { f(values: _*); f(values: _`*`); val v=1 abc }"},
        Probe{"pattern-and-call","object P { x match { case _+y => 9 }; f(values: _*) }"}}) {
      const auto r=parser.parse(probe.source);
      std::fprintf(stderr,"  probe=%s accepted=%d repaired=%d diagnostics=%zu source=%s\n",
          probe.label,int(r.accepted),int(r.cst().recovered()),r.diagnostics.size(),probe.source);
      for(const auto& d:r.diagnostics)
        std::fprintf(stderr,"    kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
      for(const auto* n:nodes<AscribeExpr>(r))
        std::fprintf(stderr,"    AscribeExpr splice=%d type=%d\n",int(n->varargsSplice),n->type!=nullptr);
      for(const auto* n:nodes<CallExpr>(r))
        std::fprintf(stderr,"    CallExpr splice=%d args=%zu\n",int(n->varargsSplice),n->args.size());
    }

    return false;
  }
  return true;
}
}
