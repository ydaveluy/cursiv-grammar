#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "url/core/ast.hpp"

namespace url {

struct Url : cursiv::GrammarSpec {
  struct SCHEME : terminal<as<[](std::string_view text) {
    return text.substr(0, text.size() - 1);
  }>(cr<"A-Za-z"> + many(cr<"A-Za-z0-9+.-">) + lit<":">)> {};
  struct USERINFO : terminal<as<[](std::string_view text) {
    return text.substr(0, text.size() - 1);
  }>(many(cr<R"(^/?#\r\n)">) + lit<"@">)> {};
  struct HOST : terminal<some(cr<R"(^@/?#\r\n)">)> {};
  struct SEGMENT : terminal<some(cr<R"(^/?#\r\n)">)> {};
  struct FIRST_SEGMENT : terminal<some(cr<R"(^:/?#\r\n)">)> {};
  struct KEY : terminal<some(cr<R"(^&=#\r\n)">)> {};
  struct VALUE : terminal<some(cr<R"(^&#\r\n)">)> {};
  struct FRAGMENT : terminal<some(cr<R"(^\r\n)">)> {};
  struct ENDLINE : terminal<some(cr<R"(\r\n)">)> {};

  struct Login : rule<assign<&ast::Login::text>(ref<USERINFO>)> {};
  struct Hostname : rule<assign<&ast::Host::text>(ref<HOST>)> {};

  struct Segment : rule<as<std::string_view>(ref<SEGMENT> | &lit<"/">)> {};
  static constexpr auto SegmentTail =
      many(lit<"/"> + append<&ast::Path::segments>(ref<Segment>)) +
      option(flag<&ast::Path::trailingSlash>(lit<"/">));
  static constexpr auto AbsolutePath =
      flag<&ast::Path::absolute>(lit<"/">) +
      option(append<&ast::Path::segments>(ref<Segment>) + SegmentTail);
  struct AuthorityPath : rule<create<ast::Path>(AbsolutePath)> {};
  struct PathRule
      : rule<create<ast::Path>(AbsolutePath |
             (append<&ast::Path::segments>(ref<SEGMENT>) + SegmentTail))> {};
  struct RelativePath
      : rule<create<ast::Path>(AbsolutePath |
             (append<&ast::Path::segments>(ref<FIRST_SEGMENT>) + SegmentTail))> {};

  struct SearchParameter
      : rule<create<ast::SearchParameter>(
            option(assign<&ast::SearchParameter::key>(ref<KEY>)) +
            option(flag<&ast::SearchParameter::hasEquals>(lit<"=">) +
                   option(assign<&ast::SearchParameter::value>(ref<VALUE>))))> {};
  static constexpr auto Authority =
      flag<&ast::Url::hasAuthority>(lit<"//">) +
      option(assign<&ast::Url::login>(ref<Login>)) +
      option(assign<&ast::Url::host>(ref<Hostname>));
  template<class Path>
  static constexpr auto Body =
      (Authority + option(assign<&ast::Url::path>(ref<AuthorityPath>))) |
      (!lit<"//"> + option(assign<&ast::Url::path>(ref<Path>)));
  struct UrlRule
      : rule<create<ast::Url>(
            ((assign<&ast::Url::scheme>(ref<SCHEME>) + Body<PathRule>) |
             Body<RelativePath>) +
            option(flag<&ast::Url::hasQuery>(lit<"?">) +
                   some(append<&ast::Url::query>(ref<SearchParameter>),
                        lit<"&">)) +
            option(flag<&ast::Url::hasFragment>(lit<"#">) +
                   option(assign<&ast::Url::frag>(ref<FRAGMENT>))) +
            option(ref<ENDLINE>))> {};
  using entry = UrlRule;
};

}  // namespace url
