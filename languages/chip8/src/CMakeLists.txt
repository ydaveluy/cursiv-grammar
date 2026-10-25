# Generate the parser and its language module.
cursiv_generate_language(ID chip8
    GRAMMAR    chip8/core/Chip8Grammar.hpp
    NAMESPACE  chip8::Chip8
    EXTENSIONS .chip8
    OUT        chip8/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/chip8/core/*.cpp)
add_library(chip8Core ${_core})
target_link_libraries(chip8Core PUBLIC chip8Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/chip8/cli/*.cpp)
if(_cli)
  add_executable(chip8_cli ${_cli})
  target_link_libraries(chip8_cli
                        PRIVATE chip8Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/chip8/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/chip8/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/chip8/lsp/main.cpp)
  add_library(chip8Lsp ${_lsp})
  target_link_libraries(chip8Lsp PUBLIC chip8Core cursiv::lsp)
  add_executable(chip8_lsp chip8/lsp/main.cpp)
  target_link_libraries(chip8_lsp PRIVATE chip8Lsp)
endif()

if(TARGET chip8Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chip8/bench/main.cpp")
  cursiv_add_benchmark(chip8
      SOURCES chip8/bench/main.cpp)
endif()

cursiv_package_language(chip8)
