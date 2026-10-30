#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include "cursiv/core/grammar/Conversion.hpp"

namespace zig {
struct DecodedLiteral {
  std::string value;
  std::string_view error;
};

// Zig string_literal.zig: byte escapes stay bytes; Unicode escapes encode UTF-8.
// Its character parser also accepts escaped surrogate values, unlike strings.
inline DecodedLiteral decodeLiteral(std::string_view text,bool character=false) {
  DecodedLiteral out;
  if(text.size()<2)return {{},"Expected a quoted literal"};
  const auto hex=[](char c)->int {
    return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
  };
  for(std::size_t i=1;i+1<text.size();++i) {
    const unsigned char byte=static_cast<unsigned char>(text[i]);
    if(byte<0x20||byte==0x7f)return {{},"A raw control character is not allowed in a literal"};
    if(text[i]!='\\') {
      if(character) {
        const unsigned width=byte<0x80?1:byte>=0xc2&&byte<=0xdf?2:
                             byte>=0xe0&&byte<=0xef?3:byte>=0xf0&&byte<=0xf4?4:0;
        if(!width||i+width+1!=text.size())return {{},"Expected one UTF-8 codepoint in a character literal"};
        std::uint32_t cp=width==1?byte:byte&((1u<<(7-width))-1);
        for(unsigned n=1;n<width;++n) {
          const auto next=static_cast<unsigned char>(text[i+n]);
          if((next&0xc0)!=0x80)return {{},"Invalid UTF-8 character literal"};
          cp=(cp<<6)|(next&63);
        }
        if((width==2&&cp<0x80)||(width==3&&cp<0x800)||(width==4&&cp<0x10000)||
           cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return {{},"Invalid UTF-8 character literal"};
        out.value.append(text.substr(i,width));i+=width-1;
      } else out.value+=text[i];
      continue;
    }
    if(++i+1>=text.size())return {{},"Expected an escape character"};
    switch(text[i]) {
      case 'n':out.value+='\n';break;
      case 'r':out.value+='\r';break;
      case 't':out.value+='\t';break;
      case '\\':case '\'':case '"':out.value+=text[i];break;
      case 'x': {
        if(i+3>=text.size()||hex(text[i+1])<0||hex(text[i+2])<0)
          return {{},"A byte escape requires two hex digits"};
        out.value+=static_cast<char>(hex(text[i+1])*16+hex(text[i+2]));i+=2;break;
      }
      case 'u': {
        if(++i+1>=text.size()||text[i]!='{')return {{},"Expected { after \\u"};
        std::uint32_t cp=0;std::size_t digits=0;
        while(++i+1<text.size()&&text[i]!='}') {
          const int digit=hex(text[i]);
          if(digit<0)return {{},"Expected a hex digit in a Unicode escape"};
          cp=cp*16+static_cast<unsigned>(digit);++digits;
          if(cp>0x10ffff)return {{},"Unicode escape is out of range"};
        }
        if(!digits||i+1>=text.size()||text[i]!='}')return {{},"Expected a nonempty Unicode escape"};
        if(!character&&cp>=0xd800&&cp<=0xdfff)return {{},"A string Unicode escape must be a scalar value"};
        if(cp<0x80)out.value+=static_cast<char>(cp);
        else if(cp<0x800){out.value+=static_cast<char>(0xc0|(cp>>6));out.value+=static_cast<char>(0x80|(cp&63));}
        else if(cp<0x10000){out.value+=static_cast<char>(0xe0|(cp>>12));out.value+=static_cast<char>(0x80|((cp>>6)&63));out.value+=static_cast<char>(0x80|(cp&63));}
        else {out.value+=static_cast<char>(0xf0|(cp>>18));out.value+=static_cast<char>(0x80|((cp>>12)&63));out.value+=static_cast<char>(0x80|((cp>>6)&63));out.value+=static_cast<char>(0x80|(cp&63));}
        break;
      }
      default:return {{},"Invalid escape character"};
    }
  }
  return out;
}

template<bool Character=false,bool Identifier=false>
inline constexpr auto checkedLiteral=[](std::string_view raw) {
  using Result=cursiv::ConversionResult<std::string_view>;
  if constexpr(Identifier)if(!raw.starts_with("@\""))return Result::ok(raw);
  const auto decoded=decodeLiteral(Identifier?raw.substr(1):raw,Character);
  if(!decoded.error.empty())return Result::err(std::string(decoded.error));
  if constexpr(Identifier) {
    if(decoded.value.empty())return Result::err("An identifier cannot be empty");
    if(decoded.value.find('\0')!=std::string::npos)return Result::err("An identifier cannot contain a null byte");
  }
  return Result::ok(raw);  // preserve spelling and the existing AST contract
};
}  // namespace zig
