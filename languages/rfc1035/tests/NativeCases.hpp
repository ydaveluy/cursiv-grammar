#pragma once
#include <string_view>
struct NativeCase { std::string_view name, file, native, error; };
inline constexpr NativeCase nativeCases[]{
{"ordinary","ordinary.rfc1035","{\"absolute\":false,\"labels\":[[65],[73,83,73],[69,68,85]]}",""},
{"absolute","absolute.rfc1035","{\"absolute\":true,\"labels\":[[101,120,97,109,112,108,101],[99,111,109],[]]}",""},
{"root","root.rfc1035","{\"absolute\":true,\"labels\":[[]]}",""},
{"empty","empty.rfc1035","{\"absolute\":false,\"labels\":[]}",""},
{"origin","origin.rfc1035","{\"absolute\":false,\"labels\":[]}",""},
{"digit","digit.rfc1035","{\"absolute\":false,\"labels\":[[51,99,111,109],[101,120,97,109,112,108,101]]}",""},
{"underscore","underscore.rfc1035","{\"absolute\":true,\"labels\":[[95,115,105,112],[95,116,99,112],[101,120,97,109,112,108,101],[]]}",""},
{"hyphen-start","hyphen-start.rfc1035","{\"absolute\":false,\"labels\":[[45,120],[101,120,97,109,112,108,101]]}",""},
{"hyphen-end","hyphen-end.rfc1035","{\"absolute\":false,\"labels\":[[120,45],[101,120,97,109,112,108,101]]}",""},
{"escaped-dot","escaped-dot.rfc1035","{\"absolute\":true,\"labels\":[[97,46,98],[101,120,97,109,112,108,101],[]]}",""},
{"escaped-space","escaped-space.rfc1035","{\"absolute\":false,\"labels\":[[97,32,98],[101,120,97,109,112,108,101]]}",""},
{"nul","nul.rfc1035","{\"absolute\":false,\"labels\":[[97,0,98],[101,120,97,109,112,108,101]]}",""},
{"high","high.rfc1035","{\"absolute\":false,\"labels\":[[255],[101,120,97,109,112,108,101]]}",""},
{"raw-high","raw-high.rfc1035","{\"absolute\":false,\"labels\":[[128],[101,120,97,109,112,108,101]]}",""},
{"literal-space","literal-space.rfc1035","{\"absolute\":false,\"labels\":[[32]]}",""},
{"literal-at","literal-at.rfc1035","{\"absolute\":false,\"labels\":[[64]]}",""},
{"literal-backslash","literal-backslash.rfc1035","{\"absolute\":true,\"labels\":[[97,92,98],[]]}",""},
{"decimal-dot","decimal-dot.rfc1035","{\"absolute\":false,\"labels\":[[46],[101,120,97,109,112,108,101]]}",""},
{"label63","label63.rfc1035","{\"absolute\":true,\"labels\":[[97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97],[]]}",""},
{"label64","label64.rfc1035","","rfc1035.label-length"},
{"domain255","domain255.rfc1035","{\"absolute\":true,\"labels\":[[97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97,97],[98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98,98],[99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99],[100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100],[]]}",""},
{"domain256","domain256.rfc1035","","rfc1035.name-length"},
{"empty-interior","empty-interior.rfc1035","","rfc1035.empty-label"},
{"escape-short","escape-short.rfc1035","","rfc1035.escape"},
{"escape-range","escape-range.rfc1035","","rfc1035.escape"},
{"escape-trailing","escape-trailing.rfc1035","","rfc1035.escape"},
{"escape-digit-junk","escape-digit-junk.rfc1035","","rfc1035.escape"}
};
