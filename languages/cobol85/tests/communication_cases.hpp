#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cobol85/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace cobol_communication_cases {

inline bool run(const auto& language) {
  namespace ast=cobol85::ast;
  using Kind=ast::CommunicationClauseKind;
  const auto fail=[](const char* reason) {
    std::fprintf(stderr,"cobol CD: %s\n",reason);return false;
  };
  const std::string source=R"COB(IDENTIFICATION DIVISION.
PROGRAM-ID. CDEXAMPLE.
DATA DIVISION.
COMMUNICATION SECTION.
CD IN-CD FOR INITIAL INPUT
 SYMBOLIC QUEUE IS Q-NAME OF PACKET
 SUB-QUEUE-1 S1 SUB-QUEUE-2 S2 SYMBOLIC SUB-QUEUE-3 S3
 MESSAGE DATE DATE-VAR MESSAGE TIME TIME-VAR SOURCE SRC
 TEXT LENGTH LEN-VAR END KEY END-VAR STATUS KEY STAT-VAR COUNT CNT.
01 IN-REC PIC X.
CD OUT-CD FOR OUTPUT DESTINATION COUNT DC TEXT LENGTH LEN-VAR
 STATUS KEY STAT-VAR DESTINATION TABLE OCCURS 3 TIMES INDEXED BY IX-A IX-B
 ERROR KEY ERR-VAR SYMBOLIC DESTINATION DEST-VAR.
01 OUT-REC PIC X.
CD IO-CD INITIAL I-O MESSAGE DATE DATE-VAR MESSAGE TIME TIME-VAR
 SYMBOLIC TERMINAL TERM-VAR TEXT LENGTH LEN-VAR END KEY END-VAR STATUS KEY STAT-VAR.
01 IO-REC PIC X.
CD POS-IN INPUT A1 A2 A3 A4 A5 A6 A7 A8 A9 A10 A11.
01 POS-IN-REC PIC X.
CD POS-IO I-O A1 A2 A3 A4 A5 A6.
01 POS-IO-REC PIC X.
CD BARE-DEST OUTPUT DESTINATION DEST-VAR.
01 BARE-REC PIC X.
PROCEDURE DIVISION.
STOP RUN.
)COB";
  const auto result=language.parser->parse(source);
  const auto* unit=cursiv::ast_cast<ast::CompilationUnit>(result.value);
  const auto* program=unit && unit->programs.size()==1?unit->programs[0]:nullptr;
  const auto* data=program?program->data:nullptr;
  if(!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
     !data || data->communications.size()!=6) {
    std::fprintf(stderr,"  accepted=%d recovered=%d diagnostics=%zu\n",result.accepted,result.cst().recovered(),result.diagnostics.size());
    for(const auto& diagnostic:result.diagnostics)
      std::fprintf(stderr,"  kind=%u begin=%u end=%u element=%u\n",unsigned(diagnostic.kind),unsigned(diagnostic.begin),unsigned(diagnostic.end),unsigned(diagnostic.element));
    return fail("native CD clause families and associated records must parse clean");
  }
  if(!cursiv::test::validationHelper(language,source).diagnostics.empty())
    return fail("CD record association must satisfy local validation");
  for(const auto* entry:data->communications) {
    const auto* record=entry->records.size()==1?cursiv::ast_cast<ast::DataItem>(entry->records[0]):nullptr;
    if(entry->parent!=data || !record || record->level!="01" || record->parent!=entry)
      return fail("CD record ownership was lost");
    for(const auto* clause:entry->clauses) {
      if(clause->parent!=entry)return fail("CD clause parent was lost");
      for(const auto* field:clause->fields)
        if(field->parent!=clause)return fail("CD target parent was lost");
    }
  }
  const auto* input=data->communications[0];
  const Kind inputKinds[]={Kind::queue,Kind::subQueue1,Kind::subQueue2,Kind::subQueue3,
      Kind::messageDate,Kind::messageTime,Kind::source,Kind::textLength,Kind::endKey,Kind::statusKey,Kind::messageCount};
  if(!input->initial || input->direction!="INPUT" || input->clauses.size()!=11)
    return fail("INPUT header or named clause count lost");
  for(unsigned i=0;i<11;++i)
    if(input->clauses[i]->kind!=inputKinds[i] || input->clauses[i]->fields.size()!=1)
      return fail("INPUT clause kinds or fields lost");
  const auto* queue=input->clauses[0]->fields[0];
  if(queue->name!="Q-NAME" || queue->qualifiers.size()!=1 || queue->qualifiers[0]!="PACKET")
    return fail("qualified CD field must reuse Identifier");
  const auto* output=data->communications[1];
  const Kind outputKinds[]={Kind::destinationCount,Kind::textLength,Kind::statusKey,Kind::destinationTable,Kind::errorKey,Kind::destination};
  if(output->initial || output->direction!="OUTPUT" || output->clauses.size()!=6)
    return fail("OUTPUT header or clauses lost");
  for(unsigned i=0;i<6;++i)
    if(output->clauses[i]->kind!=outputKinds[i])return fail("OUTPUT clause kind lost");
  const auto* table=output->clauses[3];
  if(table->occurs!="3" || table->indexes.size()!=2 || table->indexes[0]!="IX-A" || table->indexes[1]!="IX-B" || !table->fields.empty())
    return fail("DESTINATION TABLE count/index names lost");
  const auto* io=data->communications[2];
  if(!io->initial || io->direction!="I-O" || io->clauses.size()!=6 || io->clauses[2]->kind!=Kind::terminal)
    return fail("I-O header or symbolic terminal lost");
  for(unsigned i=3;i<5;++i) {
    const auto* positional=data->communications[i];
    const unsigned count=i==3?11:6;
    if(positional->clauses.size()!=1 || positional->clauses[0]->kind!=Kind::unnamed || positional->clauses[0]->fields.size()!=count ||
       positional->clauses[0]->fields[0]->name!="A1" || positional->clauses[0]->fields[count-1]->name!=(i==3?"A11":"A6"))
      return fail("positional native field count/order lost");
  }
  if(data->communications[5]->clauses.size()!=1 || data->communications[5]->clauses[0]->kind!=Kind::destination)
    return fail("unqualified DESTINATION alternative lost");
  const std::string prefix="IDENTIFICATION DIVISION. PROGRAM-ID. C. DATA DIVISION. COMMUNICATION SECTION. ";
  const std::string suffix=". 01 REC-A PIC X. PROCEDURE DIVISION. STOP RUN.";
  for(const std::string_view body:{"CD X INITIAL OUTPUT", "CD X INPUT DESTINATION COUNT DC",
      "CD X OUTPUT QUEUE Q-NAME", "CD X I-O COUNT CNT", "CD X OUTPUT DESTINATION TABLE OCCURS 3 INDEXED BY",
      "CD X INPUT A1 A2", "CD X I-O A1 A2 A3 A4 A5", "CD X INPUT UNKNOWN A1"}) {
    const std::string invalid=prefix+std::string(body)+suffix;
    const auto bad=language.parser->parse(invalid);
    if(bad.accepted && !bad.cst().recovered() && bad.diagnostics.empty())return fail("malformed/direction-incompatible CD clause accepted");
  }
  const auto missing=cursiv::test::validationHelper(language,prefix+"CD X INPUT. PROCEDURE DIVISION. STOP RUN.");
  bool found=false;
  for(const auto& diagnostic:missing.diagnostics)
    found|=diagnostic.code && *diagnostic.code=="cobol85.communication-record";
  if(!found)return fail("missing native CD record needs validation, not an invented grammar obligation");
  return true;
}

} // namespace cobol_communication_cases
