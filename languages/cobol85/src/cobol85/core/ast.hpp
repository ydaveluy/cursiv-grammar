// The cobol85 language: its AST. Data description entries stay a flat list --
// level numbers do not nest in the grammar, and a tool recovers the group
// hierarchy from `level`. Procedure division statements share a few carriers
// (RoundedTarget, ArithExpr, Condition, Operand) across verbs of the same
// shape, rather than one bespoke struct per clause. A data, screen or report
// clause written again keeps every value, in order, in its list.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cobol85::ast {

// -- operands: identifiers, literals, expressions, conditions ----

// Base of anything an operand position can hold: a data name (qualified,
// subscripted, reference-modified), a function call, a special register, or
// a literal.
struct Operand : cursiv::Extends<cursiv::AstNode> {};

struct ArithExpr : cursiv::Extends<cursiv::AstNode> {};

struct Identifier : cursiv::Extends<Operand> {
  string_view name;                // dataName, conditionName, paragraphName...
  vector<string_view> qualifiers;  // the OF/IN chain, outer to inner as written
  vector<ArithExpr*> subscripts;   // tableCall's (s1, s2, ...); empty if none
  ArithExpr* refModStart;          // identifier(start:length); null if none
  ArithExpr* refModLength;         // null when the length was omitted
};

struct FunctionCall : cursiv::Extends<Operand> {
  string_view name;
  vector<ArithExpr*> args;
  ArithExpr* refModStart;
  ArithExpr* refModLength;
};

// ADDRESS OF / LENGTH OF an identifier, or a bare register (TALLY,
// RETURN-CODE, DATE, WHEN-COMPILED...).
struct SpecialRegister : cursiv::Extends<Operand> {
  string_view kind;  // "ADDRESS OF", "LENGTH OF", or the register's own word
  Identifier* operand;
};

// NONNUMERICLITERAL, a number, TRUE/FALSE, a figurative constant (SPACES,
// HIGH-VALUES, ZEROS...) or a CICS DFHRESP/DFHVALUE literal: the source text
// as written, kept raw since no arithmetic is ever done on it here.
struct Literal : cursiv::Extends<Operand> {
  string_view text;
  boolean all;
};

struct ArithOperand : cursiv::Extends<ArithExpr> {
  Operand* value;
};

struct ArithUnary : cursiv::Extends<ArithExpr> {
  string_view op;  // "+" or "-"
  ArithExpr* operand;
};

struct ArithBinary : cursiv::Extends<ArithExpr> {
  ArithExpr* left;
  string_view op;  // "+" "-" "*" "/" "**"
  ArithExpr* right;
};

struct Condition : cursiv::Extends<cursiv::AstNode> {};

struct CondNot : cursiv::Extends<Condition> {
  Condition* operand;
};

struct CondBinary : cursiv::Extends<Condition> {
  Condition* left;
  string_view op;  // "AND" or "OR"
  Condition* right;
};

// arithmeticExpression [IS] [NOT] (> | >= | < | <= | = | <>)
// arithmeticExpression, or arithmeticExpression [IS] [NOT] (POSITIVE |
// NEGATIVE | ZERO) -- `right` is null for a sign test, and `op` carries
// POSITIVE/NEGATIVE/ZERO then.
struct RelationCond : cursiv::Extends<Condition> {
  ArithExpr* left;
  boolean negated;
  string_view op;
  ArithExpr* right;
};

// identifier [IS] [NOT] (NUMERIC | ALPHABETIC | ALPHABETIC-LOWER |
// ALPHABETIC-UPPER | DBCS | KANJI | className).
struct ClassCond : cursiv::Extends<Condition> {
  Identifier* subject;
  boolean negated;
  string_view className;
};

// A level-88 condition-name tested by name, with its own subscripts.
struct CondNameRef : cursiv::Extends<Condition> {
  Identifier* name;
};

// Abbreviated comparisons (`a > b AND < c`) do not repeat the left operand,
// so the omitted operator/operand are recorded empty/null rather than
// resolved -- resolving the implied subject is a semantic step this
// parser does not take.
struct AbbrevCond : cursiv::Extends<Condition> {
  boolean negated;
  string_view op;
  ArithExpr* value;
  AbbrevCond* nested;  // the parenthesized abbreviation this one wraps, null when absent
};

