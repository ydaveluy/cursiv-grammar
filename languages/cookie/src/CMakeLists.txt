# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cookieGenerated` (the compiled library) and `cookie_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cookie
    GRAMMAR    cookie/core/CookieGrammar.hpp
    NAMESPACE  cookie::Cookie
    EXTENSIONS .cookie
    OUT        cookie/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cookie/core/*.cpp)
add_library(cookieCore ${_core})
target_link_libraries(cookieCore PUBLIC cookieGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cookie/cli/*.cpp)
add_executable(cookie_cli ${_cli})
target_link_libraries(cookie_cli
                      PRIVATE cookieCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cookie/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cookie/lsp/main.cpp)
add_library(cookieLsp ${_lsp})
target_link_libraries(cookieLsp PUBLIC cookieCore cursiv::lsp)
add_executable(cookie_lsp cookie/lsp/main.cpp)
target_link_libraries(cookie_lsp PRIVATE cookieLsp)

cursiv_add_benchmark(cookie SOURCES cookie/bench/main.cpp)

cursiv_package_language(cookie)
