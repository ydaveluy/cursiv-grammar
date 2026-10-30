// The llvmir language: its AST. LLVM IR enumerates ~90 instruction and
// constant-expr mnemonics and ~90 debug-info fields, nearly all sharing one
// of a handful of shapes; those collapse into generic nodes (Attr,
// GenericInst, NamedField) keyed by their spelling, while the constructs a
// tool reads structurally -- types, calls, control flow, phi, metadata
// containers -- keep their own class.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace llvmir::ast {

struct Node : cursiv::Extends<cursiv::AstNode> {};

// ---- Types ----

struct Type : cursiv::Extends<Node> {};
struct VoidType : cursiv::Extends<Type> {};
struct OpaqueType : cursiv::Extends<Type> {};
struct LabelType : cursiv::Extends<Type> {};
struct TokenType : cursiv::Extends<Type> {};
struct MetadataType : cursiv::Extends<Type> {};
struct MmxType : cursiv::Extends<Type> {};
struct AmxType : cursiv::Extends<Type> {};
struct ByteType : cursiv::Extends<Type> {
  string_view text;
};
struct TargetType : cursiv::Extends<Type> {
  string_view name;
  vector<variant<Type*, string_view>> parameters;
};
struct IntType : cursiv::Extends<Type> {
  string_view text;
};
struct FloatType : cursiv::Extends<Type> {
  string_view keyword;
};
struct NamedType : cursiv::Extends<Type> {
  string_view name;
};
struct PointerType : cursiv::Extends<Type> {
  Type* pointee = nullptr;  // null: opaque `ptr`
  optional<string_view> addrSpace;
};
struct Params;
struct FunctionType : cursiv::Extends<Type> {
  Type* returnType = nullptr;
  Params* params = nullptr;
};
struct VectorType : cursiv::Extends<Type> {
  string_view length;
  boolean scalable = false;
  Type* element = nullptr;
};
struct ArrayType : cursiv::Extends<Type> {
  string_view length;
  Type* element = nullptr;
};
struct StructType : cursiv::Extends<Type> {
  vector<Type*> members;
  boolean packed = false;
};

// ---- Attributes: every param/return/func attribute, and every
// section/partition/comdat/align/gc/prefix/prologue/personality/sanitizer
// decoration, is one of a handful of shapes: a bare keyword, a keyword with
// one or two ints, a keyword with a string, a keyword with a type, a keyword
// with a typed constant, or a `"k"="v"` pair. ----
struct TypedValue;
struct AttrEntry : cursiv::Extends<Node> {
  optional<string_view> name;
  vector<string_view> values;
  vector<AttrEntry*> entries;
  boolean grouped = false;
};
struct Attr : cursiv::Extends<Node> {
  string_view name;
  optional<string_view> intArg;
  optional<string_view> intArg2;
  optional<string_view> stringArg;
  optional<Type*> typeArg;
  optional<TypedValue*> valueArg;
  vector<AttrEntry*> entries;
};

struct Param : cursiv::Extends<Node> {
  Type* type = nullptr;
  vector<Attr*> attrs;
  optional<string_view> name;
};
struct Params : cursiv::Extends<Node> {
  vector<Param*> params;
  boolean variadic = false;
};

struct CallingConv : cursiv::Extends<Node> {
  optional<string_view> keyword;
  optional<string_view> intArg;
};

// ---- Values and constants ----

struct Value : cursiv::Extends<Node> {};
// A type paired with a compile-time constant, and a type paired with any
// value, share this one shape: over-accepts a value where a strict constant
// was required.
struct TypedValue : cursiv::Extends<Value> {
  Type* type = nullptr;
  Value* value = nullptr;
  boolean inRange = false;  // a gep index's `inrange` flag, unused elsewhere
};
struct Ident : cursiv::Extends<Value> {
  string_view name;
};  // %x, @x, %42, @42
struct IntConst : cursiv::Extends<Value> {
  string_view text;
};
struct FloatConst : cursiv::Extends<Value> {
  string_view text;
};
struct BoolConst : cursiv::Extends<Value> {
  boolean value = false;
};
struct KeywordConst : cursiv::Extends<Value> {
  string_view keyword;
};  // null,none,zeroinitializer,undef,poison
struct StructConst : cursiv::Extends<Value> {
  vector<TypedValue*> members;
  boolean packed = false;
};
struct ArrayConst : cursiv::Extends<Value> {
  vector<TypedValue*> elements;
  optional<string_view> cString;  // `c"..."` form
};
struct VectorConst : cursiv::Extends<Value> {
  vector<TypedValue*> elements;
};
struct BlockAddressConst : cursiv::Extends<Value> {
  string_view function;
  string_view block;
};
struct DsoLocalEquivalentConst : cursiv::Extends<Value> {
  string_view target;
};
struct NoCfiConst : cursiv::Extends<Value> {
  string_view target;
};
struct ConstantExpr : cursiv::Extends<Value> {
  string_view op;
  vector<string_view> flags;
  optional<string_view> inRangeStart;
  optional<string_view> inRangeEnd;
  optional<string_view> predicate;  // icmp/fcmp predicate
  optional<Type*> destType;         // conversion `to` type
  optional<Type*> gepType;          // getelementptr's leading bare type
  vector<TypedValue*> operands;
};
struct InlineAsm : cursiv::Extends<Value> {
  boolean sideEffect = false;
  boolean alignStack = false;
  boolean intelDialect = false;
  boolean unwind = false;
  string_view code;
  string_view constraints;
};

