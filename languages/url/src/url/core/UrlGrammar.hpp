#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "url/core/ast.hpp"

namespace url {

struct Url : cursiv::GrammarSpec {
  struct SCHEME
      : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9+.-">) + lit<":">,
                 cursiv::text_conv<[](std::string_view text) {
                   return text.substr(0, text.size() - 1);
                 }>> {};
  struct USERINFO
      : terminal<many(cr<R"(^/?#\r\n)">) + lit<"@">,
                 cursiv::text_conv<[](std::string_view text) {
                   return text.substr(0, text.size() - 1);
                 }>> {};
  struct HOST : terminal<some(cr<R"(^@/?#\r\n)">)> {};
  struct SEGMENT : terminal<some(cr<R"(^/?#\r\n)">)> {};
  struct FIRST_SEGMENT : terminal<some(cr<R"(^:/?#\r\n)">)> {};
  struct KEY : terminal<some(cr<R"(^&=#\r\n)">)> {};
  struct VALUE : terminal<some(cr<R"(^&#\r\n)">)> {};
  struct FRAGMENT : terminal<some(cr<R"(^\r\n)">)> {};
  struct ENDLINE : terminal<some(cr<R"(\r\n)">)> {};

  struct LoginRule : rule<assign<&ast::Login::text>(ref<USERINFO>)> {};
  struct HostnameRule : rule<assign<&ast::Host::text>(ref<HOST>)> {};

  struct SegmentRule : rule<ref<SEGMENT> | &lit<"/">, std::string_view> {};
  static constexpr auto SegmentTail =
      many(lit<"/"> + append<&ast::Path::segments>(ref<SegmentRule>)) +
      option(flag<&ast::Path::trailingSlash>(lit<"/">));
  static constexpr auto AbsolutePath =
      flag<&ast::Path::absolute>(lit<"/">) +
      option(append<&ast::Path::segments>(ref<SegmentRule>) + SegmentTail);
  struct AuthorityPathRule : rule<create<ast::Path>(AbsolutePath)> {};
  struct PathRule
      : rule<create<ast::Path>(AbsolutePath |
             (append<&ast::Path::segments>(ref<SEGMENT>) + SegmentTail))> {};
  struct RelativePathRule
      : rule<create<ast::Path>(AbsolutePath |
             (append<&ast::Path::segments>(ref<FIRST_SEGMENT>) + SegmentTail))> {};

  struct SearchParameterRule
      : rule<create<ast::SearchParameter>(
            option(assign<&ast::SearchParameter::key>(ref<KEY>)) +
            option(flag<&ast::SearchParameter::hasEquals>(lit<"=">) +
                   option(assign<&ast::SearchParameter::value>(ref<VALUE>))))> {};
  static constexpr auto Authority =
      flag<&ast::Url::hasAuthority>(lit<"//">) +
      option(assign<&ast::Url::login>(ref<LoginRule>)) +
      option(assign<&ast::Url::host>(ref<HostnameRule>));
  template<class Path>
  static constexpr auto Body =
      (Authority + option(assign<&ast::Url::path>(ref<AuthorityPathRule>))) |
      (!lit<"//"> + option(assign<&ast::Url::path>(ref<Path>)));
  struct UrlRule
      : rule<create<ast::Url>(
            ((assign<&ast::Url::scheme>(ref<SCHEME>) + Body<PathRule>) |
             Body<RelativePathRule>) +
            option(flag<&ast::Url::hasQuery>(lit<"?">) +
                   some(append<&ast::Url::query>(ref<SearchParameterRule>),
                        lit<"&">)) +
            option(flag<&ast::Url::hasFragment>(lit<"#">) +
                   option(assign<&ast::Url::frag>(ref<FRAGMENT>))) +
            option(ref<ENDLINE>))> {};
  using entry = UrlRule;
};

}  // namespace url
