#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "xquery/core/ast.hpp"
namespace xquery_native_followup {
template<class Language> bool run(const Language& language) {
  namespace ast=xquery::ast;
  struct Case { const char* name; std::string_view source; bool accepted; const char* stringValue; };
  const Case cases[] = {
    {"integer", "42", true, nullptr},
    {"integer-separator", "6__3", true, nullptr},
    {"hex", "0xFF", true, nullptr},
    {"binary", "0b1111", true, nullptr},
    {"float", "1_0e+2__0", true, nullptr},
    {"bad-separator", "1_", false, nullptr},
    {"quote-double", "'a''b'", true, "a'b"},
    {"entities", "'&amp;&lt;&gt;&quot;&apos;'", true, "&<>\"'"},
    {"charrefs", "'&#65;&#x1F600;'", true, "A😀"},
    {"bad-entity", "'&unknown;'", false, nullptr},
    {"bare-amp", "'a&b'", false, nullptr},
    {"ref-zero", "'&#0;'", false, nullptr},
    {"ref-surrogate", "'&#xD800;'", false, nullptr},
    {"ref-large", "'&#x110000;'", false, nullptr},
    {"str-control", "'a\001b'", false, nullptr},
    {"str-crlf", "'a\015\012b\015c'", true, "a\012b\012c"},
    {"literal-less", "'a<b'", true, "a<b"},
    {"comment", "(: outer (: nested :) :) 1", true, nullptr},
    {"comment-control", "(:\001:)1", false, nullptr},
    {"space-ff", "1\014+2", false, nullptr},
    {"space-vt", "1\013+2", false, nullptr},
    {"sign-space", "+ - 1", true, nullptr},
    {"sign-comment", "+(:x:)-1", true, nullptr},
    {"uri-eqname", "Q{urn:example}node", true, nullptr},
    {"uri-prefixed-local", "Q{urn:example}p:node", true, nullptr},
    {"uri-amp", "Q{urn:a&b}node", false, nullptr},
    {"uri-entity", "Q{urn:a&amp;b}node", true, nullptr},
    {"wildcard-local", "*:node", true, nullptr},
    {"wildcard-prefix", "declare namespace p='urn:x';p:*", true, nullptr},
    {"wildcard-uri", "Q{urn:x}*", true, nullptr},
    {"wildcard-kind", "element(*:node)", true, nullptr},
    {"xml", "<a x=\"v\">text</a>", true, nullptr},
    {"xml-mismatch", "<a></b>", false, nullptr},
    {"xml-duplicate", "<a x=\"1\" x=\"2\"/>", false, nullptr},
    {"xml-space", "<a> </a>", true, nullptr},
    {"xml-attr-space", "<a x=\" \"/>", true, nullptr},
    {"xml-between", "<a>{1} {2}</a>", true, nullptr},
    {"xml-content-control", "<a>\001</a>", false, nullptr},
    {"xml-comment", "<!--ok-->", true, nullptr},
    {"xml-comment-bad", "<!--a--b-->", false, nullptr},
    {"xml-comment-tail", "<!--a--->", false, nullptr},
    {"xml-pi", "<?pi data?>", true, nullptr},
    {"xml-pi-empty", "<?\?>", false, nullptr},
    {"xml-pi-xml", "<?XmL data?>", false, nullptr},
    {"xml-pi-colon", "<?p:x data?>", false, nullptr},
    {"xml-cdata", "<a><![CDATA[a<b]]></a>", true, nullptr},
    {"lookup-local", "map{'a':1}?a", true, nullptr},
    {"lookup-string", "map{'a':1}?'a'", true, nullptr},
    {"lookup-string-ast", "'a'", true, "a"},
    {"lookup-filter", "map{1:2}?[?key=1]", false, nullptr},
    {"flwor", "for $x at $p in (1,2) let $y:=$x+1 where $p>0 order by $y descending return $y", true, nullptr},
    {"for-member", "for member $m in [1,2] return $m", true, nullptr},
    {"for-entry", "for key $k value $v in map{'a':1} return $v", true, nullptr},
    {"for-while", "for $x in (1,2) while $x<2 return $x", true, nullptr},
    {"window", "for tumbling window $w in (1,2,3) end $e when $e=3 return $w", true, nullptr},
    {"if", "if(1) then 2 else 3", true, nullptr},
    {"if-braced", "if(1){2}", true, nullptr},
    {"if-braced-else", "if(1){2}else{3}", true, nullptr},
    {"if-else-if", "if(1){2}else if(0){3}else{4}", true, nullptr},
    {"switch", "switch(1){case 1 return 2 default return 3}", true, nullptr},
    {"typeswitch", "typeswitch(1){case xs:integer return 2 default return 3}", true, nullptr},
    {"try", "try{error()}catch*{1}", true, nullptr},
    {"try-prefix", "try{error()}catch err:*{1}", true, nullptr},
    {"try-local", "try{error()}catch *:FOER0000{1}", true, nullptr},
    {"function", "declare function local:f($x as xs:integer:=1){$x};local:f()", true, nullptr},
    {"inline", "function($x as xs:integer){$x}", true, nullptr},
    {"inline-default", "(function($x:=1){$x})()", false, nullptr},
    {"named-type", "1 instance of function($x as xs:integer) as xs:integer", true, nullptr},
    {"keyword-arg", "declare function local:f($x:=1){$x};local:f(x:=2)", true, nullptr},
    {"annotation-bool", "declare %local:a(true()) function local:f(){1};local:f()", true, nullptr},
    {"map", "map{1:2}", true, nullptr},
    {"map-implicit", "{1:2}", true, nullptr},
    {"map-shorthand", "let $a:=1 return {$a}", true, nullptr},
    {"map-key-missing", "map{1}", true, nullptr},
    {"record-type", "map{'a':1} instance of record(a as xs:integer)", true, nullptr},
    {"array", "[1,2]", true, nullptr},
    {"array-curly", "array{1,2}", true, nullptr},
    {"type-decl", "declare type local:number as xs:integer;1 instance of local:number", true, nullptr},
    {"record-decl", "declare record local:r(a as xs:integer);local:r(1)", true, nullptr},
    {"context", "declare context item as xs:integer external :=1; .", true, nullptr},
    {"namespace", "declare namespace p='urn:x';p:a", true, nullptr},
    {"bad-prefix", "declare namespace p:x='urn:x';1", false, nullptr},
    {"version", "xquery version \"4.0\" encoding \"UTF-8\";1", true, nullptr},
    {"encoding", "xquery encoding \"UTF-8\";1", true, nullptr},
    {"template", "`a {1+2} b`", true, nullptr},
    {"string-constructor", "``[a `{1+2}` b]``", true, nullptr},
    {"qname", "#fn:true", true, nullptr},
    {"arrow", "\"abc\"=>string-length()", true, nullptr},
    {"pipeline", "\"abc\"->string-length(.)", true, nullptr},
    {"update-copy", "copy $x:=<a/> modify insert node <b/> into $x return $x", true, nullptr},
    {"name-if", "if", true, nullptr},
    {"name-map", "map", true, nullptr},
    {"name-function", "function", true, nullptr},
    {"name-for", "for", true, nullptr},
    {"call-prefixed", "fn:true()", true, nullptr},
    {"xml-attr-braces", "<a x=\"{{}}\"/>", true, nullptr},
    {"xml-attr-single-close", "<a x=\"}\"/>", false, nullptr},
    {"xml-empty-attr-expr", "<a x=\"{}\"/>", true, nullptr},
    {"xml-empty-content", "<a>{}</a>", true, nullptr},
    {"xml-ref-valid", "<a>&#65;&#x1F600;</a>", true, nullptr},
    {"xml-ref-invalid", "<a>&#0;</a>", false, nullptr},
    {"xml-attr-ref-invalid", "<a x=\"&#xD800;\"/>", false, nullptr},
    {"xml-cdata-raw-ref", "<a><![CDATA[&#0;]]></a>", true, nullptr},
    {"xml-comment-raw-ref", "<a><!--&#0;--></a>", true, nullptr},
    {"xml-pi-raw-ref", "<a><?pi &#0;?></a>", true, nullptr},
    {"uri-bad-ref", "Q{urn:&#0;}name", false, nullptr},
    {"string-amp-ref", "'&amp;#0;'", true, "&#0;"},
    {"string-cr-ref", "'a&#13;b'", true, "a\015b"},
    {"xml-pi-stylesheet", "<?xml-stylesheet data?>", true, nullptr},
    {"annotation-bare-name", "declare %local:a(foo) function local:f(){1};local:f()", false, nullptr},
    {"annotation-negative", "declare %local:a(- (:x:) 2) function local:f(){1};local:f()", true, nullptr},
    {"annotation-double-minus", "declare %local:a(--2) function local:f(){1};local:f()", false, nullptr},
    {"document-string", "1 instance of document-node('name')", false, nullptr},
    {"rich", "xquery version \"4.0\" encoding \"UTF-8\";\012declare namespace local=\"urn:local\";\012declare %local:a(\"1\", 1, true(), false(), #fn:true, -2) function local:f($x:=1) {$x};\012let $x:=<a p=\"{{}}\"><b/>{1} {2}</a>\012return (local:f(x:=2), $x, + (:gap:) - 1, if(1){2}else{3})", true, nullptr},
  };
  bool ok=true;
  for(const auto& test:cases) {
    cursiv::Document document(cursiv::TextDocument::create("file:///native-followup.xq","xquery",0,std::string(test.source)));
    document.language=&language.language;
    document.parseResult=language.language.parser->parse(document.textDocument().getText());
    const auto& parsed=document.parseResult;
    if(parsed.value) parsed.value->parent=&document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,diagnostics);
    const bool clean=parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty()&&parsed.value&&diagnostics.empty();
    if(clean!=test.accepted) {
      std::fprintf(stderr,"XQuery native %s expected=%d accepted=%d repaired=%d parse-diagnostics=%zu validation=%zu\n",test.name,test.accepted,parsed.accepted,parsed.cst().recovered(),parsed.diagnostics.size(),diagnostics.size());
      ok=false;
    }
    if(clean&&test.stringValue) {
      const auto* q=cursiv::ast_cast<ast::QueryList>(parsed.value);
      const auto* seq=q&&q->modules.size()==1&&q->modules[0]->main?cursiv::ast_cast<ast::SequenceExpr>(q->modules[0]->main->body):nullptr;
      const auto* value=seq&&seq->items.size()==1?cursiv::ast_cast<ast::StringLiteral>(seq->items[0]):nullptr;
      if(!value||value->value!=test.stringValue) {std::fprintf(stderr,"XQuery native string value lost: %s\n",test.name);ok=false;}
    }
  }
  const auto result=language.language.parser->parse(R"NATIVE(
xquery version "4.0" encoding "UTF-8";
declare namespace local="urn:local";
declare %local:a("1", 1, true(), false(), #fn:true, -2) function local:f($x:=1) {$x};
let $x:=<a p="{{}}"><b/>{1} {2}</a>
return (local:f(x:=2), $x, + (:gap:) - 1, if(1){2}else{3})
)NATIVE");
  const auto* query=cursiv::ast_cast<ast::QueryList>(result.value);
  bool encoding=query&&query->modules.size()==1&&query->modules[0]->version=="4.0"&&query->modules[0]->encoding=="UTF-8";
  bool annotations=false,signs=false,otherwise=false;
  for(const auto* node:result.nodes) {
    if(const auto* a=cursiv::ast_cast<ast::Annotation>(node);a&&a->name=="local:a"&&a->args.size()==6) {
      const auto* text=cursiv::ast_cast<ast::StringLiteral>(a->args[0]);
      const auto* number=cursiv::ast_cast<ast::IntLiteral>(a->args[1]);
      const auto* yes=cursiv::ast_cast<ast::BooleanLiteral>(a->args[2]);
      const auto* no=cursiv::ast_cast<ast::BooleanLiteral>(a->args[3]);
      const auto* name=cursiv::ast_cast<ast::QNameLiteral>(a->args[4]);
      const auto* minus=cursiv::ast_cast<ast::UnaryExpr>(a->args[5]);
      annotations=text&&text->value=="1"&&number&&number->value==1&&yes&&yes->value&&no&&!no->value&&name&&name->name=="fn:true"&&minus&&minus->signs=="-";
    }
    if(const auto* u=cursiv::ast_cast<ast::UnaryExpr>(node)) signs|=u->signs=="+-";
    if(const auto* i=cursiv::ast_cast<ast::IfExpr>(node)) otherwise|=i->thenBranch&&i->elseBranch;
  }
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned=true;
  auto walk=[&](auto&& self,const cursiv::AstNode& node)->void {
    if(!seen.insert(&node).second){owned=false;return;}
    language.language.astReflection->visitMembers(node,[&](std::string_view,std::uint32_t,cursiv::AstNode& child){
      owned &= child.parent==&node&&result.cst().contains(node.cstNodeId,child.cstNodeId);self(self,child);
    });
  };
  if(result.value)walk(walk,*result.value);
  if(!result.accepted||result.cst().recovered()||!result.diagnostics.empty()||!encoding||!annotations||!signs||!otherwise||!owned||seen.size()!=result.nodes.size()) {
    std::fprintf(stderr,"XQuery native rich AST encoding=%d annotations=%d signs=%d else=%d owned=%d nodes=%zu/%zu\n",encoding,annotations,signs,otherwise,owned,seen.size(),result.nodes.size());ok=false;
  }
  const auto whitespace=language.language.parser->parse("declare boundary-space preserve;<a x=' '> </a>");
  const auto* wsRoot=cursiv::ast_cast<ast::QueryList>(whitespace.value);
  const auto* wsSeq=wsRoot&&wsRoot->modules.size()==1&&wsRoot->modules[0]->main?cursiv::ast_cast<ast::SequenceExpr>(wsRoot->modules[0]->main->body):nullptr;
  const auto* wsElement=wsSeq&&wsSeq->items.size()==1?cursiv::ast_cast<ast::DirElemConstructor>(wsSeq->items[0]):nullptr;
  const auto* wsText=wsElement&&wsElement->content.size()==1?std::get_if<std::string_view>(&wsElement->content[0]):nullptr;
  const auto* wsAttr=wsElement&&wsElement->attributes.size()==1&&wsElement->attributes[0]->value.size()==1?std::get_if<std::string_view>(&wsElement->attributes[0]->value[0]):nullptr;
  if(!wsText||*wsText!=" "||!wsAttr||*wsAttr!=" ") {std::fprintf(stderr,"XQuery XML significant whitespace lost text=%d attr=%d\n",wsText!=nullptr,wsAttr!=nullptr);ok=false;}
  const auto encodingOnly=language.language.parser->parse("xquery encoding 'UTF-8';1");
  const auto* q=cursiv::ast_cast<ast::QueryList>(encodingOnly.value);
  if(!q||q->modules.size()!=1||!q->modules[0]->version.empty()||q->modules[0]->encoding!="UTF-8") {std::fprintf(stderr,"XQuery encoding-only declaration lost\n");ok=false;}
  return ok;
}
} // namespace xquery_native_followup