// -- data division ----

struct DataEntry : cursiv::Extends<cursiv::AstNode> {};

struct DataItem : cursiv::Extends<DataEntry> {
  string_view level;  // "01".."49", "66", "77", "88" as written
  string_view name;   // empty for FILLER
  boolean filler;
  vector<reference<DataItem>> redefines;
  vector<string_view> pictures;     // the PICTURE string as written
  vector<string_view> usages;       // COMP-3, DISPLAY, POINTER... raw spelling
  vector<string_view> values;       // VALUE items/ranges, original token spelling with canonical spaces
  vector<string_view> occursMin;    // raw digits; empty when no OCCURS
  vector<string_view> occursMax;    // OCCURS n TO m; empty otherwise
  vector<reference<DataItem>> occursDependingOn;
  vector<string_view> occursIndexes;  // INDEXED BY names
  vector<string_view> signs;  // LEADING/TRAILING; empty when unspecified
  boolean signSeparate;
  boolean justified;
  boolean blankWhenZero;
  boolean synchronized;
  boolean global;
  boolean external;
  boolean aligned;
  boolean thread_local_;
  // level 66
  vector<reference<DataItem>> renamesFrom;
  vector<reference<DataItem>> renamesTo;
  // level 88
  boolean conditionName;
};

struct EmbeddedSql : cursiv::Extends<DataEntry> {
  vector<string_view> lines;
};

struct FileControlEntry : cursiv::Extends<cursiv::AstNode> {
  string_view fileName;
  boolean optional_;
  string_view assignTo;    // the device word or name/literal after ASSIGN TO
  string_view organization;  // SEQUENTIAL/RELATIVE/INDEXED, empty if unstated
  string_view accessMode;    // SEQUENTIAL/RANDOM/DYNAMIC/EXCLUSIVE
  vector<string_view> recordKeys;  // RECORD KEY and each ALTERNATE RECORD KEY, as written
};

struct FileDescription : cursiv::Extends<cursiv::AstNode> {
  string_view fileName;
  boolean sortMerge;  // SD, rather than FD
  vector<DataEntry*> records;
};

// SCREEN SECTION: the clauses a tool would use for layout are kept; the rest
// (bell, blink, grid, highlight...) parse but write nothing -- presence, not
// value, is what they carry.
struct ScreenItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> pictures;
  vector<string_view> values;
  vector<string_view> lines;
  vector<string_view> columns;
  vector<string_view> usingOrFrom;
};

// REPORT SECTION: same proportion as ScreenItem.
struct ReportGroupItem : cursiv::Extends<cursiv::AstNode> {
  string_view level;
  string_view name;
  vector<string_view> pictures;
  vector<string_view> types;  // REPORT HEADING, PAGE FOOTING, DETAIL...
  vector<string_view> values;
  vector<string_view> sources;
  vector<string_view> lines;
  vector<string_view> columns;
};

struct ReportDescription : cursiv::Extends<cursiv::AstNode> {
  string_view reportName;
  vector<ReportGroupItem*> groups;
};

// COMMUNICATION SECTION: the CD's name and direction; its many status/queue
// clauses parse and write nothing, a teleprocessing detail no example here
// exercises meaningfully.
struct CommunicationEntry : cursiv::Extends<cursiv::AstNode> {
  string_view cdName;
  string_view direction;  // INPUT, OUTPUT, or I-O
};

struct LibraryEntry : cursiv::Extends<cursiv::AstNode> {
  string_view libraryName;
  boolean export_;  // LD ... EXPORT, rather than LB ... IMPORT
};

struct DataBaseEntry : cursiv::Extends<cursiv::AstNode> {
  string_view db;
  string_view invoke;
};

struct DataDivision : cursiv::Extends<cursiv::AstNode> {
  vector<FileDescription*> files;
  vector<DataBaseEntry*> databases;
  vector<DataEntry*> workingStorage;
  vector<DataEntry*> linkage;
  vector<CommunicationEntry*> communications;
  vector<DataEntry*> localStorage;
  vector<ScreenItem*> screens;
  vector<ReportDescription*> reports;
  vector<LibraryEntry*> libraries;
};