// ---- Metadata: every `!DIXxx(...)` node is a kind plus name:value fields,
// and every field value is one of a handful of shapes (a nested metadata
// node, a scalar token, or a `|`-joined / brace list of scalars). One
// generic representation covers the whole family. ----

struct MdTuple;
struct SpecializedMdNode;
struct DiArgList;
struct MdString;
struct MdRef;
struct MdList;
using MdVal =
    std::variant<std::monostate, std::string_view, TypedValue*, MdString*,
                 MdRef*, MdTuple*, SpecializedMdNode*, DiArgList*, MdList*>;

struct MdRef : cursiv::Extends<Node> {
  string_view id;
};
struct MdString : cursiv::Extends<Node> {
  string_view text;
};
struct MdTuple : cursiv::Extends<Node> {
  vector<MdVal> elements;
};
struct DiArgList : cursiv::Extends<Node> {
  vector<TypedValue*> args;
};
struct MdList : cursiv::Extends<Node> {
  vector<MdVal> items;
};  // operands:{...}, diFlags, spFlags
struct NamedField : cursiv::Extends<Node> {
  string_view name;
  MdVal value;
};
struct SpecializedMdNode : cursiv::Extends<Node> {
  string_view kind;  // "!DIBasicType", "!DIExpression", ...
  vector<NamedField*> fields;
  vector<string_view> positional;  // diExpression's bare token list
};
struct MdAttachment : cursiv::Extends<Node> {
  string_view name;
  MdVal node;
};

// ---- Instructions and terminators ----

struct DebugRecord : cursiv::Extends<Node> {
  string_view kind;
  vector<MdVal> args;
};

struct Instruction : cursiv::Extends<Node> {
  vector<DebugRecord*> debugRecords;
  optional<string_view> result;
  vector<MdAttachment*> metadata;
};

struct Label : cursiv::Extends<Node> {
  string_view name;
};

struct CallArg : cursiv::Extends<Node> {
  optional<Type*> type;
  vector<Attr*> attrs;
  optional<Value*> value;
  boolean isMetadata = false;
  MdVal metadata;
};

struct GenericInst : cursiv::Extends<Instruction> {
  string_view op;
  vector<string_view> flags;
  optional<string_view> predicate;
  optional<Type*> type;
  vector<TypedValue*> operands;
  vector<string_view> indices;    // extractvalue/insertvalue IntLit run
  vector<string_view> orderings;  // atomic ordering keyword(s)
  optional<string_view> syncScope;
  vector<string_view> align;
  vector<string_view> addrSpace;
  boolean toCaller = false;           // 'unwind to caller'
  optional<string_view> withinToken;  // catchpad/cleanuppad 'within' target
  vector<CallArg*> exceptionArgs;     // catchpad/cleanuppad bracket list
  vector<string_view> clauseTypes;    // landingpad catch/filter per operand
  vector<Label*> labels;              // cleanupret/catchswitch label(s)
};

struct PhiIncoming : cursiv::Extends<Node> {
  Value* value = nullptr;
  string_view block;
};
struct PhiInst : cursiv::Extends<Instruction> {
  vector<string_view> fastMathFlags;
  Type* type = nullptr;
  vector<PhiIncoming*> incomings;
};

struct OperandBundle : cursiv::Extends<Node> {
  string_view tag;
  vector<TypedValue*> args;
};
struct CallInst : cursiv::Extends<Instruction> {
  boolean forwardsVarargs; // trailing ... in a musttail call argument list
  string_view kind;  // call | invoke | callbr
  optional<string_view> tailKind;
  vector<string_view> fastMathFlags;
  optional<CallingConv*> callingConv;
  vector<Attr*> returnAttrs;
  optional<string_view> addrSpace;
  Type* type = nullptr;
  Value* callee = nullptr;
  vector<CallArg*> args;
  vector<Attr*> funcAttrs;
  vector<OperandBundle*> bundles;
  optional<Label*> toLabel;
  optional<Label*> unwindLabel;
  vector<Label*> indirectLabels;
};

struct RetInst : cursiv::Extends<Instruction> {
  optional<TypedValue*> value;
};
struct BrInst : cursiv::Extends<Instruction> {
  Label* target = nullptr;
};
struct CondBrInst : cursiv::Extends<Instruction> {
  TypedValue* cond = nullptr;
  Label* thenLabel = nullptr;
  Label* elseLabel = nullptr;
};
struct SwitchCase : cursiv::Extends<Node> {
  TypedValue* value = nullptr;
  Label* target = nullptr;
};
struct SwitchInst : cursiv::Extends<Instruction> {
  TypedValue* value = nullptr;
  Label* defaultLabel = nullptr;
  vector<SwitchCase*> cases;
};
struct IndirectBrInst : cursiv::Extends<Instruction> {
  TypedValue* address = nullptr;
  vector<Label*> targets;
};

