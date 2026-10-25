# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `janusGenerated` (the compiled library) and `janus_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID janus
    GRAMMAR    janus/core/JanusGrammar.hpp
    NAMESPACE  janus::Janus
    EXTENSIONS .janus
    OUT        janus/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/janus/core/*.cpp)
add_library(janusCore ${_core})
target_link_libraries(janusCore PUBLIC janusGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/janus/cli/*.cpp)
add_executable(janus_cli ${_cli})
target_link_libraries(janus_cli
                      PRIVATE janusCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/janus/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/janus/lsp/main.cpp)
add_library(janusLsp ${_lsp})
target_link_libraries(janusLsp PUBLIC janusCore cursiv::lsp)
add_executable(janus_lsp janus/lsp/main.cpp)
target_link_libraries(janus_lsp PRIVATE janusLsp)

cursiv_add_benchmark(janus SOURCES janus/bench/main.cpp)

cursiv_package_language(janus)