struct EnvironmentDivision : cursiv::Extends<cursiv::AstNode> {
  vector<FileControlEntry*> files;
};

// -- procedure division ----

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct RoundedTarget : cursiv::Extends<cursiv::AstNode> {
  Identifier* target;
  boolean rounded;
};

struct AcceptStatement : cursiv::Extends<Statement> {
  Identifier* target;
  string_view source;  // DATE/DAY/.../a mnemonic name/"ESCAPE KEY"/"MESSAGE COUNT"
  vector<Statement*> onException;
  vector<Statement*> notOnException;
};

// ADD, SUBTRACT, MULTIPLY, DIVIDE and COMPUTE: one shape (operands feeding
// rounded targets, guarded by ON SIZE ERROR) covers every format each of the
// five verbs writes.
struct ArithStatement : cursiv::Extends<Statement> {
  string_view verb;  // ADD/SUBTRACT/MULTIPLY/DIVIDE/COMPUTE
  string_view form;  // TO/GIVING/CORRESPONDING/INTO/BY, whichever the verb used
  vector<Operand*> operands;
  vector<RoundedTarget*> targets;
  ArithExpr* computeExpr;   // COMPUTE's right-hand side; null otherwise
  Identifier* remainder;    // DIVIDE ... REMAINDER; null otherwise
  vector<Statement*> onSizeError;
  vector<Statement*> notOnSizeError;
};

struct AlterStatement : cursiv::Extends<Statement> {
  vector<string_view> from;
  vector<string_view> to;
};

enum class CallMode { reference, value, content };
struct CallArgument : cursiv::Extends<cursiv::AstNode> {
  CallMode mode;
  string_view modifier;  // ADDRESS OF / LENGTH [OF] / INTEGER / STRING
  Operand* value;
  boolean omitted;
};

struct CallStatement : cursiv::Extends<Statement> {
  Operand* program;
  vector<CallArgument*> using_;
  Identifier* giving;
  vector<Statement*> onOverflow;
  vector<Statement*> onException;
  vector<Statement*> notOnException;
};

struct CancelStatement : cursiv::Extends<Statement> {
  vector<Operand*> targets;
};

struct CloseStatement : cursiv::Extends<Statement> {
  vector<string_view> files;
};

struct ContinueStatement : cursiv::Extends<Statement> {};

struct DeleteStatement : cursiv::Extends<Statement> {
  string_view fileName;
  vector<Statement*> invalidKey;
  vector<Statement*> notInvalidKey;
};

struct EnableDisableStatement : cursiv::Extends<Statement> {
  boolean enable;  // false for DISABLE
  string_view direction;  // INPUT/I-O/OUTPUT
  string_view cdName;
  Operand* key;
};

struct DisplayStatement : cursiv::Extends<Statement> {
  vector<Operand*> operands;
  Operand* at;
  string_view upon;
  boolean noAdvancing;
};

struct EntryStatement : cursiv::Extends<Statement> {
  string_view literalText;
  vector<Identifier*> using_;
};

struct EvaluateSubject : cursiv::Extends<cursiv::AstNode> {
  ArithExpr* value;  // identifier/literal/arithmeticExpression
  Condition* cond;   // set instead of `value` when the subject is a condition
};

struct EvaluateValue : cursiv::Extends<cursiv::AstNode> {
  boolean any;
  boolean negated;
  ArithExpr* value;
  ArithExpr* through;
  Condition* cond;
  string_view boolLiteral;  // TRUE/FALSE, spelled directly
};

struct EvaluateWhen : cursiv::Extends<cursiv::AstNode> {
  vector<EvaluateValue*> values;  // one per ALSO-joined selector
};

struct EvaluateWhenPhrase : cursiv::Extends<cursiv::AstNode> {
  vector<EvaluateWhen*> whens;
  vector<Statement*> body;
};

struct EvaluateStatement : cursiv::Extends<Statement> {
  vector<EvaluateSubject*> subjects;
  vector<EvaluateWhenPhrase*> whenPhrases;
  boolean hasWhenOther;
  vector<Statement*> whenOther;
};

