# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `httpGenerated` (the compiled library) and `http_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID http
    GRAMMAR    http/core/HttpGrammar.hpp
    NAMESPACE  http::Http
    EXTENSIONS .http
    OUT        http/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/http/core/*.cpp)
add_library(httpCore ${_core})
target_link_libraries(httpCore PUBLIC httpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/http/cli/*.cpp)
add_executable(http_cli ${_cli})
target_link_libraries(http_cli
                      PRIVATE httpCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/http/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/http/lsp/main.cpp)
add_library(httpLsp ${_lsp})
target_link_libraries(httpLsp PUBLIC httpCore cursiv::lsp)
add_executable(http_lsp http/lsp/main.cpp)
target_link_libraries(http_lsp PRIVATE httpLsp)

cursiv_add_benchmark(http SOURCES http/bench/main.cpp)

cursiv_package_language(http)
