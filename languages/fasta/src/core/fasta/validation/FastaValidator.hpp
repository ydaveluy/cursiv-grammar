#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "fasta/ast.hpp"

namespace fasta {

class FastaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkSequence(const ast::SequenceLine& line,
                     cursiv::ValidationAcceptor& accept) const;
};

struct FastaCoreServices;
void register_validation_checks(FastaCoreServices& services);

}  // namespace fasta