struct EmbeddedStatement : cursiv::Extends<Statement> {
  vector<string_view> lines;  // EXEC CICS/SQL/SQLIMS ... END-EXEC, one per line
  string_view kind;           // "CICS", "SQL", "SQLIMS"
};

struct ExhibitStatement : cursiv::Extends<Statement> {
  boolean named;
  boolean changed;
  vector<Operand*> operands;
};

struct ExitStatement : cursiv::Extends<Statement> {
  boolean program;
};

struct GenerateStatement : cursiv::Extends<Statement> {
  Identifier* reportName;
};

struct GobackStatement : cursiv::Extends<Statement> {};

struct GoToStatement : cursiv::Extends<Statement> {
  vector<string_view> targets;  // one for a plain GO TO, several for DEPENDING ON
  Identifier* dependingOn;
  boolean moreLabels;
};

struct IfStatement : cursiv::Extends<Statement> {
  Condition* cond;
  vector<Statement*> thenBranch;
  boolean thenNextSentence;
  vector<Statement*> elseBranch;
  boolean elseNextSentence;
  boolean hasElse;
};

struct InitializeReplacing : cursiv::Extends<cursiv::AstNode> {
  string_view category;  // ALPHABETIC/ALPHANUMERIC/NUMERIC/.../DBCS/EGCS
  Operand* value;
};

struct InitializeStatement : cursiv::Extends<Statement> {
  vector<Identifier*> targets;
  vector<InitializeReplacing*> replacing;
};

struct InitiateStatement : cursiv::Extends<Statement> {
  vector<Identifier*> reports;
};

// INSPECT keeps its groups, operands and boundaries for runtime evaluation.
struct InspectBoundary : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  boolean initial;
  Operand* operand;
};
struct InspectPattern : cursiv::Extends<cursiv::AstNode> {
  Operand* from;
  Operand* to;
  vector<InspectBoundary*> boundaries;
};
struct InspectGroup : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  vector<InspectPattern*> patterns;
};
struct InspectTally : cursiv::Extends<cursiv::AstNode> {
  Operand* counter;
  vector<InspectGroup*> groups;
};
struct InspectStatement : cursiv::Extends<Statement> {
  Identifier* target;
  vector<InspectTally*> tallies;
  vector<InspectGroup*> replacements;
  InspectPattern* conversion;
  boolean tallying;
  boolean replacing;
  boolean converting;
};

struct MergeStatement : cursiv::Extends<Statement> {
  string_view fileName;
  vector<string_view> keys;
  vector<string_view> using_;
  string_view outputProcedure;
  vector<string_view> giving;
};

struct MoveStatement : cursiv::Extends<Statement> {
  boolean all;
  boolean corresponding;
  Operand* from;
  vector<Identifier*> to;
};

struct OpenGroup : cursiv::Extends<cursiv::AstNode> {
  string_view mode;  // INPUT/OUTPUT/I-O/EXTEND
  vector<string_view> files;
};

struct OpenStatement : cursiv::Extends<Statement> {
  vector<OpenGroup*> groups;
};

struct PerformVarying : cursiv::Extends<cursiv::AstNode> {
  Operand* index;
  ArithExpr* from;
  ArithExpr* by;
  Condition* until;
};

struct PerformStatement : cursiv::Extends<Statement> {
  boolean inline_;
  string_view procedureFrom;
  string_view procedureThrough;
  ArithExpr* times;
  Condition* until;
  boolean testAfter;
  vector<PerformVarying*> varying;
  vector<Statement*> body;  // performInlineStatement's own statements
};

struct PurgeStatement : cursiv::Extends<Statement> {
  vector<string_view> queues;
};

struct ReadStatement : cursiv::Extends<Statement> {
  string_view fileName;
  boolean next;
  Identifier* into;
  string_view key;
  vector<Statement*> invalidKey;
  vector<Statement*> notInvalidKey;
  vector<Statement*> atEnd;
  vector<Statement*> notAtEnd;
};

struct ReceiveStatement : cursiv::Extends<Statement> {
  string_view text;
  vector<Statement*> onException;
  vector<Statement*> notOnException;
};

