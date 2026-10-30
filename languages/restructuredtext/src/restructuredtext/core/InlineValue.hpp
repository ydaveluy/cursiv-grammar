#pragma once
#include <cstddef>
#include <memory_resource>
#include <string_view>
#include "cursiv/core/grammar/GrammarSpec.hpp"
namespace restructuredtext {
template<std::size_t Left, std::size_t Right>
inline constexpr auto inline_text=[](std::string_view t, std::pmr::memory_resource* mem) {
  const auto body=t.substr(Left,t.size()-Left-Right);
  if(body.find('\\')==std::string_view::npos) return body;
  const auto room=cursiv::text_room(mem,body.size());
  std::size_t n=0;
  for(std::size_t i=0;i<body.size();++i) {
    char c=body[i];
    if(c=='\\' && i+1<body.size()) {
      c=body[++i];
      if(c==' '||c=='\t'||c=='\r'||c=='\n') continue;
    }
    room[n++]=c;
  }
  return std::string_view(room.data(),n);
};
inline constexpr auto target_name=[](std::string_view t) {
  t=t.substr(1,t.size()-2); // underscore and colon
  if(t.size()>=2 && t.front()=='`' && t.back()=='`') t=t.substr(1,t.size()-2);
  return t;
};
} // namespace restructuredtext
