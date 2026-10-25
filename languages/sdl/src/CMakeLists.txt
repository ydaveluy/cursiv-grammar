# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sdlGenerated` (the compiled library) and `sdl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sdl
    GRAMMAR    sdl/core/SdlGrammar.hpp
    NAMESPACE  sdl::Sdl
    EXTENSIONS .sdl
    OUT        sdl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sdl/core/*.cpp)
add_library(sdlCore ${_core})
target_link_libraries(sdlCore PUBLIC sdlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sdl/cli/*.cpp)
add_executable(sdl_cli ${_cli})
target_link_libraries(sdl_cli
                      PRIVATE sdlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sdl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sdl/lsp/main.cpp)
add_library(sdlLsp ${_lsp})
target_link_libraries(sdlLsp PUBLIC sdlCore cursiv::lsp)
add_executable(sdl_lsp sdl/lsp/main.cpp)
target_link_libraries(sdl_lsp PRIVATE sdlLsp)

cursiv_add_benchmark(sdl SOURCES sdl/bench/main.cpp)

cursiv_package_language(sdl)
