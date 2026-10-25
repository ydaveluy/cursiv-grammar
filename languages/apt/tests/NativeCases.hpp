#pragma once
#include <string_view>
#include <utility>
#include <vector>
struct NativeCase {
  std::string_view file, uri, suite, type;
  std::vector<std::string_view> components;
  std::vector<std::pair<std::string_view,std::string_view>> options;
  std::string_view error;
  std::size_t errorBegin;
  bool native;
  std::string_view nativeSuite;
  std::vector<std::string_view> nativeComponents;
  std::vector<std::pair<std::string_view,std::string_view>> nativeOptions;
};
inline const NativeCase nativeCases[] = {
{"ordinary.list","http://example.test/debian","stable","deb",{"main","contrib"},{},"",0,true,"stable",{"main","contrib"},{}},
{"source.list","https://example.test/debian","stable","deb-src",{"main"},{},"",0,true,"stable",{"main"},{}},
{"absolute.list","file:/srv/packages","./","deb",{},{},"",0,true,"./",{},{}},
{"quoted-uri.list","\"http://example.test/a b\"","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"quoted-suite.list","http://example.test/debian","\"stable updates\"","deb",{"main"},{},"",0,true,"stable updates",{"main"},{}},
{"quoted-component.list","http://example.test/debian","stable","deb",{"\"main tools\"","contrib"},{},"",0,true,"stable",{"main tools","contrib"},{}},
{"percent-fields.list","http://example.test/debian","stable%20updates","deb",{"main%20tools"},{},"",0,true,"stable updates",{"main tools"},{}},
{"quoted-option.list","http://example.test/debian","stable","deb",{"main"},{{"label","\"two words\""}},"",0,true,"stable",{"main"},{{"label","two words"}}},
{"unknown-option.list","http://example.test","stable","deb",{"main"},{{"x:y.z","value"},{"arch+","amd64,i386"},{"arch-","arm64"}},"",0,true,"stable",{"main"},{{"arch+","amd64,i386"},{"arch-","arm64"},{"x:y.z","value"}}},
{"option-hash.list","http://example.test","stable","deb",{"main"},{{"signed-by","/x#y"}},"",0,true,"stable",{"main"},{{"signed-by","/x#y"}}},
{"cdrom-spaces.list","cdrom:[\"Debian GNU/Linux 13\"]/","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"empty-options.list","http://example.test","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"lastline.list","http://example.test","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"crlf.list","http://example.test","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"same-line.list","http://example.test","stable","deb",{"main","deb","http://other.test","stable","contrib"},{},"",0,true,"stable",{"main","deb","http://other.test","stable","contrib"},{}},
{"comment.list","http://example.test","stable","deb",{"main"},{},"",0,true,"stable",{"main"},{}},
{"invalid-missing-components.list","http://example.test","stable","deb",{},{},"apt.components",24,false,"",{},{}},
{"invalid-absolute-components.list","file:/srv/packages","./","deb",{"main"},{},"apt.components",26,false,"",{},{}},
{"invalid-uri.list","no-colon","stable","deb",{"main"},{},"apt.uri",4,false,"",{},{}},
};
