#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "graphemes/core/GraphemeProperties.hpp"
#include "graphemes/core/ast.hpp"

namespace graphemes {
struct Graphemes : cursiv::GrammarSpec {
  static constexpr auto control = properties::Control | cr<R"(\r\n)">;
  static constexpr auto nonControl = ~control;
  static constexpr auto extend = properties::Extend;
  static constexpr auto trailing = extend | properties::ZWJ | properties::SpacingMark;
  static constexpr auto hangul =
      (many(properties::L) +
          ((some(properties::V) + many(properties::T)) |
           (properties::LV + many(properties::V) + many(properties::T)) |
           (properties::LVT + many(properties::T)))) |
      some(properties::L) | some(properties::T);
  static constexpr auto regional = properties::Regional_Indicator + option(properties::Regional_Indicator);
  static constexpr auto pictographic = properties::Extended_Pictographic + many(extend) +
      many(properties::ZWJ + properties::Extended_Pictographic + many(extend));
  static constexpr auto conjunct = properties::InCBConsonant + many(
      many(properties::InCBExtend) + properties::InCBLinker +
      many(properties::InCBExtend | properties::InCBLinker) + properties::InCBConsonant);

  struct CLUSTER : terminal<lit<"\r\n"> | control |
      (many(properties::Prepend) + (hangul | regional | pictographic | conjunct | nonControl) +
      many(trailing))> {};
  struct GraphemeClusterRule : rule<assign<&ast::GraphemeCluster::text>(ref<CLUSTER>)> {};
  struct DocRule : rule<create<ast::Document> +
      many(append<&ast::Document::clusters>(ref<GraphemeClusterRule>))> {};
  using entry = DocRule;
};
}  // namespace graphemes
