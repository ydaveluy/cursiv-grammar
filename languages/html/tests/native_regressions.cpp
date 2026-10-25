#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "html/core/CoreServices.hpp"
#include "html/core/ast.hpp"
namespace {
using J=cursiv::JsonValue;
std::string lower(std::string_view text) { std::string out(text);for(char&c:out)if(c>='A'&&c<='Z')c+='a'-'A';return out; }
J project(const html::ast::ElementContent& item,const cursiv::AstNode* parent,bool& parents) {
 return std::visit([&](const auto*n)->J {
  using T=std::remove_cv_t<std::remove_pointer_t<decltype(n)>>;
  if(!n){parents=false;return nullptr;} parents &= n->parent==parent;
  if constexpr(std::is_same_v<T,html::ast::Element>){
   J::Array attrs,children;
   for(const auto*a:n->attributes){parents &= a && a->parent==n;if(a)attrs.emplace_back(J::Object{{"name",lower(a->name)},{"value",a->value?J(*a->value):J("")}});}
   for(const auto&c:n->content)children.push_back(project(c,n,parents));
   return J::Object{{"kind","element"},{"name",lower(n->name)},{"attrs",std::move(attrs)},{"children",std::move(children)}};
  } else if constexpr(std::is_same_v<T,html::ast::Script>||std::is_same_v<T,html::ast::Style>){
   std::size_t start=0;char quote=0;
   for(;start<n->text.size();++start){char c=n->text[start];if(quote){if(c==quote)quote=0;}else if(c=='\''||c=='"')quote=c;else if(c=='>'){++start;break;}}
   const auto end=n->text.rfind("</");
   const auto body=n->text.substr(start,end-start);
   J::Array children;if(!body.empty())children.emplace_back(J::Object{{"kind","text"},{"text",body}});
   return J::Object{{"kind","element"},{"name",std::is_same_v<T,html::ast::Script>?"script":"style"},{"attrs",J::Array{}},{"children",std::move(children)}};
  } else return J::Object{{"kind","text"},{"text",n->text}};
 },item);
}
}
int main(){
 const auto language=cursiv::create_language<html::HtmlCoreServices,html::HtmlSharedCoreServices,html::install_core_module>();
 struct Case{const char*name;const char*text;const char*expected;};
 const std::vector<Case> cases={
  {"quoted-less-than","<div title=\"a < b\">x</div>","[{\"attrs\":[{\"name\":\"title\",\"value\":\"a < b\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"unicode-unquoted","<div data-x=été?λ&x a@b=\"✓\">x</div>","[{\"attrs\":[{\"name\":\"data-x\",\"value\":\"été?λ&x\"},{\"name\":\"a@b\",\"value\":\"✓\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"attribute-names","<div @click=\"f()\" :class=x $foo=y .prop=1 disabled>x</div>","[{\"attrs\":[{\"name\":\"@click\",\"value\":\"f()\"},{\"name\":\":class\",\"value\":\"x\"},{\"name\":\"$foo\",\"value\":\"y\"},{\"name\":\".prop\",\"value\":\"1\"},{\"name\":\"disabled\",\"value\":\"\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"punctuation-unquoted","<div a=a!b[]{}*|~^%+,-.:;?/#_>x</div>","[{\"attrs\":[{\"name\":\"a\",\"value\":\"a!b[]{}*|~^%+,-.:;?/#_\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"form-feed","<div\fa=x\fb=y>x</div>","[{\"attrs\":[{\"name\":\"a\",\"value\":\"x\"},{\"name\":\"b\",\"value\":\"y\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"no-trailing-space-value","<div a=value b=next>x</div>","[{\"attrs\":[{\"name\":\"a\",\"value\":\"value\"},{\"name\":\"b\",\"value\":\"next\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"quoted-newlines","<div a=\"x\ny\r\nz\">x</div>","[{\"attrs\":[{\"name\":\"a\",\"value\":\"x\\ny\\nz\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"literal-less-than","<p>a < b <3 c</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a < b <3 c\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"space-text","<pre> \n \t </pre> \n <b>x</b>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\" \\n \\t \"}],\"kind\":\"element\",\"name\":\"pre\"},{\"kind\":\"text\",\"text\":\" \\n \"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"b\"}]"},
  {"rcdata-textarea","<textarea>a<b>c</textarea><p>x</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"textarea\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"rcdata-title","<title>a<b>c</title><p>x</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"title\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"rcdata-eof","<textarea>a<b>c","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"textarea\"}]"},
  {"script-normal","<script>if (a < b) c()</script><p>x</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"if (a < b) c()\"}],\"kind\":\"element\",\"name\":\"script\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"style-normal","<style>a<b</style>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b\"}],\"kind\":\"element\",\"name\":\"style\"}]"},
  {"html-name","<x-λ @é=y>x</x-λ>","[{\"attrs\":[{\"name\":\"@é\",\"value\":\"y\"}],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"x-λ\"}]"},
  {"initial-lf-textarea","<textarea>\nbody</textarea>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"body\"}],\"kind\":\"element\",\"name\":\"textarea\"}]"},
  {"initial-crlf-pre","<pre>\r\nbody<b>x</b></pre>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"body\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"b\"}],\"kind\":\"element\",\"name\":\"pre\"}]"},
  {"initial-lf-listing","<listing>\nbody</listing>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"body\"}],\"kind\":\"element\",\"name\":\"listing\"}]"},
  {"initial-lf-title","<title>\nbody</title>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"\\nbody\"}],\"kind\":\"element\",\"name\":\"title\"}]"},
  {"document-optional-ends","<!doctype html><html><head><title>x</title><body><p>y","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"title\"}],\"kind\":\"element\",\"name\":\"head\"},{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"y\"}],\"kind\":\"element\",\"name\":\"p\"}],\"kind\":\"element\",\"name\":\"body\"}],\"kind\":\"element\",\"name\":\"html\"}]"},
  {"document-no-body-open","<!doctype html><html><head><title>x</title></head><p>y</html>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"title\"}],\"kind\":\"element\",\"name\":\"head\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"y\"}],\"kind\":\"element\",\"name\":\"p\"}],\"kind\":\"element\",\"name\":\"html\"}]"},
  {"document-head-close","<!doctype html><html><head><title>x</title><body><p>y</body></html>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"title\"}],\"kind\":\"element\",\"name\":\"head\"},{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"y\"}],\"kind\":\"element\",\"name\":\"p\"}],\"kind\":\"element\",\"name\":\"body\"}],\"kind\":\"element\",\"name\":\"html\"}]"},
  {"document-implicit-head-close","<!doctype html><html><head><title>x</title><div>x</div></html>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"title\"}],\"kind\":\"element\",\"name\":\"head\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"div\"}],\"kind\":\"element\",\"name\":\"html\"}]"},
  {"normal","<p>a<b>b</b>c</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"b\"},{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"optional-li-nested","<ul><li>a<ul><li>b<li>c</ul><li>d</ul>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"},{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"li\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"li\"}],\"kind\":\"element\",\"name\":\"ul\"}],\"kind\":\"element\",\"name\":\"li\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"d\"}],\"kind\":\"element\",\"name\":\"li\"}],\"kind\":\"element\",\"name\":\"ul\"}]"},
  {"optional-p-block","<div><p>a<div>b</div><p>c</div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"div\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"p\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"optional-dl","<dl><dt>a<dd>b<dt>c</dl>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"dt\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"dd\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"dt\"}],\"kind\":\"element\",\"name\":\"dl\"}]"},
  {"optional-select","<select><optgroup label=x><option>a<option>b<optgroup label=y><option>c</select>","[{\"attrs\":[],\"children\":[{\"attrs\":[{\"name\":\"label\",\"value\":\"x\"}],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"option\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"option\"}],\"kind\":\"element\",\"name\":\"optgroup\"},{\"attrs\":[{\"name\":\"label\",\"value\":\"y\"}],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"option\"}],\"kind\":\"element\",\"name\":\"optgroup\"}],\"kind\":\"element\",\"name\":\"select\"}]"},
  {"optional-ruby","<ruby><rt>a<rp>(<rt>b</ruby>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"rt\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"(\"}],\"kind\":\"element\",\"name\":\"rp\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"rt\"}],\"kind\":\"element\",\"name\":\"ruby\"}]"},
  {"optional-table-sections","<table><thead><tr><th>a<th>b<tbody><tr><td>c<td>d</table>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"th\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"th\"}],\"kind\":\"element\",\"name\":\"tr\"}],\"kind\":\"element\",\"name\":\"thead\"},{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"c\"}],\"kind\":\"element\",\"name\":\"td\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"d\"}],\"kind\":\"element\",\"name\":\"td\"}],\"kind\":\"element\",\"name\":\"tr\"}],\"kind\":\"element\",\"name\":\"tbody\"}],\"kind\":\"element\",\"name\":\"table\"}]"},
  {"rawtext-noembed","<noembed>a<b>c</noembed>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"noembed\"}]"},
  {"rawtext-noframes","<noframes>a<b>c</noframes>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"noframes\"}]"},
  {"rawtext-prefix","<iframe-widget>x</iframe-widget><title-widget>y</title-widget>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"iframe-widget\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"y\"}],\"kind\":\"element\",\"name\":\"title-widget\"}]"},
  {"script-escaped-exit","<script><!--<script>x-->y</script><p>z</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"<!--<script>x-->y\"}],\"kind\":\"element\",\"name\":\"script\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"z\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"script-double-more","<script><!--<script>x</script>y<script>z</script>w--></script><p>x</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"<!--<script>x</script>y<script>z</script>w-->\"}],\"kind\":\"element\",\"name\":\"script\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"x\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"script-double-upper","<SCRIPT><!--<ScRiPt>x</sCrIpT>y</SCRIPT><p>z</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"<!--<ScRiPt>x</sCrIpT>y\"}],\"kind\":\"element\",\"name\":\"script\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"z\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"optional-li","<ul><li>one<li>two</ul>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"one\"}],\"kind\":\"element\",\"name\":\"li\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"two\"}],\"kind\":\"element\",\"name\":\"li\"}],\"kind\":\"element\",\"name\":\"ul\"}]"},
  {"optional-p","<p>one<p>two","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"one\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"two\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"optional-table","<table><tr><td>a<td>b</table>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"td\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"td\"}],\"kind\":\"element\",\"name\":\"tr\"}],\"kind\":\"element\",\"name\":\"table\"}]"},
  {"script-double-escaped","<script><!--<script>x</script>y</script><p>z</p>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"<!--<script>x</script>y\"}],\"kind\":\"element\",\"name\":\"script\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"z\"}],\"kind\":\"element\",\"name\":\"p\"}]"},
  {"rawtext-xmp","<xmp>a<b>c</xmp>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"xmp\"}]"},
  {"rawtext-iframe","<iframe>a<b>c</iframe>","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"iframe\"}]"},
  {"plaintext","<plaintext>a<b>c","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a<b>c\"}],\"kind\":\"element\",\"name\":\"plaintext\"}]"},
  {"plaintext-newlines","<plaintext>a\r\nb\rc","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\\nb\\nc\"}],\"kind\":\"element\",\"name\":\"plaintext\"}]"},
  {"p-summary","<div><p>a<summary>b</summary></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"summary\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-center","<div><p>a<center>b</center></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"center\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-dir","<div><p>a<dir>b</dir></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"dir\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-xmp","<div><p>a<xmp>b</xmp></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"xmp\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-plaintext","<p>a<plaintext>b","[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"plaintext\"}]"},
  {"special-selfclose","<div><pre/></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[],\"kind\":\"element\",\"name\":\"pre\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-dialog","<div><p>a<dialog>b</dialog></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"dialog\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
  {"p-search","<div><p>a<search>b</search></div>","[{\"attrs\":[],\"children\":[{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"a\"}],\"kind\":\"element\",\"name\":\"p\"},{\"attrs\":[],\"children\":[{\"kind\":\"text\",\"text\":\"b\"}],\"kind\":\"element\",\"name\":\"search\"}],\"kind\":\"element\",\"name\":\"div\"}]"},
 };
 for(const auto&c:cases){
  const auto result=language.language.parser->parse(c.text);const auto*doc=result.value_as<html::ast::Document>();
  J::Array children;bool parents=doc!=nullptr;
  if(doc)for(const auto&item:doc->content)children.push_back(project(item,doc,parents));
  const auto actual=J(std::move(children)).toJsonString({.pretty=false});
  const auto validation=cursiv::test::validationHelper(language.language,c.text);
  if(!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !parents || actual!=c.expected || !validation.diagnostics.empty()){
   std::fprintf(stderr,"html: native regression: %s\nactual: %s\nexpected: %s\n",c.name,actual.c_str(),c.expected);
   for(const auto&d:validation.diagnostics)
     std::fprintf(stderr,"%s\n",d.message.c_str());
   return 1;
  }
 }
 for(const char* text:{"<書籍>x</書籍>","<div A=x a=y></div>","<div a=a`b></div>","<div a\"b=x></div>"}){
  const auto parse=language.language.parser->parse(text);if(!parse.accepted || parse.cst().recovered() || !parse.diagnostics.empty())return 1;
  const auto result=cursiv::test::validationHelper(language.language,text);if(result.diagnostics.size()!=1)return 1;
 }
 std::printf("html: %zu native source-tree cases and 4 runtime diagnostics\n",cases.size());
}
