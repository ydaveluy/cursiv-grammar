# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `parkingsignGenerated` (the compiled library) and `parkingsign_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID parkingsign
    GRAMMAR    parkingsign/core/ParkingsignGrammar.hpp
    NAMESPACE  parkingsign::Parkingsign
    EXTENSIONS .parkingsign
    OUT        parkingsign/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/core/*.cpp)
add_library(parkingsignCore ${_core})
target_link_libraries(parkingsignCore PUBLIC parkingsignGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/cli/*.cpp)
add_executable(parkingsign_cli ${_cli})
target_link_libraries(parkingsign_cli
                      PRIVATE parkingsignCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/parkingsign/lsp/main.cpp)
add_library(parkingsignLsp ${_lsp})
target_link_libraries(parkingsignLsp PUBLIC parkingsignCore cursiv::lsp)
add_executable(parkingsign_lsp parkingsign/lsp/main.cpp)
target_link_libraries(parkingsign_lsp PRIVATE parkingsignLsp)

cursiv_add_benchmark(parkingsign SOURCES parkingsign/bench/main.cpp)

cursiv_package_language(parkingsign)
