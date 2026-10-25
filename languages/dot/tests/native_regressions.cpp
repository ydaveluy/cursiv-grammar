#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "dot/core/CoreServices.hpp"
#include "dot/core/ast.hpp"
int main() {
 const auto language=cursiv::create_language<dot::DotCoreServices,dot::DotSharedCoreServices,dot::install_core_module>();
 struct Case { const char* name; const char* text; bool directed, strict; std::vector<std::string_view> nodes; };
 const std::vector<Case> cases={
  {"unicode-identifiers","graph G { 日本語; λ; 🙂; é; }",false,false,{"日本語","λ","🙂","é"}},
  {"keyword-unicode","digraph G { graphé; digraphλ; strict中; subgraph🙂; nodeé; edgeλ; }",true,false,{"graphé","digraphλ","strict中","subgraph🙂","nodeé","edgeλ"}},
  {"numeric-split","digraph G { 123abc -> z; }",true,false,{"123","abc","z"}},
  {"numeric-dot-split","digraph G { 123.4.5 -> z; }",true,false,{"123.4",".5","z"}},
  {"empty-id","digraph \"\" { \"\" -> z; }",true,false,{"","z"}},
  {"bom-spaced","\xEF\xBB\xBF graph G {a;}",false,false,{"a"}},
  {"bom-separated-nodes","digraph G {a; \xEF\xBB\xBF b;}",true,false,{"a","b"}},
  {"bom-name","digraph G {\xEF\xBB\xBF" "b;}",true,false,{"\xEF\xBB\xBF" "b"}},
  {"simple","digraph G { a -> b [label=\"x\"]; }",true,false,{"a","b"}},
  {"node-groups","digraph G { a,b,c [color=red]; a,b -> c,d; }",true,false,{"a","b","c","d"}},
  {"groups-ports","digraph G { a:p,b:q -> c:r,d:s [label=x]; }",true,false,{"a","b","c","d"}},
  {"subgraph-attrs","graph G { subgraph cluster { a; b; } [color=red]; }",false,false,{"a","b"}},
  {"quoted-comments","graph G { \"a\" /* \"wrong\" */ + \"b\"; }",false,false,{"ab"}},
  {"quoted-line-comments","graph G { \"a\" // \"wrong\"\n+ \"b\"; }",false,false,{"ab"}},
  {"quoted-html-concat","graph G { <a> + \"b\"; }",false,false,{"ab"}},
  {"quoted-escapes","graph G { \"a\\\"b\\nc\\\\d\"; }",false,false,{"a\"b\\nc\\\\d"}},
  {"quoted-lf","graph G { \"a\\\nb\"; }",false,false,{"ab"}},
  {"quoted-crlf","graph G { \"a\\\r\nb\"; }",false,false,{"a\\\r\nb"}},
  {"html-nested","graph G { <a <b<c>d>e>; }",false,false,{"a <b<c>d>e"}},
  {"case-keywords","StRiCt DiGrApH G { NoDe [shape=box]; a -> b; }",true,true,{"a","b"}},
  {"keywords-as-names","graph G { graphx; node2; strict_x; }",false,false,{"graphx","node2","strict_x"}},
  {"attributes","graph G { a [label=one color=red][shape=box]; key=value; }",false,false,{"a"}},
  {"attr-macro","graph G { node defaults=[shape=box]; a; }",false,false,{"a"}},
 };
 for(const auto& c:cases) {
  const auto result=language.language.parser->parse(c.text);
  const auto*graph=result.value_as<dot::ast::Graph>();
  std::vector<std::string_view> names;
  bool parents=true;
  for(const auto*n:result.nodes) {
   if(n!=result.value && !n->parent)parents=false;
   if(const auto*id=cursiv::ast_cast<dot::ast::NodeId>(n)) {
    names.push_back(id->id);
    const auto*group=cursiv::ast_cast<dot::ast::NodeGroup>(id->parent);
    if(!group || std::find(group->nodes.begin(),group->nodes.end(),id)==group->nodes.end())parents=false;
   }
  }
  std::sort(names.begin(),names.end());names.erase(std::unique(names.begin(),names.end()),names.end());
  auto expected=c.nodes;std::sort(expected.begin(),expected.end());
  const auto validation=cursiv::test::validationHelper(language.language,c.text);
  if(!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !parents ||
     !graph || graph->directed!=c.directed || graph->strict_!=c.strict || names!=expected || !validation.diagnostics.empty()) {
    std::fprintf(stderr,"dot: native regression: %s\n",c.name);return 1;
  }
 }
 const auto groups=language.language.parser->parse("digraph G { a:p,b:q -> c:r,d:s [label=x]; }");
 const auto*g=groups.value_as<dot::ast::Graph>();
 const auto*stmt=g && g->statements.size()==1?cursiv::ast_cast<dot::ast::CompoundStmt>(g->statements[0]):nullptr;
 if(!stmt || stmt->endpoints.size()!=2 || stmt->ops.size()!=1 || stmt->ops[0]!=dot::ast::EdgeOp::directed ||
    !stmt->attrs || stmt->attrs->pairs.size()!=1 || stmt->attrs->pairs[0]->value!="x")return 1;
 for(std::size_t i=0;i<2;++i) {
  const auto*group=std::get<dot::ast::NodeGroup*>(stmt->endpoints[i]);
  if(!group || group->parent!=stmt || group->nodes.size()!=2 ||
     group->nodes[0]->portName!=(i==0?"p":"r") || group->nodes[1]->portName!=(i==0?"q":"s"))return 1;
 }
 for(const char* text:{"graph G { a -> b; }","digraph G { a -- b; }","graph G { a [label]; }"}) {
  const auto parse=language.language.parser->parse(text);
  if(!parse.accepted || parse.cst().recovered() || !parse.diagnostics.empty())return 1;
  const auto result=cursiv::test::validationHelper(language.language,text);
  if(result.diagnostics.size()!=1)return 1;
 }
 const std::string numericSource = "digraph G { 123abc -> z; }";
 const auto numeric = language.language.parser->parse(numericSource);
 const auto* ng = numeric.value_as<dot::ast::Graph>();
 const auto* first = ng && ng->statements.size()==2 ? cursiv::ast_cast<dot::ast::CompoundStmt>(ng->statements[0]) : nullptr;
 const auto* second = ng && ng->statements.size()==2 ? cursiv::ast_cast<dot::ast::CompoundStmt>(ng->statements[1]) : nullptr;
 if(!first || !second || !first->ops.empty() || second->ops.size()!=1 || first->parent!=ng || second->parent!=ng)return 1;
 const auto span = numeric.cst()[first->cstNodeId];
 if(numericSource.substr(span.begin,span.end-span.begin)!="123")return 1;
 const auto bom = language.language.parser->parse("\xEF\xBB\xBFgraph G {a;}");
 if(bom.accepted && !bom.cst().recovered() && bom.diagnostics.empty())return 1;
 for(const auto& c : {std::pair{"graph G { a -> b; }","dot.edge-operator"},
                       std::pair{"graph G { a [label]; }","dot.attribute-value"}}) {
  const auto result=cursiv::test::validationHelper(language.language,c.first);
  if(result.diagnostics.size()!=1 || !result.diagnostics[0].code ||
     !result.diagnostics[0].code->isString() || result.diagnostics[0].code->string()!=c.second ||
     result.diagnostics[0].range.begin>=result.diagnostics[0].range.end ||
     result.diagnostics[0].range.end>std::string_view(c.first).size())return 1;
 }
 std::printf("dot: %zu native cases, grouped endpoint AST and 3 runtime diagnostics\n",cases.size());
}
