#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "stacktrace/core/ast.hpp"

namespace stacktrace {

struct Stacktrace : cursiv::GrammarSpec {
  struct HSP : terminal<some(cr<R"( \t\f)">)> {};
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct JAVA_WORD : terminal<some(cr<R"(^.()/:@ \t\r\n\f)">)> {};
  struct NUMBER : terminal<some(d)> {};
  struct OriginPart : terminal<some(cr<R"(^/() \t\r\n\f)">) + lit<"/">> {};
  struct Origin : rule<ref<OriginPart> + option(ref<OriginPart> | lit<"/">),
                       std::string_view> {};
  struct FileName : terminal<some(cr<R"(^():\r\n)">)> {};
  struct Message : terminal<lit<":"> + many(cr<R"(^\r\n)">),
                           cursiv::text_conv<[](std::string_view text) {
                             text.remove_prefix(1);
                             if (text.starts_with(' ')) text.remove_prefix(1);
                             return text;
                           }>> {};
  struct Thread : terminal<lit<R"(Exception in thread ")"> +
                              many(cr<R"(^"\r\n)">) + lit<"\"">,
                           cursiv::text_conv<[](std::string_view text) {
                             return text.substr(21, text.size() - 22);
                           }>> {};
  struct Circular : terminal<lit<"[CIRCULAR REFERENCE: "> +
                                many(cr<R"(^\r\n)">),
                             cursiv::text_conv<[](std::string_view text) {
                               text.remove_prefix(21);
                               if (text.ends_with(']')) text.remove_suffix(1);
                               return text;
                             }>> {};
  struct Caption : terminal<lit<"Caused by:"> | lit<"Suppressed:">> {};
  struct QualifiedClass : rule<some(ref<JAVA_WORD>, lit<".">)> {};
  struct MessageLine
      : rule<option(assign<&ast::MessageLine::thread>(ref<Thread>) + ref<HSP>) +
             assign<&ast::MessageLine::className>(ref<QualifiedClass>) +
             option(assign<&ast::MessageLine::message>(ref<Message>))> {};
  struct AtLine
      : rule<option(assign<&ast::AtLine::indentation>(ref<HSP>)) + lit<"at"> +
             ref<HSP> + option(assign<&ast::AtLine::origin>(ref<Origin>)) +
             assign<&ast::AtLine::className>(ref<QualifiedClass>) + lit<"."> +
             option(assign<&ast::AtLine::method>(ref<JAVA_WORD>)) + lit<"("> +
             assign<&ast::AtLine::file>(ref<FileName>) +
             option(lit<":"> + assign<&ast::AtLine::line>(ref<NUMBER>)) + lit<")">> {};
  struct EllipsisLine
      : rule<option(assign<&ast::EllipsisLine::indentation>(ref<HSP>)) + lit<"..."> +
             ref<HSP> + assign<&ast::EllipsisLine::more>(ref<NUMBER>) + ref<HSP> + lit<"more">> {};
  static constexpr auto end = ref<EOL> | eof;
  struct Section
      : rule<option(assign<&ast::Section::indentation>(ref<HSP>)) +
             assign<&ast::Section::caption>(ref<Caption>) + ref<HSP> +
             (assign<&ast::Section::circularReference>(ref<Circular>) |
              assign<&ast::Section::message>(ref<MessageLine>)) + end +
             many(append<&ast::Section::frames>(ref<AtLine> | ref<EllipsisLine>) + end)> {};
  struct StackTrace
      : rule<some(append<&ast::StackTrace::messages>(ref<MessageLine>) + end) +
             many(append<&ast::StackTrace::frames>(ref<AtLine> | ref<EllipsisLine>) + end) +
             many(append<&ast::StackTrace::sections>(ref<Section>))> {};
  using entry = StackTrace;
};

}  // namespace stacktrace
