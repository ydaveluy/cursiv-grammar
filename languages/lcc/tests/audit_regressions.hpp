#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "lcc/core/CoreServices.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <array>
#include <string>
#include <utility>
#include <vector>
namespace audit_regressions {
inline bool run(const lcc::LccCoreServices& services) {
  struct Case { const char* source; const char* letters; const char* number;
    std::vector<std::pair<std::string,std::string>> cutters; const char* edition; const char* item; };
  const std::array cases = {
    Case{"QB981 .H377 1998","QB","981",{{"H","377"}},"1998",nullptr},
    Case{"KFA1245 .A1 2001","KFA","1245",{{"A","1"}},"2001",nullptr},
    Case{"QA76.73.P98 D67 2020","QA","76.73",{{"P","98"},{"D","67"}},"2020",nullptr},
    Case{"qa76.73 .p98 2020b","qa","76.73",{{"p","98"}},"2020b",nullptr},
    Case{"QA76 P98 2000","QA","76",{{"P","98"}},"2000",nullptr},
    Case{"QB981 .H377","QB","981",{{"H","377"}},nullptr,nullptr},
    Case{"KFA1245  .A1  2001c","KFA","1245",{{"A","1"}},"2001c",nullptr},
    Case{"QA76 .ABC123 2010","QA","76",{{"ABC","123"}},"2010",nullptr},
    Case{"QA76.73.P98 D67 2020 v.2","QA","76.73",{{"P","98"},{"D","67"}},"2020","v.2"},
    Case{"QA76 .P98 v.2","QA","76",{{"P","98"}},nullptr,"v.2"},
    Case{"QA76 .P98 2020 v.2 c.1","QA","76",{{"P","98"}},"2020","v.2 c.1"},
    Case{"QA76 .P98 D67 2020 Volume 2","QA","76",{{"P","98"},{"D","67"}},"2020","Volume 2"},
    Case{"QA76 .P98 Volume2","QA","76",{{"P","98"}},nullptr,"Volume2"},
    Case{"QA76 .P98D67 2020","QA","76",{{"P","98"},{"D","67"}},"2020",nullptr},
    Case{"QA76\t.P98\t2020\tv.2","QA","76",{{"P","98"}},"2020","v.2"},
    Case{"QA76 .P98 2020b copy 1","QA","76",{{"P","98"}},"2020b","copy 1"},
    Case{"QA76 .P98 12345","QA","76",{{"P","98"}},nullptr,"12345"},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result=services.parser->parse(source);
    const auto* doc=result.value_as<lcc::ast::CallNumber>();
    if (!result.accepted || result.cst().recovered() || !doc ||
        !result.diagnostics.empty() || doc->cutters.size()!=test.cutters.size())
      return false;
    std::string letters(doc->topic);
    if (doc->subtopic) letters+=*doc->subtopic;
    std::string number;
    for (auto p:doc->subclasses) { if (!number.empty()) number+='.'; number+=p; }
    if (letters!=test.letters || number!=test.number ||
        (test.edition ? !doc->date || *doc->date!=test.edition : doc->date.has_value()) ||
        (test.item ? !doc->item || *doc->item!=test.item : doc->item.has_value()))
      return false;
    for (std::size_t i=0;i<test.cutters.size();++i)
      if (doc->cutters[i]->parent!=doc ||
          doc->cutters[i]->letter!=test.cutters[i].first ||
          doc->cutters[i]->digits!=test.cutters[i].second)
        return false;
    if (!cursiv::test::validationHelper(services,source).diagnostics.empty())
      return false;
  }
  for (auto test:std::array{
      std::pair{"ABCD123 .A1 2000", "lcc.class-letters"},
      std::pair{"QA10000 .A1 2020", "lcc.class-number"},
      std::pair{"QA76.73.4 .A1 2020", "lcc.class-number"},
      std::pair{"QA123 .ABCD1 2000", "lcc.cutter-letters"}}) {
    const std::string source(test.first);
    const auto result=services.parser->parse(source);
    if (!result.accepted || result.cst().recovered()) return false;
    bool found=false;
    for (const auto& d:cursiv::test::validationHelper(services,source).diagnostics)
      found |= d.code && d.code->isString() && d.code->string()==test.second;
    if (!found) return false;
  }
  return true;
}
}
