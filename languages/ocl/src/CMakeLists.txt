# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `oclGenerated` (the compiled library) and `ocl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ocl
    GRAMMAR    ocl/core/OclGrammar.hpp
    NAMESPACE  ocl::Ocl
    EXTENSIONS .ocl
    OUT        ocl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ocl/core/*.cpp)
add_library(oclCore ${_core})
target_link_libraries(oclCore PUBLIC oclGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ocl/cli/*.cpp)
add_executable(ocl_cli ${_cli})
target_link_libraries(ocl_cli
                      PRIVATE oclCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ocl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ocl/lsp/main.cpp)
add_library(oclLsp ${_lsp})
target_link_libraries(oclLsp PUBLIC oclCore cursiv::lsp)
add_executable(ocl_lsp ocl/lsp/main.cpp)
target_link_libraries(ocl_lsp PRIVATE oclLsp)

cursiv_add_benchmark(ocl SOURCES ocl/bench/main.cpp)

cursiv_package_language(ocl)
