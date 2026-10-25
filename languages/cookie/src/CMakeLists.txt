# Generate the parser and its language module.
cursiv_generate_language(ID cookie
    GRAMMAR    cookie/core/CookieGrammar.hpp
    NAMESPACE  cookie::Cookie
    EXTENSIONS .cookie
    OUT        cookie/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cookie/core/*.cpp)
add_library(cookieCore ${_core})
target_link_libraries(cookieCore PUBLIC cookieGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cookie/cli/*.cpp)
if(_cli)
  add_executable(cookie_cli ${_cli})
  target_link_libraries(cookie_cli
                        PRIVATE cookieCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cookie/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cookie/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cookie/lsp/main.cpp)
  add_library(cookieLsp ${_lsp})
  target_link_libraries(cookieLsp PUBLIC cookieCore cursiv::lsp)
  add_executable(cookie_lsp cookie/lsp/main.cpp)
  target_link_libraries(cookie_lsp PRIVATE cookieLsp)
endif()

if(TARGET cookieLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cookie/bench/main.cpp")
  cursiv_add_benchmark(cookie
      SOURCES cookie/bench/main.cpp)
endif()

cursiv_package_language(cookie)
