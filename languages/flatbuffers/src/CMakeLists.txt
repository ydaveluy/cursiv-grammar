# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `flatbuffersGenerated` (the compiled library) and `flatbuffers_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID flatbuffers
    GRAMMAR    flatbuffers/core/FlatbuffersGrammar.hpp
    NAMESPACE  flatbuffers::Flatbuffers
    EXTENSIONS .fbs
    OUT        flatbuffers/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/core/*.cpp)
add_library(flatbuffersCore ${_core})
target_link_libraries(flatbuffersCore PUBLIC flatbuffersGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/cli/*.cpp)
add_executable(flatbuffers_cli ${_cli})
target_link_libraries(flatbuffers_cli
                      PRIVATE flatbuffersCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers/lsp/main.cpp)
add_library(flatbuffersLsp ${_lsp})
target_link_libraries(flatbuffersLsp PUBLIC flatbuffersCore cursiv::lsp)
add_executable(flatbuffers_lsp flatbuffers/lsp/main.cpp)
target_link_libraries(flatbuffers_lsp PRIVATE flatbuffersLsp)

cursiv_add_benchmark(flatbuffers SOURCES flatbuffers/bench/main.cpp)

cursiv_package_language(flatbuffers)
