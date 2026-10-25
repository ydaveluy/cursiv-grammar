# Generate the parser and its language module.
cursiv_generate_language(ID zig
    GRAMMAR    zig/core/ZigGrammar.hpp
    NAMESPACE  zig::Zig
    EXTENSIONS .zig
    OUT        zig/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/zig/core/*.cpp)
add_library(zigCore ${_core})
target_link_libraries(zigCore PUBLIC zigGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/zig/cli/*.cpp)
if(_cli)
  add_executable(zig_cli ${_cli})
  target_link_libraries(zig_cli
                        PRIVATE zigCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/zig/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/zig/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/zig/lsp/main.cpp)
  add_library(zigLsp ${_lsp})
  target_link_libraries(zigLsp PUBLIC zigCore cursiv::lsp)
  add_executable(zig_lsp zig/lsp/main.cpp)
  target_link_libraries(zig_lsp PRIVATE zigLsp)
endif()

if(TARGET zigLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zig/bench/main.cpp")
  cursiv_add_benchmark(zig
      SOURCES zig/bench/main.cpp)
endif()

cursiv_package_language(zig)
