# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `matlabGenerated` (the compiled library) and `matlab_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID matlab
    GRAMMAR    matlab/core/MatlabGrammar.hpp
    NAMESPACE  matlab::Matlab
    EXTENSIONS .matlab
    OUT        matlab/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/matlab/core/*.cpp)
add_library(matlabCore ${_core})
target_link_libraries(matlabCore PUBLIC matlabGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/matlab/cli/*.cpp)
add_executable(matlab_cli ${_cli})
target_link_libraries(matlab_cli
                      PRIVATE matlabCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/matlab/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/matlab/lsp/main.cpp)
add_library(matlabLsp ${_lsp})
target_link_libraries(matlabLsp PUBLIC matlabCore cursiv::lsp)
add_executable(matlab_lsp matlab/lsp/main.cpp)
target_link_libraries(matlab_lsp PRIVATE matlabLsp)

cursiv_add_benchmark(matlab SOURCES matlab/bench/main.cpp)

cursiv_package_language(matlab)
