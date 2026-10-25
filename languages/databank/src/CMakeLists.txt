# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `databankGenerated` (the compiled library) and `databank_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID databank
    GRAMMAR    databank/core/DatabankGrammar.hpp
    NAMESPACE  databank::Databank
    EXTENSIONS .databank
    OUT        databank/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/databank/core/*.cpp)
add_library(databankCore ${_core})
target_link_libraries(databankCore PUBLIC databankGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/databank/cli/*.cpp)
add_executable(databank_cli ${_cli})
target_link_libraries(databank_cli
                      PRIVATE databankCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/databank/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/databank/lsp/main.cpp)
add_library(databankLsp ${_lsp})
target_link_libraries(databankLsp PUBLIC databankCore cursiv::lsp)
add_executable(databank_lsp databank/lsp/main.cpp)
target_link_libraries(databank_lsp PRIVATE databankLsp)

cursiv_add_benchmark(databank SOURCES databank/bench/main.cpp)

cursiv_package_language(databank)
