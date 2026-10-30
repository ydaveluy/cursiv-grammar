// The postalcode language: a Canadian postal code, three letters alternating
// with three digits.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "postalcode/core/ast.hpp"

namespace postalcode {

struct Postalcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\v\f\uFEFF\u2028\u2029)"> | uc::Zs)> {
  };

  struct LETTER : terminal<cr<"A-Za-z">> {};
  struct DIGIT : terminal<cr<"0-9">> {};

  struct ForwardSortationArea : rule<ref<LETTER> + ref<DIGIT> + ref<LETTER>> {};
  struct LocalDeliveryUnit : rule<ref<DIGIT> + ref<LETTER> + ref<DIGIT>> {};

  struct PostalCode : rule<assign<&ast::PostalCode::forwardSortationArea>(
                                   ref<ForwardSortationArea>) +
                               option(lit<"-">) +
                               assign<&ast::PostalCode::localDeliveryUnit>(
                                   ref<LocalDeliveryUnit>)> {};

  using entry = PostalCode;
  using ignored = WS;
};

}  // namespace postalcode