struct ReleaseStatement : cursiv::Extends<Statement> {
  string_view recordName;
  Identifier* from;
};

struct ReturnStatement : cursiv::Extends<Statement> {
  string_view fileName;
  Identifier* into;
  vector<Statement*> atEnd;
  vector<Statement*> notAtEnd;
};

struct RewriteStatement : cursiv::Extends<Statement> {
  string_view recordName;
  Identifier* from;
  vector<Statement*> invalidKey;
  vector<Statement*> notInvalidKey;
};

struct SearchWhen : cursiv::Extends<cursiv::AstNode> {
  Condition* cond;
  vector<Statement*> body;
  boolean nextSentence;
};

struct SearchStatement : cursiv::Extends<Statement> {
  boolean all;
  string_view tableName;
  Identifier* varying;
  vector<Statement*> atEnd;
  vector<SearchWhen*> whens;
};

struct SendStatement : cursiv::Extends<Statement> {
  string_view text;
};

struct SetStatement : cursiv::Extends<Statement> {
  vector<Identifier*> targets;
  string_view op;  // TO/UP BY/DOWN BY
  vector<Operand*> values;
};

struct SortStatement : cursiv::Extends<Statement> {
  string_view fileName;
  vector<string_view> keys;
  vector<string_view> using_;
  string_view inputProcedure;
  string_view outputProcedure;
  vector<string_view> giving;
};

struct StartStatement : cursiv::Extends<Statement> {
  string_view fileName;
  string_view op;  // relational operator text, empty if the KEY clause is absent
  string_view key;
  vector<Statement*> invalidKey;
  vector<Statement*> notInvalidKey;
};

struct StopStatement : cursiv::Extends<Statement> {
  boolean run;
  Operand* value;  // STOP literal, null when STOP RUN
};

struct StringSendItem : cursiv::Extends<cursiv::AstNode> {
  vector<Operand*> operands;
  string_view delimitedBy;  // SIZE, or an operand's raw text
  Operand* forPhrase;
};

struct StringStatement : cursiv::Extends<Statement> {
  vector<StringSendItem*> sending;
  Identifier* into;
  Identifier* pointer;
  vector<Statement*> onOverflow;
  vector<Statement*> notOnOverflow;
};

struct TerminateStatement : cursiv::Extends<Statement> {
  Identifier* reportName;
};

struct UnstringInto : cursiv::Extends<cursiv::AstNode> {
  Identifier* target;
  Identifier* delimiterIn;
  Identifier* countIn;
};

struct UnstringStatement : cursiv::Extends<Statement> {
  Operand* sending;
  vector<Operand*> delimiters;
  vector<UnstringInto*> into;
  Identifier* pointer;
  Identifier* tallying;
  vector<Statement*> onOverflow;
  vector<Statement*> notOnOverflow;
};

struct UseStatement : cursiv::Extends<Statement> {
  string_view text;
};

struct WriteStatement : cursiv::Extends<Statement> {
  string_view recordName;
  Operand* from;
  string_view advancing;  // PAGE, a line count's raw text, or a mnemonic name
  vector<Statement*> atEndOfPage;
  vector<Statement*> notAtEndOfPage;
  vector<Statement*> invalidKey;
  vector<Statement*> notInvalidKey;
};

struct Paragraph : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Statement*> statements;
};

struct ProcedureSection : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Paragraph*> paragraphs;
};

struct Declarative : cursiv::Extends<cursiv::AstNode> {
  string_view sectionName;
  UseStatement* use;
  vector<Paragraph*> paragraphs;
};

struct ProcedureDivision : cursiv::Extends<cursiv::AstNode> {
  vector<Identifier*> using_;
  string_view giving;
  vector<Declarative*> declaratives;
  vector<Statement*> statements;  // the leading, unnamed run before any paragraph
  vector<Paragraph*> paragraphs;
  vector<ProcedureSection*> sections;
};

// -- program ----

struct ProgramUnit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  EnvironmentDivision* environment;
  DataDivision* data;
  ProcedureDivision* procedure;
  vector<ProgramUnit*> nested;
  boolean hasEndProgram;
  string_view endName;
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<ProgramUnit*> programs;
};

}  // namespace cobol85::ast
