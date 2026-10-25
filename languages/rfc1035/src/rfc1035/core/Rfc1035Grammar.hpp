#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc1035/core/ast.hpp"

namespace rfc1035 {

struct Rfc1035 : cursiv::GrammarSpec {
  struct LabelByte
      : terminal<(lit<"\\"> + option(any)) | (!(lit<"."> | lit<"\\">) + any)> {};
  struct Label : rule<many(ref<LabelByte>), std::string_view> {};
  struct DomainRule
      : rule<create<ast::Domain>(
            some(append<&ast::Domain::labels>(ref<Label>), lit<".">))> {};

  using entry = DomainRule;
};

}  // namespace rfc1035
