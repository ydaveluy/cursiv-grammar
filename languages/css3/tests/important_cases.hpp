#pragma once

#include <cstdio>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "css3/core/CoreServices.hpp"
#include "css3/core/ValueConverters.hpp"
#include "css3/core/ast.hpp"

namespace css3_important_cases {
namespace a=css3::ast;
inline std::string signature(const a::ComponentValue* value, std::pmr::memory_resource* memory);
template <class Values>
std::string sequence(const Values& values,std::pmr::memory_resource* memory) {
  std::string result;
  for (const auto* value:values) {
    if (!result.empty()) result += "|";
    result += signature(value,memory);
  }
  return result;
}
inline std::string signature(const a::ComponentValue* value,std::pmr::memory_resource* memory) {
  if (const auto* n=cursiv::ast_cast<a::IdentValue>(value)) return "id:"+std::string(css3::decode_css(n->name,memory));
  if (const auto* n=cursiv::ast_cast<a::PunctValue>(value)) return "p:"+std::string(n->text);
  if (const auto* n=cursiv::ast_cast<a::StringLit>(value)) return "s:"+std::string(n->value);
  if (const auto* n=cursiv::ast_cast<a::FunctionCall>(value))
    return "fn:"+std::string(css3::decode_css(n->name,memory))+"("+sequence(n->args,memory)+")";
  if (const auto* n=cursiv::ast_cast<a::GroupValue>(value))
    return std::string(n->bracketed?"[]":"()")+"("+sequence(n->items,memory)+")";
  return "?";
}
inline bool run(const css3::Css3CoreServices& services) {
  bool ok=true;
  const auto check=[&](bool value,const char* name,const char* message) {
    if (!value) { std::fprintf(stderr,"css3 important %s: %s\n",name,message); ok=false; }
  };
  struct Case { const char* name; const char* source; bool important; const char* signature; };
  // 28 fresh tinycss2 agreements plus2 CSSSyntax3§5.4.6 normative controls.
  // The independent library loses the second bang candidate after a prior
  // important pair; those two observations remain immutable divergences.
  const Case cases[] = {
    {"canonical", "a{x:red!important;}", true, "id:red"},
    {"uppercase", "a{x:red !IMPORTANT;}", true, "id:red"},
    {"escaped-i", "a{x:red !\\69mportant;}", true, "id:red"},
    {"escaped-i-six", "a{x:red !\\000069mportant;}", true, "id:red"},
    {"escaped-I-six", "a{x:red !\\000049MPORTANT;}", true, "id:red"},
    {"simple-escapes", "a{x:red !\\i\\m\\p\\o\\r\\t\\61nt;}", true, "id:red"},
    {"hex-space", "a{x:red !\\69 mportant;}", true, "id:red"},
    {"hex-crlf", "a{x:red !\\69\r\nmportant;}", true, "id:red"},
    {"short-t-greedy", "a{x:red !impor\\74ant;}", false, "id:red|p:!|id:impor\u074ant"},
    {"six-t-boundary", "a{x:red !impor\\000074ant;}", true, "id:red"},
    {"short-t-space", "a{x:red !impor\\74 ant;}", true, "id:red"},
    {"simple-a-is-hex", "a{x:red !import\\ant;}", false, "id:red|p:!|id:import\nnt"},
    {"unknown-bang", "a{x:!foo;}", false, "p:!|id:foo"},
    {"marker-not-final", "a{x:!important x;}", false, "p:!|id:important|id:x"},
    {"marker-comment-not-final", "a{x:!important/**/x;}", false, "p:!|id:important|id:x"},
    {"trailing-comment", "a{x:red!important/*tail*/;}", true, "id:red"},
    {"double-marker", "a{x:red!important!important;}", true, "id:red|p:!|id:important"},
    {"important-function", "a{x:red!important();}", false, "id:red|p:!|fn:important()"},
    {"bang-function", "a{x:!important(foo);}", false, "p:!|fn:important(id:foo)"},
    {"identifier-boundary", "a{x:!importantx;}", false, "p:!|id:importantx"},
    {"quoted-marker", "a{x:\"!important\";}", false, "s:!important"},
    {"escaped-bang", "a{x:\\21important;}", false, "id:!important"},
    {"two-spaced-markers", "a{x:!important ! /*x*/important;}", true, "p:!|id:important"},
    {"bare-bang", "a{x:!;}", false, "p:!"},
    {"empty-value", "a{x:;}", false, ""},
    {"comment-separated-marker", "a{x:red !/*x*/important /*tail*/;}", true, "id:red"},
    {"comments-within-word", "a{x:red !im/**/portant;}", false, "id:red|p:!|id:im|id:portant"},
    {"six-t-crlf", "a{x:red !impor\\000074\r\nant;}", true, "id:red"},
    {"escaped-suffix", "a{x:red !\\69mportant\\x;}", false, "id:red|p:!|id:importantx"},
    {"nested-marker", "a{x:[!important];}", false, "[](p:!|id:important)"},
  };
  for (const auto& test:cases) {
    const auto parsed=services.parser->parse(test.source);
    const bool clean=parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean,test.name,"CSS Syntax component declaration parses clean");
    if (!clean) {
      for (const auto& d:parsed.diagnostics)
        std::fprintf(stderr,"  kind=%u element=%u bytes=%u..%u\n",unsigned(d.kind),d.element,d.begin,d.end);
      continue;
    }
    check(parsed.cst().text()==test.source,test.name,"complete spelling/comments retained");
    const a::Declaration* declaration=nullptr;
    std::size_t count=0;
    for (const auto* node:parsed.nodes) if (const auto* n=cursiv::ast_cast<a::Declaration>(node)) { declaration=n; ++count; }
    std::pmr::monotonic_buffer_resource memory;
    check(count==1 && declaration && declaration->property=="x",test.name,"one typed declaration retained");
    if (declaration) {
      check(declaration->important==test.important,test.name,"only final decoded important marker sets flag");
      check(sequence(declaration->value,&memory)==test.signature,test.name,"all non-marker bangs/identifiers/groups remain in ordered value AST");
    }
    if (std::string_view(test.name).find("crlf")!=std::string_view::npos)
      check(std::string_view(test.source).find("\r\n")!=std::string_view::npos,test.name,"real CRLF bytes reach lexer");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work={parsed.value};
    while (!work.empty()) {
      const auto* node=work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false,test.name,"unique AST owner"); continue; }
      services.astReflection->visitMembers(*node,[&](auto,auto,const auto& child) {
        check(child.parent==node && node->cstNodeId<parsed.cst().size() && child.cstNodeId<parsed.cst().size() &&
              parsed.cst().contains(node->cstNodeId,child.cstNodeId),test.name,"all parent links and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size()==parsed.nodes.size() && !parsed.value->parent,test.name,"every allocated node reachable once");
  }
  std::pmr::monotonic_buffer_resource memory;
  for (std::string_view text:{"","a","'","'a","a'","'a\""}) {
    const auto value=css3::unquote(text,&memory);
    check(!value && !value.error.message.empty(),"unquote-totality","malformed fragment gives a bounded error");
  }
  {
    const auto empty=css3::unquote("''",&memory);
    const auto escape=css3::unquote("'\\41'",&memory);
    const auto newline=css3::unquote("'a\\\r\nb'",&memory);
    check(empty && empty.value->empty() && escape && *escape.value=="A" && newline && *newline.value=="ab",
          "unquote-values","existing CSS escape decoding and continuation preserved");
  }
  for (const auto* source:{"a{x:'a;}","a{x:\"a;}","a{x:url('a);}"}) {
    const auto parsed=services.parser->parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),"string-recovery","invalid neighbors produce diagnostics without a converter exception");
  }
  return ok;
}
}  // namespace css3_important_cases
