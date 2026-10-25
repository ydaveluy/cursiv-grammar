# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `siciGenerated` (the compiled library) and `sici_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sici
    GRAMMAR    sici/core/SiciGrammar.hpp
    NAMESPACE  sici::Sici
    EXTENSIONS .sici
    OUT        sici/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sici/core/*.cpp)
add_library(siciCore ${_core})
target_link_libraries(siciCore PUBLIC siciGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sici/cli/*.cpp)
add_executable(sici_cli ${_cli})
target_link_libraries(sici_cli
                      PRIVATE siciCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sici/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sici/lsp/main.cpp)
add_library(siciLsp ${_lsp})
target_link_libraries(siciLsp PUBLIC siciCore cursiv::lsp)
add_executable(sici_lsp sici/lsp/main.cpp)
target_link_libraries(sici_lsp PRIVATE siciLsp)

cursiv_add_benchmark(sici SOURCES sici/bench/main.cpp)

cursiv_package_language(sici)
