# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cmakeGenerated` (the compiled library) and `cmake_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cmake
    GRAMMAR    cmake/core/CmakeGrammar.hpp
    NAMESPACE  cmake::Cmake
    EXTENSIONS .cmake
    OUT        cmake/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/core/*.cpp)
add_library(cmakeCore ${_core})
target_link_libraries(cmakeCore PUBLIC cmakeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cli/*.cpp)
add_executable(cmake_cli ${_cli})
target_link_libraries(cmake_cli
                      PRIVATE cmakeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lsp/main.cpp)
add_library(cmakeLsp ${_lsp})
target_link_libraries(cmakeLsp PUBLIC cmakeCore cursiv::lsp)
add_executable(cmake_lsp cmake/lsp/main.cpp)
target_link_libraries(cmake_lsp PRIVATE cmakeLsp)

cursiv_add_benchmark(cmake SOURCES cmake/bench/main.cpp)

cursiv_package_language(cmake)
