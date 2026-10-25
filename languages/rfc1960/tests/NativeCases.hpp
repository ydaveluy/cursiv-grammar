#pragma once
#include <string_view>
struct NativeCase {
  std::string_view name, file, native, error;
};
inline constexpr NativeCase nativeCases[]{
    {"equal", "equal.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[70,114,101,"
     "100]}",
     ""},
    {"approx", "approx.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"~=\",\"value\":[70,114,101,"
     "100]}",
     ""},
    {"ge", "ge.rfc1960",
     "{\"attr\":\"age\",\"kind\":\"simple\",\"op\":\">=\",\"value\":[49,50]}",
     ""},
    {"le", "le.rfc1960",
     "{\"attr\":\"age\",\"kind\":\"simple\",\"op\":\"<=\",\"value\":[52,50]}",
     ""},
    {"empty-value", "empty-value.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[]}", ""},
    {"presence", "presence.rfc1960", "{\"attr\":\"cn\",\"kind\":\"present\"}",
     ""},
    {"initial", "initial.rfc1960",
     "{\"attr\":\"cn\",\"final\":null,\"initial\":[97],\"kind\":\"substring\","
     "\"middle\":[]}",
     ""},
    {"final", "final.rfc1960",
     "{\"attr\":\"cn\",\"final\":[122],\"initial\":null,\"kind\":\"substring\","
     "\"middle\":[]}",
     ""},
    {"middle", "middle.rfc1960",
     "{\"attr\":\"cn\",\"final\":null,\"initial\":null,\"kind\":\"substring\","
     "\"middle\":[[109]]}",
     ""},
    {"all-parts", "all-parts.rfc1960",
     "{\"attr\":\"cn\",\"final\":[122],\"initial\":[97],\"kind\":\"substring\","
     "\"middle\":[[98],[99]]}",
     ""},
    {"legacy-star", "legacy-star.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[74,101,110,"
     "115,101,110,42]}",
     ""},
    {"legacy-parens", "legacy-parens.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[40,120,41]}",
     ""},
    {"legacy-slash", "legacy-slash.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[97,92,98]}",
     ""},
    {"hex-star", "hex-star.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[97,42,98]}",
     ""},
    {"hex-nul", "hex-nul.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[0]}", ""},
    {"utf8", "utf8.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[195,137,108,"
     "111,100,105,101]}",
     ""},
    {"and", "and.rfc1960",
     "{\"kind\":\"and\",\"operands\":[{\"attr\":\"cn\",\"kind\":\"simple\","
     "\"op\":\"=\",\"value\":[97]},{\"attr\":\"sn\",\"kind\":\"simple\",\"op\":"
     "\"=\",\"value\":[98]}]}",
     ""},
    {"or", "or.rfc1960",
     "{\"kind\":\"or\",\"operands\":[{\"attr\":\"cn\",\"kind\":\"simple\","
     "\"op\":\"=\",\"value\":[97]},{\"attr\":\"sn\",\"kind\":\"simple\",\"op\":"
     "\"=\",\"value\":[98]}]}",
     ""},
    {"not", "not.rfc1960",
     "{\"kind\":\"not\",\"operands\":[{\"attr\":\"cn\",\"kind\":\"simple\","
     "\"op\":\"=\",\"value\":[97]}]}",
     ""},
    {"nested", "nested.rfc1960",
     "{\"kind\":\"and\",\"operands\":[{\"kind\":\"or\",\"operands\":[{\"attr\":"
     "\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[97]},{\"attr\":\"sn\","
     "\"kind\":\"simple\",\"op\":\"=\",\"value\":[98]}]},{\"kind\":\"not\","
     "\"operands\":[{\"attr\":\"uid\",\"kind\":\"simple\",\"op\":\"=\","
     "\"value\":[55]}]}]}",
     ""},
    {"spaces", "spaces.rfc1960",
     "{\"kind\":\"and\",\"operands\":[{\"attr\":\"cn\",\"kind\":\"simple\","
     "\"op\":\"=\",\"value\":[97]},{\"attr\":\"sn\",\"kind\":\"simple\",\"op\":"
     "\"=\",\"value\":[98]}]}",
     ""},
    {"leading-space", "leading-space.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[97]}", ""},
    {"attribute-option", "attribute-option.rfc1960",
     "{\"attr\":\"cn;lang-en\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[70,"
     "114,101,100]}",
     ""},
    {"oid", "oid.rfc1960",
     "{\"attr\":\"1.2.3\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[97,98,99]"
     "}",
     ""},
    {"ext-type", "ext-type.rfc1960",
     "{\"attr\":\"cn\",\"dn\":false,\"kind\":\"extensible\",\"rule\":null,"
     "\"value\":[70,114,101,100]}",
     ""},
    {"ext-rule", "ext-rule.rfc1960",
     "{\"attr\":\"cn\",\"dn\":false,\"kind\":\"extensible\",\"rule\":"
     "\"caseExactMatch\",\"value\":[70,114,101,100]}",
     ""},
    {"ext-dn", "ext-dn.rfc1960",
     "{\"attr\":\"cn\",\"dn\":true,\"kind\":\"extensible\",\"rule\":"
     "\"caseExactMatch\",\"value\":[70,114,101,100]}",
     ""},
    {"ext-rule-only", "ext-rule-only.rfc1960",
     "{\"attr\":null,\"dn\":false,\"kind\":\"extensible\",\"rule\":\"1.2.3\","
     "\"value\":[70,114,101,100]}",
     ""},
    {"ext-dn-rule", "ext-dn-rule.rfc1960",
     "{\"attr\":null,\"dn\":true,\"kind\":\"extensible\",\"rule\":\"1.2.3\","
     "\"value\":[70,114,101,100]}",
     ""},
    {"empty-and", "empty-and.rfc1960", "{\"kind\":\"and\",\"operands\":[]}",
     ""},
    {"empty-or", "empty-or.rfc1960", "{\"kind\":\"or\",\"operands\":[]}", ""},
    {"bad-attribute", "bad-attribute.rfc1960", "", "rfc1960.attribute"},
    {"empty-attribute", "empty-attribute.rfc1960", "", "rfc1960.attribute"},
    {"escape-junk", "escape-junk.rfc1960", "", "rfc1960.value"},
    {"escape-short", "escape-short.rfc1960", "", "rfc1960.value"},
    {"stars", "stars.rfc1960", "", "rfc1960.substring"},
    {"comparison-star", "comparison-star.rfc1960", "", "rfc1960.value"},
    {"bad-ext-dn", "bad-ext-dn.rfc1960", "", "rfc1960.matching-rule"},
    {"bad-ext-qualifiers", "bad-ext-qualifiers.rfc1960", "",
     "rfc1960.matching-rule"},
    {"empty-not", "empty-not.rfc1960", "", "rfc1960.not"},
    {"escaped-substring", "escaped-substring.rfc1960",
     "{\"attr\":\"cn\",\"final\":[99,100],\"initial\":[97,98,42],\"kind\":"
     "\"substring\",\"middle\":[]}",
     ""},
    {"value-spaces", "value-spaces.rfc1960",
     "{\"attr\":\"cn\",\"kind\":\"simple\",\"op\":\"=\",\"value\":[32,70,114,"
     "101,100,32]}",
     ""},
    {"ext-DN", "ext-DN.rfc1960",
     "{\"attr\":\"cn\",\"dn\":true,\"kind\":\"extensible\",\"rule\":\"1.2.3\","
     "\"value\":[120]}",
     ""}};
