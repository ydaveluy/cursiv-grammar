# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `doiurlGenerated` (the compiled library) and `doiurl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID doiurl
    GRAMMAR    doiurl/core/DoiurlGrammar.hpp
    NAMESPACE  doiurl::Doiurl
    EXTENSIONS .doiurl
    OUT        doiurl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/core/*.cpp)
add_library(doiurlCore ${_core})
target_link_libraries(doiurlCore PUBLIC doiurlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/cli/*.cpp)
add_executable(doiurl_cli ${_cli})
target_link_libraries(doiurl_cli
                      PRIVATE doiurlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/lsp/main.cpp)
add_library(doiurlLsp ${_lsp})
target_link_libraries(doiurlLsp PUBLIC doiurlCore cursiv::lsp)
add_executable(doiurl_lsp doiurl/lsp/main.cpp)
target_link_libraries(doiurl_lsp PRIVATE doiurlLsp)

cursiv_add_benchmark(doiurl SOURCES doiurl/bench/main.cpp)

cursiv_package_language(doiurl)
