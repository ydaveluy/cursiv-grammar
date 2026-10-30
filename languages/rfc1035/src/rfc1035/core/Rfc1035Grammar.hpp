#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc1035/core/ast.hpp"

namespace rfc1035 {

struct Rfc1035 : cursiv::GrammarSpec {
  struct LabelByte
      : terminal<(lit<"\\"> + option(any)) | (!(lit<"."> | lit<"\\">) + any)> {};
  struct Label : rule<as<std::string_view>(many(ref<LabelByte>))> {};
  struct Domain
      : rule<create<ast::Domain>(
            some(append<&ast::Domain::labels>(ref<Label>), lit<".">))> {};

  using entry = Domain;
};

}  // namespace rfc1035
