# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `dcmGenerated` (the compiled library) and `dcm_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID dcm
    GRAMMAR    dcm/core/DcmGrammar.hpp
    NAMESPACE  dcm::Dcm
    EXTENSIONS .dcm
    OUT        dcm/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dcm/core/*.cpp)
add_library(dcmCore ${_core})
target_link_libraries(dcmCore PUBLIC dcmGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dcm/cli/*.cpp)
add_executable(dcm_cli ${_cli})
target_link_libraries(dcm_cli
                      PRIVATE dcmCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dcm/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dcm/lsp/main.cpp)
add_library(dcmLsp ${_lsp})
target_link_libraries(dcmLsp PUBLIC dcmCore cursiv::lsp)
add_executable(dcm_lsp dcm/lsp/main.cpp)
target_link_libraries(dcm_lsp PRIVATE dcmLsp)

cursiv_add_benchmark(dcm SOURCES dcm/bench/main.cpp)

cursiv_package_language(dcm)
