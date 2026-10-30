#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <variant>

#include "protobuf/core/Numbers.hpp"
#include "protobuf/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace protobuf_proto2_extensions {

struct Case {
  const char* name;
  std::string_view source;
  bool clean;
  const char* validationCode;
};
inline constexpr Case cases[] = {
  {"group-message", R"proto(syntax="proto2";message M { optional group Result=1 [deprecated=true] { option deprecated=true; optional int32 value=2; message Child{} enum E{ A=0; } extensions 100 to max; } })proto", true, ""},
  {"group-repeated", R"proto(syntax="proto2";message M { repeated group Values=1 { required string text=2; } })proto", true, ""},
  {"group-oneof", R"proto(syntax="proto2";message M { oneof choice { group Pick=1 { optional int32 value=2; } string text=3; } })proto", true, ""},
  {"group-extend", R"proto(syntax="proto2";message M { extensions 100 to max; } extend M { optional group Extra=100 { optional int32 value=1; } })proto", true, ""},
  {"group-nested-extend", R"proto(syntax="proto2";message M { extensions 100 to 110; } message N { extend M { repeated group Extra=101 { optional bool flag=1; } } })proto", true, ""},
  {"group-nested", R"proto(syntax="proto2";message M { optional group Outer=1 { optional group Inner=2 { optional int32 value=3; } } })proto", true, ""},
  {"contextual-names", R"proto(syntax="proto2";message group {} message extensions {} message M { optional string group=1; optional extensions extensions=2; optional .group other=3; })proto", true, ""},
  {"extensions-options", R"proto(syntax="proto2";message M { extensions 100, 200 to 210, 300 to max [verification=UNVERIFIED]; })proto", true, ""},
  {"extensions-octal-hex", R"proto(syntax="proto2";message M { extensions 0100, 0x100 to 0x10f; })proto", true, ""},
  {"extensions-in-group", R"proto(syntax="proto2";message M { optional group G=1 { extensions 100 to max; } })proto", true, ""},
  {"extensions-messageset", R"proto(syntax="proto2";message M { option message_set_wire_format=true; extensions 536870912 to 2147483646; extensions 100 to 200 [verification=UNVERIFIED]; })proto", true, ""},
  {"extensions-messageset-max", R"proto(syntax="proto2";message M { option message_set_wire_format=true; extensions 536870912 to max; })proto", true, ""},
  {"extensions-internal-range", R"proto(syntax="proto2";message M { extensions 19000 to 19999; })proto", true, ""},
  {"group-proto3", R"proto(syntax="proto3";message M { optional group G=1 {} })proto", true, "protobuf.proto3-group"},
  {"extensions-proto3", R"proto(syntax="proto3";message M { extensions 100 to max; })proto", true, "protobuf.proto3-extensions"},
  {"group-lowercase", R"proto(syntax="proto2";message M { optional group lower=1 {} })proto", true, "protobuf.group-name"},
  {"group-label-missing", R"proto(syntax="proto2";message M { group G=1 {} })proto", true, "protobuf.field-label"},
  {"group-number-zero", R"proto(syntax="proto2";message M { optional group G=0 {} })proto", true, "protobuf.field-number"},
  {"group-required-extension", R"proto(syntax="proto2";message M { extensions 100 to 110; } extend M { required group G=100 {} })proto", true, "protobuf.extension-label"},
  {"group-default", R"proto(syntax="proto2";message M { optional group G=1 [default=1] {} })proto", true, "protobuf.default-type"},
  {"group-lowered-name-collision", R"proto(syntax="proto2";message M { optional group SomeName=1 {} optional int32 somename=2; })proto", true, "protobuf.duplicate-field"},
  {"group-reserved-name", R"proto(syntax="proto2";message M { reserved "somename"; optional group SomeName=1 {} })proto", true, "protobuf.reserved-field"},
  {"extensions-zero", R"proto(syntax="proto2";message M { extensions 0; })proto", true, "protobuf.reserved-range"},
  {"extensions-descending", R"proto(syntax="proto2";message M { extensions 110 to 100; })proto", true, "protobuf.reserved-range"},
  {"extensions-too-large", R"proto(syntax="proto2";message M { extensions 536870912; })proto", true, "protobuf.reserved-range"},
  {"extensions-overlap", R"proto(syntax="proto2";message M { extensions 100 to 110, 110 to 120; })proto", true, "protobuf.extension-overlap"},
  {"extensions-overlap-reserved", R"proto(syntax="proto2";message M { reserved 100 to 105; extensions 105 to 110; })proto", true, "protobuf.extension-overlap"},
  {"extensions-field-collision", R"proto(syntax="proto2";message M { extensions 100 to 110; optional int32 value=100; })proto", true, "protobuf.extension-field"},
  {"extensions-group-collision", R"proto(syntax="proto2";message M { extensions 100 to 110; optional group G=100 {} })proto", true, "protobuf.extension-field"},
  {"group-oneof-label", R"proto(syntax="proto2";message M { oneof choice { optional group Pick=1 {} } })proto", false, ""},
  {"group-no-body", R"proto(syntax="proto2";message M { optional group G=1; })proto", false, ""},
  {"extensions-empty", R"proto(syntax="proto2";message M { extensions; })proto", false, ""},
  {"extensions-negative", R"proto(syntax="proto2";message M { extensions -1; })proto", false, ""},
  {"extensions-trailing-comma", R"proto(syntax="proto2";message M { extensions 100,; })proto", false, ""},
  {"extensions-empty-options", R"proto(syntax="proto2";message M { extensions 100 []; })proto", false, ""},
  {"extensions-in-extend", R"proto(syntax="proto2";message M { extensions 100 to max; } extend M { extensions 200; })proto", false, ""},
  {"extensions-in-oneof", R"proto(syntax="proto2";message M { oneof choice { extensions 100; } })proto", false, ""},
  {"extensions-int32max", R"proto(syntax="proto2";message M { option message_set_wire_format=true; extensions 2147483647; })proto", true, "protobuf.reserved-range"},
};