struct BasicBlock : cursiv::Extends<Node> {
  optional<string_view> label;
  vector<Instruction*> instructions;
  Instruction* terminator = nullptr;
};

struct UseListOrder;
struct FuncBody : cursiv::Extends<Node> {
  vector<BasicBlock*> blocks;
  vector<UseListOrder*> useListOrders;
};

// ---- Top level ----

struct TopLevelEntity : cursiv::Extends<Node> {};

struct SummaryField;
struct SummaryValue : cursiv::Extends<Node> {
  boolean grouped = false;
  vector<string_view> atoms;
  vector<variant<SummaryField*, SummaryValue*>> elements;
};
struct SummaryField : cursiv::Extends<Node> {
  string_view name;
  SummaryValue* value = nullptr;
};
struct SummaryEntry : cursiv::Extends<TopLevelEntity> {
  string_view id;
  SummaryField* definition = nullptr;
};
struct AsmProperty : cursiv::Extends<Node> {
  string_view name;
  string_view value;
};
struct SourceFilename : cursiv::Extends<TopLevelEntity> {
  string_view text;
};
struct TargetDef : cursiv::Extends<TopLevelEntity> {
  string_view kind;  // datalayout | triple
  string_view text;
};
struct ModuleAsm : cursiv::Extends<TopLevelEntity> {
  vector<AsmProperty*> properties;
  vector<string_view> lines;
};
struct TypeDef : cursiv::Extends<TopLevelEntity> {
  string_view name;
  Type* type = nullptr;
};
struct ComdatDef : cursiv::Extends<TopLevelEntity> {
  string_view name;
  string_view kind;
};
struct GlobalVar : cursiv::Extends<TopLevelEntity> {
  string_view name;
  optional<string_view> linkage;
  optional<string_view> preemption;
  optional<string_view> visibility;
  optional<string_view> dllStorageClass;
  boolean threadLocal = false;
  optional<string_view> tlsModel;
  optional<string_view> unnamedAddr;
  optional<string_view> addrSpace;
  boolean externallyInitialized = false;
  string_view immutable;  // constant | global
  Type* type = nullptr;
  Value* initializer = nullptr;  // null: a declaration
  vector<Attr*> fields;          // section/partition/comdat/align/sanitizer
  vector<MdAttachment*> metadata;
  vector<Attr*> funcAttrs;
};
struct IndirectSymbol : cursiv::Extends<TopLevelEntity> {
  string_view name;
  optional<string_view> linkage;
  optional<string_view> preemption;
  optional<string_view> visibility;
  optional<string_view> dllStorageClass;
  boolean threadLocal = false;
  optional<string_view> tlsModel;
  optional<string_view> unnamedAddr;
  string_view kind;  // alias | ifunc
  Type* type = nullptr;
  Value* target = nullptr;
  vector<string_view> partitions;
  vector<MdAttachment*> metadata;
};
struct FuncHeader : cursiv::Extends<Node> {
  optional<string_view> linkage;
  optional<string_view> preemption;
  optional<string_view> visibility;
  optional<string_view> dllStorageClass;
  optional<CallingConv*> callingConv;
  vector<Attr*> returnAttrs;
  Type* returnType = nullptr;
  string_view name;
  Params* params = nullptr;
  optional<string_view> unnamedAddr;
  optional<string_view> addrSpace;
  vector<Attr*>
      fields;  // section/partition/comdat/align/gc/prefix/prologue/personality/attributes
};
struct FuncDecl : cursiv::Extends<TopLevelEntity> {
  vector<MdAttachment*> metadata;
  FuncHeader* header = nullptr;
};
struct FuncDef : cursiv::Extends<TopLevelEntity> {
  FuncHeader* header = nullptr;
  vector<MdAttachment*> metadata;
  FuncBody* body = nullptr;
};
struct AttrGroupDef : cursiv::Extends<TopLevelEntity> {
  string_view id;
  vector<Attr*> attrs;
};
struct NamedMetadataDef : cursiv::Extends<TopLevelEntity> {
  string_view name;
  vector<MdVal> nodes;
};
struct MetadataDef : cursiv::Extends<TopLevelEntity> {
  string_view id;
  boolean distinct = false;
  variant<MdTuple*, SpecializedMdNode*> value;
};
struct UseListOrder : cursiv::Extends<TopLevelEntity> {
  TypedValue* value = nullptr;
  vector<string_view> order;
};
struct UseListOrderBB : cursiv::Extends<TopLevelEntity> {
  string_view func;
  string_view block;
  vector<string_view> order;
};

struct Module : cursiv::Extends<Node> {
  vector<TopLevelEntity*> entities;
};

}  // namespace llvmir::ast
