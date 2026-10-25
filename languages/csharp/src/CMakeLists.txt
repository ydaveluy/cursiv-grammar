# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `csharpGenerated` (the compiled library) and `csharp_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID csharp
    GRAMMAR    csharp/core/CSharpGrammar.hpp
    NAMESPACE  csharp::CSharp
    EXTENSIONS .cs
    OUT        csharp/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csharp/core/*.cpp)
add_library(csharpCore ${_core})
target_link_libraries(csharpCore PUBLIC csharpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csharp/cli/*.cpp)
add_executable(csharp_cli ${_cli})
target_link_libraries(csharp_cli
                      PRIVATE csharpCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csharp/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/csharp/lsp/main.cpp)
add_library(csharpLsp ${_lsp})
target_link_libraries(csharpLsp PUBLIC csharpCore cursiv::lsp)
add_executable(csharp_lsp csharp/lsp/main.cpp)
target_link_libraries(csharp_lsp PRIVATE csharpLsp)

cursiv_add_benchmark(csharp SOURCES csharp/bench/main.cpp)

cursiv_package_language(csharp)
