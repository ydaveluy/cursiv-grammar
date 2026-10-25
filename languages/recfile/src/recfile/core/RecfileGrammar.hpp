#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "recfile/core/ast.hpp"

namespace recfile {

struct Recfile : cursiv::GrammarSpec {
  struct LF : terminal<lit<"\n">> {};
  struct HWS : terminal<some(cr<" \t">)> {};
  static constexpr auto NameShape = !(cr<" \t"> | lit<"#">)+some(cr<R"(^:\n)">);
  static constexpr auto FieldShape = many(cr<" \t">) + NameShape + lit<":">;
  static constexpr auto CommentShape = many(cr<" \t">) + lit<"#">;
  static constexpr auto BlankShape =
      (many(cr<" \t">) + lit<"\n">) | (some(cr<" \t">) + !any);
  struct NAME : terminal<NameShape> {};
  struct BLANK : terminal<BlankShape> {};
  struct TEXT : terminal<some(!(lit<"\\\n"> | lit<"\n">)+any)> {};
  struct REST : terminal<some(cr<R"(^\n)">)> {};
  struct RAW : terminal<!(FieldShape | CommentShape | BlankShape) +
                        some(cr<R"(^\n)">)> {};
  struct FIELD_END : terminal<lit<"\n"> + !lit<"+">> {};
  struct PLUS : terminal<lit<"\n+">, value<ast::Join::newline>> {};
  struct SPLICE : terminal<lit<"\\\n">, value<ast::Join::splice>> {};
  struct PartText : rule<option(ref<TEXT>)> {};
  struct CommentText : rule<option(ref<REST>)> {};
  struct LineEnd : rule<ref<LF> | eof> {};

  struct InitialPart : rule<create<ast::ValuePart>(
                           assign<&ast::ValuePart::text>(ref<PartText>))> {};
  struct ContinuedPart
      : rule<create<ast::ValuePart>(
            assign<&ast::ValuePart::join>(ref<PLUS> | ref<SPLICE>) +
            assign<&ast::ValuePart::text>(ref<PartText>))> {};
  struct FieldRule
      : rule<create<ast::Field>(
            option(assign<&ast::Field::indentation>(ref<HWS>)) +
            assign<&ast::Field::key>(ref<NAME>) + lit<":"> +
            append<&ast::Field::parts>(ref<InitialPart>) +
            many(append<&ast::Field::parts>(ref<ContinuedPart>)) +
            (flag<&ast::Field::terminated>(ref<FIELD_END>) | eof))> {};
  struct CommentRule
      : rule<create<ast::Comment>(
            option(assign<&ast::Comment::indentation>(ref<HWS>)) + lit<"#"> +
            append<&ast::Comment::lines>(ref<CommentText>) + ref<LineEnd> +
            many(lit<"#"> + append<&ast::Comment::lines>(ref<CommentText>) +
                 ref<LineEnd>) +
            !lit<"#">)> {};
  struct RawLineRule
      : rule<create<ast::RawLine>(assign<&ast::RawLine::text>(ref<RAW>) +
                                  ref<LineEnd>)> {};
  struct RecordRule
      : rule<create<ast::Record>(
            append<&ast::Record::lines>(ref<FieldRule>) +
            many(append<&ast::Record::lines>(ref<FieldRule> | ref<CommentRule> |
                                             ref<RawLineRule>)) +
            ((&ref<BLANK>) | eof))> {};
  struct DocRule : rule<create<ast::Document>(many(
                       ref<BLANK> | append<&ast::Document::entries>(
                                        ref<RecordRule> | ref<CommentRule> |
                                        ref<RawLineRule>)))> {};

  using entry = DocRule;
};

} // namespace recfile
