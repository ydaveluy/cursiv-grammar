# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `urlGenerated` (the compiled library) and `url_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID url
    GRAMMAR    url/core/UrlGrammar.hpp
    NAMESPACE  url::Url
    EXTENSIONS .url
    OUT        url/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/url/core/*.cpp)
add_library(urlCore ${_core})
target_link_libraries(urlCore PUBLIC urlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/url/cli/*.cpp)
add_executable(url_cli ${_cli})
target_link_libraries(url_cli
                      PRIVATE urlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/url/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/url/lsp/main.cpp)
add_library(urlLsp ${_lsp})
target_link_libraries(urlLsp PUBLIC urlCore cursiv::lsp)
add_executable(url_lsp url/lsp/main.cpp)
target_link_libraries(url_lsp PRIVATE urlLsp)

cursiv_add_benchmark(url SOURCES url/bench/main.cpp)

cursiv_package_language(url)
