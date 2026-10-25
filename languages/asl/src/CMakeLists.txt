# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `aslGenerated` (the compiled library) and `asl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID asl
    GRAMMAR    asl/core/AslGrammar.hpp
    NAMESPACE  asl::Asl
    EXTENSIONS .asl
    OUT        asl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asl/core/*.cpp)
add_library(aslCore ${_core})
target_link_libraries(aslCore PUBLIC aslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asl/cli/*.cpp)
add_executable(asl_cli ${_cli})
target_link_libraries(asl_cli
                      PRIVATE aslCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/asl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/asl/lsp/main.cpp)
add_library(aslLsp ${_lsp})
target_link_libraries(aslLsp PUBLIC aslCore cursiv::lsp)
add_executable(asl_lsp asl/lsp/main.cpp)
target_link_libraries(asl_lsp PRIVATE aslLsp)

cursiv_add_benchmark(asl SOURCES asl/bench/main.cpp)

cursiv_package_language(asl)
