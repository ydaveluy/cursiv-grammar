#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <variant>

#include "protobuf/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace protobuf_keyword_context {

struct Case { const char* name; std::string_view source; bool clean; };
// Fresh root libprotoc 36.1 receipt: protobuf-keyword-context-native-v2.json.
inline constexpr Case cases[] = {
  {"message-bare", R"proto(syntax="proto3"; message message {} message M { message f=1; })proto", false},
  {"message-qualified", R"proto(syntax="proto3"; message message {} message M { .message f=1; })proto", true},
  {"message-labelled", R"proto(syntax="proto3"; message message {} message M { optional message f=1; })proto", true},
  {"enum-bare", R"proto(syntax="proto3"; message enum {} message M { enum f=1; })proto", false},
  {"enum-qualified", R"proto(syntax="proto3"; message enum {} message M { .enum f=1; })proto", true},
  {"enum-labelled", R"proto(syntax="proto3"; message enum {} message M { optional enum f=1; })proto", true},
  {"oneof-bare", R"proto(syntax="proto3"; message oneof {} message M { oneof f=1; })proto", false},
  {"oneof-qualified", R"proto(syntax="proto3"; message oneof {} message M { .oneof f=1; })proto", true},
  {"oneof-labelled", R"proto(syntax="proto3"; message oneof {} message M { optional oneof f=1; })proto", true},
  {"extend-bare", R"proto(syntax="proto3"; message extend {} message M { extend f=1; })proto", false},
  {"extend-qualified", R"proto(syntax="proto3"; message extend {} message M { .extend f=1; })proto", true},
  {"extend-labelled", R"proto(syntax="proto3"; message extend {} message M { optional extend f=1; })proto", true},
  {"reserved-bare", R"proto(syntax="proto3"; message reserved {} message M { reserved f=1; })proto", false},
  {"reserved-qualified", R"proto(syntax="proto3"; message reserved {} message M { .reserved f=1; })proto", true},
  {"reserved-labelled", R"proto(syntax="proto3"; message reserved {} message M { optional reserved f=1; })proto", true},
  {"extensions-bare", R"proto(syntax="proto3"; message extensions {} message M { extensions f=1; })proto", false},
  {"extensions-qualified", R"proto(syntax="proto3"; message extensions {} message M { .extensions f=1; })proto", true},
  {"extensions-labelled", R"proto(syntax="proto3"; message extensions {} message M { optional extensions f=1; })proto", true},
  {"optional-bare", R"proto(syntax="proto3"; message optional {} message M { optional f=1; })proto", false},
  {"optional-qualified", R"proto(syntax="proto3"; message optional {} message M { .optional f=1; })proto", true},
  {"optional-labelled", R"proto(syntax="proto3"; message optional {} message M { optional optional f=1; })proto", true},
  {"required-bare", R"proto(syntax="proto3"; message required {} message M { required f=1; })proto", false},
  {"required-qualified", R"proto(syntax="proto3"; message required {} message M { .required f=1; })proto", true},
  {"required-labelled", R"proto(syntax="proto3"; message required {} message M { optional required f=1; })proto", true},
  {"repeated-bare", R"proto(syntax="proto3"; message repeated {} message M { repeated f=1; })proto", false},
  {"repeated-qualified", R"proto(syntax="proto3"; message repeated {} message M { .repeated f=1; })proto", true},
  {"repeated-labelled", R"proto(syntax="proto3"; message repeated {} message M { optional repeated f=1; })proto", true},
  {"service-bare", R"proto(syntax="proto3"; message service {} message M { service f=1; })proto", true},
  {"service-qualified", R"proto(syntax="proto3"; message service {} message M { .service f=1; })proto", true},
  {"service-labelled", R"proto(syntax="proto3"; message service {} message M { optional service f=1; })proto", true},
  {"oneof-option-dispatch", R"proto(syntax="proto3"; message option {} message M { oneof choice { option f=1; } })proto", true},
  {"oneof-message-type", R"proto(syntax="proto3"; message message {} message M { oneof choice { message f=1; } })proto", true},
};

inline bool run(const cursiv::Parser& parser) {
  namespace ast = protobuf::ast;
  const auto fail=[](const char* name,const char* reason) {
    std::fprintf(stderr,"protobuf contextual %s: %s\n",name,reason);return false;
  };
  for(const auto& test:cases) {
    const auto result=parser.parse(test.source);
    const bool clean=result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    if(clean!=test.clean) {
      std::fprintf(stderr,"protobuf contextual %s: accepted=%d recovered=%d diagnostics=%zu source=%.*s\n",
                   test.name,result.accepted,result.cst().recovered(),result.diagnostics.size(),
                   static_cast<int>(test.source.size()),test.source.data());
      return fail(test.name,"native keyword dispatch differs");
    }
    if(!clean)continue;
    const auto* document=cursiv::ast_cast<ast::Document>(result.value);
    if(!document || document->messages.size()!=2 || document->messages[1]->name!="M")
      return fail(test.name,"keyword-named type or containing message lost");
    const auto* message=document->messages[1];
    const auto name=std::string_view(test.name);
    if(name=="oneof-option-dispatch") {
      if(message->oneofs.size()!=1)return fail(test.name,"oneof missing");
      const auto* oneof=message->oneofs[0];
      if(oneof->parent!=message || !oneof->fields.empty() || !oneof->groups.empty() || oneof->options.size()!=1)
        return fail(test.name,"option was captured as a field");
      const auto* option=oneof->options[0];
      const auto* value=std::get_if<ast::NumberLiteral*>(&option->value);
      if(option->parent!=oneof || !option->name || option->name->name!="f" ||
         option->name->parent!=option || !value || !*value || (*value)->text!="1" || (*value)->parent!=option)
        return fail(test.name,"oneof option value or parent lost");
    } else if(name=="oneof-message-type") {
      if(message->oneofs.size()!=1)return fail(test.name,"oneof missing");
      const auto* oneof=message->oneofs[0];
      if(oneof->parent!=message || oneof->fields.size()!=1 || !oneof->options.empty())
        return fail(test.name,"message remains a field type inside oneof");
      const auto* field=oneof->fields[0];
      if(field->type!="message" || field->name!="f" || field->number!="1" || field->parent!=oneof)
        return fail(test.name,"oneof contextual type or field parent lost");
    } else {
      const auto word=name.substr(0,name.find('-'));
      const std::string expectedType=(name.ends_with("-qualified")?".":"")+std::string(word);
      if(document->messages[0]->name!=word || message->fields.size()!=1)
        return fail(test.name,"qualified or labelled keyword type lost");
      const auto* field=message->fields[0];
      const bool labelCorrect=name.ends_with("-labelled")
          ? field->label==ast::FieldLabel::optional_ : !field->label;
      if(field->type!=std::string_view(expectedType) || field->name!="f" || field->number!="1" ||
         !labelCorrect || field->parent!=message)
        return fail(test.name,"type, label, field number or ownership changed");
    }
  }
  return true;
}

} // namespace protobuf_keyword_context
