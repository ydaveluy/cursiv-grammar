#pragma once
#include <string_view>
struct MetadataCase {
  const char *file;
  std::string_view data;
};
inline const MetadataCase cases[] = {
    {"exact.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"0"}]}])gold"},
    {"spaces.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"0"}]}])gold"},
    {"explicit.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"2"}]}])gold"},
    {"following.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[],"name":"Red","value":"4"},{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"5"},{"attrs":[],"name":"Green","value":"6"}]}])gold"},
    {"comment.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"0"}]}])gold"},
    {"metadata.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null},{"name":"tag","value":"blue"}],"name":"Blue","value":"0"}]}])gold"},
    {"trailing.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[{"name":"deprecated","value":null}],"name":"Blue","value":"0"}]}])gold"},
    {"ordinary.fbs",
     R"gold([{"name":"Color","values":[{"attrs":[],"name":"Blue","value":"0"}]}])gold"},
};
