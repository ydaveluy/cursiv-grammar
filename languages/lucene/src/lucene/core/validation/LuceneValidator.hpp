#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lucene/core/ast.hpp"
namespace lucene {
class LuceneValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkQuery(const ast::Query &, cursiv::ValidationAcceptor &) const;
  void checkClause(const ast::Clause &, cursiv::ValidationAcceptor &) const;
  void checkWord(const ast::WordTerm &, cursiv::ValidationAcceptor &) const;
  void checkQuoted(const ast::QuotedTerm &, cursiv::ValidationAcceptor &) const;
  void checkBound(const ast::RangeBound &, cursiv::ValidationAcceptor &) const;
  void checkTerm(const ast::Term &, cursiv::ValidationAcceptor &) const;
  void checkGroup(const ast::GroupingExpr &,
                  cursiv::ValidationAcceptor &) const;
};
struct LuceneCoreServices;
void register_validation_checks(LuceneCoreServices &services);
} // namespace lucene
