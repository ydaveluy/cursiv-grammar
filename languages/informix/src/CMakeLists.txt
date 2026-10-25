# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `informixGenerated` (the compiled library) and `informix_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID informix
    GRAMMAR    informix/core/InformixGrammar.hpp
    NAMESPACE  informix::Informix
    EXTENSIONS .4gl
    OUT        informix/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/informix/core/*.cpp)
add_library(informixCore ${_core})
target_link_libraries(informixCore PUBLIC informixGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/informix/cli/*.cpp)
add_executable(informix_cli ${_cli})
target_link_libraries(informix_cli
                      PRIVATE informixCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/informix/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/informix/lsp/main.cpp)
add_library(informixLsp ${_lsp})
target_link_libraries(informixLsp PUBLIC informixCore cursiv::lsp)
add_executable(informix_lsp informix/lsp/main.cpp)
target_link_libraries(informix_lsp PRIVATE informixLsp)

cursiv_add_benchmark(informix SOURCES informix/bench/main.cpp)

cursiv_package_language(informix)