inline bool run(const auto& language) {
  namespace ast = protobuf::ast;
  const auto fail=[](const char* name,const char* reason) {
    std::fprintf(stderr,"protobuf proto2 %s: %s\n",name,reason);return false;
  };
  for(const auto& test:cases) {
    const auto result=language.parser->parse(test.source);
    const bool clean=result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    if(clean!=test.clean) {
      std::fprintf(stderr,"protobuf proto2 %s: accepted=%d recovered=%d diagnostics=%zu source=%.*s\n",
                   test.name,result.accepted,result.cst().recovered(),result.diagnostics.size(),
                   static_cast<int>(test.source.size()),test.source.data());
      for(const auto& diagnostic:result.diagnostics)
        std::fprintf(stderr,"  kind=%u begin=%u end=%u element=%u\n",
                     unsigned(diagnostic.kind),unsigned(diagnostic.begin),unsigned(diagnostic.end),unsigned(diagnostic.element));
      return fail(test.name,"unexpected syntactic acceptance");
    }
    if(!clean)continue;
    const auto* document=cursiv::ast_cast<ast::Document>(result.value);
    if(!document)return fail(test.name,"document AST missing");
    for(const auto* node:result.nodes)
      if(node!=document && !node->parent)return fail(test.name,"unowned AST node");
    const auto validation=cursiv::test::validationHelper(language,std::string(test.source));
    if(*test.validationCode) {
      bool found=false;
      for(const auto& diagnostic:validation.diagnostics)
        found|=diagnostic.code && *diagnostic.code==test.validationCode;
      if(!found)return fail(test.name,test.validationCode);
    } else if(!validation.diagnostics.empty()) {
      for(const auto& diagnostic:validation.diagnostics)
        std::fprintf(stderr,"  validation=%s\n",diagnostic.message.c_str());
      return fail(test.name,"valid source produced validation diagnostics");
    }
    const auto name=std::string_view(test.name);
    if(name=="group-message") {
      if(document->messages.size()!=1 || document->messages[0]->groups.size()!=1)
        return fail(test.name,"message group missing");
      const auto* message=document->messages[0];const auto* group=message->groups[0];
      if(group->parent!=message || group->name!="Result" || group->number!="1" ||
         group->label!=ast::FieldLabel::optional_ || group->fieldOptions.size()!=1 ||
         group->options.size()!=1 || group->fields.size()!=1 || group->messages.size()!=1 ||
         group->enums.size()!=1 || group->extensions.size()!=1)
        return fail(test.name,"group header or shared message body lost");
      const auto* fieldOption=group->fieldOptions[0];const auto* option=group->options[0];
      if(fieldOption->parent!=group || option->parent!=group || !fieldOption->name || !option->name ||
         fieldOption->name->parent!=fieldOption || option->name->parent!=option ||
         fieldOption->name->name!="deprecated" || option->name->name!="deprecated" ||
         !std::holds_alternative<bool>(fieldOption->value) || !std::get<bool>(fieldOption->value) ||
         !std::holds_alternative<bool>(option->value) || !std::get<bool>(option->value))
        return fail(test.name,"field and message options merged or unowned");
      if(group->fields[0]->parent!=group || group->fields[0]->name!="value" ||
         group->messages[0]->parent!=group || group->messages[0]->name!="Child" ||
         group->enums[0]->parent!=group || group->enums[0]->name!="E" ||
         group->extensions[0]->parent!=group || !group->extensions[0]->ranges[0]->toMax)
        return fail(test.name,"group contents or parents changed");
    } else if(name=="group-oneof") {
      if(document->messages.size()!=1 || document->messages[0]->oneofs.size()!=1)
        return fail(test.name,"oneof missing");
      const auto* oneof=document->messages[0]->oneofs[0];
      if(oneof->groups.size()!=1 || oneof->fields.size()!=1 ||
         oneof->groups[0]->parent!=oneof || oneof->groups[0]->name!="Pick" ||
         oneof->groups[0]->label || oneof->groups[0]->fields.size()!=1 ||
         oneof->groups[0]->fields[0]->parent!=oneof->groups[0])
        return fail(test.name,"oneof group header or ownership lost");
    } else if(name=="group-extend" || name=="group-nested-extend") {
      const auto* extension=name=="group-extend"?
          (document->extends_.size()==1?document->extends_[0]:nullptr):
          (document->messages.size()==2 && document->messages[1]->extends_.size()==1?
             document->messages[1]->extends_[0]:nullptr);
      if(!extension || extension->target!="M" || extension->groups.size()!=1 ||
         extension->groups[0]->parent!=extension || extension->groups[0]->name!="Extra" ||
         extension->groups[0]->fields.size()!=1 || extension->groups[0]->fields[0]->parent!=extension->groups[0])
        return fail(test.name,"extension group or shared body lost");
    } else if(name=="group-nested") {
      const auto* outer=document->messages.size()==1 && document->messages[0]->groups.size()==1?
          document->messages[0]->groups[0]:nullptr;
      if(!outer || outer->groups.size()!=1 || outer->groups[0]->name!="Inner" ||
         outer->groups[0]->parent!=outer || outer->groups[0]->fields.size()!=1 ||
         outer->groups[0]->fields[0]->number!="3")return fail(test.name,"recursive group body lost");
    } else if(name=="extensions-options") {
      if(document->messages.size()!=1 || document->messages[0]->extensions.size()!=1)
        return fail(test.name,"extensions declaration missing");
      const auto* declaration=document->messages[0]->extensions[0];
      if(declaration->parent!=document->messages[0] || declaration->ranges.size()!=3 || declaration->options.size()!=1)
        return fail(test.name,"range list or shared options lost");
      const auto* first=declaration->ranges[0];const auto* middle=declaration->ranges[1];const auto* last=declaration->ranges[2];
      const auto* option=declaration->options[0];
      const auto* value=std::get_if<ast::IdentRef*>(&option->value);
      if(first->from!="100" || first->to || first->toMax || middle->from!="200" ||
         !middle->to || *middle->to!="210" || middle->toMax || last->from!="300" || !last->toMax ||
         first->parent!=declaration || middle->parent!=declaration || last->parent!=declaration ||
         option->parent!=declaration || !option->name || option->name->name!="verification" ||
         !value || !*value || (*value)->name!="UNVERIFIED" || (*value)->parent!=option)
        return fail(test.name,"range bounds, option values or ownership changed");
    } else if(name=="contextual-names") {
      if(document->messages.size()!=3 || document->messages[0]->name!="group" ||
         document->messages[1]->name!="extensions" || document->messages[2]->fields.size()!=3 ||
         document->messages[2]->fields[0]->name!="group" || document->messages[2]->fields[1]->type!="extensions" ||
         document->messages[2]->fields[1]->name!="extensions" || document->messages[2]->fields[2]->type!=".group")
        return fail(test.name,"contextual group/extensions identifier changed");
    }
  }
  return true;
}

} // namespace protobuf_proto2_extensions
