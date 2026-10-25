# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `webidlGenerated` (the compiled library) and `webidl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID webidl
    GRAMMAR    webidl/core/WebidlGrammar.hpp
    NAMESPACE  webidl::Webidl
    EXTENSIONS .webidl
    OUT        webidl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/webidl/core/*.cpp)
add_library(webidlCore ${_core})
target_link_libraries(webidlCore PUBLIC webidlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/webidl/cli/*.cpp)
add_executable(webidl_cli ${_cli})
target_link_libraries(webidl_cli
                      PRIVATE webidlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/webidl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/webidl/lsp/main.cpp)
add_library(webidlLsp ${_lsp})
target_link_libraries(webidlLsp PUBLIC webidlCore cursiv::lsp)
add_executable(webidl_lsp webidl/lsp/main.cpp)
target_link_libraries(webidl_lsp PRIVATE webidlLsp)

cursiv_add_benchmark(webidl SOURCES webidl/bench/main.cpp)

cursiv_package_language(webidl)
