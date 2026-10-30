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
  struct PartText : rule<option(ref<TEXT>)> {};
  struct CommentText : rule<option(ref<REST>)> {};
  struct LineEnd : rule<ref<LF> | eof> {};

  struct InitialPart : rule<create<ast::ValuePart>(
                           assign<&ast::ValuePart::text>(ref<PartText>))> {};
  struct ContinuedPart
      : rule<create<ast::ValuePart>(
            assign<&ast::ValuePart::join>(as<ast::Join::newline>(lit<"\n+">) | as<ast::Join::splice>(lit<"\\\n">)) +
            assign<&ast::ValuePart::text>(ref<PartText>))> {};
  struct Field
      : rule<create<ast::Field>(
            option(assign<&ast::Field::indentation>(ref<HWS>)) +
            assign<&ast::Field::key>(ref<NAME>) + lit<":"> +
            append<&ast::Field::parts>(ref<InitialPart>) +
            many(append<&ast::Field::parts>(ref<ContinuedPart>)) +
            (flag<&ast::Field::terminated>(ref<FIELD_END>) | eof))> {};
  struct Comment
      : rule<create<ast::Comment>(
            option(assign<&ast::Comment::indentation>(ref<HWS>)) + lit<"#"> +
            append<&ast::Comment::lines>(ref<CommentText>) + ref<LineEnd> +
            many(lit<"#"> + append<&ast::Comment::lines>(ref<CommentText>) +
                 ref<LineEnd>) +
            !lit<"#">)> {};
  struct RawLine
      : rule<create<ast::RawLine>(assign<&ast::RawLine::text>(ref<RAW>) +
                                  ref<LineEnd>)> {};
  struct Record
      : rule<create<ast::Record>(
            append<&ast::Record::lines>(ref<Field>) +
            many(append<&ast::Record::lines>(ref<Field> | ref<Comment> |
                                             ref<RawLine>)) +
            ((&ref<BLANK>) | eof))> {};
  struct Doc : rule<create<ast::Document>(many(
                       ref<BLANK> | append<&ast::Document::entries>(
                                        ref<Record> | ref<Comment> |
                                        ref<RawLine>)))> {};

  using entry = Doc;
};

} // namespace recfile
