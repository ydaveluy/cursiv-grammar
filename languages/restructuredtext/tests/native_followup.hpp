#pragma once
#include <cstdio>
#include <string>
inline int rst_native_followup(const restructuredtext::RestructuredtextCoreServices& services) {
  namespace ast=restructuredtext::ast;
  for(const auto source:{"A\n=\n", "Hi\n--\n", "Hi\n--", "Hi\n~~~"}) {
    const auto r=services.parser->parse(source);
    const auto* d=r.value_as<ast::Document>();
    if(!r.accepted||r.cst().recovered()||!d||d->blocks.size()!=1||
       !std::holds_alternative<ast::Section*>(d->blocks[0])) {
      std::fprintf(stderr,"rst: native short title lost: %s\n",source);return 1;
    }
  }
  for(const auto source:{".. _site: https://example.org/\n", ".. _site: https://example.org/",
      ".. _`two words`: https://example.org/\n", ".. _place:\n", ".. __: https://example.org/\n"}) {
    const auto r=services.parser->parse(source);
    const auto* d=r.value_as<ast::Document>();
    const auto* t=d&&d->blocks.size()==1?std::get_if<ast::ExplicitTarget*>(&d->blocks[0]):nullptr;
    if(!r.accepted||r.cst().recovered()||!t||!*t||(*t)->parent!=d) {
      std::fprintf(stderr,"rst: native target lost: accepted=%d repaired=%d diagnostics=%zu blocks=%zu first-kind=%zu target=%d owned=%d: %s\n",
          static_cast<int>(r.accepted),static_cast<int>(r.cst().recovered()),r.diagnostics.size(),
          d?d->blocks.size():0,d&&!d->blocks.empty()?d->blocks[0].index():999,
          static_cast<int>(t&&*t),static_cast<int>(t&&*t&&(*t)->parent==d),source);return 1;
    }
    const auto* n=*t;
    if(std::string_view(source).find("__:")!=std::string_view::npos) {
      if(!n->anonymous||!n->name.empty()) return 1;
    } else if(n->name!="site"&&n->name!="two words"&&n->name!="place") return 1;
    if(n->name!="place"&&n->destination!="https://example.org/") return 1;
  }
  for(const auto source:{R"rst(\*text\*)rst", "before ` after", "See _target here."}) {
    const auto r=services.parser->parse(source);
    const auto* d=r.value_as<ast::Document>();
    const auto* p=d&&d->blocks.size()==1?std::get_if<ast::Paragraph*>(&d->blocks[0]):nullptr;
    if(!r.accepted||r.cst().recovered()||!p||!*p) return 1;
    for(const auto& s:(*p)->spans) if(!std::holds_alternative<std::string_view>(s)) {
      std::fprintf(stderr,"rst: literal text became inline markup: %s\n",source);return 1;
    }
  }
  {
    const auto r=services.parser->parse("snake_case_\n");
    const auto* d=r.value_as<ast::Document>();
    const auto* p=d&&d->blocks.size()==1?std::get_if<ast::Paragraph*>(&d->blocks[0]):nullptr;
    const auto* ref=p&&*p&&(*p)->spans.size()==1?std::get_if<ast::Reference*>(&(*p)->spans[0]):nullptr;
    if(!r.accepted||r.cst().recovered()||!ref||!*ref||(*ref)->name!="snake_case") return 1;
  }
  const std::string source="See _`my place` here.\n";
  const auto r=services.parser->parse(source);
  const auto* d=r.value_as<ast::Document>();
  const auto* p=d&&d->blocks.size()==1?std::get_if<ast::Paragraph*>(&d->blocks[0]):nullptr;
  bool found=false;
  if(p&&*p) for(const auto& s:(*p)->spans)
    if(const auto* t=std::get_if<ast::HyperlinkTarget*>(&s);t&&*t&&(*t)->name=="my place") found=true;
  if(!r.accepted||r.cst().recovered()||!found||r.cst().text()!=source) return 1;
  return 0;
}
