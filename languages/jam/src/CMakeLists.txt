# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `jamGenerated` (the compiled library) and `jam_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID jam
    GRAMMAR    jam/core/JamGrammar.hpp
    NAMESPACE  jam::Jam
    EXTENSIONS .jam
    OUT        jam/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jam/core/*.cpp)
add_library(jamCore ${_core})
target_link_libraries(jamCore PUBLIC jamGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jam/cli/*.cpp)
add_executable(jam_cli ${_cli})
target_link_libraries(jam_cli
                      PRIVATE jamCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/jam/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/jam/lsp/main.cpp)
add_library(jamLsp ${_lsp})
target_link_libraries(jamLsp PUBLIC jamCore cursiv::lsp)
add_executable(jam_lsp jam/lsp/main.cpp)
target_link_libraries(jam_lsp PRIVATE jamLsp)

cursiv_add_benchmark(jam SOURCES jam/bench/main.cpp)

cursiv_package_language(jam)
