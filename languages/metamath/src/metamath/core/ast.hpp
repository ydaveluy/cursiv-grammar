#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace metamath::ast {

struct IncludeStmt : cursiv::Extends<cursiv::AstNode> {
  string_view filename;
};

using MathItem = std::variant<std::string_view, IncludeStmt *>;

struct MathStatement : cursiv::Extends<cursiv::AstNode> {
  vector<MathItem> math;
};
struct ConstantStmt : cursiv::Extends<MathStatement> {};
struct VariableStmt : cursiv::Extends<MathStatement> {};
struct DisjointStmt : cursiv::Extends<MathStatement> {};

struct LabeledStatement : cursiv::Extends<MathStatement> {
  string_view label;
};
struct FloatingStmt : cursiv::Extends<LabeledStatement> {};
struct EssentialStmt : cursiv::Extends<LabeledStatement> {};
struct AxiomStmt : cursiv::Extends<LabeledStatement> {};

struct ProofStep : cursiv::Extends<cursiv::AstNode> {
  string_view target;
  string_view local;
  string_view label;
};
using ProofItem = std::variant<std::string_view, ProofStep *, IncludeStmt *>;
struct UncompressedProof : cursiv::Extends<cursiv::AstNode> {
  vector<ProofItem> steps;
};
struct CompressedProof : cursiv::Extends<cursiv::AstNode> {
  vector<MathItem> labels;
  vector<MathItem> blocks;
};
using Proof = std::variant<UncompressedProof *, CompressedProof *>;
struct ProvableStmt : cursiv::Extends<LabeledStatement> {
  Proof proof;
};

struct DeferredStmt : cursiv::Extends<MathStatement> {
  Proof proof;
};
struct ScopeOpen : cursiv::Extends<cursiv::AstNode> {};
struct ScopeClose : cursiv::Extends<cursiv::AstNode> {};
using Stmt =
    std::variant<IncludeStmt *, DeferredStmt *, ScopeOpen *, ScopeClose *,
                 ConstantStmt *, VariableStmt *, DisjointStmt *, FloatingStmt *,
                 EssentialStmt *, AxiomStmt *, ProvableStmt *>;
struct Database : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt> stmts;
};

} // namespace metamath::ast
