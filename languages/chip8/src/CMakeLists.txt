# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `chip8Generated` (the compiled library) and `chip8_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID chip8
    GRAMMAR    chip8/core/Chip8Grammar.hpp
    NAMESPACE  chip8::Chip8
    EXTENSIONS .chip8
    OUT        chip8/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/chip8/core/*.cpp)
add_library(chip8Core ${_core})
target_link_libraries(chip8Core PUBLIC chip8Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/chip8/cli/*.cpp)
add_executable(chip8_cli ${_cli})
target_link_libraries(chip8_cli
                      PRIVATE chip8Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/chip8/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/chip8/lsp/main.cpp)
add_library(chip8Lsp ${_lsp})
target_link_libraries(chip8Lsp PUBLIC chip8Core cursiv::lsp)
add_executable(chip8_lsp chip8/lsp/main.cpp)
target_link_libraries(chip8_lsp PRIVATE chip8Lsp)

cursiv_add_benchmark(chip8 SOURCES chip8/bench/main.cpp)

cursiv_package_language(chip8)
